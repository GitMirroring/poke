# GNU poke

# Copyright (C) 2019, 2020, 2021, 2022, 2023 Jose E. Marchesi

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

ACLOCAL_AMFLAGS = -I m4 -I m4/libpoke
SUBDIRS = jitter gl maps pickles gl-libpoke libpoke poke poked utils \
          doc man testsuite etc po

EXTRA_DIST = INSTALL.generic DEPENDENCIES $(top_srcdir)/.version
BUILT_SOURCES = $(top_srcdir)/.version

noinst_SCRIPTS = run

# Support for generating a cyclomatic complexity report using the
# pmccabe tool.

CYCLO_SRCS = ${top_srcdir}/poke/*.[ch] $(top_srcdir)/libpoke/*.[ch]

cyclo-$(PACKAGE).html: $(CYCLO_SRCS)
	$(PMCCABE) $(CYCLO_SRCS) \
		| sort -nr \
		| $(AWK) -f ${top_srcdir}/build-aux/pmccabe2html \
			-v lang=html -v name="$(PACKAGE_NAME)" \
			-v vcurl="https://git.savannah.gnu.org/gitweb/?p=$(PACKAGE).git;a=blob;f=%FILENAME%;hb=HEAD" \
			-v url="https://www.jemarch.net/poke" \
			-v css=${top_srcdir}/build-aux/pmccabe.css \
			-v cut_dir=${top_srcdir}/ \
			> $@-tmp
	mv $@-tmp $@

# Update the HACKING file from the contents of etc/hacking.org.
# This requires Emacs.

update-hacking:
	emacs ${srcdir}/etc/hacking.org --batch -f org-ascii-export-to-ascii --kill
	mv -f ${srcdir}/etc/hacking.txt ${srcdir}/HACKING

# Support for git-version-gen
$(top_srcdir)/.version: $(top_srcdir)/configure
	echo '$(VERSION)' > $@-t
	mv $@-t $@

dist-hook:
	echo '$(VERSION)' > $(distdir)/.tarball-version

.PHONY = update-hacking
