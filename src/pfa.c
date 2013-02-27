/*** pfa.c -- portfolio positions and queues thereof
 *
 * Copyright (C) 2013 Sebastian Freundt
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

#include "pfa.h"
#include "nifty.h"
#include "logger.h"
#define STATIC_GQ_GUTS
#include "gq.h"

#if defined DEBUG_FLAG && !defined BENCHMARK
# include <assert.h>
# define PFA_DEBUG(args...)	fprintf(logerr, args)
# define MAYBE_NOINLINE		__attribute__((noinline))
# define MAYBE_UNUSED(x)	x
#else  /* !DEBUG_FLAG */
# define PFA_DEBUG(args...)
# define assert(x)
# define MAYBE_NOINLINE
# define MAYBE_UNUSED(x)	UNUSED(x)
#endif	/* DEBUG_FLAG */

typedef struct pfa_pqq_s *pfa_pqq_t;

/* the pfate-queue pfate, i.e. an item of the pfate queue */
struct pfa_pqq_s {
	struct gq_item_s i;

	struct pfa_s p;
#if defined ASPECT_PFA_AGE
	uint32_t last_dsm;
#endif	/* ASPECT_PFA_AGE */
};

struct pfaq_s {
	struct gq_s q[1];
	/* stuff rcvd off the wire */
	struct gq_ll_s rbuf[1];
};


/* queues and stuff */
#include "gq.c"

static pfa_pqq_t
make_pqq(pfaq_t pq)
{
	pfa_pqq_t res;

	if (pq->q->free->i1st == NULL) {
		size_t nitems = pq->q->nitems / sizeof(*res);

		assert(pq->q->free->ilst == NULL);
		PFA_DEBUG("RESZ  PQ  ->%zu\n", nitems + 64);
		init_gq(pq->q, 256U, sizeof(*res));
	}
	/* get us a new client and populate the object */
	res = (void*)gq_pop_head(pq->q->free);
	memset(res, 0, sizeof(*res));
	return res;
}

static void
free_pqq(pfaq_t pq, pfa_pqq_t q)
{
	gq_push_tail(pq->q->free, (gq_item_t)q);
	return;
}

static pfa_pqq_t
pop_pqq(pfaq_t pq)
{
	return (pfa_pqq_t)gq_pop_head(pq->rbuf);
}

static void
bang_pqq(pfaq_t pq, pfa_pqq_t p)
{
/* put p to position list of pq */
	gq_push_tail(pq->rbuf, (gq_item_t)p);
	return;
}


/* public funs */
pfaq_t
make_pfaq(void)
{
	static struct pfaq_s pq = {0};
	return &pq;
}

void
free_pfaq(pfaq_t q)
{
	fini_gq(q->q);
	return;
}

#define SILLY_CAST(x, o)	*((x*)&(o))

void
pfaq_add(pfaq_t pq, struct pfa_s p)
{
	pfa_pqq_t pi = make_pqq(pq);

	/* always clone upfront */
	pi->p = p;
	bang_pqq(pq, pi);
	return;
}

void
pfaq_flush_cb(pfaq_t pq, void(*cb)(struct pfa_s, void*), void *clo)
{
	pfa_pqq_t pi;

	while ((pi = pop_pqq(pq)) != NULL) {
		PFA_DEBUG("FLSH  %s@%s %.4f %.4f\n",
			pi->p.sym, pi->p.ac, pi->p.lqty, pi->p.sqty);

		/* call the callback */
		cb(pi->p, clo);

		/* finalise the cells */
		free_pqq(pq, pi);
	}
	return;
}

/* pfa.c ends here */
