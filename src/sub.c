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
#else  /* !DEBUG_FLAG */
# define SUB_DEBUG(args...)
# define assert(x)
# define MAYBE_NOINLINE
#endif	/* DEBUG_FLAG */

typedef struct sub_qqq_s *sub_qqq_t;

/* the sub-queue sub, i.e. an item of the subscription queue */
struct sub_qqq_s {
	struct gq_item_s i;

	struct sub_s s;
	uint32_t last_dsm;
};

struct subq_s {
	struct gq_s q[1];
	struct gq_ll_s sbuf[1];
};


/* queues and stuff */
#include "gq.c"

static void
check_q(subq_t sq)
{
#if defined DEBUG_FLAG
	/* count all items */
	size_t ni = 0;

	for (gq_item_t ip = sq->q->free->i1st; ip; ip = ip->next, ni++);
	for (gq_item_t ip = sq->sbuf->i1st; ip; ip = ip->next, ni++);
	assert(ni == sq->q->nitems / sizeof(struct sub_qqq_s));

	ni = 0;
	for (gq_item_t ip = sq->q->free->ilst; ip; ip = ip->prev, ni++);
	for (gq_item_t ip = sq->sbuf->ilst; ip; ip = ip->prev, ni++);
	assert(ni == sq->q->nitems / sizeof(struct sub_qqq_s));
#endif	/* DEBUG_FLAG */
	return;
}

static sub_qqq_t
pop_q(subq_t sq)
{
	sub_qqq_t res;

	if (sq->q->free->i1st == NULL) {
		size_t nitems = sq->q->nitems / sizeof(*res);
		ptrdiff_t df;

		assert(sq->q->free->ilst == NULL);
		SUB_DEBUG("SQ RESIZE -> %zu\n", nitems + 64);
		df = init_gq(sq->q, sizeof(*res), nitems + 64);
		gq_rbld_ll(sq->sbuf, df);
		check_q(sq);
	}
	/* get us a new client and populate the object */
	res = (void*)gq_pop_head(sq->q->free);
	memset(res, 0, sizeof(*res));
	return res;
}

static sub_qqq_t
find_cell(gq_ll_t lst, uint32_t idx)
{
	for (gq_item_t ip = lst->ilst; ip; ip = ip->prev) {
		sub_qqq_t sp = (void*)ip;

		if (sp->s.idx == idx) {
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
	fini_gq(q->q);
	return;
}

void
subq_add(subq_t sq, struct sub_s s)
{
	/* get us a free item */
	sub_qqq_t si = pop_q(sq);

	si->s = s;
	gq_push_tail(sq->sbuf, (gq_item_t)si);
	SUB_DEBUG("PUSH SQ %p\n", si);
	return;
}

struct sub_s
subq_find_by_idx(subq_t sq, uint32_t idx)
{
	sub_qqq_t sp;

	if (LIKELY((sp = find_cell(sq->sbuf, idx)) != NULL)) {
		return sp->s;
	}
	return (struct sub_s){0};
}

/* sub.c ends here */
