pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = libenet.pc

enetincludedir=$(includedir)/enet
enetinclude_HEADERS = \
	include/enet/callbacks.h \
	include/enet/enet.h \
	include/enet/list.h \
	include/enet/protocol.h \
	include/enet/time.h \
	include/enet/types.h \
	include/enet/unix.h \
	include/enet/utility.h \
	include/enet/win32.h

lib_LTLIBRARIES = libenet.la
libenet_la_SOURCES = callbacks.c compress.c host.c list.c packet.c peer.c protocol.c unix.c win32.c
# see info '(libtool) Updating version info' before making a release
VERSIONINFO = -version-info 7:1:0
if MINGW
libenet_la_LDFLAGS = -no-undefined -lwinmm -lws2_32 $(AM_LDFLAGS) $(VERSIONINFO)
else
libenet_la_LDFLAGS = $(AM_LDFLAGS) $(VERSIONINFO)
endif
AM_CPPFLAGS = -I$(top_srcdir)/include

ACLOCAL_AMFLAGS = -Im4
