/*** sdef-twsxml.cpp -- security definitions, twsxml part
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
#include "logger.h"
#ifdef HAVE_TWSAPI_TWSAPI_CONFIG_H
# include <twsapi/twsapi_config.h>
#endif /* HAVE_TWSAPI_TWSAPI_CONFIG_H */
#include <twsapi/Contract.h>

#if !defined SDEF_WRONLY
# include "proto-tx-ns.h"
# include "proto-twsxml-attr.h"
# include "proto-twsxml-tag.h"
# include "proto-twsxml-reqtyp.h"
# include "sdef-private.h"
#endif	/* !SDEF_WRONLY */
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
# include "proto-twsxml-attr.c"
# include "proto-twsxml-tag.c"
# include "proto-twsxml-reqtyp.c"
#endif  /* !SDEF_WRONLY */

#if defined __INTEL_COMPILER
# pragma warning (default:869)
#elif defined HAD_STDC_INLINE
/* redefine the guy again */
# define __GNUC_STDC_INLINE__
#endif	/* __INTEL_COMPILER || __GNUC_STDC_INLINE__ */
#endif	/* HAVE_GPERF */


#if !defined SDEF_WRONLY
static tws_xml_aid_t
__tx_aid_from_attr_l(const char *attr, size_t len)
{
	const struct tws_xml_attr_s *a = __aiddify(attr, len);
	return a ? a->aid : TX_ATTR_UNK;
}

static tws_xml_tid_t
sax_tx_tid_from_tag(const char *tag)
{
	size_t tlen = strlen(tag);
	const struct tws_xml_tag_s *t = __tiddify(tag, tlen);
	return t ? t->tid : TX_TAG_UNK;
}

static tws_xml_aid_t
sax_tx_aid_from_attr(const char *attr)
{
	size_t alen = strlen(attr);
	const struct tws_xml_attr_s *a = __aiddify(attr, alen);
	return a ? a->aid : TX_ATTR_UNK;
}

static tws_xml_aid_t
check_tx_attr(__ctx_t ctx, const char *attr)
{
	const char *rattr = tag_massage(attr);
	const tws_xml_aid_t aid = sax_tx_aid_from_attr(rattr);

	if (!ptx_pref_p(ctx, attr, rattr - attr)) {
		/* dont know what to do */
		TX_DEBUG("unknown namespace %s\n", attr);
		return TX_ATTR_UNK;
	}
	return aid;
}


static void
proc_COMBOLEG_attr(
	tws_cont_t leg, tx_nsid_t UNUSED(n), tws_xml_aid_t aid, const char *val)
{
	IB::ComboLeg *c = (IB::ComboLeg*)leg;

	switch ((tws_xml_aid_t)aid) {
	case TX_ATTR_CONID:
		c->conId = strtol(val, NULL, 10);
		break;
	case TX_ATTR_RATIO:
		c->ratio = strtol(val, NULL, 10);
		break;
	case TX_ATTR_ACTION:
		c->action = std::string(val);
		break;
	case TX_ATTR_OPENCLOSE:
		c->openClose = strtol(val, NULL, 10);
		break;
	case TX_ATTR_EXCHANGE:
		c->exchange = std::string(val);
		break;
	default:
		break;
	}
	return;
}

static void
proc_REQCONTRACT_attr(
	tws_cont_t ins, tx_nsid_t UNUSED(n), tws_xml_aid_t aid, const char *val)
{
	IB::Contract *c = (IB::Contract*)ins;

	switch ((tws_xml_aid_t)aid) {
	case TX_ATTR_CONID:
		c->conId = strtol(val, NULL, 10);
		break;
	case TX_ATTR_SYMBOL:
		c->symbol = std::string(val);
		break;
	case TX_ATTR_CURRENCY:
		c->currency = std::string(val);
		break;
	case TX_ATTR_SECTYPE:
		c->secType = std::string(val);
		break;
	case TX_ATTR_EXCHANGE:
		c->exchange = std::string(val);
		break;
	default:
		break;
	}
	return;
}

static void
proc_QMETA_attr(
	struct tws_sreq_s *sr, tx_nsid_t UNUSED(ns),
	tws_xml_aid_t aid, const char *val)
{
	switch ((tws_xml_aid_t)aid) {
	case TX_ATTR_NICK:
		/* we use the comboLegsDescrip field for our nicks */
		sr->nick = strdup(val);
		break;
	case TX_ATTR_TWS:
		sr->tws = strdup(val);
		break;
	default:
		break;
	}
	return;
}

static void
proc_TWSXML_attr(__ctx_t ctx, const char *attr, const char *UNUSED(val))
{
	const char *rattr = tag_massage(attr);
	tws_xml_aid_t aid;

	if (UNLIKELY(rattr > attr && !ptx_pref_p(ctx, attr, rattr - attr))) {
		aid = __tx_aid_from_attr_l(attr, rattr - attr - 1);
	} else {
		aid = sax_tx_aid_from_attr(rattr);
	}

	switch (aid) {
	case TX_ATTR_XMLNS:
		/* should have been initted already */
		break;
	default:
		TX_DEBUG("WARN: unknown attr %s\n", attr);
		break;
	}
	return;
}


void
sax_bo_TWSXML_elt(__ctx_t ctx, const char *elem, const char **attr)
{
	const tws_xml_tid_t tid = sax_tx_tid_from_tag(elem);

	/* all the stuff that needs a new sax handler */
	switch (tid) {
	case TX_TAG_TWSXML:
		if (UNLIKELY(attr == NULL)) {
			break;
		}

		for (const char **ap = attr; ap && *ap; ap += 2) {
			proc_TWSXML_attr(ctx, ap[0], ap[1]);
		}
		break;

	case TX_TAG_REQUEST:
		if (LIKELY(ctx->next != NULL)) {
			void *sr = calloc(1, sizeof(*ctx->next));
			ctx->next->next = (tws_sreq_t)sr;
			ctx->next = (struct tws_sreq_s*)sr;
		} else {
			void *sr = calloc(1, sizeof(*ctx->next));
			ctx->next = (struct tws_sreq_s*)sr;
			ctx->sreq = (tws_sreq_t)sr;
		}
		break;

	case TX_TAG_QMETA:
		/* get all them metas */
		for (const char **ap = attr; ap && *ap; ap += 2) {
			const tws_xml_aid_t aid = check_tx_attr(ctx, ap[0]);

			proc_QMETA_attr(
				ctx->next, TX_NS_TWSXML_0_1, aid, ap[1]);
		}
		break;

	case TX_TAG_QUERY:
	case TX_TAG_RESPONSE:
		break;

	case TX_TAG_REQCONTRACT: {
		tws_cont_t ins = (tws_cont_t)new IB::Contract;

		/* get all them contract specs */
		for (const char **ap = attr; ap && *ap; ap += 2) {
			const tws_xml_aid_t aid = check_tx_attr(ctx, ap[0]);

			proc_REQCONTRACT_attr(
				ins, TX_NS_TWSXML_0_1, aid, ap[1]);
		}
		if (UNLIKELY(ctx->next->c != NULL)) {
			delete (IB::Contract*)ctx->next->c;
		}
		ctx->next->c = ins;
		break;
	}

	case TX_TAG_COMBOLEGS: {
		IB::Contract *ins = (IB::Contract*)ctx->next->c;

#if TWSAPI_IB_VERSION_NUMBER <= 966
		if (ins->comboLegs == NULL) {
			ins->comboLegs = new IB::Contract::ComboLegList();
		}
#else
		if (ins->comboLegs.get() == NULL) {
			ins->comboLegs = IB::Contract::ComboLegListSPtr(
				new IB::Contract::ComboLegList());
		}
#endif /* TWSAPI_IB_VERSION_NUMBER <= 966 */
		break;
	}

	case TX_TAG_COMBOLEG: {
		IB::Contract *ins = (IB::Contract*)ctx->next->c;
		IB::ComboLeg *leg = new IB::ComboLeg;

		/* get all them contract specs */
		for (const char **ap = attr; ap && *ap; ap += 2) {
			const tws_xml_aid_t aid = check_tx_attr(ctx, ap[0]);

			proc_COMBOLEG_attr(
				leg, TX_NS_TWSXML_0_1, aid, ap[1]);
		}
		/* and add to the leg list */
#if TWSAPI_IB_VERSION_NUMBER <= 966
		ins->comboLegs->push_back(leg);
#else
		ins->comboLegs->push_back(IB::ComboLegSPtr(leg));
#endif /* TWSAPI_IB_VERSION_NUMBER <= 966 */
		break;
	}

	default:
		break;
	}
	return;
}

void
sax_eo_TWSXML_elt(__ctx_t ctx, const char *elem)
{
	tws_xml_tid_t tid = sax_tx_tid_from_tag(elem);

	/* stuff that needed to be done, fix up state etc. */
	switch (tid) {
		/* top-levels */
	case TX_TAG_TWSXML:
		break;

	case TX_TAG_REQUEST:
		TX_DEBUG("CREQ  %p  %s", ctx->next->c, ctx->next->nick);
		break;
	case TX_TAG_QUERY:
	case TX_TAG_RESPONSE:
		break;

		/* non top-levels without children */
	case TX_TAG_REQCONTRACT:
	case TX_TAG_COMBOLEGS:
	case TX_TAG_COMBOLEG:
		break;

	default:
		break;
	}
	return;
}

static __attribute__((unused)) tws_xml_req_typ_t
parse_req_typ(const char *typ)
{
	const struct tws_xml_rtcell_s *rtc = __rtiddify(typ, strlen(typ));

	return rtc->rtid;
}
#endif  /* !SDEF_WRONLY */


/* other stuff that has no better place */
tws_cont_t
tws_dup_cont(tws_const_cont_t c)
{
	const IB::Contract *ibc = (const IB::Contract*)c;
	IB::Contract *res = new IB::Contract;

	*res = *ibc;
	if (ibc->comboLegs != NULL) {
		IB::Contract::CloneComboLegs(*res->comboLegs, *ibc->comboLegs);
	}
	return (tws_cont_t)res;
}

void
tws_free_cont(tws_cont_t c)
{
	IB::Contract *ibc = (IB::Contract*)c;

	if (ibc->comboLegs != NULL) {
#if TWSAPI_IB_VERSION_NUMBER <= 966
		IB::Contract::CleanupComboLegs(*ibc->comboLegs);
		delete ibc->comboLegs;
#endif	/* TWSAPI_IB_VERSION_NUMBER */
	}
	delete ibc;
	return;
}

tws_sdef_t
tws_dup_sdef(tws_const_sdef_t s)
{
	const IB::ContractDetails *ibs = (const IB::ContractDetails*)s;
	IB::ContractDetails *res = new IB::ContractDetails;

	*res = *ibs;
	return (tws_sdef_t)res;
}

void
tws_free_sdef(tws_sdef_t s)
{
	delete (IB::ContractDetails*)s;
	return;
}

tws_cont_t
tws_sdef_make_cont(tws_const_sdef_t x)
{
	const IB::ContractDetails *ibcd = (const IB::ContractDetails*)x;
	IB::Contract *res = new IB::Contract;

	*res = ibcd->summary;
	return (tws_cont_t)res;
}

const char*
tws_cont_nick(tws_const_cont_t cont)
{
/* our nick names look like CONID_SECTYP_EXCH */
	static char nick[64];
	const IB::Contract *c = (const IB::Contract*)cont;
	long int cid = c->conId;
	const char *sty = c->secType.c_str();
	const char *xch = c->exchange.c_str();
	const char *sym;
	const char *ccy;

	if (*xch == '\0') {
		xch = c->primaryExchange.c_str();
	}
	if ((sym = c->comboLegsDescrip.c_str()) != NULL && sym[0]) {
		goto pr_rolf_style;
	} else if (cid) {
		snprintf(nick, sizeof(nick), "%ld_%s_%s", cid, sty, xch);
	} else if ((sym = c->localSymbol.c_str()) != NULL && sym[0]) {
	pr_rolf_style:
		if (sty[0] && xch[0]) {
			snprintf(nick, sizeof(nick), "%s_%s_%s", sym, sty, xch);
		} else {
			snprintf(nick, sizeof(nick), "%s", sym);
		}
	} else if ((sym = c->symbol.c_str()) != NULL && sym[0] &&
		   (ccy = c->currency.c_str()) != NULL && ccy[0]) {
		snprintf(nick, sizeof(nick), "%s/%s_%s_%s", sym, ccy, sty, xch);
	} else {
		/* we give up */
		snprintf(nick, sizeof(nick), "p%p_%s_%s", c, sty, xch);
	}
	return nick;
}

const char*
tws_sdef_nick(tws_const_sdef_t sdef)
{
/* like tws_cont_nick() but for the contract matching sdef */
	const IB::ContractDetails *sd = (const IB::ContractDetails*)sdef;
	return tws_cont_nick((tws_const_cont_t)&sd->summary);
}

/* sdef-twsxml.cpp ends here */
