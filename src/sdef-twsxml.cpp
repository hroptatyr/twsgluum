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
#include "logger.h"
#include <twsapi/Contract.h>

#include "proto-tx-ns.h"
#include "proto-twsxml-attr.h"
#include "proto-twsxml-tag.h"
#include "proto-twsxml-reqtyp.h"
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

#include "proto-twsxml-attr.c"
#include "proto-twsxml-tag.c"
#include "proto-twsxml-reqtyp.c"

#if defined __INTEL_COMPILER
# pragma warning (default:869)
#elif defined HAD_STDC_INLINE
/* redefine the guy again */
# define __GNUC_STDC_INLINE__
#endif	/* __INTEL_COMPILER || __GNUC_STDC_INLINE__ */
#endif	/* HAVE_GPERF */


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
proc_TX_xmlns(__ctx_t ctx, const char *pref, const char *value)
{
	TX_DEBUG("reg'ging name space %s <- %s\n", pref, value);
	ptx_reg_ns(ctx, pref, value);
	return;
}

static void
proc_COMBOLEG_attr(
	tws_cont_t leg, tx_nsid_t ns, tws_xml_aid_t aid, const char *val)
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
	tws_cont_t ins, tx_nsid_t ns, tws_xml_aid_t aid, const char *val)
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
proc_TWSXML_attr(__ctx_t ctx, const char *attr, const char *value)
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
		proc_TX_xmlns(ctx, rattr == attr ? NULL : rattr, value);
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
		ptx_init(ctx);

		if (UNLIKELY(attr == NULL)) {
			break;
		}

		for (const char **ap = attr; ap && *ap; ap += 2) {
			proc_TWSXML_attr(ctx, ap[0], ap[1]);
		}
		break;

	case TX_TAG_REQUEST: {
		tx_tid_t x = {tid};
		push_state(ctx, TX_NS_TWSXML_0_1, x, NULL);
		break;
	}

	case TX_TAG_QUERY:
	case TX_TAG_RESPONSE: {
		tx_tid_t x = {tid};
		push_state(ctx, TX_NS_TWSXML_0_1, x, NULL);
		break;
	}

	case TX_TAG_REQCONTRACT: {
		tws_cont_t ins = (tws_cont_t)new IB::Contract;
		tx_tid_t x = {tid};

		/* get all them contract specs */
		for (const char **ap = attr; ap && *ap; ap += 2) {
			const tws_xml_aid_t aid = check_tx_attr(ctx, ap[0]);

			proc_REQCONTRACT_attr(
				ins, TX_NS_TWSXML_0_1, aid, ap[1]);
		}
		push_state(ctx, TX_NS_TWSXML_0_1, x, ins);
		break;
	}

	case TX_TAG_COMBOLEGS: {
		IB::Contract *ins = (IB::Contract*)get_state_object(ctx);
		tx_tid_t x = {tid};

		if (ins->comboLegs == NULL) {
			ins->comboLegs = new IB::Contract::ComboLegList();
		}

		/* combolegs should have no attrs, aye? */
		TX_DEBUG("got instrument %p\n", ins);
		push_state(ctx, TX_NS_TWSXML_0_1, x, ins->comboLegs);
		break;
	}

	case TX_TAG_COMBOLEG: {
		IB::Contract::ComboLegList* cmb =
			(IB::Contract::ComboLegList*)get_state_object(ctx);
		IB::ComboLeg *leg = new IB::ComboLeg;

		/* get all them contract specs */
		for (const char **ap = attr; ap && *ap; ap += 2) {
			const tws_xml_aid_t aid = check_tx_attr(ctx, ap[0]);

			proc_COMBOLEG_attr(
				leg, TX_NS_TWSXML_0_1, aid, ap[1]);
		}
		/* and add to the leg list */
		cmb->push_back(leg);
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
	case TX_TAG_QUERY:
	case TX_TAG_RESPONSE:
		(void)pop_state(ctx);
		break;

		/* non top-levels without children */
	case TX_TAG_REQCONTRACT: {
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

	case TX_TAG_COMBOLEGS: {
		IB::Contract::ComboLegList *cmb =
			(IB::Contract::ComboLegList*)pop_state(ctx);

		if (UNLIKELY(cmb == NULL)) {
			TX_DEBUG("no legs in a combo\n");
			break;
		}
		break;
	}

	case TX_TAG_COMBOLEG: {
		TX_DEBUG("/leg\n");
		break;
	}

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

/* sdef-twsxml.cpp ends here */
