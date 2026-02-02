# GNU poke

# Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026 Jose E.
# Marchesi

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = subdir-objects

MOSTLYCLEANFILES =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES =

BUILT_SOURCES =

EXTRA_DIST =

appdir = $(pkgdatadir)/poke

dist_app_DATA = poke.pk \
                pk-tracer.pk \
                pk-cmd.pk \
                pk-cmd-dump.pk \
                pk-cmd-save.pk \
                pk-cmd-copy.pk \
                pk-cmd-diff.pk \
                pk-cmd-extract.pk \
                pk-cmd-scrabble.pk \
                pk-cmd-set.pk \
                pk-cmd-help.pk \
                pk-cmd-info.pk \
                pk-cmd-ios.pk \
                pk-cmd-misc.pk

bin_PROGRAMS = poke
poke_SOURCES = poke.c poke.h \
               pk-term.c pk-term.h \
               pk-table.c pk-table.h \
               pk-repl.c pk-repl.h \
               pk-cmd.c pk-cmd.h \
               pk-cmd-ios.c pk-cmd-info.c pk-cmd-misc.c \
               pk-cmd-help.c pk-cmd-def.c pk-cmd-vm.c pk-cmd-compiler.c \
               pk-cmd-set.c pk-cmd-editor.c \
               pk-ios.c pk-ios.h

poke_SOURCES += ../common/pk-utils.c ../common/pk-utils.h

poke_CPPFLAGS = -I$(top_builddir)/gl -I$(top_srcdir)/gl \
                -I$(top_srcdir)/common \
                -I$(top_srcdir)/libpoke -I$(top_builddir)/libpoke \
                -DJITTER_VERSION=\"$(JITTER_VERSION)\" \
                -DPKGINFODIR=\"$(infodir)\" \
                -DLOCALEDIR=\"$(localedir)\"
poke_CFLAGS = -Wall
poke_LDADD = $(top_builddir)/gl/libgnu.la \
             $(top_builddir)/libpoke/libpoke.la \
             $(LTLIBREADLINE) \
             $(LTLIBTEXTSTYLE) \
             $(LTLIBTERMINFO)
poke_LDFLAGS =

if HSERVER
  poke_SOURCES += pk-hserver.h pk-hserver.c
  poke_LDADD += -lpthread
endif
# We install pk-hserver.pk even if HSERVER is not enabled.
dist_app_DATA += pk-hserver.pk

# nodelist is now treated by doc/Makefile.am.
MOSTLYCLEANFILES += nodelist

# End of Makefile.am
