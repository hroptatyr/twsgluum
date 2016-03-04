/*** ibfx.c -- simple multicast FX quote streamer
 *
 * Copyright (C) 2012-2016 Sebastian Freundt
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
#include <sys/utsname.h>
#include "dfp754_d32.h"
#include "dfp754_d64.h"

/* the tws api */
#include "daemonise.h"
#include "tws.h"
#include "sdef.h"
#include "nifty.h"
#include "tws-uri.h"
#include "tws-sock.h"

#if defined __INTEL_COMPILER
# pragma warning (disable:981)
#endif	/* __INTEL_COMPILER */

#define MCAST_ADDR	"ff05::134"
#define MCAST_PORT	7878

typedef _Decimal32 px_t;
typedef _Decimal64 qx_t;

#define pxtostr		d32tostr
#define qxtostr		d64tostr

typedef struct ctx_s {
	struct tws_s tws[1];
	struct tws_uri_s uri[1];

	unsigned int nsubs;
	const char *const *subs;

	int ch;
} *ctx_t;

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

typedef struct {
	unsigned int i;
	px_t b;
	px_t a;
} quo_t;

static const char *cons[100U];


#define uerror(...)	errno = 0, serror(__VA_ARGS__)

static __attribute__((format(printf, 1, 2))) void
serror(const char *fmt, ...)
{
	va_list vap;
	va_start(vap, fmt);
	vfprintf(stderr, fmt, vap);
	va_end(vap);
	if (errno) {
		fputc(':', stderr);
		fputc(' ', stderr);
		fputs(strerror(errno), stderr);
	}
	fputc('\n', stderr);
	return;
}


static int
route_quote(ctx_t ctx, quo_t q)
{
/* extract bid, ask and instrument and send to mcast channel */
	static const char suf[] = " IDEALPRO";
	char buf[256U];
	size_t len = 0U;

	/* instrument */
	len += (memcpy(buf, cons[q.i], 6U), 6U);
	len += (memcpy(buf + 6, suf, strlenof(suf)), strlenof(suf));
	buf[len++] = '\t';

	if (LIKELY(q.b)) {
		len += pxtostr(buf + len, sizeof(buf) - len, q.b);
	}
	buf[len++] = '\t';
	if (LIKELY(q.a)) {
		len += pxtostr(buf + len, sizeof(buf) - len, q.a);
	}
	/* and finalise */
	buf[len++] = '\n';
	buf[len] = '\0';

	send(ctx->ch, buf, len, 0);
	return 0;
}


/* tws interaction */
static void
infra_cb(tws_t UNUSED(tws), tws_cb_t what, struct tws_infra_clo_s clo)
{
/* called from tws api for infra messages */
	switch (what) {
	case TWS_CB_INFRA_ERROR:
		uerror("NFRA  oid %u  code %u: %s",
			clo.oid, clo.code, (const char*)clo.data);
		break;
	case TWS_CB_INFRA_CONN_CLOSED:
		uerror("NFRA  connection closed");
		break;
	case TWS_CB_INFRA_READY:
		uerror("NFRA  RDY");
		break;
	default:
		uerror("NFRA  what %u  oid %u  code %u  data %p",
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
	case TWS_CB_PRE_PRICE: {
		quo_t q = {
			.i = clo.oid,
			.b = clo.tt == 1 ? (px_t)clo.val : 0.df,
			.a = clo.tt == 2 ? (px_t)clo.val : 0.df,
		};
		route_quote((ctx_t)tws, q);
		break;
	}

	case TWS_CB_PRE_SIZE:
	case TWS_CB_PRE_CONT_DTL:
	case TWS_CB_PRE_CONT_DTL_END:
		break;

	default:
		uerror("FUCK  PRE  what %u  oid %u  tt %u  data %p",
			what, clo.oid, clo.tt, clo.data);
		break;
	}
	return;
}

static int
subs(tws_t tws, const char *sub)
{
	char ccy[8U];

	memcpy(ccy, sub, 3U);
	ccy[3U] = '\0';
	memcpy(ccy + 4U, sub + 3U, 3U);
	ccy[7U] = '\0';

	with (tws_cont_t con = tws_sdef_make_cash(ccy, ccy + 4U)) {
		tws_oid_t i;

		if (UNLIKELY(con == NULL)) {
			uerror("\
Error: cannot create contract from %s", sub);
			return -1;
		} else if (!(i = tws_sub_quo_cont(tws, con))) {
			uerror("\
Error: cannot subscribe to quotes of %s", sub);
			return -1;
		}
		/* otherwise everything's tickety-boo */
		cons[i] = sub;
		tws_free_cont(con);
	}
	return 0;
}

static void
twsc_conn_clos(ctx_t ctx)
{
	/* just chuck the whole tws object */
	(void)fini_tws(ctx->tws);
	return;
}


/* socket goodies */
#include "tws-sock.c"
#include "mc6-sock.c"
#include "tws-uri.c"


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
twsc_cb(EV_P_ ev_io w[static 1], int rev)
{
	static char noop[1];
	ctx_t ctx = w->data;

	if ((rev & EV_CUSTOM)/*hangup requested*/ ||
	    recv(w->fd, noop, sizeof(noop), MSG_PEEK) <= 0) {
		/* perform some clean up work on our data */
		if (LIKELY(ctx != NULL)) {
			twsc_conn_clos(ctx);
		}
		/* uh oh */
		ev_io_shut(EV_A_ w);
		w->fd = -1;
		w->data = NULL;
		/* we should set a timer here for retrying */
		uerror("AXAX  scheduling reconnect\n");
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
	int cli;

	/* going down? */
	if (UNLIKELY(w == NULL)) {
		uerror("FINI  %d", twsc->fd);
		/* call that twsc watcher manually to free subq resources
		 * mainloop isn't running any more */
		twsc_cb(EV_A_ twsc, EV_CUSTOM);
		return;
	}

	switch ((ctx = w->data, tws_state(ctx->tws))) {
	case TWS_ST_SUP:
	case TWS_ST_RDY:
	default:
		/* connection is up and running */
		return;
	case TWS_ST_UNK:
	case TWS_ST_DWN:
		break;
	}

	/* otherwise proceed with the (re)connect */
	if ((s = make_tws_sock(ctx->uri)) < 0) {
		serror("tws connection setup failed");
		return;
	}

	uerror("CONN  %d", s);
	twsc->data = ctx;
	ev_io_init(twsc, twsc_cb, s, EV_READ);
	ev_io_start(EV_A_ twsc);

	if (UNLIKELY((cli = ctx->uri->cli) <= 0)) {
		cli = time(NULL);
	}
	if (UNLIKELY(init_tws(ctx->tws, s, cli) < 0)) {
		uerror("DOWN  %d", s);
		ev_io_shut(EV_A_ twsc);
		return;
	}
	return;
}

static void
prep_cb(EV_P_ ev_prepare *w, int UNUSED(revents))
{
	static ev_timer reco[1];
	static tws_st_t old_st;
	ctx_t ctx = w->data;
	tws_st_t st;

	st = tws_state(ctx->tws);
	switch (st) {
	case TWS_ST_UNK:
		if (reco->data == NULL) {
			ev_timer_init(reco, reco_cb, 0.0, 2.0/*option?*/);
			ev_timer_start(EV_A_ reco);
			reco->data = ctx;
		}
	case TWS_ST_DWN:
		/* let the reco timer do the work */
		break;

	case TWS_ST_RDY:
		if (old_st != TWS_ST_RDY) {
			uerror("SUBS");
			for (unsigned int i = 0; i < ctx->nsubs; i++) {
				subs(ctx->tws, ctx->subs[i]);
			}
		}
	case TWS_ST_SUP:
		/* former ST_RDY/ST_SUP case pair in chck_cb() */
		tws_send(ctx->tws);
		break;
	default:
		uerror("unknown state: %u", tws_state(ctx->tws));
		abort();
	}

	old_st = st;
	return;
}

static void
sigint_cb(EV_P_ ev_signal *UNUSED(w), int UNUSED(revents))
{
	ev_unloop(EV_A_ EVUNLOOP_ALL);
	return;
}


#include "ibfx.yucc"

int
main(int argc, char *argv[])
{
	static yuck_t argi[1U];
	static struct ctx_s ctx[1U];
	EV_P = ev_default_loop(EVFLAG_AUTO);
	ev_prepare prep[1U];
	ev_signal sigint_watcher[1];
	ev_signal sigterm_watcher[1];
	int rc = 0;

	if (yuck_parse(argi, argc, argv) < 0) {
		rc = 1;
		goto out;
	}

	/* snarf host name and port */
	if (argi->tws_arg) {
		*ctx->uri = make_uri(argi->tws_arg);
	} else {
		*ctx->uri = make_uri("localhost");
	}

	/* make sure we know where to find the subscription files */
	ctx->nsubs = argi->nargs;
	ctx->subs = argi->args;

	/* and just before we're entering that REPL check for daemonisation */
	if (argi->daemonise_flag && detach() < 0) {
		serror("\
Error: cannot run in daemon mode");
		rc = 1;
		goto out;
	}

	/* open multicast channel */
	if (UNLIKELY((ctx->ch = mc6_socket()) < 0)) {
		serror("\
Error: cannot create multicast socket");
		rc = 1;
		goto out;
	} else if (mc6_set_pub(ctx->ch, MCAST_ADDR, MCAST_PORT, NULL) < 0) {
		serror("\
Error: cannot activate publishing mode on socket %d", ctx->ch);
		rc = 1;
		goto out;
	}

	ev_signal_init(sigint_watcher, sigint_cb, SIGINT);
	ev_signal_start(EV_A_ sigint_watcher);
	ev_signal_init(sigterm_watcher, sigint_cb, SIGTERM);
	ev_signal_start(EV_A_ sigterm_watcher);

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
	ev_signal_stop(EV_A_ sigint_watcher);
	ev_signal_stop(EV_A_ sigterm_watcher);
	ev_prepare_stop(EV_A_ prep);

	/* propagate tws shutdown and resource freeing */
	reco_cb(EV_A_ NULL, EV_CUSTOM | EV_CLEANUP);

out:
	/* kill mcast channel */
	mc6_unset_pub(ctx->ch);

	/* destroy the default evloop */
	ev_default_destroy();
	yuck_free(argi);
	return rc;
}

/* ibfx.c ends here */
