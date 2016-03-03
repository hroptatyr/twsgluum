/*** tws-sock.h -- provide tws connection sockets
 *
 * Copyright (C) 2013 Sebastian Freundt
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "tws-sock.h"
#include "ud-sock.h"


DEFUN int
make_dccp_sock(void)
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

DEFUN int
make_tcp_sock(void)
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

DEFUN int
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

DEFUN int
make_tws_sock(const struct tws_uri_s uri[static 1])
{
	struct addrinfo *aires;
	struct addrinfo hints = {0};
	int s;

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

	if (getaddrinfo(uri->h, uri->p, &hints, &aires) < 0) {
		return -1;
	}
	/* now try them all */
	for (const struct addrinfo *ai = aires;
	     ai != NULL &&
		     ((s = socket(ai->ai_family, ai->ai_socktype, 0)) < 0 ||
		      connect(s, ai->ai_addr, ai->ai_addrlen) < 0);
	     close(s), s = -1, ai = ai->ai_next);

	freeaddrinfo(aires);
	return s;
}

/* tws-sock.c ends here */
