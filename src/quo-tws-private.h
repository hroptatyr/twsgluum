/*** quo-tws-private.h -- private data flow guts
 *
 * Copyright (C) 2012 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of unsermarkt.
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
#if !defined INCLUDED_quo_tws_private_h_
#define INCLUDED_quo_tws_private_h_

#if defined __cplusplus
extern "C" {
#endif	/* __cplusplus */

typedef void *quo_qq_t;
typedef struct quo_s *quo_t;

typedef enum {
	QUO_TYP_UNK,
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

struct quo_s {
	uint16_t idx;
	quo_typ_t typ;
	double val;
};

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_quo_tws_private_h_ */
