/*** websvc.c -- web services
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
#if HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
#include <string.h>
#include <stdio.h>

#include "websvc.h"
#include "sdef.h"
#include "sub.h"
#include "nifty.h"
#include "logger.h"

#if defined DEBUG_FLAG && !defined BENCHMARK
# include <assert.h>
# define WS_DEBUG(args...)	fprintf(logerr, args)
# define MAYBE_NOINLINE		__attribute__((noinline))
#else  /* !DEBUG_FLAG */
# define WS_DEBUG(args...)
# define assert(x)
# define MAYBE_NOINLINE
#endif	/* DEBUG_FLAG */


/* public funs */
websvc_f_t
websvc_from_request(struct websvc_s *tgt, const char *req, size_t UNUSED(len))
{
	static const char get_slash[] = "GET /";
	const char *p;
	websvc_f_t res;
	char *eo;

	if ((eo = strchr(req, '\n')) == NULL) {
		/* huh? */
		return WEBSVC_F_UNK;
	}

	/* quite illegal */
	*eo = '\0';
	logger("WSRQ  %s", req);

	if ((p = strstr(req, get_slash))) {
		if (strncmp(p += sizeof(get_slash) - 1, "secdef?", 7) == 0) {
			const char *q;

			tgt->ty = WEBSVC_F_SECDEF;
			if ((q = strstr(p += 7, "idx="))) {
				/* let's see what idx they want */
				long int idx = strtol(q + 4, &eo, 10);

				tgt->secdef.idx = (uint16_t)idx;
			}
		}
		res = tgt->ty;
	}
	return res;
}

size_t
websvc_secdef(char *restrict tgt, size_t tsz, subq_t sq, struct websvc_s sd)
{
	sub_t sub;
	unsigned int idx;
	ssize_t res;

	if (UNLIKELY(sd.ty != WEBSVC_F_SECDEF)) {
		return 0UL;
	} else if (UNLIKELY((idx = sd.secdef.idx) == 0)) {
		return 0UL;
	} else if (UNLIKELY((sub = subq_find_by_idx(sq, idx)) == NULL)) {
		return 0UL;
	} else if (UNLIKELY(sub->sdef == NULL)) {
		return 0UL;
	} else if ((res = tws_ser_sdef(tgt, tsz, sub->sdef)) < 0) {
		return 0UL;
	}
	return (size_t)res;
}

/* websvc.c ends here */
