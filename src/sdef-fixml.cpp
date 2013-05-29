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
#include <stdio.h>
#include <string.h>
#ifdef HAVE_TWSAPI_TWSAPI_CONFIG_H
# include <twsapi/twsapi_config.h>
#endif /* HAVE_TWSAPI_TWSAPI_CONFIG_H */
#include <twsapi/Contract.h>
#include "logger.h"

#if !defined SDEF_WRONLY
# include "proto-tx-ns.h"
# include "proto-fixml-attr.h"
# include "proto-fixml-tag.h"
# include "sdef-private.h"
#endif  /* !SDEF_WRONLY */
#include "sdef-seria.h"

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

#if !defined SDEF_WRONLY
# include "proto-fixml-attr.c"
# include "proto-fixml-tag.c"
#endif  /* !SDEF_WRONLY */

#if defined __INTEL_COMPILER
# pragma warning (default:869)
#elif defined HAD_STDC_INLINE
/* redefine the guy again */
# define __GNUC_STDC_INLINE__
#endif	/* __INTEL_COMPILER || __GNUC_STDC_INLINE__ */
#endif	/* HAVE_GPERF */


#if !defined SDEF_WRONLY
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
proc_FIXML_attr(__ctx_t ctx, const char *attr, const char *UNUSED(value))
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
		/* should have been initted already */
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

		if (LIKELY(ctx->next != NULL)) {
			void *sr = calloc(1, sizeof(*ctx->next));
			ctx->next->next = (tws_sreq_t)sr;
			ctx->next = (struct tws_sreq_s*)sr;
		} else {
			void *sr = calloc(1, sizeof(*ctx->next));
			ctx->next = (struct tws_sreq_s*)sr;
			ctx->sreq = (tws_sreq_t)sr;
		}
		for (const char **ap = attr; ap && *ap; ap += 2) {
			const fixml_aid_t aid = check_fix_attr(ctx, ap[0]);

			proc_SECDEF_attr(ins, TX_NS_FIXML_5_0, aid, ap[1]);
		}
		if (UNLIKELY(ctx->next->c != NULL)) {
			delete (IB::Contract*)ctx->next->c;
		}
		ctx->next->c = ins;
		break;
	}

	case FIX_TAG_INSTRMT: {
		tws_cont_t ins = (tws_cont_t)ctx->next->c;

		for (const char **ap = attr; ap && *ap; ap += 2) {
			const fixml_aid_t aid = check_fix_attr(ctx, ap[0]);

			proc_INSTRMT_attr(ins, TX_NS_FIXML_5_0, aid, ap[1]);
		}
		break;
	}

	default:
		break;
	}
	return;
}

void
sax_eo_FIXML_elt(__ctx_t UNUSED(ctx), const char *elem)
{
	fixml_tid_t tid = sax_fix_tid_from_tag(elem);

	/* stuff that needed to be done, fix up state etc. */
	switch (tid) {
		/* top-levels */
	case FIX_TAG_FIXML:
	case FIX_TAG_BATCH:
		break;

	case FIX_TAG_SECDEF:
	case FIX_TAG_INSTRMT:
		break;

	default:
		break;
	}
	return;
}
#endif  /* !SDEF_WRONLY */


/* serialiser */
static size_t
__add(char *restrict tgt, size_t tsz, const char *src, size_t ssz)
{
	if (ssz < tsz) {
		memcpy(tgt, src, ssz);
		tgt[ssz] = '\0';
		return ssz;
	}
	return 0;
}

ssize_t
tws_ser_sdef_fix(char *restrict buf, size_t bsz, tws_const_sdef_t src)
{
#define ADDv(tgt, tsz, s, ssz)	tgt += __add(tgt, tsz, s, ssz)
#define ADDs(tgt, tsz, string)	tgt += __add(tgt, tsz, string, strlen(string))
#define ADDl(tgt, tsz, ltrl)	tgt += __add(tgt, tsz, ltrl, sizeof(ltrl) - 1)
#define ADDc(tgt, tsz, c)	(tsz > 1 ? *tgt++ = c, 1 : 0)
#define ADDF(tgt, tsz, args...)	tgt += snprintf(tgt, tsz, args)

#define ADD_STR(tgt, tsz, tag, slot)			    \
	do {						    \
		const char *__c__ = slot.data();	    \
		const size_t __z__ = slot.size();	    \
							    \
		if (__z__) {				    \
			ADDl(tgt, tsz, " " tag "=\"");	    \
			ADDv(tgt, tsz, __c__, __z__);	    \
			ADDc(tgt, tsz, '\"');		    \
		}					    \
	} while (0)

	const IB::ContractDetails *d = (const IB::ContractDetails*)src;
	char *restrict p = buf;

#define REST	buf + bsz - p

	ADDl(p, REST, "<SecDef");

	// do we need BizDt and shit?

	// we do want the currency though
	ADD_STR(p, REST, "Ccy", d->summary.currency);

	ADDc(p, REST, '>');

	// instrmt tag
	ADDl(p, REST, "<Instrmt");

	// start out with symbol stuff
	ADD_STR(p, REST, "Sym", d->summary.localSymbol);

	if (const long int cid = d->summary.conId) {
		ADDF(p, REST, " ID=\"%ld\" Src=\"M\"", cid);
	}

	ADD_STR(p, REST, "SecTyp", d->summary.secType);
	ADD_STR(p, REST, "Exch", d->summary.exchange);

	ADD_STR(p, REST, "MatDt", d->summary.expiry);

	// right and strike
	ADD_STR(p, REST, "PutCall", d->summary.right);
	if (const double strk = d->summary.strike) {
		ADDF(p, REST, " StrkPx=\"%.6f\"", strk);
	}

	ADD_STR(p, REST, "Mult", d->summary.multiplier);
	ADD_STR(p, REST, "Desc", d->longName);

	if (const double mintick = d->minTick) {
		long int mult = strtol(d->summary.multiplier.c_str(), NULL, 10);

		ADDF(p, REST, " MinPxIncr=\"%.6f\"", mintick);
		if (mult) {
			double amt = mintick * (double)mult;
			ADDF(p, REST, " MinPxIncrAmt=\"%.6f\"", amt);
		}
	}

	ADD_STR(p, REST, "MMY", d->contractMonth);

	// finishing <Instrmt> open tag, Instrmt children will follow
	ADDc(p, REST, '>');

	// none yet

	// closing <Instrmt> tag, children of SecDef will follow
	ADDl(p, REST, "</Instrmt>");

#if TWSAPI_IB_VERSION_NUMBER <= 966
	if (IB::Contract::ComboLegList *cl = d->summary.comboLegs)
#else
	if (IB::Contract::ComboLegList *cl = d->summary.comboLegs.get())
#endif /* TWSAPI_IB_VERSION_NUMBER <= 966 */
	{
		for (IB::Contract::ComboLegList::iterator it = cl->begin(),
			     end = cl->end(); it != end; it++) {
			ADDl(p, REST, "<Leg");
			if (const long int cid = (*it)->conId) {
				ADDF(p, REST, " ID=\"%ld\" Src=\"M\"", cid);
			}
			ADD_STR(p, REST, "Exch", (*it)->exchange);
			ADD_STR(p, REST, "Side", (*it)->action);
			ADDF(p, REST, " RatioQty=\"%.6f\"",
				(double)(*it)->ratio);
			ADDc(p, REST, '>');
			ADDl(p, REST, "</Leg>");
		}
	}

	if (IB::UnderComp *undly = d->summary.underComp) {
		if (const long int cid = undly->conId) {
			ADDl(p, REST, "<Undly");
			ADDF(p, REST, " ID=\"%ld\" Src=\"M\"", cid);
			ADDc(p, REST, '>');
			ADDl(p, REST, "</Undly>");
		}
	}

	// finalise the whole shebang
	ADDl(p, REST, "</SecDef>");
	return p - buf;
}

/* sdef-fixml.cpp ends here */
