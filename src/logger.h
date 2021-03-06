/*** logger.h -- helpers for logging
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
#if !defined INCLUDED_logger_h_
#define INCLUDED_logger_h_

#include <stdarg.h>

#if defined __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* this really is a FILE* */
extern void *logerr;

/**
 * Open the logfile with pathname LOGFN. */
extern int open_logerr(const char *logfn);
/**
 * Rotate the log file.
 * This actually just closes and opens the file again. */
extern void rotate_logerr(void);

/**
 * Like perror() but for our log file. */
extern __attribute__((format(printf, 2, 3))) void
error(int eno, const char *fmt, ...);

/**
 * For generic logging without errno indication. */
#if !defined __cplusplus
# define logger(args...)	error(0, args)
#else  /* __cplusplus */
# define logger(args...)	::error(0, args)
#endif	/* __cplusplus */

#if defined __cplusplus
}
#endif	/* __cplusplus */

#endif	/* INCLUDED_logger_h_ */
