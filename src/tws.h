/*** tws.h -- tws c portion
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
#if !defined INCLUDED_tws_h_
#define INCLUDED_tws_h_

#include <stdbool.h>

#if defined __cplusplus
extern "C" {
#endif	/* __cplusplus */

typedef struct tws_s *tws_t;

typedef enum {
	/** no state can be determined */
	TWS_ST_UNK,
	/** in the state of setting up the connection */
	TWS_ST_SUP,
	/** ready state, you should be able to read and write */
	TWS_ST_RDY,
	/** down state, either finish the conn or re-set it up */
	TWS_ST_DWN,
} tws_st_t;


/* connection guts */
extern tws_t init_tws(int sock, int client);
extern int fini_tws(tws_t);
extern void rset_tws(tws_t);

extern int tws_recv(tws_t);
extern int tws_send(tws_t);

extern tws_st_t tws_state(tws_t);

/**
 * Request security reference data of the contract C. */
extern int tws_req_sdef(tws_t, const void *c);

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_tws_h_ */
