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
#include <sys/time.h>

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
struct key_s {
	uint32_t idx;
	union {
		struct {
			uint32_t subtyp:1;
			uint32_t suptyp:31;
		};
		quo_typ_t typ;
	};
};

/* the quote-queue quote, i.e. an item of the quote queue */
struct quo_qqq_s {
	struct gq_item_s i;

	struct quo_s q;
#if defined ASPECT_QUO_AGE
	uint32_t last_dsm;
#endif	/* ASPECT_QUO_AGE */
};

struct quoq_s {
	struct gq_s q[1];
	/* stuff to send to the wire */
	struct gq_ll_s sbuf[1];
	/* price cells */
	struct gq_ll_s pbuf[1];
};


/* the quotes array */
static inline __attribute__((pure)) int
q30_price_typ_p(struct key_s q)
{
	return q.subtyp == 0U;
}

static inline int
matches_q30_p(quo_qqq_t cell, struct key_s q)
{
	return cell->q.idx == q.idx && cell->q.suptyp == q.suptyp;
}


/* queues and stuff */
#include "gq.c"

static void
check_q(quoq_t qq)
{
#if defined DEBUG_FLAG
	/* count all items */
	size_t ni;

	ni = 0;
	for (gq_item_t ip = qq->q->free->i1st; ip; ip = ip->next, ni++);
	for (gq_item_t ip = qq->sbuf->i1st; ip; ip = ip->next, ni++);
	for (gq_item_t ip = qq->pbuf->i1st; ip; ip = ip->next, ni++);
	assert(ni == qq->q->nitems / sizeof(struct quo_qqq_s));

	ni = 0;
	for (gq_item_t ip = qq->q->free->ilst; ip; ip = ip->prev, ni++);
	for (gq_item_t ip = qq->sbuf->ilst; ip; ip = ip->prev, ni++);
	for (gq_item_t ip = qq->pbuf->ilst; ip; ip = ip->prev, ni++);
	assert(ni == qq->q->nitems / sizeof(struct quo_qqq_s));
#endif	/* DEBUG_FLAG */
	return;
}

static quo_qqq_t
make_qqq(quoq_t qq)
{
	quo_qqq_t res;

	if (qq->q->free->i1st == NULL) {
		size_t nitems = qq->q->nitems / sizeof(*res);
		ptrdiff_t df;

		assert(qq->q->free->ilst == NULL);
		QUO_DEBUG("RESZ  QQ  ->%zu\n", nitems + 64);
		df = init_gq(qq->q, sizeof(*res), nitems + 64);
		gq_rbld_ll(qq->sbuf, df);
		check_q(qq);
	}
	/* get us a new client and populate the object */
	res = (void*)gq_pop_head(qq->q->free);
	memset(res, 0, sizeof(*res));
	return res;
}

static void
free_qqq(quoq_t qq, quo_qqq_t q)
{
	gq_push_tail(qq->q->free, (gq_item_t)q);
	return;
}

static quo_qqq_t
pop_qqq(quoq_t qq)
{
	return (quo_qqq_t)gq_pop_head(qq->sbuf);
}

static void
bang_qqq(quoq_t qq, quo_qqq_t q)
{
/* put q to price-quote list of qq */
	gq_push_tail(qq->pbuf, (gq_item_t)q);
	return;
}

static quo_qqq_t
find_p_cell(gq_ll_t lst, struct key_s k)
{
	for (gq_item_t ip = lst->ilst; ip; ip = ip->prev) {
		quo_qqq_t qp = (void*)ip;

		if (matches_q30_p(qp, k) && q30_price_typ_p(k)) {
			/* yay */
			return qp;
		}
	}
	return NULL;
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
#define SILLY_CAST(x, o)	*((x*)&(o))
	quo_qqq_t qi = make_qqq(qq);
	struct key_s k = SILLY_CAST(struct key_s, q);

	if (!q30_price_typ_p(k)) {
		/* price cell, always gets pushed */
		qi->q = q;
		/* make sure the qty slot is naught */
		qi->q.q = 0U;
	} else {
		/* qty cell, find the last price */
		quo_qqq_t qp;

		/* clone it all */
		qi->q = q;
		/* try the current queue first */
		if (LIKELY((qp = find_p_cell(qq->sbuf, k)) != NULL)) {
			/* just add the bugger if q slot is unset */
			if (qp->q.q == 0U) {
				qp->q.q = q.q;
				free_qqq(qq, qi);
				return;
			}
			/* otherwise clone the whole thing */
			goto clone;
		}
		/* try the price queue next */
		if ((qp = find_p_cell(qq->pbuf, k)) != NULL) {
		clone:
			/* clone the thing right away */
			qi->q.p = qp->q.p;
		} else {
			/* entirely new to us, make sure the price slot is 0 */
			qi->q.p = 0U;
		}
	}
	gq_push_tail(qq->sbuf, (gq_item_t)qi);
	return;
}

void
quoq_flush_cb(quoq_t qq, quoq_cb_f cb, void *clo)
{
#if defined ASPECT_QUO_AGE
	struct timeval now[1];
#endif	/* ASPECT_QUO_AGE */
	struct quoq_cb_asp_s asp = {
		.type = QUOQ_CB_FLUSH,
	};
	quo_qqq_t qi;

#if defined ASPECT_QUO_AGE
	/* time */
	gettimeofday(now, NULL);
#endif	/* ASPECT_QUO_AGE */

	while ((qi = pop_qqq(qq)) != NULL) {
		quo_qqq_t qp;
		struct key_s k = SILLY_CAST(struct key_s, qi->q);

		QUO_DEBUG("FLSH  %u %u %u %u\n",
			qi->q.idx, qi->q.typ, qi->q.p, qi->q.q);

#if defined ASPECT_QUO_AGE
		/* keep a note about dissemination */
		qi->last_dsm = now->tv_sec;
#endif	/* ASPECT_QUO_AGE */

		/* find the cell in the pbuf */
		if ((qp = find_p_cell(qq->pbuf, k)) != NULL) {
#if defined ASPECT_QUO_AGE
			asp.age = qi->last_dsm - qp->last_dsm;
		} else {
			asp.age = -1;
#endif	/* ASPECT_QUO_AGE */
		}

		/* call the callback */
		cb(asp, qi->q, clo);

		/* finalise the cells */
		if (qp != NULL) {
			qp->q = qi->q;
#if defined ASPECT_QUO_AGE
			qp->last_dsm = qi->last_dsm;
#endif	/* ASPECT_QUO_AGE */
			free_qqq(qq, qi);
		} else {
			bang_qqq(qq, qi);
		}
	}
	return;
}

/* quo.c ends here */
