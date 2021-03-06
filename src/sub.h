/*** sub.h -- subscriptions
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
#if !defined INCLUDED_sub_h_
#define INCLUDED_sub_h_

#include <stdint.h>
#include "sdef.h"

#if defined __cplusplus
extern "C" {
#endif	/* __cplusplus */

/** a simple subscription object */
typedef struct sub_s *sub_t;
/** a queue of subscription objects */
typedef struct subq_s *subq_t;

struct sub_s {
	/** idx as assigned by the tws api */
	uint32_t idx;
	/** our idx as used for advertising */
	uint32_t uidx;
	/** the id of the sdef req sent */
	uint32_t sreq;
	/** time stamp of last dissemination */
	uint32_t last_dsm;
	tws_sdef_t sdef;
	char *nick;
};

typedef void(*subq_cb_f)(struct sub_s, void*);


/* ctors dtors */
extern subq_t make_subq(void);
extern void free_subq(subq_t);

/* and accessors */
extern void subq_add(subq_t sq, struct sub_s s);

/**
 * Return the sub_t object on SQ that has idx IDX. */
extern sub_t subq_find_idx(subq_t sq, uint32_t idx);

/**
 * Return the sub_t object on SQ that has idx IDX. */
extern sub_t subq_find_uidx(subq_t sq, uint32_t idx);

/**
 * Return the sub_t object on SQ that has nick NICK. */
extern sub_t subq_find_nick(subq_t sq, const char *nick);

/**
 * Return the sub_t object on SQ that has sdef request id IDX. */
extern sub_t subq_find_sreq(subq_t sq, uint32_t idx);

/* flushing iterator */
extern void subq_flush_cb(subq_t sq, subq_cb_f cb, void *clo);

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_sub_h_ */
