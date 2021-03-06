/*** quo.h -- quotes and queues of quotes
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
#if !defined INCLUDED_quo_h_
#define INCLUDED_quo_h_

#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif	/* __cplusplus */

/** a single quote object for internal passing around */
typedef struct quo_s *quo_t;
/** a queue of quote objects */
typedef struct quoq_s *quoq_t;

typedef uint32_t qv_t;

typedef enum {
	QUO_TYP_BID,
	QUO_TYP_BSZ,
	QUO_TYP_ASK,
	QUO_TYP_ASZ,
	QUO_TYP_TRA,
	QUO_TYP_TSZ,
	QUO_TYP_VWP,
	QUO_TYP_VOL,
	QUO_TYP_CLO,
	QUO_TYP_CSZ,
} quo_typ_t;

struct quo_s {
	uint32_t idx;
	union {
		struct {
			uint32_t subtyp:1;
			uint32_t suptyp:31;
		};
		quo_typ_t typ;
	};
	qv_t p;
	qv_t q;
};

typedef void(*quoq_cb_f)(struct quo_s, void*);


/* ctors/dtors */
extern quoq_t make_quoq(void);
extern void free_quoq(quoq_t q);

/**
 * Add Q to the quote queue QQ. */
extern void quoq_add(quoq_t qq, struct quo_s q);

/**
 * Convert ticks on the queue QQ to uterus sl1t. */
extern void
quoq_flush_cb(quoq_t qq, quoq_cb_f cb, void *clo);

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_quo_h_ */
