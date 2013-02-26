/*** sdef.h -- security definitions
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
#if !defined INCLUDED_sdef_h_
#define INCLUDED_sdef_h_

#if defined __cplusplus
extern "C" {
# if defined __GNUC__
#  define restrict	__restrict__
# else
#  define restrict
# endif
#endif	/* __cplusplus */

typedef void *tws_cont_t;
typedef const void *tws_const_cont_t;

typedef void *tws_sdef_t;
typedef const void *tws_const_sdef_t;

typedef const struct tws_sreq_s *tws_sreq_t;
struct tws_sreq_s {
	tws_sreq_t next;

	tws_cont_t c;
	const char *tws;
	const char *nick;
};

/**
 * Serialise SDEF into BUF of size BSZ, return the number of bytes written. */
extern ssize_t tws_ser_sdef(char *restrict buf, size_t bsz, tws_const_sdef_t);

extern tws_sreq_t tws_deser_sreq(const char *xml, size_t len);

/**
 * Return a copy of CONT. */
extern tws_cont_t tws_dup_cont(tws_const_cont_t);

/**
 * Free resources associated with CONT. */
extern void tws_free_cont(tws_cont_t);

/**
 * Return a copy of SDEF. */
extern tws_sdef_t tws_dup_sdef(tws_const_sdef_t);

/**
 * Free resources associated with SDEF. */
extern void tws_free_sdef(tws_sdef_t);

/**
 * Return a contract object that matches the security definition SDEF. */
extern tws_cont_t tws_sdef_make_cont(tws_const_sdef_t);

/**
 * Return a nick name for given contract. */
extern const char *tws_cont_nick(tws_const_cont_t);

/**
 * Return a nick name for given secdef. */
extern const char *tws_sdef_nick(tws_const_sdef_t);

/**
 * Free resources associated with SREQ. */
extern void tws_free_sreq(tws_sreq_t);

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_sdef_h_ */
