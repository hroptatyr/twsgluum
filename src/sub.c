/*** sub.c -- subscriptions
 *
 * Copyright (C) 2012-2013 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of twsgluum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***/
#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
#include <stdlib.h>
#include <stdio.h>
#include "sub.h"
#include "nifty.h"
#include "logger.h"
#define STATIC_GQ_GUTS
#include "gq.h"

#if defined DEBUG_FLAG && !defined BENCHMARK
# include <assert.h>
# define SUB_DEBUG(args...)	fprintf(logerr, args)
# define MAYBE_NOINLINE		__attribute__((noinline))
# define MAYBE_UNUSED(x)	x
#else  /* !DEBUG_FLAG */
# define SUB_DEBUG(args...)
# define assert(x)
# define MAYBE_NOINLINE
# define MAYBE_UNUSED(x)	UNUSED(x)
#endif	/* DEBUG_FLAG */

typedef struct sub_qqq_s *sub_qqq_t;

/* the sub-queue sub, i.e. an item of the subscription queue */
struct sub_qqq_s {
	struct gq_item_s i;

	struct sub_s s;
};

struct subq_s {
	struct gq_s q[1];
	struct gq_ll_s sbuf[1];
	struct gq_ll_s norm[1];
};


/* queues and stuff */
#include "gq.c"

static sub_qqq_t
make_qqq(subq_t sq)
{
	sub_qqq_t res;

	if (sq->q->free->i1st == NULL) {
		assert(sq->q->free->ilst == NULL);
		SUB_DEBUG("RESZ  SQ  ->+%u\n", 64U);
		init_gq(sq->q, 64U, sizeof(*res));
		SUB_DEBUG("RESZ  SQ  ->%zu\n", sq->q->nitems / sizeof(*res));
	}
	/* get us a new client and populate the object */
	res = (void*)gq_pop_head(sq->q->free);
	memset(res, 0, sizeof(*res));
	return res;
}

static void
free_qqq(subq_t sq, sub_qqq_t s)
{
	gq_push_tail(sq->q->free, (gq_item_t)s);
	return;
}

static sub_qqq_t
pop_qqq(subq_t sq)
{
	return (sub_qqq_t)gq_pop_head(sq->sbuf);
}

static sub_qqq_t
find_cell(gq_ll_t lst, uint32_t idx)
{
	for (gq_item_t ip = lst->i1st; ip; ip = ip->next) {
		sub_qqq_t sp = (void*)ip;

		if (sp->s.idx == idx) {
			return sp;
		}
	}
	return NULL;
}

static sub_qqq_t
find_uidx(gq_ll_t lst, uint32_t uidx)
{
	for (gq_item_t ip = lst->i1st; ip; ip = ip->next) {
		sub_qqq_t sp = (void*)ip;

		if (sp->s.uidx == uidx) {
			return sp;
		}
	}
	return NULL;
}

static sub_qqq_t
find_sreq(gq_ll_t lst, uint32_t idx)
{
	for (gq_item_t ip = lst->i1st; ip; ip = ip->next) {
		sub_qqq_t sp = (void*)ip;

		if (sp->s.sreq == idx) {
			return sp;
		}
	}
	return NULL;
}

static sub_qqq_t
find_nick(gq_ll_t lst, const char nick[1])
{
	for (gq_item_t ip = lst->i1st; ip; ip = ip->next) {
		sub_qqq_t sp = (void*)ip;

		if (LIKELY(sp->s.nick != NULL) && !strcmp(sp->s.nick, nick)) {
			return sp;
		}
	}
	return NULL;
}


/* public funs */
subq_t
make_subq(void)
{
	static struct subq_s sq = {0};
	return &sq;
}

void
free_subq(subq_t q)
{
	/* free items on the norm queue */
	for (gq_item_t ip = q->norm->i1st; ip; ip = ip->next) {
		free(((sub_qqq_t)ip)->s.nick);
	}
	fini_gq(q->q);
	return;
}

void
subq_add(subq_t sq, struct sub_s s)
{
	static uint32_t uidx;

	/* try and find the cell with s.nick on the queue */
	sub_qqq_t ni;
	if (UNLIKELY(s.nick == NULL)) {
		ni = NULL;
	} else if ((ni = find_nick(sq->norm, s.nick)) != NULL) {
		s.uidx = ni->s.uidx;
	} else {
		ni = make_qqq(sq);
		ni->s.uidx = s.uidx = ++uidx;
		ni->s.nick = strdup(s.nick);
		gq_push_tail(sq->norm, (gq_item_t)ni);
	}

	/* get us a free item */
	sub_qqq_t si = make_qqq(sq);
	/* just copy the whole shebang */
	si->s = s;
	/* and push it */
	gq_push_tail(sq->sbuf, (gq_item_t)si);
	SUB_DEBUG("PUSH  SQ  %p %u\n", si, s.uidx);
	return;
}

sub_t
subq_find_idx(subq_t sq, uint32_t idx)
{
	sub_qqq_t sp;

	if (LIKELY((sp = find_cell(sq->sbuf, idx)) != NULL)) {
		return &sp->s;
	}
	return NULL;
}

sub_t
subq_find_uidx(subq_t sq, uint32_t uidx)
{
	sub_qqq_t sp;

	if (LIKELY((sp = find_uidx(sq->sbuf, uidx)) != NULL)) {
		return &sp->s;
	}
	return NULL;
}

sub_t
subq_find_sreq(subq_t sq, uint32_t idx)
{
	sub_qqq_t sp;

	if (LIKELY((sp = find_sreq(sq->sbuf, idx)) != NULL)) {
		return &sp->s;
	}
	return NULL;
}

sub_t
subq_find_nick(subq_t sq, const char *nick)
{
	sub_qqq_t sp;

	if (UNLIKELY(nick == NULL)) {
		;
	} else if (LIKELY((sp = find_nick(sq->sbuf, nick)) != NULL)) {
		return &sp->s;
	}
	return NULL;
}

void
subq_flush_cb(subq_t sq, subq_cb_f cb, void *clo)
{
	for (sub_qqq_t si; (si = pop_qqq(sq)) != NULL; free_qqq(sq, si)) {
		SUB_DEBUG("POP!  SQ  %p\n", si);
		/* call the callback */
		cb(si->s, clo);
	}
	/* leave the norm queue untouched */
	return;
}

/* sub.c ends here */
