/*** sdef.c -- security definitions
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
#include <string.h>
#include <stdbool.h>
#if defined HAVE_EXPAT_H
# include <expat.h>
#endif	/* HAVE_EXPAT_H */
#include "sdef.h"
#include "nifty.h"
#include "logger.h"

#include "sdef.h"
#include "proto-tx-ns.h"
#include "proto-tx-attr.h"
#include "proto-twsxml-tag.h"
#include "proto-fixml-tag.h"
#include "sdef-private.h"

#if defined DEBUG_FLAG
# define TX_DEBUG(args...)	logger(args)
#else  /* !DEBUG_FLAG */
# define TX_DEBUG(args...)
#endif	/* DEBUG_FLAG */


#if defined HAVE_GPERF
/* all the generated stuff */
#if defined __INTEL_COMPILER
# pragma warning (disable:869)
#elif defined __GNUC_STDC_INLINE__
# define HAD_STDC_INLINE
# undef __GNUC_STDC_INLINE__
#endif	/* __INTEL_COMPILER || __GNUC_STDC_INLINE__ */

#include "proto-tx-ns.c"
#include "proto-tx-attr.c"

#if defined __INTEL_COMPILER
# pragma warning (default:869)
#elif defined HAD_STDC_INLINE
/* redefine the guy again */
# define __GNUC_STDC_INLINE__
#endif	/* __INTEL_COMPILER || __GNUC_STDC_INLINE__ */
#endif	/* HAVE_GPERF */


#if defined HAVE_EXPAT_H
static ptx_ns_t
__pref_to_ns(__ctx_t ctx, const char *pref, size_t pref_len)
{
	if (UNLIKELY(ctx->ns[0].nsid == TX_NS_UNK)) {
		/* bit of a hack innit? */
		return ctx->ns;

	} else if (LIKELY(pref_len == 0 && ctx->ns[0].pref == NULL)) {
		/* most common case when people use the default ns */
		return ctx->ns;
	}
	/* special service for us because we're lazy:
	 * you can pass pref = "foo:" and say pref_len is 4
	 * easier to deal with when strings are const etc. */
	if (pref[pref_len - 1] == ':') {
		pref_len--;
	}
	for (size_t i = (ctx->ns[0].pref == NULL); i < ctx->nns; i++) {
		if (strncmp(ctx->ns[i].pref, pref, pref_len) == 0) {
			return ctx->ns + i;
		}
	}
	return NULL;
}

static tx_nsid_t
__tx_nsid_from_href(const char *href)
{
	size_t hlen = strlen(href);
	const struct tx_nsuri_s *n = __nsiddify(href, hlen);
	return n != NULL ? n->nsid : TX_NS_UNK;
}

void
ptx_reg_ns(__ctx_t ctx, const char *pref, const char *href)
{
	if (ctx->nns >= countof(ctx->ns)) {
		error(0, "too many name spaces");
		return;
	}

	if (UNLIKELY(href == NULL)) {
		/* bollocks, user MUST be a twat */
		return;
	}

	/* get us those lovely ns ids */
	{
		const tx_nsid_t nsid = __tx_nsid_from_href(href);

		switch (nsid) {
			size_t i;
		case TX_NS_TWSXML_0_1:
		case TX_NS_FIXML_5_0:
		case TX_NS_FIXML_4_4:
			if (UNLIKELY(ctx->ns[0].href != NULL)) {
				i = ctx->nns++;
				ctx->ns[i] = ctx->ns[0];
			}
			/* oh, it's our fave, make it the naught-th one */
			ctx->ns[0].pref = (pref ? strdup(pref) : NULL);
			ctx->ns[0].href = strdup(href);
			ctx->ns[0].nsid = nsid;
			break;

		case TX_NS_UNK:
		default:
			i = ctx->nns++;
			ctx->ns[i].pref = pref ? strdup(pref) : NULL;
			ctx->ns[i].href = strdup(href);
			ctx->ns[i].nsid = nsid;
			break;
		}
	}
	return;
}



static tx_aid_t
__tx_aid_from_attr_l(const char *attr, size_t len)
{
	const struct tx_attr_s *a = tx_aiddify(attr, len);
	return a ? a->aid : TX_ATTR_UNK;
}

static tx_aid_t
sax_tx_aid_from_attr(const char *attr)
{
	size_t alen = strlen(attr);
	const struct tx_attr_s *a = tx_aiddify(attr, alen);
	return a ? a->aid : TX_ATTR_UNK;
}
#endif	/* HAVE_EXPAT_H */


#if defined HAVE_EXPAT_H
static void
proc_TX_xmlns(__ctx_t ctx, const char *pref, const char *value)
{
	TX_DEBUG("XMLR  %s <- %s", pref, value);
	ptx_reg_ns(ctx, pref, value);
	return;
}

static void
proc_UNK_attr(__ctx_t ctx, const char *attr, const char *value)
{
	const char *rattr = tag_massage(attr);
	tx_aid_t aid;

	if (UNLIKELY(rattr > attr && !ptx_pref_p(ctx, attr, rattr - attr))) {
		aid = __tx_aid_from_attr_l(attr, rattr - attr - 1);
	} else {
		aid = sax_tx_aid_from_attr(rattr);
	}

	switch (aid) {
	case TX_ATTR_XMLNS:
		proc_TX_xmlns(ctx, rattr == attr ? NULL : rattr, value);
		break;
	default:
		break;
	}
	return;
}

static void
el_sta(void *clo, const char *elem, const char **attr)
{
	__ctx_t ctx = clo;
	/* where the real element name starts, sans ns prefix */
	const char *relem = tag_massage(elem);
	ptx_ns_t ns = __pref_to_ns(ctx, elem, relem - elem);

	if (UNLIKELY(ns == NULL)) {
		TX_DEBUG("unknown prefix in tag %s\n", elem);
		return;
	}

retry:
	switch (ns->nsid) {
	case TX_NS_TWSXML_0_1: {
		sax_bo_TWSXML_elt(ctx, relem, attr);
		break;
	}

	case TX_NS_FIXML_4_4:
	case TX_NS_FIXML_5_0:
		sax_bo_FIXML_elt(ctx, relem, attr);
		break;

	case TX_NS_UNK:
		for (const char **ap = attr; ap && *ap; ap += 2) {
			proc_UNK_attr(ctx, ap[0], ap[1]);
		}
		if ((ns = ctx->ns)->nsid != TX_NS_UNK) {
			goto retry;
		}

	default:
		TX_DEBUG("unknown namespace %s (%s)\n", elem, ns->href);
		break;
	}
	return;
}

static void
el_end(void *clo, const char *elem)
{
	__ctx_t ctx = clo;
	/* where the real element name starts, sans ns prefix */
	const char *relem = tag_massage(elem);
	ptx_ns_t ns = __pref_to_ns(ctx, elem, relem - elem);

	switch (ns->nsid) {
	case TX_NS_TWSXML_0_1:
		sax_eo_TWSXML_elt(ctx, relem);
		break;

	case TX_NS_FIXML_4_4:
	case TX_NS_FIXML_5_0:
		sax_eo_FIXML_elt(ctx, relem);
		break;

	case TX_NS_UNK:
	default:
		TX_DEBUG("unknown namespace %s (%s)\n", elem, ns->href);
		break;
	}
	return;
}
#endif	/* HAVE_EXPAT_H */


/* public funs */
#if defined HAVE_EXPAT_H
tws_sreq_t
tws_deser_sreq(const char *xml, size_t len)
{
	XML_Parser hdl;
	struct __ctx_s clo = {0};

	if ((hdl = XML_ParserCreate(NULL)) == NULL) {
		return NULL;
	}

	XML_SetElementHandler(hdl, el_sta, el_end);
	XML_SetUserData(hdl, &clo);

	if (XML_Parse(hdl, xml, len, XML_TRUE) == XML_STATUS_ERROR) {
		TX_DEBUG("XML parser error\n");
		tws_free_sreq(clo.sreq);
		return NULL;
	}

	/* get rid of resources */
	XML_ParserFree(hdl);
	return clo.sreq;
}

#else  /* HAVE_EXPAT_H */
tws_sreq_t
tws_deser_sreq(const char *UNUSED(xml), size_t UNUSED(len))
{
	return NULL;
}
#endif	/* HAVE_EXPAT_H */

static void*
unconst(const void *p)
{
	union {
		const void *c;
		void *p;
	} x = {p};
	return x.p;
}

void
tws_free_sreq(tws_sreq_t sreq)
{
	struct __sreq_s {
		struct __sreq_s *next;
	};

	if (UNLIKELY(sreq == NULL)) {
		return;
	}
	for (struct __sreq_s *sr = unconst(sreq), *srnx; sr; sr = srnx) {
		tws_sreq_t this = (tws_sreq_t)sr;

		srnx = sr->next;
		tws_free_cont(this->c);
		if (this->nick != NULL) {
			free(unconst(this->nick));
		}
		free(sr);
	}
	return;
}


/* serialiser */
ssize_t
tws_ser_sdef(char *restrict buf, size_t bsz, tws_const_sdef_t sd)
{
	static char hdr[] = "\
<?xml version=\"1.0\"?>\n\
<FIXML xmlns=\"http://www.fixprotocol.org/FIXML-5-0-SP2\"/>\
";
	static char ftr[] = "\
</FIXML>\n\
";
	char *restrict p;

	if (bsz < sizeof(hdr)) {
		/* completely fucked */
		return -1;
	}

	/* always start out with the hdr,
	 * which for efficiency contains the empty case already */
	strncpy(p = buf, hdr, bsz);

	if (UNLIKELY(sd == NULL)) {
		/* this is convenience for lazy definitions in the higher
		 * level, undocumented though */
		return sizeof(hdr) - 1;
	}

	/* modify the contents so far */
	p[sizeof(hdr) - 3] = '>';
	p[sizeof(hdr) - 2] = '\n';
	/* 1 for the / we discarded, one for the \0 */
	p += sizeof(hdr) - 1 - 1;

	/* it's just one contract, yay
	 * we quickly give an estimate of the space left
	 * we used to count in the space we need for the footer,
	 * but that would give us a headache when we switch to incremental
	 * string building. */
	{
		static ssize_t(*cb)() = tws_ser_sdef_fix;
		size_t spc_left = bsz - (p - buf);
		ssize_t tmp;

		if ((tmp = cb(p, spc_left, sd)) < 0) {
			/* grrrr */
			return -1;
		}
		/* otherwise */
		p += tmp;
	}

	/* and the footer now */
	if (p + sizeof(ftr) < buf + bsz) {
		memcpy(p, ftr, sizeof(ftr));
		p += sizeof(ftr) - 1;
	}
	return p - buf;
}

/* sdef.c ends here */
