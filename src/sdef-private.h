/*** sdef.c -- security definitions, private parts
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
#if !defined INCLUDED_sdef_private_h_
#define INCLUDED_sdef_private_h_

#include <string.h>
#include <stdbool.h>
#include "sdef.h"
#include "nifty.h"

#if defined __cplusplus
extern "C" {
# if defined __GNUC__
#  define restrict	__restrict__
# else
#  define restrict
# endif
#endif	/* __cplusplus */

typedef struct __ctx_s *__ctx_t;
typedef struct ptx_ns_s *ptx_ns_t;

typedef union tx_tid_u tx_tid_t;

typedef struct tws_xml_req_s *tws_xml_req_t;

union tx_tid_u {
	unsigned int u;
#if defined INCLUDED_proto_twsxml_tag_h_
	tws_xml_tid_t tx;
#endif	/* INCLUDED_proto_twsxml_tag_h_ */
#if defined INCLUDED_proto_fixml_tag_h_
	fixml_tid_t fix;
#endif	/* INCLUDED_proto_fixml_tag_h_ */
};

struct ptx_ns_s {
	char *pref;
	char *href;
	tx_nsid_t nsid;
};

/* contextual callbacks */
struct __ctx_s {
	struct ptx_ns_s ns[16];
	size_t nns;
	/* stuff buf */
#define INITIAL_STUFF_BUF_SIZE	(4096)
	char *sbuf;
	size_t sbsz;
	size_t sbix;

	tws_sreq_t sreq;
	struct tws_sreq_s *next;
};


/* twsxml */
extern void sax_bo_TWSXML_elt(__ctx_t, const char *elem, const char **attr);
extern void sax_eo_TWSXML_elt(__ctx_t ctx, const char *elem);

/* fixml */
extern void sax_bo_FIXML_elt(__ctx_t, const char *elem, const char **attr);
extern void sax_eo_FIXML_elt(__ctx_t, const char *elem);

/* generic */
extern void ptx_reg_ns(__ctx_t, const char *pref, const char *href);


static const char*
tag_massage(const char *tag)
{
/* return the real part of a (ns'd) tag or attribute,
 * i.e. foo:that_tag becomes that_tag */
	const char *p = strchr(tag, ':');

	if (p) {
		/* skip over ':' */
		return p + 1;
	}
	/* otherwise just return the tag as is */
	return tag;
}

static bool
ptx_pref_p(__ctx_t ctx, const char *pref, size_t pref_len)
{
	/* we sorted our namespaces so that ptx is always at index 0 */
	if (UNLIKELY(ctx->ns[0].href == NULL)) {
		return false;

	} else if (LIKELY(ctx->ns[0].pref == NULL)) {
		/* prefix must not be set here either */
		return pref == NULL || pref_len == 0;

	} else if (UNLIKELY(pref_len == 0)) {
		/* this node's prefix is "" but we expect a prefix of
		 * length > 0 */
		return false;

	} else {
		/* special service for us because we're lazy:
		 * you can pass pref = "foo:" and say pref_len is 4
		 * easier to deal with when strings are const etc. */
		if (pref[pref_len - 1] == ':') {
			pref_len--;
		}
		return memcmp(pref, ctx->ns[0].pref, pref_len) == 0;
	}
}

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_sdef_private_h_ */
