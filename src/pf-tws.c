/*** pf-tws.c -- portfolio updates from tws
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
#include <sys/utsname.h>
#include <unserding/unserding.h>

/* the tws api */
#define STATIC_TWS_URI_GUTS
#define STATIC_TWS_SOCK_GUTS
#include "pf-tws.h"
#include "pfa.h"
#include "logger.h"
#include "daemonise.h"
#include "tws.h"
#include "sdef.h"
#include "nifty.h"
#include "tws-uri.h"
#include "tws-sock.h"

#if defined __INTEL_COMPILER
# pragma warning (disable:981)
#endif	/* __INTEL_COMPILER */

#if defined DEBUG_FLAG && !defined BENCHMARK
# include <assert.h>
# define PF_DEBUG(args...)	fprintf(logerr, args)
# define MAYBE_NOINLINE		__attribute__((noinline))
#else  /* !DEBUG_FLAG */
# define PF_DEBUG(args...)
# define assert(x)
# define MAYBE_NOINLINE
#endif	/* DEBUG_FLAG */

#define TWS_ALL_ACCOUNTS	(NULL)

typedef struct ctx_s *ctx_t;

struct ctx_s {
	struct tws_s tws[1];

	/* parsed up uri string */
	struct tws_uri_s uri[1];

	/* accounts under management */
	char *ac_names;
	size_t nac_names;

	/* position queue */
	pfaq_t pq;

	ud_sock_t beef;
};


#include "tws-uri.c"
#include "tws-sock.c"


static void
um_pack_pr(ud_sock_t where, const char *what)
{
/* one day might go back to unsermarkt */
	return;
}

static void
pq_flush_cb(struct pfa_s p, ud_sock_t beef)
{
	const char *pr;

	/* stuff him up the socket's sleeve */
	um_pack_pr(beef, pr);
	return;
}

static void
pfaq_flush(ctx_t ctx)
{
	/* get the packet ctor'd */
	pfaq_flush_cb(ctx->pq, (void(*)())pq_flush_cb, ctx->beef);

	/* make sure nothing gets buffered */
	ud_flush(ctx->beef);
	return;
}

static const char*
find_ac_name(ctx_t ctx, const char *src)
{
	size_t az;
	size_t nac = 0;
	const char *ac;
	size_t srz = strlen(src);

	/* find that  */
	for (ac = ctx->ac_names, nac = 0;
	     nac < ctx->nac_names &&
		     (az = strlen(ac), srz != az || memcmp(ac, src, az));
	     nac++, ac += az + 1);

	return nac < ctx->nac_names ? ac : NULL;
}


/* tws interaction */
static void
infra_cb(tws_t UNUSED(tws), tws_cb_t what, struct tws_infra_clo_s clo)
{
/* called from tws api for infra messages */
	switch (what) {
	case TWS_CB_INFRA_ERROR:
		PF_DEBUG("NFRA  oid %u  code %u: %s\n",
			clo.oid, clo.code, (const char*)clo.data);
		break;
	case TWS_CB_INFRA_CONN_CLOSED:
		PF_DEBUG("NFRA  connection closed\n");
		break;
	case TWS_CB_INFRA_READY:
		PF_DEBUG("NFRA  RDY\n");
		break;
	default:
		PF_DEBUG("NFRA  what %u  oid %u  code %u  data %p\n",
			what, clo.oid, clo.code, clo.data);
		break;
	}
	return;
}

static void
post_cb(tws_t tws, tws_cb_t what, struct tws_post_clo_s clo)
{
	switch (what) {
	case TWS_CB_POST_MNGD_AC: {
		ctx_t ctx = (void*)tws;
		const char *str = clo.data;

		PF_DEBUG("tws %p post MNGD_AC: %s\n", tws, str);

		if (ctx->ac_names != NULL) {
			free(ctx->ac_names);
		}
		if (UNLIKELY(str == NULL)) {
			ctx->ac_names = NULL;
			break;
		}
		ctx->ac_names = strdup(str);
		ctx->nac_names = 0UL;
		/* quick counting */
		for (char *p = ctx->ac_names; *p; p++) {
			if (*p == ',') {
				*p = '\0';
				ctx->nac_names++;
			}
		}
		if (ctx->nac_names == 0UL) {
			ctx->nac_names = 1;
		}
		break;
	}
	case TWS_CB_POST_ACUP: {
		ctx_t ctx = (void*)tws;
		const struct tws_post_acup_clo_s *rclo = clo.data;
		struct pfa_s pos;

		PF_DEBUG("tws %p: post ACUP: %s %s <- %.4f  (%.4f)\n",
			tws, rclo->ac_name, tws_cont_nick(rclo->cont),
			rclo->pos, rclo->val);

		pos.ac = find_ac_name(ctx, rclo->ac_name);
		pos.sym = tws_cont_nick(rclo->cont);
		pos.lqty = rclo->pos > 0 ? rclo->pos : 0.0;
		pos.sqty = rclo->pos < 0 ? -rclo->pos : 0.0;
		pfaq_add(ctx->pq, pos);
		break;
	}
	case TWS_CB_POST_ACUP_END: {
		static const char *nex = NULL;
		ctx_t ctx = (void*)tws;

		if (ctx->nac_names <= 1) {
			/* nothing to wrap around */
			PF_DEBUG("single a/c, no further subscriptions\n");
			break;
		} else if (nex == NULL) {
			nex = ctx->ac_names;
		}
		if (*(nex += strlen(nex) + 1) == '\0') {
			nex = ctx->ac_names;
			PF_DEBUG("last a/c, no further subscriptions\n");
			break;
		}
		PF_DEBUG("subscribing to a/c %s\n", nex);
		tws_sub_ac(tws, nex);
		break;
	}
	default:
		PF_DEBUG("%p post called: what %u  oid %u  data %p\n",
			tws, what, clo.oid, clo.data);
	}
	return;
}


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
twsc_cb(EV_P_ ev_io *w, int UNUSED(rev))
{
	static char noop[1];
	ctx_t ctx = w->data;

	PF_DEBUG("BANG  %x\n", rev);
	if (recv(w->fd, noop, sizeof(noop), MSG_PEEK) <= 0) {
		/* uh oh */
		ev_io_shut(EV_A_ w);
		w->fd = -1;
		w->data = NULL;
		(void)fini_tws(ctx->tws);
		/* we should set a timer here for retrying */
		PF_DEBUG("AXAX  scheduling reconnect\n");
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
		PF_DEBUG("FINI  %d\n", twsc->fd);
		ev_io_shut(EV_A_ twsc);
		return;
	}
	/* otherwise proceed normally */
	ctx = w->data;
	if ((s = make_tws_sock(ctx->uri)) < 0) {
		error(errno, "tws connection setup failed");
		return;
	}

	PF_DEBUG("CONN  %d\n", s);
	twsc->data = ctx;
	ev_io_init(twsc, twsc_cb, s, EV_READ);
	ev_io_start(EV_A_ twsc);

	if (UNLIKELY((cli = ctx->uri->cli) <= 0)) {
		cli = time(NULL);
	}
	if (UNLIKELY(init_tws(ctx->tws, s, cli) < 0)) {
		PF_DEBUG("DOWN  %d\n", s);
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

	PF_DEBUG("PREP\n");

	st = tws_state(ctx->tws);
	PF_DEBUG("STAT  %u\n", st);
	switch (st) {
	case TWS_ST_UNK:
	case TWS_ST_DWN:
		/* is there a timer already? */
		if (reco->data == NULL) {
			/* start the reconnection timer */
			reco->data = ctx;
			ev_timer_init(reco, reco_cb, 0.0, 2.0/*option?*/);
			ev_timer_start(EV_A_ reco);
			PF_DEBUG("RECO\n");
		}
		break;
	case TWS_ST_RDY:
		if (old_st != TWS_ST_RDY) {
			PF_DEBUG("SUBS\n");
			tws_sub_ac(ctx->tws, TWS_ALL_ACCOUNTS);
		}
	case TWS_ST_SUP:
		/* former ST_RDY/ST_SUP case pair in chck_cb() */
		tws_send(ctx->tws);
		/* and flush the queue */
		pfaq_flush(ctx);
		break;
	default:
		PF_DEBUG("unknown state: %u\n", tws_state(ctx->tws));
		abort();
	}

	old_st = st;
	return;
}

static void
beef_cb(EV_P_ ev_io *w, int UNUSED(revents))
{
/* should not be called at all */
	static char junk[1];

	(void)recv(w->fd, junk, sizeof(junk), MSG_TRUNC);
	PF_DEBUG("JUNK\n");
	return;
}

static void
sigall_cb(EV_P_ ev_signal *UNUSED(w), int UNUSED(revents))
{
	ev_unloop(EV_A_ EVUNLOOP_ALL);
	PF_DEBUG("UNLO\n");
	return;
}


#if defined __INTEL_COMPILER
# pragma warning (disable:593)
# pragma warning (disable:181)
#elif defined __GNUC__
# pragma GCC diagnostic ignored "-Wswitch"
# pragma GCC diagnostic ignored "-Wswitch-enum"
#endif /* __INTEL_COMPILER */
#include "pf-tws-clo.h"
#include "pf-tws-clo.c"
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
	struct pf_args_info argi[1];
	/* use the default event loop unless you have special needs */
	struct ev_loop *loop;
	/* ev goodies */
	ev_signal sigint_watcher[1];
	ev_signal sighup_watcher[1];
	ev_signal sigterm_watcher[1];
	ev_prepare prep[1];
	ev_io ctrl[1];
	/* final result */
	int res = 0;

	/* big assignment for logging purposes */
	logerr = stderr;

	/* parse the command line */
	if (pf_parser(argc, argv, argi)) {
		res = 1;
		goto out;
	}

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

	/* snarf host name and port */
	if (argi->tws_given) {
		*ctx->uri = make_uri(argi->tws_arg);
	} else {
		*ctx->uri = make_uri("localhost");
	}

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
	{
		struct ud_sockopt_s opt = {
			UD_PUB,
			.port = (short unsigned int)argi->beef_arg,
		};
		if ((ctx->beef = ud_socket(opt)) == NULL) {
			perror("cannot connect to beef channel");
			goto unlo;
		}
	}

	/* initialise a sig C-c handler */
	ev_signal_init(sigint_watcher, sigall_cb, SIGINT);
	ev_signal_start(EV_A_ sigint_watcher);
	ev_signal_init(sigterm_watcher, sigall_cb, SIGTERM);
	ev_signal_start(EV_A_ sigterm_watcher);
	ev_signal_init(sighup_watcher, sigall_cb, SIGHUP);
	ev_signal_start(EV_A_ sighup_watcher);

	/* get the queues ready */
	ctx->pq = make_pfaq();

	/* prepare the context and the tws */
	ctx->tws->post_cb = post_cb;
	ctx->tws->infra_cb = infra_cb;

	/* prepare for hard slavery */
	prep->data = ctx;
	ev_prepare_init(prep, prep_cb);
	ev_prepare_start(EV_A_ prep);

	/* now wait for events to arrive */
	ev_loop(EV_A_ 0);

	/* cancel them timers and stuff */
	ev_prepare_stop(EV_A_ prep);

	/* propagate tws shutdown and resource freeing */
	reco_cb(EV_A_ NULL, EV_CUSTOM | EV_CLEANUP);

	/* don't need the queue no more do we */
	free_pfaq(ctx->pq);

unlo:
	/* free uri resources */
	free_uri(ctx->uri);

	/* destroy the default evloop */
	ev_default_destroy();
out:
	pf_parser_free(argi);
	return res;
}

/* pf-tws.c ends here */
