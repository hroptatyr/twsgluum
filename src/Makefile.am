### Makefile.am

AM_CPPFLAGS = -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600 -D_BSD_SOURCE
AM_LDFLAGS =

bin_PROGRAMS =
noinst_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
noinst_HEADERS =
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST = $(BUILT_SOURCES)

noinst_HEADERS += nifty.h


bin_PROGRAMS += quo-tws
quo_tws_SOURCES = quo-tws.c quo-tws-clo.ggo
BUILT_SOURCES += quo-tws-clo.c quo-tws-clo.h
quo_tws_SOURCES += tws.cpp tws.h
quo_tws_CPPFLAGS = $(AM_CPPFLAGS)
quo_tws_CPPFLAGS += $(twsapi_CFLAGS)
quo_tws_CPPFLAGS += $(libev_CFLAGS)
quo_tws_LDFLAGS = $(AM_LDFLAGS)
quo_tws_LDFLAGS += $(twsapi_LIBS) -lstdc++
quo_tws_LDFLAGS += $(libev_LIBS)
quo_tws_LDADD =
quo_tws_LDADD += $(twsapi_LIBS)

## our rule for gengetopt
%.c %.h: %.ggo
	$(AM_V_GEN) gengetopt -l -i $< -F $*

# 
# Help the developers get nice post-processed source files

## Create preprocessor output (debugging purposes only)
.c.i:
	$(COMPILE) -E -o $@ $<

## Create assembler output (debugging purposes only)
.c.s:
	$(COMPILE) -S -c $(AM_CFLAGS) $<
