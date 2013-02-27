/*** strops.h -- useful string operations
 *
 * Copyright (C) 2011-2012 Sebastian Freundt
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
 **/
#if !defined INCLUDED_strops_h_
#define INCLUDED_strops_h_

#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif	/* __cplusplus */

#if !defined UNUSED
# define UNUSED(_x)	_x __attribute__((unused))
#endif	/* !UNUSED */

static inline size_t
ui8tostr(char *restrict buf, size_t UNUSED(bsz), uint8_t d)
{
/* all strings should be little */
#define C(x, d)	(char)((x) / (d) % 10 + '0')
	size_t i = 0;

	if (d >= 100) {
		buf[i++] = C(d, 100);
	}
	if (d >= 10) {
		buf[i++] = C(d, 10);
	}
	buf[i++] = C(d, 1);
	return i;
}

static inline size_t
ui8tostr_pad(char *restrict buf, size_t UNUSED(bsz), uint8_t d, size_t pad)
{
/* all strings should be little */
#define C(x, d)	(char)((x) / (d) % 10 + '0')
	switch (pad) {
	case 3:
		buf[pad - 3] = C(d, 100);
	case 2:
		buf[pad - 2] = C(d, 10);
	case 1:
		buf[pad - 1] = C(d, 1);
		break;
	case 0:
	default:
		pad = 0;
		break;
	}
	return pad;
}

static inline size_t
ui16tostr(char *restrict buf, size_t UNUSED(bsz), uint16_t d)
{
/* all strings should be little */
#define C(x, d)	(char)((x) / (d) % 10 + '0')
	size_t i = 0;

	if (d >= 10000) {
		buf[i++] = C(d, 10000);
	}
	if (d >= 1000) {
		buf[i++] = C(d, 1000);
	}
	if (d >= 100) {
		buf[i++] = C(d, 100);
	}
	if (d >= 10) {
		buf[i++] = C(d, 10);
	}
	buf[i++] = C(d, 1);
	return i;
}

static inline size_t
ui16tostr_pad(char *restrict buf, size_t UNUSED(bsz), uint16_t d, size_t pad)
{
/* all strings should be little */
#define C(x, d)	(char)((x) / (d) % 10 + '0')
	switch (pad) {
	case 5:
		buf[pad - 5] = C(d, 10000);
	case 4:
		buf[pad - 4] = C(d, 1000);
	case 3:
		buf[pad - 3] = C(d, 100);
	case 2:
		buf[pad - 2] = C(d, 10);
	case 1:
		buf[pad - 1] = C(d, 1);
		break;
	case 0:
	default:
		pad = 0;
		break;
	}
	return pad;
}

static inline size_t
ui32tostr(char *restrict buf, size_t UNUSED(bsz), uint32_t d)
{
/* all strings should be little */
#define C(x, d)	(char)((x) / (d) % 10 + '0')
	size_t i = 0;

	if (d >= 1000000000) {
		buf[i++] = C(d, 1000000000);
	}
	if (d >= 100000000) {
		buf[i++] = C(d, 100000000);
	}
	if (d >= 10000000) {
		buf[i++] = C(d, 10000000);
	}
	if (d >= 1000000) {
		buf[i++] = C(d, 1000000);
	}
	if (d >= 100000) {
		buf[i++] = C(d, 100000);
	}
	if (d >= 10000) {
		buf[i++] = C(d, 10000);
	}
	if (d >= 1000) {
		buf[i++] = C(d, 1000);
	}
	if (d >= 100) {
		buf[i++] = C(d, 100);
	}
	if (d >= 10) {
		buf[i++] = C(d, 10);
	}
	buf[i++] = C(d, 1);
	return i;
}


static inline char*
__c2p(const char *p)
{
	union {
		char *p;
		const char *c;
	} res = {.c = p};
	return res.p;
}

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_strops_h_ */
