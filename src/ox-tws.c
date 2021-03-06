/*** ox-tws.c -- populate order events to tws
 *
 * Copyright (C) 2012-2015 Sebastian Freundt
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

/* the tws api */
#include "ox-tws.h"
#include "logger.h"
#include "daemonise.h"
#include "tws.h"
#include "nifty.h"

#if defined __INTEL_COMPILER
# pragma warning (disable:981)
#endif	/* __INTEL_COMPILER */

#if defined DEBUG_FLAG && !defined BENCHMARK
# include <assert.h>
# define OX_DEBUG(args...)	fprintf(logerr, args)
# define MAYBE_NOINLINE		__attribute__((noinline))
#else  /* !DEBUG_FLAG */
# define OX_DEBUG(args...)
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
};


/* sock helpers, should be somwhere else */
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

	OX_DEBUG("BANG  %x\n", rev);
	if (recv(w->fd, noop, sizeof(noop), MSG_PEEK) <= 0) {
		/* uh oh */
		ev_io_shut(EV_A_ w);
		w->fd = -1;
		w->data = NULL;
		(void)fini_tws(ctx->tws);
		/* we should set a timer here for retrying */
		OX_DEBUG("AXAX  scheduling reconnect\n");
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
		OX_DEBUG("FINI  %d\n", twsc->fd);
		ev_io_shut(EV_A_ twsc);
		return;
	}
	/* otherwise proceed normally */
	ctx = w->data;
	if ((s = tws_sock(ctx->host, ctx->port)) < 0) {
		error(errno, "tws connection setup failed");
		return;
	}

	OX_DEBUG("CONN  %d\n", s);
	twsc->data = ctx;
	ev_io_init(twsc, twsc_cb, s, EV_READ);
	ev_io_start(EV_A_ twsc);

	if (UNLIKELY(init_tws(ctx->tws, s, ctx->client) < 0)) {
		OX_DEBUG("DOWN  %d\n", s);
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
	ctx_t ctx = w->data;
	tws_st_t st;

	OX_DEBUG("PREP\n");

	st = tws_state(ctx->tws);
	OX_DEBUG("STAT  %u\n", st);
	switch (st) {
	case TWS_ST_UNK:
	case TWS_ST_DWN:
		/* is there a timer already? */
		if (reco->data == NULL) {
			/* start the reconnection timer */
			reco->data = ctx;
			ev_timer_init(reco, reco_cb, 0.0, 2.0/*option?*/);
			ev_timer_start(EV_A_ reco);
			OX_DEBUG("RECO\n");
		}
		break;
	case TWS_ST_RDY:
	case TWS_ST_SUP:
		break;
	default:
		OX_DEBUG("unknown state: %u\n", tws_state(ctx->tws));
		abort();
	}
	return;
}

static void
chck_cb(EV_P_ ev_check *w, int UNUSED(revents))
{
	ctx_t ctx = w->data;
	tws_st_t st;

	OX_DEBUG("CHCK\n");

	st = tws_state(ctx->tws);
	OX_DEBUG("STAT  %u\n", st);
	switch (st) {
	case TWS_ST_SUP:
	case TWS_ST_RDY:
		tws_send(ctx->tws);
		break;
	case TWS_ST_UNK:
	case TWS_ST_DWN:
		break;
	default:
		OX_DEBUG("unknown state: %u\n", tws_state(ctx->tws));
		abort();
	}
	return;
}

static void
sigall_cb(EV_P_ ev_signal *UNUSED(w), int UNUSED(revents))
{
	ev_unloop(EV_A_ EVUNLOOP_ALL);
	OX_DEBUG("UNLO\n");
	return;
}


#include "ox-tws.yucc"

int
main(int argc, char *argv[])
{
	static yuck_t argi[1U];
	struct ctx_s ctx[1] = {{0}};
	/* use the default event loop unless you have special needs */
	struct ev_loop *loop;
	/* ev goodies */
	ev_signal sigint_watcher[1];
	ev_signal sighup_watcher[1];
	ev_signal sigterm_watcher[1];
	ev_prepare prep[1];
	ev_check chck[1];
	/* final result */
	int rc = 0;

	/* big assignment for logging purposes */
	logerr = stderr;

	/* parse the command line */
	if (yuck_parse(argi, argc, argv) < 0) {
		rc = 1;
		goto out;
	}

	/* snarf host name and port */
	if (argi->tws_host_arg) {
		ctx->host = argi->tws_host_arg;
	} else {
		ctx->host = "localhost";
	}
	if (argi->tws_port_arg) {
		long unsigned int p = strtoul(argi->tws_port_arg, NULL, 0);
		ctx->port = (uint16_t)p;
	} else {
		ctx->port = (uint16_t)7474;
	}
	if (argi->tws_client_id_arg) {
		long unsigned int c = strtoul(argi->tws_client_id_arg, NULL, 0);
		ctx->client = c;
	} else {
		struct timeval now[1];

		(void)gettimeofday(now, NULL);
		ctx->client = now->tv_sec;
	}

	/* initialise the main loop */
	loop = ev_default_loop(EVFLAG_AUTO);

	/* initialise a sig C-c handler */
	ev_signal_init(sigint_watcher, sigall_cb, SIGINT);
	ev_signal_start(EV_A_ sigint_watcher);
	ev_signal_init(sigterm_watcher, sigall_cb, SIGTERM);
	ev_signal_start(EV_A_ sigterm_watcher);
	ev_signal_init(sighup_watcher, sigall_cb, SIGHUP);
	ev_signal_start(EV_A_ sighup_watcher);

	/* and just before we're entering that REPL check for daemonisation */
	if (argi->daemonise_flag && detach() < 0) {
		perror("daemonisation failed");
		rc = 1;
		goto out;
	} else if (argi->log_arg && open_logerr(argi->log_arg) < 0) {
		perror("cannot open log file");
		rc = 1;
		goto out;
	}

	/* prepare for hard slavery */
	prep->data = ctx;
	ev_prepare_init(prep, prep_cb);
	ev_prepare_start(EV_A_ prep);

	chck->data = ctx;
	ev_check_init(chck, chck_cb);
	ev_check_start(EV_A_ chck);

	/* now wait for events to arrive */
	ev_loop(EV_A_ 0);

	/* cancel them timers and stuff */
	ev_prepare_stop(EV_A_ prep);
	ev_check_stop(EV_A_ chck);

	/* get rid of the tws intrinsics */
	OX_DEBUG("FINI\n");
	(void)fini_tws(ctx->tws);
	reco_cb(EV_A_ NULL, 0);

	/* destroy the default evloop */
	ev_default_destroy();
out:
	yuck_free(argi);
	return rc;
}

/* ox-tws.c ends here */
