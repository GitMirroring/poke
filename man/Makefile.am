# Makefile.am for poke/man

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

# In the rules to build manpages below, please depend on the source,
# not the binary; we don't need to regenerate the binary when any
# source file changes, only the main one.  Use -o so that the
# `missing' program can infer the output file.

if NATIVE_BUILD

man_MANS = poke.1
poke.1: $(top_srcdir)/poke/poke.c $(common_mandeps)
	$(MAKE) $(AM_MAKEFLAGS) $(builddir)/../poke/poke$(EXEEXT)
	$(top_builddir)/run \
	  $(HELP2MAN) -p poke --name="The GNU extensible binary editor" \
	      $(builddir)/../poke/poke$(EXEEXT) -o $(srcdir)/poke.1
EXTRA_DIST = poke.1
MAINTAINERCLEANFILES = poked.1

if ENABLE_POKED
man_MANS += poked.1
poked.1: $(top_srcdir)/poked/poked.c $(common_mandeps)
	$(MAKE) $(AM_MAKEFLAGS) $(builddir)/../poked/poked$(EXEEXT)
	$(top_builddir)/run \
	  $(HELP2MAN) -p poked --name="The poke daemon" -N \
	      $(builddir)/../poked/poked$(EXEEXT) -o $(srcdir)/poked.1
EXTRA_DIST += poked.1
MAINTAINERCLEANFILES += poked.1
endif

if ENABLE_POKEFMT
man_MANS += pokefmt.1
pokefmt.1: $(top_srcdir)/pokefmt/pokefmt.l $(common_mandeps)
	$(MAKE) $(AM_MAKEFLAGS) $(builddir)/../pokefmt/pokefmt$(EXEEXT)
	$(top_builddir)/run \
	  $(HELP2MAN) -p pokefmt --name="The poke templating system" -N \
	      $(builddir)/../pokefmt/pokefmt$(EXEEXT) -o $(srcdir)/pokefmt.1
EXTRA_DIST += pokefmt.1
MAINTAINERCLEANFILES += pokefmt.1
endif

# The man pages depend on the version number.
common_mandeps = $(top_srcdir)/configure.ac

endif
