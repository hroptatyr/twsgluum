/*** mc6-sock.c -- provide some multicast goodness
 *
 * Copyright (C) 2013-2016 Sebastian Freundt
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
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include "ud-sock.h"

#define UDP_MULTICAST_TTL	64


/* socket goodies */
static int
mc6_socket(void)
{
	volatile int s;

	/* try v6 first */
	if ((s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP)) < 0) {
		return -1;
	}

#if defined IPV6_V6ONLY
	{
		int yes = 1;
		setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes));
	}
#endif	/* IPV6_V6ONLY */
	/* be less blocking */
	setsock_nonblock(s);
	return s;
}

static __attribute__((unused)) int
mc6_join_group(
	struct ipv6_mreq *tgt, int s,
	const char *addr, short unsigned int port, const char *iface)
{
	struct sockaddr_in6 sa = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(port),
		.sin6_scope_id = 0,
	};
	struct ipv6_mreq r = {
		.ipv6mr_interface = 0,
	};
	int opt;

	/* allow many many many subscribers on that port */
	setsock_reuseaddr(s);

#if defined IPV6_V6ONLY
	opt = 0;
	setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif	/* IPV6_V6ONLY */
#if defined IPV6_USE_MIN_MTU
	/* use minimal mtu */
	opt = 1;
	setsockopt(s, IPPROTO_IPV6, IPV6_USE_MIN_MTU, &opt, sizeof(opt));
#endif
#if defined IPV6_DONTFRAG
	/* rather drop a packet than to fragment it */
	opt = 1;
	setsockopt(s, IPPROTO_IPV6, IPV6_DONTFRAG, &opt, sizeof(opt));
#endif
#if defined IPV6_RECVPATHMTU
	/* obtain path mtu to send maximum non-fragmented packet */
	opt = 1;
	setsockopt(s, IPPROTO_IPV6, IPV6_RECVPATHMTU, &opt, sizeof(opt));
#endif
#if defined IPV6_MULTICAST_HOPS
	opt = UDP_MULTICAST_TTL;
	/* turn into a mcast sock and set a TTL */
	setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &opt, sizeof(opt));
#endif	/* IPV6_MULTICAST_HOPS */

	if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		return -1;
	}

	/* set up the membership request */
	if (inet_pton(AF_INET6, addr, &r.ipv6mr_multiaddr) < 0) {
		return -1;
	}
	if (iface != NULL) {
		r.ipv6mr_interface = if_nametoindex(iface);
	}

	/* now truly join */
	*tgt = r;
	return setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, &r, sizeof(r));
}

static __attribute__((unused)) void
mc6_leave_group(int s, struct ipv6_mreq *mreq)
{
	/* drop mcast6 group membership */
	setsockopt(s, IPPROTO_IPV6, IPV6_LEAVE_GROUP, mreq, sizeof(*mreq));
	return;
}

static __attribute__((unused)) int
mc6_set_pub(int s, const char *addr, short unsigned int port, const char *iface)
{
	struct sockaddr_in6 sa = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(port),
		.sin6_flowinfo = 0,
		.sin6_scope_id = 0,
	};

	/* we pick link-local here for simplicity */
	if (inet_pton(AF_INET6, addr, &sa.sin6_addr) < 0) {
		return -1;
	}
	/* scope id */
	if (iface != NULL) {
		sa.sin6_scope_id = if_nametoindex(iface);
	}
	/* and do the connect() so we can use send() instead of sendto() */
	return connect(s, (struct sockaddr*)&sa, sizeof(sa));
}

static __attribute__((unused)) int
mc6_unset_pub(int s)
{
	if (s >= 0) {
		close(s);
	}
	return 0;
}

/* sock.c ends here */
