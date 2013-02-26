/*** quo-tws.c -- quotes and trades from tws
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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
/* for gmtime_r */
#include <time.h>
/* for gettimeofday() */
#include <sys/time.h>
/* for mmap */
#include <sys/mman.h>
#if defined HAVE_EV_H
# include <ev.h>
# undef EV_P
# define EV_P  struct ev_loop *loop __attribute__((unused))
#endif	/* HAVE_EV_H */
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <unserding/unserding.h>
#if defined HAVE_UTERUS_UTERUS_H
# include <uterus/uterus.h>
# include <uterus/m30.h>
#elif defined HAVE_UTERUS_H
# include <uterus.h>
# include <m30.h>
#else
# error uterus headers are mandatory
#endif	/* HAVE_UTERUS_UTERUS_H || HAVE_UTERUS_H */

#include <svc-uterus.h>

/* the tws api */
#include "quo-tws.h"
#include "quo.h"
#include "sub.h"
#include "logger.h"
#include "daemonise.h"
#include "tws.h"
#include "sdef.h"
#include "websvc.h"
#include "nifty.h"
#include "ud-sock.h"

#if defined __INTEL_COMPILER
# pragma warning (disable:981)
#endif	/* __INTEL_COMPILER */

#if defined DEBUG_FLAG && !defined BENCHMARK
# include <assert.h>
# define QUO_DEBUG(args...)	fprintf(logerr, args)
# define MAYBE_NOINLINE		__attribute__((noinline))
#else  /* !DEBUG_FLAG */
# define QUO_DEBUG(args...)
# define assert(x)
# define MAYBE_NOINLINE
#endif	/* DEBUG_FLAG */

typedef struct ctx_s *ctx_t;

struct ctx_s {
	struct tws_s tws[1];

	/* static context */
	const char *host;
	uint16_t port;
	int client;

	unsigned int nsubf;
	const char *const *subf;

	/* quote queue */
	quoq_t qq;
	/* subscription queue */
	subq_t sq;

	ud_sock_t beef;
};


/* sock helpers, should be somwhere else */
static int
make_dccp(void)
{
	int s;

	if ((s = socket(AF_INET6, SOCK_DCCP, IPPROTO_DCCP)) < 0) {
		return s;
	}
	/* mark the address as reusable */
	setsock_reuseaddr(s);
	/* impose a sockopt service, we just use 1 for now */
	set_dccp_service(s, 1);
	/* make a timeout for the accept call below */
	setsock_rcvtimeo(s, 1000);
	/* make socket non blocking */
	setsock_nonblock(s);
	/* turn off nagle'ing of data */
	setsock_nodelay(s);
	return s;
}

static int
make_tcp(void)
{
	int s;

	if ((s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return s;
	}
	/* reuse addr in case we quickly need to turn the server off and on */
	setsock_reuseaddr(s);
	/* turn lingering on */
	setsock_linger(s, 1);
	return s;
}

static int
sock_listener(int s, struct sockaddr_in6 *sa)
{
	if (s < 0) {
		return s;
	}

	if (bind(s, (struct sockaddr*)sa, sizeof(*sa)) < 0) {
		return -1;
	}

	return listen(s, MAX_DCCP_CONNECTION_BACK_LOG);
}

static int
tws_sock(const char *host, short unsigned int port)
{
	static char pstr[32];
	struct addrinfo *aires;
	struct addrinfo hints = {0};
	int s = -1;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
#if defined AI_ADDRCONFIG
	hints.ai_flags |= AI_ADDRCONFIG;
#endif	/* AI_ADDRCONFIG */
#if defined AI_V4MAPPED
	hints.ai_flags |= AI_V4MAPPED;
#endif	/* AI_V4MAPPED */
	hints.ai_protocol = 0;

	/* port number as string */
	snprintf(pstr, sizeof(pstr) - 1, "%hu", port);

	if (getaddrinfo(host, pstr, &hints, &aires) < 0) {
		goto out;
	}
	/* now try them all */
	for (const struct addrinfo *ai = aires;
	     ai != NULL &&
		     ((s = socket(ai->ai_family, ai->ai_socktype, 0)) < 0 ||
		      connect(s, ai->ai_addr, ai->ai_addrlen) < 0);
	     close(s), s = -1, ai = ai->ai_next);

out:
	freeaddrinfo(aires);
	return s;
}


/* unserding goodies */
#define BRAG_INTV	(10)

/* looks like dccp://host:port/secdef?idx=00000 */
static char brag_uri[INET6_ADDRSTRLEN];
/* offset nto brag_uris idx= field */
static size_t brag_uri_offset = 0;

static int
make_brag_uri(int sock, struct sockaddr_in6 *sa, socklen_t sz)
{
	struct utsname uts[1];
	char dnsdom[64];
	char *curs;
	size_t rest;
	int proto;
	uint16_t p;

	if (uname(uts) < 0) {
		return -1;
	} else if (getdomainname(dnsdom, sizeof(dnsdom)) < 0) {
		return -1;
	} else if (UNLIKELY(sz < sizeof(struct sockaddr_in6))) {
		return -1;
	} else if ((proto = getsock_proto(sock)) < 0) {
		return -1;
	}

	switch (proto) {
		static char http_designator[] = "http://";
		static char dccp_designator[] = "dccp://";
	case IPPROTO_DCCP:
	case 0:
	default:
		memcpy(brag_uri, dccp_designator, sizeof(dccp_designator));
		curs = brag_uri + sizeof(dccp_designator) - 1;
		rest = sizeof(brag_uri) - sizeof(dccp_designator);
		break;
	case IPPROTO_TCP:
		memcpy(brag_uri, http_designator, sizeof(http_designator));
		curs = brag_uri + sizeof(http_designator) - 1;
		rest = sizeof(brag_uri) - sizeof(http_designator);
		break;
	}

	p = ntohs(sa->sin6_port);
	curs += snprintf(
		curs, rest, "%s.%s:%hu/secdef?idx=",
		uts->nodename, dnsdom, p);

	brag_uri_offset = curs - brag_uri;
	QUO_DEBUG("ADVN  %s\n", brag_uri);
	return 0;
}

static int
brag(ctx_t ctx, sub_t sub)
{
	struct um_qmeta_s brg;
	const char *sym = sub->nick;
	size_t syz = strlen(sym);
	size_t brag_urz = brag_uri_offset;

	/* put stuff in our uri */
	brag_urz += snprintf(
		brag_uri + brag_uri_offset, sizeof(brag_uri) - brag_uri_offset,
		"%u", sub->uidx);
	/* pack the qmeta message */
	brg.idx = sub->uidx;
	brg.sym = sym;
	brg.symlen = syz;
	brg.uri = brag_uri;
	brg.urilen = brag_urz;
	um_pack_brag(ctx->beef, &brg);
	return 0;
}

static inline unsigned int
q30_sl1t_typ(quo_typ_t qtyp)
{
/* q's typ slot was designed to coincide with ute's sl1t types */
	return qtyp / 2 + SL1T_TTF_BID;
}

struct flush_clo_s {
	ctx_t ctx;
	struct sl1t_s l1t[1];
};

static void
qq_flush_cb(struct quoq_cb_asp_s asp, struct quo_s q, struct flush_clo_s *clo)
{
	ctx_t ctx = clo->ctx;
	sl1t_t l1t = clo->l1t;
	/* try and find the sdef for this quote */
	uint32_t idx = q.idx;
	sub_t sub = subq_find_idx(ctx->sq, idx);

	assert(asp.type == QUOQ_CB_FLUSH);

	if (UNLIKELY(sub == NULL)) {
		/* probably unsub'd, fuck the quote */
		return;
	}

	{
	/* check if it's time to brag about this thing again */
		uint32_t now = sl1t_stmp_sec(l1t);

		if (UNLIKELY(now - sub->last_dsm >= BRAG_INTV)) {
			brag(clo->ctx, sub);
			/* update sub */
			sub->last_dsm = now;
		}
	}

	/* fill in the rest of the l1t info */
	sl1t_set_tblidx(l1t, (uint16_t)sub->uidx);
	sl1t_set_ttf(l1t, (uint16_t)q30_sl1t_typ(q.typ));
	l1t->pri = q.p;
	l1t->qty = q.q;
	/* stuff him up the socket's sleeve */
	um_pack_sl1t(ctx->beef, l1t);
	return;
}

static void
quoq_flush_maybe(ctx_t ctx)
{
	struct flush_clo_s clo = {
		.ctx = ctx,
	};

	/* populate l1t somewhat */
	{
		struct timeval now[1];

		gettimeofday(now, NULL);
		sl1t_set_stmp_sec(clo.l1t, now->tv_sec);
		sl1t_set_stmp_msec(clo.l1t, now->tv_usec / 1000);
	}

	/* get the packet ctor'd */
	quoq_flush_cb(ctx->qq, (void(*)())qq_flush_cb, &clo);

	/* make sure nothing gets buffered */
	ud_flush(ctx->beef);
	return;
}


/* tws interaction */
static void
infra_cb(tws_t UNUSED(tws), tws_cb_t what, struct tws_infra_clo_s clo)
{
/* called from tws api for infra messages */
	switch (what) {
	case TWS_CB_INFRA_ERROR:
		QUO_DEBUG("NFRA  oid %u  code %u: %s\n",
			clo.oid, clo.code, (const char*)clo.data);
		break;
	case TWS_CB_INFRA_CONN_CLOSED:
		QUO_DEBUG("NFRA  connection closed\n");
		break;
	case TWS_CB_INFRA_READY:
		QUO_DEBUG("NFRA  RDY\n");
		break;
	default:
		QUO_DEBUG("NFRA  what %u  oid %u  code %u  data %p\n",
			what, clo.oid, clo.code, clo.data);
		break;
	}
	return;
}

static void
pre_cb(tws_t tws, tws_cb_t what, struct tws_pre_clo_s clo)
{
/* called from tws api for pre messages */

	switch (what) {
	case TWS_CB_PRE_PRICE:
	case TWS_CB_PRE_SIZE: {
		struct quo_s q;
		m30_t v;

		switch (clo.tt) {
			/* hardcoded non-sense here!!! */
		case 0:		/* size */
			q.typ = QUO_TYP_BSZ;
			break;
		case 1:		/* price */
		case 9:		/* price */
		case 8:		/* size */
			q.typ = (quo_typ_t)(clo.tt - 1);
			break;
		case 2:		/* price */
		case 4:		/* price */
		case 3:		/* size */
		case 5:		/* size */
			q.typ = (quo_typ_t)clo.tt;
			break;
		default:
			goto fucked;
		}
		v = ffff_m30_get_d(clo.val);
		q.idx = clo.oid;
		if (q.subtyp == 0U/*means price*/) {
			q.p = v.u;
		} else {
			q.q = v.u;
		}
		QUO_DEBUG("TICK  what %u  oid %u  tt %u  data %p\n",
			what, clo.oid, clo.tt, clo.data);
		quoq_add(((ctx_t)tws)->qq, q);
		break;
	}

	case TWS_CB_PRE_CONT_DTL:
		QUO_DEBUG("SDEF  %u %p\n", clo.oid, clo.data);
		if (clo.oid && clo.data) {
			uint32_t idx = tws_sub_quo(tws, clo.data);
			tws_sdef_t sdef = tws_dup_sdef(clo.data);
			const char *nick = tws_sdef_nick(sdef);
			sub_t s = subq_find_sreq(((ctx_t)tws)->sq, clo.oid);

			/* there should be one on the queue */
			if (UNLIKELY(s == NULL || s->sdef != NULL)) {
				/* big bugger :|, unsub? */
				struct sub_s t = {
					.idx = idx,
					.sdef = sdef,
					.sreq = clo.oid,
					.nick = strdup(nick),
				};
				subq_add(((ctx_t)tws)->sq, t);
			} else {
				/* unsub that old guy, sub the new one */
				QUO_DEBUG("USUB  %u rplcs %u\n", idx, s->idx);
				tws_rem_quo(tws, s->idx);
				s->idx = idx;
				/* change sdef slot (for the better?) */
				if (UNLIKELY(s->sdef != NULL)) {
					/* who put that there? */
					tws_free_sdef(s->sdef);
				}
				s->sdef = sdef;
				/* chuck out the old nick */
				if (LIKELY(s->nick != NULL)) {
					free(s->nick);
				}
				s->nick = strdup(nick);
				/* also, no need to add him again */
				;
			}
		}
		break;
	case TWS_CB_PRE_CONT_DTL_END:
		break;

	default:
	fucked:
		QUO_DEBUG("FUCK  PRE  what %u  oid %u  tt %u  data %p\n",
			what, clo.oid, clo.tt, clo.data);
		break;
	}
	return;
}

static int
__sub_sdef(tws_t tws, tws_sreq_t sr)
{
/* subscribe to INS
 * we actually subscribe instruments twice, once here using the
 * instrument specified in INS and then we request security definitions
 * and upon a successful definition response we subscribe
 * those responses again */
	struct sub_s s;
	tws_cont_t ins = sr->c;

	QUO_DEBUG("SUBC  %p\n", ins);
	if (!(s.idx = tws_sub_quo_cont(tws, ins))) {
		logger("cannot subscribe to quotes of %p", ins);
	} else if (!(s.sreq = tws_req_sdef(tws, ins))) {
		logger("cannot acquire secdefs of %p", ins);
	}

	/* fill in sub for our tracking */
	s.sdef = NULL;
	s.nick = strdup(sr->nick);
	subq_add(((ctx_t)tws)->sq, s);

	/* just to have some more juice to work with */
	QUO_DEBUG("SUBC  NICK %s  SQUO %u  SREQ %u\n", s.nick, s.idx, s.sreq);

	/* indicate that we don't need INS any longer */
	return 1;
}

static void
init_subs(tws_t tws, const char *file)
{
#define PR	(PROT_READ)
#define MS	(MAP_SHARED)
	void *fp;
	int fd;
	struct stat st;
	ssize_t fsz;

	if (stat(file, &st) < 0 || (fsz = st.st_size) < 0) {
		error(errno, "subscription file %s invalid", file);
	} else if ((fd = open(file, O_RDONLY)) < 0) {
		error(errno, "cannot read subscription file %s", file);
	} else if ((fp = mmap(NULL, fsz, PR, MS, fd, 0)) == MAP_FAILED) {
		error(errno, "cannot read subscription file %s", file);
	} else {
		tws_sreq_t sr;

		QUO_DEBUG("SUBS\n");
		sr = tws_deser_sreq(fp, fsz);

		for (tws_sreq_t s = sr; s; s = s->next) {
			__sub_sdef(tws, s);
		}

		tws_free_sreq(sr);
	}
	return;
}

static void
sq_chuck_cb(struct sub_s s, void *UNUSED(clo))
{
	tws_free_sdef(s.sdef);
	free(s.nick);
	return;
}

static void
twsc_conn_clos(ctx_t ctx)
{
	/* get rid of our subscription queue */
	subq_flush_cb(ctx->sq, sq_chuck_cb, NULL);
	/* lastly just chuck the whole tws object */
	(void)fini_tws(ctx->tws);
	return;
}


struct dccp_conn_s {
	ev_io io[1];
	union {
		struct sockaddr sa[1];
		struct sockaddr_storage ss[1];
	};
	socklen_t sz[1];
};

static inline void
shut_sock(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
	return;
}

static void
ev_io_shut(EV_P_ ev_io w[static 1])
{
	ev_io_stop(EV_A_ w);
	shut_sock(w->fd);
	w->fd = -1;
	return;
}

static void
dccp_data_cb(EV_P_ ev_io *w, int UNUSED(re))
{
	/* the final \n will be subst'd later on */
	static const char hdr[] = "\
HTTP/1.1 200 OK\r\n\
Server: quo-tws\r\n\
Content-Length: % 5zu\r\n\
Content-Type: text/xml\r\n\
\r";
	/* hdr is a format string and hdr_len is as wide as the result printed
	 * later on */
	static const size_t hdr_len = sizeof(hdr);
	/* buffer, used for logging, input and output */
        static char buf[4096];
	struct dccp_conn_s *cdata = (void*)w;
	ctx_t ctx = w->data;
        /* the address in human readable form */
        const char *a;
        /* the port (in host-byte order) */
        uint16_t p;
	ssize_t nrd;
	char *rsp = buf + hdr_len;
	const size_t rsp_len = sizeof(buf) - hdr_len;
	size_t cont_len;
	struct websvc_s voodoo;

        /* obtain the address in human readable form */
        {
		const struct sockaddr_in6 *s6 = (const void*)cdata->ss;
		const struct in6_addr *ap = &s6->sin6_addr;

		a = inet_ntop(s6->sin6_family, ap, buf, sizeof(buf));
		p = ntohs(s6->sin6_port);
        }

	logger("DCCP  %d  from [%s]:%d", w->fd, a, p);

	if ((nrd = recv(w->fd, buf, sizeof(buf), 0)) <= 0) {
		goto clo;
	} else if ((size_t)nrd < sizeof(buf)) {
		buf[nrd] = '\0';
	} else {
		/* uh oh mega request wtf? */
		buf[sizeof(buf) - 1] = '\0';
	}

	switch (websvc_from_request(&voodoo, buf, nrd)) {
	default:
	case WEBSVC_F_UNK:
		goto clo;

	case WEBSVC_F_SECDEF:
		cont_len = websvc_secdef(rsp, rsp_len, ctx->sq, voodoo);
		break;
	}

	/* prepare the header */
	(void)snprintf(buf, sizeof(buf), hdr, cont_len);
	buf[hdr_len - 1] = '\n';

	/* and append the actual contents */
	send(w->fd, buf, hdr_len + cont_len, 0);

	/* leave the connection open */
	return;
clo:
	ev_io_shut(EV_A_ w);
	return;
}

static void
twsc_cb(EV_P_ ev_io w[static 1], int rev)
{
	static char noop[1];
	ctx_t ctx = w->data;

	QUO_DEBUG("BANG  %x\n", rev);
	if ((rev & EV_CUSTOM)/*hangup requested*/ ||
	    recv(w->fd, noop, sizeof(noop), MSG_PEEK) <= 0) {
		/* perform some clean up work on our data */
		twsc_conn_clos(ctx);
		/* uh oh */
		ev_io_shut(EV_A_ w);
		w->fd = -1;
		w->data = NULL;
		/* we should set a timer here for retrying */
		QUO_DEBUG("AXAX  scheduling reconnect\n");
		return;
	}
	/* otherwise go ahead and read things */
	(void)tws_recv(ctx->tws);
	return;
}

static void
reco_cb(EV_P_ ev_timer *w, int UNUSED(revents))
{
/* reconnect to the tws service, socket level */
	static ev_io twsc[1];
	ctx_t ctx;
	int s;

	/* going down? */
	if (UNLIKELY(w == NULL)) {
		QUO_DEBUG("FINI  %d\n", twsc->fd);
		/* call that twsc watcher manually to free subq resources
		 * mainloop isn't running any more */
		twsc_cb(EV_A_ twsc, EV_CUSTOM);
		return;
	}
	/* otherwise proceed normally */
	ctx = w->data;
	if ((s = tws_sock(ctx->host, ctx->port)) < 0) {
		error(errno, "tws connection setup failed");
		return;
	}

	QUO_DEBUG("CONN  %d\n", s);
	twsc->data = ctx;
	ev_io_init(twsc, twsc_cb, s, EV_READ);
	ev_io_start(EV_A_ twsc);

	if (UNLIKELY(init_tws(ctx->tws, s, ctx->client) < 0)) {
		QUO_DEBUG("DOWN  %d\n", s);
		ev_io_shut(EV_A_ twsc);
		return;
	}
	/* and lastly, stop ourselves */
	ev_timer_stop(EV_A_ w);
	w->data = NULL;
	return;
}

static void
prep_cb(EV_P_ ev_prepare *w, int UNUSED(revents))
{
	static ev_timer reco[1];
	static tws_st_t old_st;
	ctx_t ctx = w->data;
	tws_st_t st;

	QUO_DEBUG("PREP\n");

	st = tws_state(ctx->tws);
	QUO_DEBUG("STAT  %u\n", st);
	switch (st) {
	case TWS_ST_UNK:
	case TWS_ST_DWN:
		/* is there a timer already? */
		if (reco->data == NULL) {
			/* start the reconnection timer */
			reco->data = ctx;
			ev_timer_init(reco, reco_cb, 0.0, 2.0/*option?*/);
			ev_timer_start(EV_A_ reco);
			QUO_DEBUG("RECO\n");
		}
		break;
	case TWS_ST_RDY:
		if (old_st != TWS_ST_RDY) {
			for (unsigned int i = 0; i < ctx->nsubf; i++) {
				init_subs(ctx->tws, ctx->subf[i]);
			}
		}
	case TWS_ST_SUP:
		/* former ST_RDY/ST_SUP case pair in chck_cb() */
		tws_send(ctx->tws);
		/* and flush the queue */
		quoq_flush_maybe(ctx);
		break;
	default:
		QUO_DEBUG("unknown state: %u\n", tws_state(ctx->tws));
		abort();
	}

	old_st = st;
	return;
}

static void
dccp_cb(EV_P_ ev_io *w, int UNUSED(re))
{
	static struct dccp_conn_s conns[8];
	static size_t next = 0;
	ctx_t ctx = w->data;
	int s;

	/* going down? */
	if (UNLIKELY(w == NULL)) {
		for (size_t i = 0; i < countof(conns); i++) {
			if (conns[i].io->fd > 0) {
				QUO_DEBUG("FINI  dccp/%d\n", conns[i].io->fd);
				ev_io_shut(EV_A_ conns[i].io);
			}
		}
		return;
	}

	QUO_DEBUG("DCCP  %d\n", w->fd);

	/* make way for this request */
	if (conns[next].io->fd > 0) {
		ev_io_shut(EV_A_ conns[next].io);
	}

	conns[next].sz[0] = sizeof(*conns[next].ss);
	if ((s = accept(w->fd, conns[next].sa, conns[next].sz)) < 0) {
		return;
	}

	conns[next].io->data = ctx;
	ev_io_init(conns[next].io, dccp_data_cb, s, EV_READ);
	ev_io_start(EV_A_ conns[next].io);
	if (++next >= countof(conns)) {
		next = 0;
	}
	return;
}

static void
beef_cb(EV_P_ ev_io *w, int UNUSED(revents))
{
/* should not be called at all */
	static char junk[1];

	(void)recv(w->fd, junk, sizeof(junk), MSG_TRUNC);
	QUO_DEBUG("JUNK\n");
	return;
}

static void
sighup_cb(EV_P_ ev_signal *UNUSED(w), int UNUSED(revents))
{
	QUO_DEBUG("HUP!\n");
	/* just act as though we're going down */
	reco_cb(EV_A_ NULL, EV_CUSTOM | EV_CLEANUP);
	/* HUP the logfile */
	rotate_logerr();
	return;
}

static void
sigusr1_cb(EV_P_ ev_signal *UNUSED(w), int UNUSED(revents))
{
/* for log rotation only */
	QUO_DEBUG("USR1\n");
	/* HUP the logfile */
	rotate_logerr();
	return;
}

static void
sigall_cb(EV_P_ ev_signal *UNUSED(w), int UNUSED(revents))
{
	ev_unloop(EV_A_ EVUNLOOP_ALL);
	QUO_DEBUG("UNLO\n");
	return;
}


#if defined __INTEL_COMPILER
# pragma warning (disable:593)
# pragma warning (disable:181)
#elif defined __GNUC__
# pragma GCC diagnostic ignored "-Wswitch"
# pragma GCC diagnostic ignored "-Wswitch-enum"
#endif /* __INTEL_COMPILER */
#include "quo-tws-clo.h"
#include "quo-tws-clo.c"
#if defined __INTEL_COMPILER
# pragma warning (default:593)
# pragma warning (default:181)
#elif defined __GNUC__
# pragma GCC diagnostic warning "-Wswitch"
# pragma GCC diagnostic warning "-Wswitch-enum"
#endif	/* __INTEL_COMPILER */

int
main(int argc, char *argv[])
{
	struct ctx_s ctx[1] = {{0}};
	/* args */
	struct quo_args_info argi[1];
	/* use the default event loop unless you have special needs */
	struct ev_loop *loop;
	/* ev goodies */
	ev_signal sigint_watcher[1];
	ev_signal sighup_watcher[1];
	ev_signal sigusr1_watcher[1];
	ev_signal sigterm_watcher[1];
	ev_prepare prep[1];
	ev_io ctrl[1];
	ev_io dccp[2];
	/* final result */
	int res = 0;

	/* big assignment for logging purposes */
	logerr = stderr;

	/* parse the command line */
	if (quo_parser(argc, argv, argi)) {
		res = 1;
		goto out;
	}

	/* snarf host name and port */
	if (argi->tws_host_given) {
		ctx->host = argi->tws_host_arg;
	} else {
		ctx->host = "localhost";
	}
	if (argi->tws_port_given) {
		ctx->port = (uint16_t)argi->tws_port_arg;
	} else {
		ctx->port = (uint16_t)7474;
	}
	if (argi->tws_client_id_given) {
		ctx->client = argi->tws_client_id_arg;
	} else {
		struct timeval now[1];

		(void)gettimeofday(now, NULL);
		ctx->client = now->tv_sec;
	}

	/* make sure we know where to find the subscription files */
	ctx->nsubf = argi->inputs_num;
	ctx->subf = argi->inputs;

	/* and just before we're entering that REPL check for daemonisation */
	if (argi->daemonise_given && detach() < 0) {
		perror("daemonisation failed");
		res = 1;
		goto out;
	} else if (argi->log_given && open_logerr(argi->log_arg) < 0) {
		perror("cannot open log file");
		res = 1;
		goto out;
	}

	/* initialise the main loop */
	loop = ev_default_loop(EVFLAG_AUTO);

	/* initialise a sig C-c handler */
	ev_signal_init(sigint_watcher, sigall_cb, SIGINT);
	ev_signal_start(EV_A_ sigint_watcher);
	ev_signal_init(sigterm_watcher, sigall_cb, SIGTERM);
	ev_signal_start(EV_A_ sigterm_watcher);
	ev_signal_init(sighup_watcher, sighup_cb, SIGHUP);
	ev_signal_start(EV_A_ sighup_watcher);
	ev_signal_init(sigusr1_watcher, sigusr1_cb, SIGUSR1);
	ev_signal_start(EV_A_ sigusr1_watcher);

	/* attach a multicast listener */
	{
		struct ud_sockopt_s opt = {
			UD_SUB,
		};
		if ((ctrl->data = ud_socket(opt)) != NULL) {
			ud_sock_t s = ctrl->data;
			ev_io_init(ctrl, beef_cb, s->fd, EV_READ);
			ev_io_start(EV_A_ ctrl);
		}
	}
	/* go through all beef channels */
	if (argi->beef_given) {
		struct ud_sockopt_s opt = {
			UD_PUB,
			.port = (short unsigned int)argi->beef_arg,
		};
		ctx->beef = ud_socket(opt);
	}

	/* make a channel for http/dccp requests */
	{
		struct sockaddr_in6 sa = {
			.sin6_family = AF_INET6,
			.sin6_addr = in6addr_any,
			.sin6_port = 0,
		};
		socklen_t sa_len = sizeof(sa);
		int s;

		if ((s = make_dccp()) < 0) {
			/* just to indicate we have no socket */
			dccp[0].fd = -1;
		} else if (sock_listener(s, &sa) < 0) {
			/* grrr, whats wrong now */
			close(s);
			s = dccp[0].fd = -1;
		} else {
			/* everything's brilliant */
			dccp[0].data = ctx;
			ev_io_init(dccp + 0, dccp_cb, s, EV_READ);
			ev_io_start(EV_A_ dccp);

			getsockname(s, (struct sockaddr*)&sa, &sa_len);
		}

		if (s >= 0) {
			make_brag_uri(s, &sa, sa_len);
		}
	}

	{
		struct sockaddr_in6 sa = {
			.sin6_family = AF_INET6,
			.sin6_addr = in6addr_any,
			.sin6_port = 0,
		};
		socklen_t sa_len = sizeof(sa);
		int s;

		if (countof(dccp) < 2) {
			;
		} else if ((s = make_tcp()) < 0) {
			/* just to indicate we have no socket */
			dccp[1].fd = -1;
		} else if (sock_listener(s, &sa) < 0) {
			/* bugger */
			close(s);
			s = dccp[1].fd = -1;
		} else {
			/* yay */
			dccp[1].data = ctx;
			ev_io_init(dccp + 1, dccp_cb, s, EV_READ);
			ev_io_start(EV_A_ dccp + 1);

			getsockname(s, (struct sockaddr*)&sa, &sa_len);
		}

		if (s >= 0) {
			make_brag_uri(s, &sa, sa_len);
		}
	}

	/* get ourselves a quote and sub queue */
	ctx->qq = make_quoq();
	ctx->sq = make_subq();

	/* prepare the context and the tws */
	ctx->tws->pre_cb = pre_cb;
	ctx->tws->infra_cb = infra_cb;
	/* prepare for hard slavery */
	prep->data = ctx;
	ev_prepare_init(prep, prep_cb);
	ev_prepare_start(EV_A_ prep);

	/* now wait for events to arrive */
	ev_loop(EV_A_ 0);

	/* cancel them timers and stuff */
	QUO_DEBUG("FINI\n");
	ev_prepare_stop(EV_A_ prep);

	/* propagate tws shutdown and resource freeing */
	reco_cb(EV_A_ NULL, EV_CUSTOM | EV_CLEANUP);

	/* finalise quote queue */
	free_quoq(ctx->qq);
	/* and the sub queue */
	free_subq(ctx->sq);

	/* detaching beef and ctrl channels */
	if (ctx->beef) {
		ud_close(ctx->beef);
	}
	{
		ud_sock_t c = ctrl->data;

		ev_io_stop(EV_A_ ctrl);
		ud_close(c);
	}

	/* detach http/dccp */
	dccp_cb(EV_A_ NULL, 0);
	for (size_t i = 0; i < countof(dccp); i++) {
		int s = dccp[i].fd;

		if (s > 0) {
			ev_io_shut(EV_A_ dccp + i);
		}
	}

	/* destroy the default evloop */
	ev_default_destroy();
out:
	quo_parser_free(argi);
	return res;
}

/* quo-tws.c ends here */
