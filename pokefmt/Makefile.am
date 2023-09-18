# pokefmt Makefile.am

# Copyright (C) 2023 Mohammad-Reza Nabipoor

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

appdir = $(pkgdatadir)/pokefmt

dist_app_DATA = pokefmt.pk

AM_LFLAGS = -d
# The Automake generated .l.c rule is broken: When executed in a VPATH build,
#+   - The .c file gets generated in the build directory. But since it requires
#     special tools to rebuild it, we need to distribute it in the tarballs,
#     and by the GNU Coding Standards
#     <https://www.gnu.org/prep/standards/html_node/Makefile-Basics.html>
#     the file should be generated in the source directory.
#   - The #line directives in the .c file refer to a nonexistent file once it
#     has been moved from the build directory to the source directory. This
#     leads to error if 'lcov' is used later.
# Additionally, here we assume Flex and therefore don't need the ylwrap script.
# Therefore we override this rule.
# Since this is a rule that produces multiple files, we apply the idiom from
# <https://lists.gnu.org/archive/html/bug-make/2020-09/msg00008.html>, so that
# it works also in parallel 'make'.
generate-pokefmt:
	$(AM_V_LEX)$(LEX) $(LFLAGS) $(AM_LFLAGS) -t $(srcdir)/pokefmt.l > pokefmt.c \
	&& test ':' = '$(LEX)' || { \
	  sed -e 's|".*/pokefmt\.l"|"pokefmt.l"|' \
	      -e 's|"lex\.yy\.c"|"pokefmt.c"|' \
	      < pokefmt.c > pokefmt.c-tmp \
	  && sed -e 's|".*/pokefmt\.l"|"pokefmt.l"|' \
	         < pokefmt.h > pokefmt.h-tmp \
	  && rm -f pokefmt.c pokefmt.h \
	  && mv pokefmt.c-tmp $(srcdir)/pokefmt.c \
	  && mv pokefmt.h-tmp $(srcdir)/pokefmt.h; \
	}
.PHONY: generate-pokefmt
# The above rule will generate files with time-stamp order
# pokefmt.l <= pokefmt.c <= pokefmt.h.
pokefmt.c: pokefmt.l
	@{ test -f $(srcdir)/pokefmt.c && test ! $(srcdir)/pokefmt.c -ot $(srcdir)/pokefmt.l; } || $(MAKE) generate-pokefmt
pokefmt.h: pokefmt.c
	@{ test -f $(srcdir)/pokefmt.h && test ! $(srcdir)/pokefmt.h -ot $(srcdir)/pokefmt.c; } || $(MAKE) generate-pokefmt
BUILT_SOURCES += pokefmt.c pokefmt.h
MOSTLYCLEANFILES += pokefmt.c-tmp pokefmt.h-tmp
MAINTAINERCLEANFILES += $(srcdir)/pokefmt.c $(srcdir)/pokefmt.h
EXTRA_DIST += pokefmt.l pokefmt.c pokefmt.h

bin_PROGRAMS = pokefmt
pokefmt_SOURCES = pokefmt.c pokefmt.h

pokefmt_SOURCES += ../common/pk-utils.c ../common/pk-utils.h

pokefmt_CPPFLAGS = -I$(top_srcdir)/common \
                   -I$(top_builddir)/gl -I$(top_srcdir)/gl \
                   -I$(top_srcdir)/libpoke -I$(top_builddir)/libpoke
pokefmt_CFLAGS = -Wall
pokefmt_LDADD = $(top_builddir)/gl/libgnu.la \
                $(top_builddir)/libpoke/libpoke.la
pokefmt_LDFLAGS =
