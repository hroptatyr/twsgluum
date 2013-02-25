dnl sxe-twsapi.m4 -- our means of communicating with the tws
dnl
dnl Copyright (C) 2012-2013 Sebastian Freundt
dnl
dnl Author: Sebastian Freundt <freundt@ga-group.nl>
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl
dnl 3. Neither the name of the author nor the names of any contributors
dnl    may be used to endorse or promote products derived from this
dnl    software without specific prior written permission.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
dnl IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
dnl WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
dnl DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
dnl FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
dnl CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
dnl SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
dnl BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
dnl WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
dnl OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
dnl IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
dnl
dnl This file is part of twsgluum

AC_DEFUN([SXE_CHECK_TWSAPI], [
dnl Usage: SXE_CHECK_TWSAPI([ACTION_IF_FOUND], [ACTION_IF_NOT_FOUND])
dnl   def: sxe_cv_feat_twsapi yes|no

	AC_CACHE_VAL([sxe_cv_feat_twsapi], [
		sxe_cv_feat_twsapi="no"
		PKG_CHECK_MODULES([twsapi], [twsapi >= 0.4.1], [
			## pkg found
			sxe_cv_feat_twsapi="yes"
			$1
		], $2)
	])

])dnl SXE_CHECK_TWSAPI

AC_DEFUN([SXE_CHECK_TWSAPI_HANDSHAKE], [

	AC_CACHE_CHECK([if twsapi has handshake functions], 
		[sxe_cv_feat_twsapi_handshake], [
		## check for handshake functions in twsapi
		SXE_DUMP_LIBS
		AC_LANG([C++])
		CPPFLAGS="${CPPFLAGS} ${twsapi_CFLAGS}"
		LIBS="${LIBS} ${twsapi_LIBS} -lstdc++"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <twsapi/EPosixClientSocket.h>
]], [[
			IB::EPosixClientSocket *cli = NULL;
			cli->prepareHandshake(-1, 0);
			cli->handshake();
		]])], [
			sxe_cv_feat_twsapi_handshake="yes"
		], [
			sxe_cv_feat_twsapi_handshake="no"
		])

		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <twsapi/EPosixClientSocket.h>
]], [[
			IB::EPosixClientSocket *cli = NULL;
			cli->wavegoodbye();
		]])], [
			sxe_cv_feat_twsapi_wavegoodbye="yes"
		], [
			sxe_cv_feat_twsapi_wavegoodbye="no"
		])
		SXE_RESTORE_LIBS
		AC_LANG([C])
	])

	if test "${sxe_cv_feat_twsapi_handshake}" = "yes"; then
		:
		$1
		if test "${sxe_cv_feat_twsapi_wavegoodbye}" = "yes"; then
			AC_DEFINE_UNQUOTED([wavegoodbye], [eDisconnect],
				[how to mimic wavegoodbye()])
		fi
	else
		:
		$2
	fi
])dnl SXE_CHECK_TWSAPI_HANDSHAKE
