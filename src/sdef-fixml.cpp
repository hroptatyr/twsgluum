/*** sdef-fixml.cpp -- security definitions, fixml part
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
#include <unistd.h>
#include <twsapi/Contract.h>
#include "logger.h"

#include "proto-tx-ns.h"
#include "proto-fixml-attr.h"
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

#include "proto-fixml-attr.c"
#include "proto-fixml-tag.c"

#if defined __INTEL_COMPILER
# pragma warning (default:869)
#elif defined HAD_STDC_INLINE
/* redefine the guy again */
# define __GNUC_STDC_INLINE__
#endif	/* __INTEL_COMPILER || __GNUC_STDC_INLINE__ */
#endif	/* HAVE_GPERF */


static fixml_aid_t
__fix_aid_from_attr_l(const char *attr, size_t len)
{
	const struct fixml_attr_s *a = __fix_aiddify(attr, len);
	return a ? a->aid : FIX_ATTR_UNK;
}

static fixml_tid_t
sax_fix_tid_from_tag(const char *tag)
{
	size_t tlen = strlen(tag);
	const struct fixml_tag_s *t = __fix_tiddify(tag, tlen);
	return t ? t->tid : FIX_TAG_UNK;
}

static fixml_aid_t
sax_fix_aid_from_attr(const char *attr)
{
	size_t alen = strlen(attr);
	const struct fixml_attr_s *a = __fix_aiddify(attr, alen);
	return a ? a->aid : FIX_ATTR_UNK;
}

static fixml_aid_t
check_fix_attr(__ctx_t ctx, const char *attr)
{
	const char *rattr = tag_massage(attr);
	const fixml_aid_t aid = sax_fix_aid_from_attr(rattr);

	if (!ptx_pref_p(ctx, attr, rattr - attr)) {
		/* dont know what to do */
		TX_DEBUG("unknown namespace %s\n", attr);
		return FIX_ATTR_UNK;
	}
	return aid;
}

static void
tws_cont_fix(tws_cont_t tgt, unsigned int aid, const char *val)
{
	IB::Contract *c = (IB::Contract*)tgt;

	switch ((fixml_aid_t)aid) {
	case FIX_ATTR_CCY:
		c->currency = std::string(val);
		break;
	case FIX_ATTR_EXCH:
		c->exchange = std::string(val);
		break;
	case FIX_ATTR_SYM:
		c->localSymbol = std::string(val);
		break;
	case FIX_ATTR_SECTYP:
		if (!strcmp(val, "FXSPOT")) {
			c->secType = std::string("CASH");
		}
		break;
	default:
		break;
	}
	return;
}

static void
tws_cont_x(tws_cont_t tgt, unsigned int nsid, unsigned int aid, const char *val)
{
	switch ((tx_nsid_t)nsid) {
	case TX_NS_FIXML_5_0:
		tws_cont_fix(tgt, aid, val);
		break;
	case TX_NS_TWSXML_0_1:
	default:
		abort();
	}
	return;
}


static void
proc_FIX_xmlns(__ctx_t ctx, const char *pref, const char *value)
{
	TX_DEBUG("reg'ging name space %s <- %s\n", pref, value);
	ptx_reg_ns(ctx, pref, value);
	return;
}

static void
proc_INSTRMT_attr(tws_cont_t ins, tx_nsid_t ns, fixml_aid_t aid, const char *v)
{
	tws_cont_x(ins, ns, aid, v);
	return;
}

static void
proc_SECDEF_attr(tws_cont_t ins, tx_nsid_t ns, fixml_aid_t aid, const char *v)
{
	tws_cont_x(ins, ns, aid, v);
	return;
}

static void
proc_FIXML_attr(__ctx_t ctx, const char *attr, const char *value)
{
	const char *rattr = tag_massage(attr);
	fixml_aid_t aid;

	if (UNLIKELY(rattr > attr && !ptx_pref_p(ctx, attr, rattr - attr))) {
		aid = __fix_aid_from_attr_l(attr, rattr - attr - 1);
	} else {
		aid = sax_fix_aid_from_attr(rattr);
	}

	switch (aid) {
	case FIX_ATTR_XMLNS:
		proc_FIX_xmlns(ctx, rattr == attr ? NULL : rattr, value);
		break;
	case FIX_ATTR_S:
	case FIX_ATTR_R:
		/* we're so not interested in version mumbo jumbo */
		break;
	case FIX_ATTR_V:
		break;
	default:
		TX_DEBUG("WARN: unknown attr %s\n", attr);
		break;
	}
	return;
}


void
sax_bo_FIXML_elt(__ctx_t ctx, const char *elem, const char **attr)
{
	const fixml_tid_t tid = sax_fix_tid_from_tag(elem);

	/* all the stuff that needs a new sax handler */
	switch (tid) {
	case FIX_TAG_FIXML:
		ptx_init(ctx);

		if (UNLIKELY(attr == NULL)) {
			break;
		}

		for (const char **ap = attr; ap && *ap; ap += 2) {
			proc_FIXML_attr(ctx, ap[0], ap[1]);
		}
		break;

	case FIX_TAG_BATCH:
		break;

	case FIX_TAG_SECDEF: {
		tws_cont_t ins = (tws_cont_t)new IB::Contract;
		tx_tid_t x = {tid};

		for (const char **ap = attr; ap && *ap; ap += 2) {
			const fixml_aid_t aid = check_fix_attr(ctx, ap[0]);

			proc_SECDEF_attr(ins, TX_NS_FIXML_5_0, aid, ap[1]);
		}
		push_state(ctx, TX_NS_FIXML_5_0, x, ins);
		break;
	}

	case FIX_TAG_INSTRMT: {
		tws_cont_t ins = get_state_object(ctx);
		tx_tid_t x = {tid};

		for (const char **ap = attr; ap && *ap; ap += 2) {
			const fixml_aid_t aid = check_fix_attr(ctx, ap[0]);

			proc_INSTRMT_attr(ins, TX_NS_FIXML_5_0, aid, ap[1]);
		}
		push_state(ctx, TX_NS_FIXML_5_0, x, ins);
		break;
	}

	default:
		break;
	}
	return;
}

void
sax_eo_FIXML_elt(__ctx_t ctx, const char *elem)
{
	fixml_tid_t tid = sax_fix_tid_from_tag(elem);

	/* stuff that needed to be done, fix up state etc. */
	switch (tid) {
		/* top-levels */
	case FIX_TAG_FIXML:
	case FIX_TAG_BATCH:
		break;

	case FIX_TAG_SECDEF: {
		tws_cont_t ins = pop_state(ctx);

		if (UNLIKELY(ins == NULL)) {
			TX_DEBUG("internal parser error, cont is NULL\n");
			break;
		} else if (ctx->cont_cb == NULL ||
			   ctx->cont_cb(ins, ctx->cbclo) < 0) {
			delete (IB::Contract*)ins;
		}
		break;
	}

	case FIX_TAG_INSTRMT:
		pop_state(ctx);
		break;

	default:
		break;
	}
	return;
}

/* sdef-fixml.cpp ends here */
