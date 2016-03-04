### Makefile.am
include $(top_builddir)/version.mk

AM_CFLAGS = $(EXTRA_CFLAGS)
AM_CXXFLAGS = $(EXTRA_CXXFLAGS)
AM_CPPFLAGS = -D_POSIX_C_SOURCE=201001L -D_XOPEN_SOURCE=700 -D_BSD_SOURCE
AM_CPPFLAGS += -D_HAVE_VERSION_H
AM_LDFLAGS = $(XCCLDFLAGS)

bin_PROGRAMS =
noinst_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
noinst_HEADERS =
BUILT_SOURCES =
EXTRA_DIST = $(BUILT_SOURCES)
CLEANFILES =
SUFFIXES =


## version rules
version.c: version.c.in $(top_builddir)/.version
	$(AM_V_GEN) PATH="$(top_builddir)/build-aux:$${PATH}" \
		yuck$(EXEEXT) scmver --ignore-noscm --force -o $@ \
			--use-reference --reference $(top_builddir)/.version $<

noinst_HEADERS += nifty.h
noinst_HEADERS += logger.h
noinst_HEADERS += daemonise.h

bin_PROGRAMS += quo-tws
quo_tws_SOURCES = quo-tws.c quo-tws.h quo-tws.yuck
EXTRA_quo_tws_SOURCES =
quo_tws_SOURCES += tws.cpp tws.h
quo_tws_SOURCES += logger.c logger.h
quo_tws_SOURCES += daemonise.c daemonise.h
quo_tws_SOURCES += sdef.c sdef.h sdef-private.h sdef-seria.h
quo_tws_SOURCES += quo.c quo.h
quo_tws_SOURCES += sub.c sub.h
quo_tws_SOURCES += websvc.c websvc.h
quo_tws_SOURCES += ud-sock.h
EXTRA_quo_tws_SOURCES += proto-tx-ns.gperf proto-tx-ns.h
EXTRA_quo_tws_SOURCES += proto-tx-attr.gperf proto-tx-attr.h
EXTRA_quo_tws_SOURCES += proto-twsxml-tag.gperf proto-twsxml-tag.h
EXTRA_quo_tws_SOURCES += proto-twsxml-attr.gperf proto-twsxml-attr.h
EXTRA_quo_tws_SOURCES += proto-twsxml-reqtyp.gperf proto-twsxml-reqtyp.h
EXTRA_quo_tws_SOURCES += proto-fixml-tag.gperf proto-fixml-tag.h
EXTRA_quo_tws_SOURCES += proto-fixml-attr.gperf proto-fixml-attr.h
EXTRA_quo_tws_SOURCES += gq.c gq.h
EXTRA_quo_tws_SOURCES += tws-uri.c tws-uri.h
EXTRA_quo_tws_SOURCES += tws-sock.c tws-sock.h
BUILT_SOURCES += proto-tx-attr.c
BUILT_SOURCES += proto-tx-ns.c
BUILT_SOURCES += proto-twsxml-tag.c
BUILT_SOURCES += proto-twsxml-attr.c
BUILT_SOURCES += proto-twsxml-reqtyp.c
BUILT_SOURCES += proto-fixml-tag.c
BUILT_SOURCES += proto-fixml-attr.c
quo_tws_SOURCES += sdef-twsxml.cpp
quo_tws_SOURCES += sdef-fixml.cpp
quo_tws_CPPFLAGS = $(AM_CPPFLAGS) -DASPECT_QUO
quo_tws_CPPFLAGS += $(twsapi_CFLAGS)
quo_tws_CPPFLAGS += $(libev_CFLAGS)
quo_tws_CPPFLAGS += $(expat_CFLAGS)
quo_tws_CPPFLAGS += $(unserding_CFLAGS) -DUD_NEW_API
quo_tws_CPPFLAGS += -I$(unserincdir)
quo_tws_LDFLAGS = $(AM_LDFLAGS)
quo_tws_LDFLAGS += $(twsapi_LIBS) -lstdc++
quo_tws_LDFLAGS += $(libev_LIBS)
quo_tws_LDFLAGS += $(expat_LIBS)
quo_tws_LDFLAGS += $(unserding_LIBS)
quo_tws_LDADD =
quo_tws_LDADD += $(unsermoddir)/libsvc-uterus.a
quo_tws_LDADD += $(quo_tws_LDFLAGS)
BUILT_SOURCES += quo-tws.yucc

bin_PROGRAMS += pf-tws
pf_tws_SOURCES = pf-tws.c pf-tws.h pf-tws.yuck
EXTRA_pf_tws_SOURCES =
pf_tws_SOURCES += tws.cpp tws.h
pf_tws_SOURCES += logger.c logger.h
pf_tws_SOURCES += daemonise.c daemonise.h
pf_tws_SOURCES += sdef.c sdef.h sdef-private.h sdef-seria.h
pf_tws_SOURCES += sdef-twsxml.cpp
pf_tws_SOURCES += sdef-fixml.cpp
pf_tws_SOURCES += pfa.c pfa.h
pf_tws_SOURCES += strops.h
EXTRA_pf_tws_SOURCES += tws-uri.c tws-uri.h
EXTRA_pf_tws_SOURCES += tws-sock.c tws-sock.h
pf_tws_CPPFLAGS = $(AM_CPPFLAGS) -DSDEF_WRONLY
pf_tws_CPPFLAGS += $(twsapi_CFLAGS)
pf_tws_CPPFLAGS += $(libev_CFLAGS)
pf_tws_CPPFLAGS += $(unserding_CFLAGS) -DUD_NEW_API
pf_tws_LDFLAGS = $(AM_LDFLAGS)
pf_tws_LDFLAGS += $(twsapi_LIBS) -lstdc++
pf_tws_LDFLAGS += $(libev_LIBS)
pf_tws_LDFLAGS += $(unserding_LIBS)
pf_tws_LDADD = $(pf_tws_LDFLAGS)
BUILT_SOURCES += pf-tws.yucc

bin_PROGRAMS += ox-tws
ox_tws_SOURCES = ox-tws.c ox-tws.h ox-tws.yuck
ox_tws_SOURCES += tws.cpp tws.h
ox_tws_SOURCES += logger.c logger.h
ox_tws_SOURCES += daemonise.c daemonise.h
ox_tws_CPPFLAGS = $(AM_CPPFLAGS) -DASPECT_PF
ox_tws_CPPFLAGS += $(twsapi_CFLAGS)
ox_tws_CPPFLAGS += $(libev_CFLAGS)
ox_tws_LDFLAGS = $(AM_LDFLAGS)
ox_tws_LDFLAGS += $(twsapi_LIBS) -lstdc++
ox_tws_LDFLAGS += $(libev_LIBS)
ox_tws_LDADD = $(ox_tws_LDFLAGS)
BUILT_SOURCES += ox-tws.yucc

bin_PROGRAMS += ibfx
ibfx_SOURCES = ibfx.c ibfx.yuck
EXTRA_ibfx_SOURCES =
ibfx_SOURCES += tws.cpp tws.h
ibfx_SOURCES += daemonise.c daemonise.h
ibfx_SOURCES += quo.c quo.h
ibfx_SOURCES += sdef-twsxml.cpp
ibfx_SOURCES += dfp754_d32.c dfp754_d32.h
ibfx_SOURCES += dfp754_d64.c dfp754_d64.h
EXTRA_ibfx_SOURCES += proto-tx-ns.gperf proto-tx-ns.h
EXTRA_ibfx_SOURCES += proto-tx-attr.gperf proto-tx-attr.h
EXTRA_ibfx_SOURCES += proto-twsxml-tag.gperf proto-twsxml-tag.h
EXTRA_ibfx_SOURCES += proto-twsxml-attr.gperf proto-twsxml-attr.h
EXTRA_ibfx_SOURCES += proto-twsxml-reqtyp.gperf proto-twsxml-reqtyp.h
EXTRA_ibfx_SOURCES += proto-fixml-tag.gperf proto-fixml-tag.h
EXTRA_ibfx_SOURCES += proto-fixml-attr.gperf proto-fixml-attr.h
EXTRA_ibfx_SOURCES += tws-uri.c tws-uri.h
EXTRA_ibfx_SOURCES += mc6-sock.c
BUILT_SOURCES += proto-tx-attr.c
BUILT_SOURCES += proto-tx-ns.c
BUILT_SOURCES += proto-twsxml-tag.c
BUILT_SOURCES += proto-twsxml-attr.c
BUILT_SOURCES += proto-twsxml-reqtyp.c
BUILT_SOURCES += proto-fixml-tag.c
BUILT_SOURCES += proto-fixml-attr.c
ibfx_CPPFLAGS = $(AM_CPPFLAGS) -DASPECT_QUO
ibfx_CPPFLAGS += -D__STDC_WANT_DEC_FP__
ibfx_CPPFLAGS += $(twsapi_CFLAGS)
ibfx_CPPFLAGS += $(libev_CFLAGS)
ibfx_CPPFLAGS += $(expat_CFLAGS)
ibfx_LDFLAGS = $(AM_LDFLAGS)
ibfx_LDFLAGS += $(twsapi_LIBS) -lstdc++
ibfx_LDFLAGS += $(libev_LIBS)
ibfx_LDFLAGS += $(expat_LIBS)
ibfx_LDADD = $(ibfx_LDFLAGS)
BUILT_SOURCES += ibfx.yucc


## yuck rule
SUFFIXES += .yuck
SUFFIXES += .yucc
.yuck.yucc:
	$(AM_V_GEN) PATH=$(top_builddir)/build-aux:"$${PATH}" \
		yuck$(EXEEXT) gen -o $@ $<

## our rule for gperf
SUFFIXES += .gperf
.gperf.c:
	$(AM_V_GEN) $(GPERF) -L ANSI-C $< --output-file $@


## Create preprocessor output (debugging purposes only)
.c.i:
	$(COMPILE) -E -o $@ $<

## Create assembler output (debugging purposes only)
.c.s:
	$(COMPILE) -S -c $(AM_CFLAGS) $<
