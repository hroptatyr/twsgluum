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
			uint32_t subtyp:1;
			uint32_t suptyp:31;
		};
		uint32_t typ:32;
	};
	uint32_t idx;
};

/* the quote-queue quote, i.e. an item of the quote queue */
struct quo_qqq_s {
	struct gq_item_s i;

	q30_t t;
	m30_t p;
	m30_t q;
	uint32_t last_dsm;
};

struct quoq_s {
	struct gq_s q[1];
	/* stuff to send to the wire */
	struct gq_ll_s sbuf[1];
	/* price cells */
	struct gq_ll_s pbuf[1];
};


/* the quotes array */
static inline q30_t
make_q30(uint32_t iidx, quo_typ_t t)
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

static inline __attribute__((pure)) int
q30_price_typ_p(q30_t q)
{
	return q.subtyp == 0U;
}

static inline unsigned int
q30_sl1t_typ(q30_t q)
{
/* q's typ slot was designed to coincide with ute's sl1t types */
	return q.typ / 2 + SL1T_TTF_BID;
}

static unsigned int
q30_sl1t_idx(q30_t q)
{
	return q.idx;
}

static inline int
matches_q30_p(quo_qqq_t cell, q30_t q)
{
	return cell->t.idx == q.idx && cell->t.suptyp == q.suptyp;
}


/* uterus glue */
static int
fill_sl1t(struct sl1t_s tgt[static 1], quo_qqq_t qi)
{
	unsigned int tix;
	unsigned int ttf;

	if (UNLIKELY((tix = q30_sl1t_idx(qi->t)) == 0U)) {
		return -1;
	} else if (UNLIKELY((ttf = q30_sl1t_typ(qi->t)) == SCOM_TTF_UNK)) {
		return -1;
	}

	sl1t_set_ttf(tgt, (uint16_t)ttf);
	sl1t_set_tblidx(tgt, (uint16_t)tix);

	tgt->pri = qi->p.u;
	tgt->qty = qi->q.u;
	return 0;
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
		QUO_DEBUG("RESZ QQ -> %zu\n", nitems + 64);
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
find_p_cell(gq_ll_t lst, q30_t tgt)
{
	for (gq_item_t ip = lst->ilst; ip; ip = ip->prev) {
		quo_qqq_t qp = (void*)ip;

		if (matches_q30_p(qp, tgt) &&
		    q30_price_typ_p(qp->t)) {
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
	quo_qqq_t qi;
	q30_t tgt;
	m30_t val;

	if (!(tgt = make_q30(q.idx, q.typ)).idx) {
		return;
	}

	/* just to relieve inlining pressure */
	val = ffff_m30_get_d(q.val);
	/* and some more */
	qi = make_qqq(qq);

	if (!q30_price_typ_p(tgt)) {
		/* find the last price */
		quo_qqq_t qp;
		m30_t pv;

		/* try the current queue first */
		if (LIKELY((qp = find_p_cell(qq->sbuf, tgt)) != NULL)) {
			/* just add the bugger if q slot is unset */
			if (qp->q.u == 0U) {
				qp->q = val;
				free_qqq(qq, qi);
				return;
			}
			/* otherwise clone the whole thing */
			goto clone;
		}
		/* try the price queue next */
		if ((qp = find_p_cell(qq->pbuf, tgt)) != NULL) {
		clone:
			/* clone the thing right away */
			tgt = qp->t;
			pv = qp->p;
		} else {
			/* entirely new to us */
			pv.u = 0U;
		}
		qi->t = tgt;
		qi->p = pv;
		qi->q = val;
	} else {
		qi->t = tgt;
		qi->p = val;
	}
	gq_push_tail(qq->sbuf, (gq_item_t)qi);
	return;
}

void
quoq_flush(quoq_t qq)
{
	quo_qqq_t qi;

	while ((qi = pop_qqq(qq)) != NULL) {
		quo_qqq_t qp;

		QUO_DEBUG("FLSH %u %u %i %i\n",
			qi->t.idx, qi->t.typ, (int)qi->p.mant, (int)qi->q.mant);

		if ((qp = find_p_cell(qq->pbuf, qi->t)) != NULL) {
			qp->p = qi->p;
			qp->q = qi->q;
			free_qqq(qq, qi);
		} else {
			bang_qqq(qq, qi);
		}
	}
	return;
}

void
quoq_flush_cb(quoq_t qq, quoq_cb_f cb, void *clo)
{
	struct timeval now[1];
	struct sl1t_s l1t[1];
	struct quoq_cb_asp_s asp = {
		.type = QUOQ_CB_FLUSH,
	};
	quo_qqq_t qi;

	/* time */
	gettimeofday(now, NULL);

	/* populate l1t somewhat */
	sl1t_set_stmp_sec(l1t, now->tv_sec);
	sl1t_set_stmp_msec(l1t, now->tv_usec / 1000);

	while ((qi = pop_qqq(qq)) != NULL) {
		quo_qqq_t qp;

		QUO_DEBUG("FLSH %u %u %i %i\n",
			qi->t.idx, qi->t.typ, (int)qi->p.mant, (int)qi->q.mant);

		if (fill_sl1t(l1t, qi) < 0) {
			goto free;
		}

		/* keep a note about dissemination */
		qi->last_dsm = now->tv_sec;

		/* find the cell in the pbuf */
		if ((qp = find_p_cell(qq->pbuf, qi->t)) != NULL) {
#if defined ASPECT_QUO_AGE
			asp.age = qi->last_dsm - qp->last_dsm;
		} else {
			asp.age = -1;
#endif	/* ASPECT_QUO_AGE */
		}

		/* call the callback */
		cb(asp, l1t, clo);

		/* finalise the cells */
		if (qp != NULL) {
			qp->p = qi->p;
			qp->q = qi->q;
			qp->last_dsm = qi->last_dsm;
		free:
			free_qqq(qq, qi);
		} else {
			bang_qqq(qq, qi);
		}
	}
	return;
}

/* quo.c ends here */
