/*** quo.c -- quotes and queues of quotes
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
#if defined HAVE_UTERUS_UTERUS_H
# include <uterus/uterus.h>
# include <uterus/m30.h>
#elif defined HAVE_UTERUS_H
# include <uterus.h>
# include <m30.h>
#else
# error uterus headers are mandatory
#endif	/* HAVE_UTERUS_UTERUS_H || HAVE_UTERUS_H */

#include "quo.h"
#include "nifty.h"
#include "logger.h"
#define STATIC_GQ_GUTS
#include "gq.h"

#if defined DEBUG_FLAG && !defined BENCHMARK
# include <assert.h>
# define QUO_DEBUG(args...)	fprintf(logerr, args)
# define MAYBE_NOINLINE		__attribute__((noinline))
#else  /* !DEBUG_FLAG */
# define QUO_DEBUG(args...)
# define assert(x)
# define MAYBE_NOINLINE
#endif	/* DEBUG_FLAG */

typedef struct quo_qqq_s *quo_qqq_t;
typedef struct q30_s q30_t;

/* indexing into the quo_sub_s->quos */
struct q30_s {
	union {
		struct {
			size_t subtyp:1;
			size_t:1;
		};
		size_t typ:2;
	};
	size_t idx:16;
};

/* the quote-queue quote, i.e. an item of the quote queue */
struct quo_qqq_s {
	struct gq_item_s i;

	/* pointer into our quotes array */
	q30_t q;
};

struct quoq_s {
	struct gq_s q[1];
	struct gq_ll_s sbuf[1];
};


/* the quotes array */
static inline q30_t
make_q30(uint16_t iidx, quo_typ_t t)
{
#if defined HAVE_ANON_STRUCTS_INIT
	if (LIKELY(t >= QUO_TYP_BID && t <= QUO_TYP_ASZ)) {
		return __extension__(q30_t){.typ = t - 1, .idx = iidx};
	}
	return __extension__(q30_t){0};
#else  /* !HAVE_ANON_STRUCTS_INIT */
	struct q30_s res = {0};

	if (LIKELY(t >= QUO_TYP_BID && t <= QUO_TYP_ASZ)) {
		res.typ = t - 1;
		res.idx = iidx;
	}
	return res;
#endif	/* HAVE_ANON_STRUCTS_INIT */
}

static inline __attribute__((unused)) uint16_t
q30_idx(q30_t q)
{
	return (uint16_t)q.idx;
}

static inline __attribute__((unused)) quo_typ_t
q30_typ(q30_t q)
{
	return (quo_typ_t)q.typ;
}

static inline __attribute__((unused)) unsigned int
q30_sl1t_typ(q30_t q)
{
	return q30_typ(q) / 2 + SL1T_TTF_BID;
}


/* queues and stuff */
#include "gq.c"

static void
check_q(quoq_t qq)
{
#if defined DEBUG_FLAG
	/* count all items */
	size_t ni = 0;

	for (gq_item_t ip = qq->q->free->i1st; ip; ip = ip->next, ni++);
	for (gq_item_t ip = qq->sbuf->i1st; ip; ip = ip->next, ni++);
	assert(ni == qq->q->nitems / sizeof(struct quo_qqq_s));

	ni = 0;
	for (gq_item_t ip = qq->q->free->ilst; ip; ip = ip->prev, ni++);
	for (gq_item_t ip = qq->sbuf->ilst; ip; ip = ip->prev, ni++);
	assert(ni == qq->q->nitems / sizeof(struct quo_qqq_s));
#endif	/* DEBUG_FLAG */
	return;
}

static quo_qqq_t
pop_q(quoq_t qq)
{
	quo_qqq_t res;

	if (qq->q->free->i1st == NULL) {
		size_t nitems = qq->q->nitems / sizeof(*res);
		ptrdiff_t df;

		assert(qq->q->free->ilst == NULL);
		QUO_DEBUG("QQ RESIZE -> %zu\n", nitems + 64);
		df = init_gq(qq->q, sizeof(*res), nitems + 64);
		gq_rbld_ll(qq->sbuf, df);
		check_q(qq);
	}
	/* get us a new client and populate the object */
	res = (void*)gq_pop_head(qq->q->free);
	memset(res, 0, sizeof(*res));
	return res;
}


/* public funs */
quoq_t
make_quoq(void)
{
	static struct quoq_s qq = {0};
	return &qq;
}

void
free_quoq(quoq_t q)
{
	fini_gq(q->q);
	return;
}

void
quoq_add(quoq_t qq, struct quo_s q)
{
/* shall we rely on c++ code passing us a pointer we handed out earlier? */
	q30_t tgt;

	/* use the dummy ute file to do the sym2idx translation */
	if (q.idx == 0) {
		return;
	} else if (!(tgt = make_q30(q.idx, q.typ)).idx) {
		return;
#if 0
/* subject to subscription handling */
	} else if (q.idx > subs.nsubs) {
		/* that's actually so fatal I wanna vomit
		 * that means IB sent us ticker ids we've never requested */
		return;
#endif
	}

#if 0
/* subject to subscription handling */
	/* only when the coffee is roasted to perfection:
	 * update the slot TGT ... */
	subs.quos[tgt.idx - 1][tgt.typ] = ffff_m30_get_d(q.val);
#endif
	/* ... and push a reminder on the queue */
	{
		quo_qqq_t qi = pop_q(qq);

		qi->q = tgt;
		qi->q.subtyp = 0UL;
		gq_push_tail(qq->sbuf, (gq_item_t)qi);
		QUO_DEBUG("PUSH QQ %p\n", qi);
	}
	return;
}

/* quo.c ends here */
