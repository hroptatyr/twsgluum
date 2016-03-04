/*** ibex.c -- delegate FX orders to tws
 *
 * Copyright (C) 2015-2016 Sebastian Freundt
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
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
#include <netdb.h>
#include <sys/utsname.h>
#include "dfp754_d32.h"
#include "dfp754_d64.h"

/* the tws api */
#include "daemonise.h"
#include "tws.h"
#include "tws-uri.h"
#include "tws-sock.h"
#include "nifty.h"

#if defined __INTEL_COMPILER
# pragma warning (disable:981)
#endif	/* __INTEL_COMPILER */

#define MCAST_ADDR	"ff05::134"
#define MCAST_PORT	7879

typedef _Decimal32 px_t;
typedef _Decimal64 qx_t;

#define strtopx		strtod32
#define strtoqx		strtod64
#define pxtostr		d32tostr
#define qxtostr		d64tostr

typedef struct ctx_s {
	struct tws_s tws[1U];
	struct tws_uri_s uri[1U];

	int ch;
	struct ipv6_mreq r[1U];
} *ctx_t;

typedef union __attribute__((packed, transparent_union)) {
	char full[8U];
	uintptr_t hash;
	struct {
		char base[3U];
		char sep;
		char term[3U];
		char nul;
	};
} ccy_t;

typedef uintptr_t instr_t;
#define NOT_A_INSTR	(instr_t)-1

typedef struct {
	instr_t ins;
	qx_t amt;
	px_t lp;
	px_t sl;
	char sufx;
	char tif;
#define TIF_GTC	'1'
#define TIF_GTD	'6'
#define TIF_IOC	'3'
#define TIF_FOK	'4'
} ord_t;

static const char ccyflt[] = " IDEALPRO";


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


/* trading and accounting */
static ccy_t instrs[64U];
static size_t ninstrs;
static struct {
	qx_t reco;
	px_t tp, sl;
} accs[64U];
static uint64_t pend;
static qx_t dflt_amt = 500000.dd;
static qx_t halt_amt = 500000.dd;

static ccy_t
ccy(const char str[static 6U], size_t UNUSED(len))
{
	ccy_t x;
	memcpy(x.base, str, 3U);
	x.sep = '\0';
	memcpy(x.term, str + 3U, 3U);
	x.nul = '\0';
	return x;
}

static instr_t
find_instr(const char str[static 8U])
{
	ccy_t x = ccy(str, strlenof(ccy_t));

	for (size_t i = 0U; i < ninstrs; i++) {
		if (instrs[i].hash == x.hash) {
			return i;
		}
	}
	return NOT_A_INSTR;
}

static instr_t
intern_instr(const char str[static 8U])
{
	ccy_t c = ccy(str, strlenof(ccy_t));

	for (size_t i = 0U; i < ninstrs; i++) {
		if (instrs[i].hash == c.hash) {
			return i;
		}
	}
	if (UNLIKELY(ninstrs >= countof(instrs))) {
		/* big fuck */
		return NOT_A_INSTR;
	}
	/* otherwise bang one */
	instrs[ninstrs] = c;
	accs[ninstrs].reco = 0.dd;
	accs[ninstrs].tp = accs[ninstrs].sl = 0.df;
	return ninstrs++;
}

static ccy_t
instr_ccy(instr_t ii)
{
	return instrs[ii];
}


static inline bool
is_pending_p(instr_t ii)
{
	return (pend >> ii) & 0b1U;
}

static inline void
set_pending(instr_t ii)
{
	pend |= 1ULL << ii;
	return;
}

static inline void
clr_pending(instr_t ii)
{
	pend &= ~(1ULL << ii);
	return;
}


static int
halt(void)
{
	uerror("Notice: halting everything ...");
	dflt_amt = 0.dd;
	return 0;
}

static int
resume(void)
{
	uerror("Notice: resuming operations ...");
	dflt_amt = halt_amt;
	return 0;
}

static inline  int
o(tws_t tws, ord_t o)
{
	ccy_t pair = instr_ccy(o.ins);
	struct tws_order_s ord = {
		.sym = pair.base,
		.ccy = pair.term,
		.xch = "IDEALPRO",
		.typ = "CASH",
		.tif = "IOC",
		.amt = o.amt,
		.lmt = o.lp,
	};
	char buf[1536U];
	size_t bi;

	with (struct timespec tsp) {
		clock_gettime(CLOCK_REALTIME_COARSE, &tsp);
		bi = snprintf(buf, sizeof(buf),
			"%ld.%09ld", tsp.tv_sec, tsp.tv_nsec);
	}
	buf[bi++] = '\t';
	memcpy(buf + bi + 0U, pair.base, 3U);
	memcpy(buf + bi + 3U, pair.term, 3U);
	bi += 6U;
	buf[bi++] = '\t';
	bi += qxtostr(buf + bi, sizeof(buf) - bi, o.amt);
	buf[bi++] = '\t';
	bi += pxtostr(buf + bi, sizeof(buf) - bi, o.lp);
	buf[bi++] = '\n';
	buf[bi] = '\0';
	fwrite(buf, 1, bi, stdout);
	fflush(stdout);
	return tws_order(tws, ord);
}

static int
ex(tws_t tws, char *restrict msg, size_t msz)
{
/* snarf info from exec channel and produce limit orders and brackets */
	px_t tp = 0.df;
	px_t sl = 0.df;
	ord_t ord = {};
	size_t ix;

	switch (msg[0U]) {
	case 'L'/*ONG*/:
		ix = 4U;
		break;
	case 'S'/*HORT*/:
		ix = 5U;
		break;
	case 'E'/*MERGCLOSE*/:
		ix = 10U;
		break;
	case 'C'/*ANCEL*/:
		ix = 6U;
		break;

	case 'H'/*ALT*/:
		if (0) {
			;
		} else if (!memcmp(msg + 1U, "ALT", 3U)) {
			return halt();
		} else if (!memcmp(msg + 1U, "EARTBEAT", 8U)) {
			return -1;
		}
		/*@fallthrough@*/
	case 'I'/*NFO*/:
		if (0) {
			;
		} else if (!memcmp(msg + 1U, "NFO", 3U)) {
			return -1;
		}
		/*@fallthrough@*/
	case 'R'/*ESET*/:
		if (0) {
			;
		} else if (!memcmp(msg + 1U, "ESUME", 5U)) {
			/* resume operations */
			return resume();
		}
		/*@fallthrough@*/
	default:
	bork:
		uerror("\
received BORK: %.*s", (int)(msz - 1U), msg);
		return -1;
	}

	if (UNLIKELY(msg[ix++] != '\t')) {
		goto bork;
	}
	/* next should be an instrument, always */
	if (msg[ix + 3U] == '/') {
		/* wrong instrument */
		return -1;
	} else if (UNLIKELY(memcmp(msg + ix + 6U, ccyflt, strlenof(ccyflt)))) {
		goto bork;
	}
	with (const char *instr = msg + ix) {
		msg[ix += 6U] = '\0';
		/* find him */
		if (UNLIKELY((ord.ins = intern_instr(instr)) == NOT_A_INSTR)) {
			/* i'm gonna hang myself */
			uerror("\
Warning: instrument table full, cannot execute opportunity");
			return -1;
		}
	}

	if (is_pending_p(ord.ins) && false) {
		return -1;
	}

	/* is there a limit price? */
	ix += strlenof(ccyflt);
	switch (msg[ix++]) {
		char *on;

	case '\t':
		/* could be */
		switch (msg[0U]) {
		case 'L'/*ONG*/:
			ord.amt = dflt_amt;
			break;
		case 'S'/*HORT*/:
			ord.amt = -dflt_amt;
			break;
		default:
			uerror("\
Error: bracket orders only allowed for LONG and SHORT trigger");
			return -1;
		}
		ord.lp = strtopx(msg + ix, &on);
		ord.sufx = 'O';
		ord.tif = TIF_IOC;
		if (UNLIKELY(*on != '\t' && *on != '\n')) {
			uerror("\
Error: cannot read limit price `%.*s'", (int)(on - (msg + ix)), msg + ix);
			return -1;
		} else if (UNLIKELY(*on == '\n')) {
			/* only a limit price? so be it */
			break;
		}
		/* also try and read take-profit and stop-loss prices */
		ix = (++on - msg);
		tp = strtopx(msg + ix, &on);
		if (UNLIKELY(*on != '\t')) {
			/* we need the tab */
			uerror("\
Error: cannot read take profit price `%.*s'", (int)(on - (msg + ix)), msg + ix);
			return -1;
		}
		/* and lastly the S/L */
		ix = (++on - msg);
		sl = strtopx(msg + ix, &on);
		if (UNLIKELY(*on != '\t')) {
			/* we don't care */
			;
		}
		break;

	case '\n':
		/* nope */
		switch (msg[0U]) {
		case 'L'/*ONG*/:
			ord.amt = dflt_amt;
			ord.sufx = 'O';
			break;
		case 'S'/*HORT*/:
			ord.amt = -dflt_amt;
			ord.sufx = 'O';
			break;
		case 'E'/*MERGCLOSE*/:
			if (!accs[ord.ins].reco) {
				return 0;
			}
		case 'C'/*ANCEL*/:
			ord.amt = -accs[ord.ins].reco;
			ord.sufx = 'C';
			break;
		}
		/* market */
		ord.tif = TIF_FOK;
		break;

	default:
		goto bork;
	}

	switch (msg[0U]) {
	case 'L'/*ONG*/:
	case 'S'/*HORT*/:
		ord.amt = instr_ccy(ord.ins).base[0] != 'X'
			? ord.amt
			: ord.amt > 10000.dd ? ord.amt / 10000.dd : 1.dd;
		/*@fallthrough@*/
	default:
		break;
	}

	/* send him off */
	if (UNLIKELY(o(tws, ord) < 0)) {
		return -1;
	}
	/* update accounts */
	set_pending(ord.ins);
	accs[ord.ins].tp = tp;
	accs[ord.ins].sl = sl;
	uerror("\
Notice: activated %zu (%s)", ord.ins, instr_ccy(ord.ins).full);
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
trd_cb(tws_t UNUSED(tws), tws_cb_t what, struct tws_trd_clo_s clo)
{
	with (struct timespec tsp) {
		clock_gettime(CLOCK_REALTIME_COARSE, &tsp);
		printf("%ld.%09ld\t", tsp.tv_sec, tsp.tv_nsec);
	}

	switch (what) {
	case TWS_CB_TRD_ORD_STATUS: {
		const struct tws_trd_ord_status_clo_s *os = clo.data;
		printf("8=FIX.OUR|35=8|39=%c|14=%f|6=%f|\n",
		       (char)(os->er.ord_status ?: '?'),
		       os->er.last_qty, os->er.last_prc);
		break;
	}
	case TWS_CB_TRD_OPEN_ORD: {
		const struct tws_trd_open_ord_clo_s *oo = clo.data;
		printf("8=FIX.OUR|35=8|39=%c|\n", (char)(oo->st.state ?: '?'));
		break;
	}
	case TWS_CB_TRD_EXEC_DTL: {
		const struct tws_trd_exec_dtl_clo_s *ex = clo.data;
		printf("8=FIX.OUT|35=8|39=%c|55=%s|14=%f|6=%f|\n",
		       (char)(ex->er.ord_status ?: '?'),
		       tws_cont_nick(ex->cont),
		       ex->er.last_qty, ex->er.last_prc);
		break;
	}
	default:
		printf("TRAD  what %u  oid %u  data %p\n",
		       what, clo.oid, clo.data);
		break;
	}
	fflush(stdout);
	return;
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


static void
ev_io_shut(EV_P_ ev_io *w)
{
	int fd = w->fd;

	ev_io_stop(EV_A_ w);
	shutdown(fd, SHUT_RDWR);
	close(fd);
	w->fd = -1;
	return;
}

static void
beef_cb(EV_P_ ev_io *w, int UNUSED(revents))
{
	char buf[1536U];
	ssize_t nrd;
	ctx_t ctx = w->data;

	(void)EV_A;
	/* snarf the line */
	if (UNLIKELY((nrd = recv(w->fd, buf, sizeof(buf), 0)) < 0)) {
		return;
	}
	/* finalise */
	buf[nrd] = '\0';
	/* execute */
	ex(ctx->tws, buf, nrd);
	return;
}

static void
twsc_cb(EV_P_ ev_io *w, int UNUSED(rev))
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
	case TWS_ST_SUP:
		/* former ST_RDY/ST_SUP case pair in chck_cb() */
		tws_send(ctx->tws);
		break;
	default:
		uerror("unknown state: %u\n", tws_state(ctx->tws));
		abort();
	}
	return;
}

static void
sigint_cb(EV_P_ ev_signal *UNUSED(w), int UNUSED(revents))
{
	ev_unloop(EV_A_ EVUNLOOP_ALL);
	return;
}


#include "ibex.yucc"

int
main(int argc, char *argv[])
{
	static yuck_t argi[1U];
	static struct ctx_s ctx[1U];
	EV_P = ev_default_loop(EVFLAG_AUTO);
	/* ev goodies */
	ev_signal sigint_watcher[1];
	ev_signal sigterm_watcher[1];
	ev_prepare prep[1];
	ev_io beef[1U];
	int rc = 0;

	/* parse the command line */
	if (yuck_parse(argi, argc, argv) < 0) {
		rc = 1;
		goto out;
	}

	if (argi->amount_arg) {
		if (!(halt_amt = strtoqx(argi->amount_arg, NULL))) {
			uerror("\
Error: cannot read amount `%s'", argi->amount_arg);
			rc = 1;
			goto out;
		}
		dflt_amt = halt_amt;
	}

	/* snarf host name and port */
	if (argi->tws_arg) {
		*ctx->uri = make_uri(argi->tws_arg);
	} else {
		*ctx->uri = make_uri("localhost");
	}

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
	} else if (mc6_join_group(
			   ctx->r, ctx->ch, MCAST_ADDR, MCAST_PORT, NULL) < 0) {
		serror("\
Error: cannot join multicast group on socket %d", ctx->ch);
		rc = 1;
		goto out;
	}
	/* yay, hook the chan into our event loop */
	beef->data = ctx;
	ev_io_init(beef, beef_cb, ctx->ch, EV_READ);
	ev_io_start(EV_A_ beef);

	/* initialise a sig C-c handler */
	ev_signal_init(sigint_watcher, sigint_cb, SIGINT);
	ev_signal_start(EV_A_ sigint_watcher);
	ev_signal_init(sigterm_watcher, sigint_cb, SIGTERM);
	ev_signal_start(EV_A_ sigterm_watcher);

	/* prepare the context and the tws */
	ctx->tws->trd_cb = trd_cb;
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
	mc6_leave_group(ctx->ch, ctx->r);

	/* destroy the default evloop */
	ev_default_destroy();
	yuck_free(argi);
	return rc;
}

/* ibex.c ends here */
