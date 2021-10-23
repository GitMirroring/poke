## Process this file with automake to produce Makefile.in .
# Copyright (C) 2016, 2017, 2018, 2019, 2020 Luca Saiu
# Updated in 2021 by Luca Saiu
# Written by Luca Saiu

# This file is part of Jitter.

# Jitter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# Jitter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Jitter.  If not, see <http://www.gnu.org/licenses/>.


# A remark about libraries
# ################################################################

# The Jitter runtime consists of one library per dispatch, of which the
# default-dispatch library is also provided with a different name.  Such
# libraries are built internally by joining together several Libtool convenience
# libraries (please search for "convenience library" in the GNU Libtool and the
# GNU Automake manuals: the phrase has a precise meaning), and are themselves
# built as convenience libraries.

# This allows to link the Jitter runtime both statically and dynamially and,
# which is important, to build a larger *library* instead of a program
# containing the Jitter runtime.  I expect such larger libraries to be
# a common use case, when the user abstract functionality built on top of a
# Jittery VM into a different software object intended for the user, with a
# simple API.

# Since the libraries built here are all convenience libraries I cannot use the
# name "convenience" to distinguish some intermediate part from one of the final
# runtimes.  I will call "partial" the libraries which are not useful without
# other parts added.


# Global configration.
# ################################################################

AUTOMAKE_OPTIONS = subdir-objects info-in-builddir

# I prefer to define all variables as empty in the beginning, and then add to
# each group of them as part of the implementation of the same feature.
AM_CFLAGS =
AM_CPPFLAGS =
AM_LDFLAGS =
BUILT_SOURCES =
DISTCLEANFILES =
EXTRA_DIST =
EXTRA_LTLIBRARIES =
EXTRA_PROGRAMS =
SUFFIXES =
TESTS =
lib_LTLIBRARIES =
bin_PROGRAMS =
bin_SCRIPTS =
check_PROGRAMS =
check_SCRIPTS =
dist_flag_DATA =
dist_lisp_DATA =
dist_man1_MANS =
dist_template_DATA =
info_TEXINFOS =
jitterinclude_HEADERS =
nodist_jitterinclude_HEADERS =
jittermachineinclude_HEADERS =
noinst_PROGRAMS =

# This will be augmented with optional libraries and their dependencies.
LDADD_FOR_EXAMPLES =

if JITTER_SUBPACKAGE
# Non-Libtool static libraries are only used in sub-package mode, and built
# in an unusual way using Libtool to extract them from Libtool libraries
# (which is why this uses DATA rather than LIBRARIES).

# These libraries have clean dependencies specified here, with each .a files
# depending on its .la equivalent.  See "Non-Libtool static libraries for
# sub-package mode" below.

# It is better to define this conditionally, so as not to use non-Libtool
# libraries by mistake in ordinary non-sub-package builds.
noinst_DATA =
endif # JITTER_SUBPACKAGE


# Installation directories.
# ################################################################

# I use the default directories for most installed files.  The few exceptions
# are template and flag files, kept in a version-specific subdirectories; this
# will make it easy to have multiple versions installed at the same time.
# Notice that templates are architecture-independent but flags are
# architecture-specific.
#
# In sub-package mode simply read templates from the source directory and
# flags from the build directory.
if JITTER_SUBPACKAGE
flagdir = $(abs_top_builddir)/flags
templatedir = $(abs_top_srcdir)/templates
else
flagdir = $(exec_prefix)/$(JITTER_FLAG_SUBDIRECTORY)
templatedir = $(datarootdir)/$(JITTER_TEMPLATE_SUBDIRECTORY)
endif # JITTER_SUBPACKAGE

# An installed jitterc will need its installation prefix (even if that can be
# overridden with a command-line option) to access template files but the prefix
# is only known very late, at make time.
CPPFLAGS_FOR_PATHS = \
  -DJITTER_FLAGDIR=$(flagdir) \
  -DJITTER_TEMPLATEDIR=$(templatedir)

# FIXME: I probably should do the same for headers.
# jitterincludedir = $(includedir)/$(PACKAGE)/$(VERSION)

# The headers needed to compile generated code are all installed in the same
# package-specific directory, even if they are laid out differently in the
# source tree.
jitterincludedir = $(includedir)/$(PACKAGE)
jittermachineincludedir = $(includedir)/$(PACKAGE)/machine

# Where to install the Autoconf macro file for Jitter.
aclocaldir = $(datarootdir)/aclocal


# Autoconf macros.
# ################################################################

# We have one single file containing Autoconf macro definitions, to be
# distributed and installed.
aclocal_DATA = autoconf/jitter.m4
EXTRA_DIST += $(aclocal_DATA)


# Libtool updates.
# ################################################################

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool
# FIXME: what target should depend on libtool ?


# GNUlib settings.
# ################################################################

SUBDIRS = gnulib-local
ACLOCAL_AMFLAGS = -I build-aux
EXTRA_DIST += build-aux/gnulib-cache.m4

# Add other source files to the Gnulib library; this helps on stupid platforms
# relying on the order of libraries in a link command line, where having the
# extension files as part of the Jitter utility library wouldn't work.
EXTRA_LTLIBRARIES += \
  lib/libgnulib-extension--partial.la
lib_libgnulib_extension__partial_la_SOURCES = \
  jitter/jitter-missing.h jitter/jitter-missing.c
lib_libgnulib_extension__partial_la_CPPFLAGS = \
  $(CPPFLAGS_FOR_GNULIB)
EXTRA_LTLIBRARIES += \
  lib/libgnulib--partial.la
lib_libgnulib__partial_la_SOURCES =
lib_libgnulib__partial_la_LIBADD = \
  gnulib-local/libgnulib.la \
  lib/libgnulib-extension--partial.la

# If the project used Gnulib for every compilation, as it is normally the case, I
# would simply do
# AM_CPPFLAGS += -I$(top_builddir)/gnulib-local -I$(top_srcdir)/gnulib-local
# LDADD = gnulib-local/libgnulib.la lib/libgnulib-extension.la ; but I don't
# want this here because the generated code must run without Gnulib.
# I *could* link libjitter against Gnulib, and that might be a good idea for the
# future; however Gnulib's CPP include paths would not be used in generated code.
CPPFLAGS_FOR_GNULIB = -I$(top_builddir)/gnulib-local -I$(top_srcdir)/gnulib-local
LDADD_FOR_GNULIB = lib/libgnulib--partial.la

# Gnulib scrits tell me I should link with these libraries; the variables expand
# to nothing on GNU, but they should be needed on some other system.
LDADD_FOR_GNULIB += $(LIB_CLOCK_GETTIME) $(LIB_GETRANDOM) $(LTLIBINTL)

# Allow everything but Gnulib to be built with a single-level Makefile, as I
# prefer; the only recusrive make invokation we need is for building libgnulib .
gnulib-local/libgnulib.la:
	$(MAKE) -C gnulib-local


# Files machine-generated by shell scripts.
# ################################################################

# By having the generated file depend on the M4sh source rather than on its
# expanded shell script I can avoid re-generating on user machines lacking
# the tools to rebuild the generator, unless the generator source is really
# modified -- in which case generation will fail, as it is supposed to.
$(srcdir)/jitter/jitter-fast-branch-machine-generated.h: scripts/generate-fast-branches.in.m4sh
	$(MAKE) scripts/generate-fast-branches \
	  && (echo "Generating fast-branch header: this might take a while."; \
              ./scripts/generate-fast-branches \
	      > $(srcdir)/jitter/jitter-fast-branch-machine-generated.h)

BUILT_SOURCES += \
  jitter/jitter-fast-branch-machine-generated.h


# Files machine-generated by Flex and Bison.
# ################################################################

# We have several scanners and parsers in this distribution, generated by Flex
# and Bison in the source directory.

# The actual scanner and parser sources are ".l" and ".y" files.  We will add to
# this variable incrementally.
FRONTEND_NON_GENERATED_FILES =

# From the actual sources we generate ".c" and ".h" files.  We will add to this
# incrementally as well.
FRONTEND_GENERATED_FILES =

# Of course we distribute the actual source files.  And we distribute the
# generated C files as well, as per the GNU Coding standards.
EXTRA_DIST += $(FRONTEND_NON_GENERATED_FILES) $(FRONTEND_GENERATED_FILES)

# The generated scanner and parser are built early in the make process.  (Notice
# that we can't to to the same with Jitter-generated files, even if conceptually
# they would be similar: Jitter-generated files rely, of course, on Jitter, and
# we don't have it yet.)
BUILT_SOURCES += $(FRONTEND_GENERATED_FILES)


# Files machine-generated by Jitter.
# ################################################################

# The C files generated by Jitter are, in a sense, similar to the C files
# generated by Flex and Bison; we want to distribute the generated files as
# well, mostly for the convenience of cross-compiling users and in the spirit of
# the GNU Coding Standards, which suggest this practice in similar cases.
# However this case is more difficult: we can't use use BUILT_SOURCES and have
# the generated files built early in the make process -- not in *this* project,
# as Jitter itself hasn't been built yet.

# Jitter machine specifications are the actual sources.  This will be added to
# incrementally.
JITTER_NON_GENERATED_FILES =

# From the actual sources we generate ".c" and ".h" files.  We will add to this
# incrementally as well.
JITTER_GENERATED_FILES =

# We want to distribute Jitter VM specifications, but not their generated files.
#
# Rationale:
# In an ordinary application it would make sense to distribute generated files
# as well, as per the ordinary GNU convention; but here we are in the build
# system of Jitter itself, which allows to rebuild those files.  And they are
# only examples, so they are not crucial to build, nor indeed built by default.
# If the user is able to run the examples or the testsuite, then there is some
# emulator allowing her to regenerate the files as well.
EXTRA_DIST += $(JITTER_NON_GENERATED_FILES)

# ...But in this case we can't use BUILT_SOURCES.  We will use regular
# dependencies.

# A final remark on JITTER_GENERATED_FILES : it would make sense that such
# generated files depended on the bin/jitter$(EXEEXT), but adding such
# dependencies would break distcheck , forcing writes to the source directory
# after bin/jitter$(EXEEXT) is rebuilt.
#
# In practice I find distcheck more important that my own convenience when
# testing with examples after modifying Jitter; an ordinary user calling an
# installed Jitter will never see this problem anyway.  On the other hand,
# distcheck may be useful for people other than the maintainer.


# Machine-generated C files.
# ################################################################

# This is convenient both as a user target and as a dependency.  It is
# impossible in general to build everything in parallel here, since Make needs
# to have the C files available to discover their dependencies; hence the
# recursive $(MAKE) invocation.
generated-c-files: $(BUILT_SOURCES)
	$(MAKE) bin/jitter$(EXEEXT) && \
          $(MAKE) $(JITTER_GENERATED_FILES)


# Jitter utility library.
# ################################################################

# Code within the Jitter runtime itself and the Jitter C code generator needs to
# be preprocessed with these flags to avoid failing for not having any variant of
# JITTER_CPPFLAGS used.
CPPFLAGS_FOR_INTERNAL_USE = \
  -DJITTER_INTERNAL=1

# This utility library is used both at generation time by jitterc, and at run
# time by the programs using VMs (see the SOURCES_FOR_LIBJITTER definition
# below); it will be contained within other libraries.
# Some parts of this, such as jitter-malloc.[ch], are actually redundant when
# using Gnulib; however we need them at runtime, since the generated code does
# not rely on Gnulib.
EXTRA_LTLIBRARIES += lib/libjitterutility--partial.la
lib_libjitterutility__partial_la_SOURCES = \
  jitter/jitter-aligned-block.h jitter/jitter-aligned-block.c \
  jitter/jitter-arithmetic.h jitter/jitter-arithmetic.c \
  jitter/jitter-bitwise.h \
  jitter/jitter-cpp.h \
  jitter/jitter-fatal.h jitter/jitter-fatal.c \
  jitter/jitter-malloc.h jitter/jitter-malloc.c \
  jitter/jitter-dynamic-buffer.h jitter/jitter-dynamic-buffer.c \
  jitter/jitter-hash.h jitter/jitter-hash.c jitter/jitter-hash-random-table.c \
  jitter/jitter-heap.h jitter/jitter-heap.c \
  jitter/jitter-heap-debug.h jitter/jitter-heap-debug.c \
  jitter/jitter-human-prefix.h jitter/jitter-human-prefix.c \
  jitter/jitter-list.h jitter/jitter-list.c \
  jitter/jitter-parse-int.h jitter/jitter-parse-int.c \
  jitter/jitter-print.h jitter/jitter-print.c \
  jitter/jitter-readline.h \
  jitter/jitter-string.h jitter/jitter-string.c \
  jitter/jitter-time.h jitter/jitter-time.c \
  jitter/jitter-word-set.h jitter/jitter-word-set.c
lib_libjitterutility__partial_la_CPPFLAGS = \
  $(AM_CPPFLAGS) $(CPPFLAGS_FOR_INTERNAL_USE)


# The headers need to be installed and be available to the user or to the
# Jitter-generated code in VMs, so they belong in the jitterinclude directory
# along with the rest of the runtime library.
jitterinclude_HEADERS += \
  jitter/jitter-aligned-block.h \
  jitter/jitter-arithmetic.h \
  jitter/jitter-bitwise.h \
  jitter/jitter-cpp.h \
  jitter/jitter-fatal.h \
  jitter/jitter-malloc.h \
  jitter/jitter-dynamic-buffer.h \
  jitter/jitter-hash.h \
  jitter/jitter-heap.h \
  jitter/jitter-heap-debug.h \
  jitter/jitter-human-prefix.h \
  jitter/jitter-list.h \
  jitter/jitter-missing.h \
  jitter/jitter-parse-int.h \
  jitter/jitter-print.h \
  jitter/jitter-readline.h \
  jitter/jitter-string.h \
  jitter/jitter-time.h \
  jitter/jitter-word-set.h

# Unit tests for libjitterutility.
if JITTER_ENABLE_TEST_SUITE
  check_PROGRAMS += tests/unit/heap-test
tests_unit_heap_test_SOURCES = tests/unit/heap-test.c
tests_unit_heap_test_LDADD = \
  lib/libjitterutility--partial.la \
  $(LDADD_FOR_GNULIB)
tests_unit_heap_test_CPPFLAGS = \
  $(AM_CPPFLAGS) $(CPPFLAGS_FOR_INTERNAL_USE)
endif # if JITTER_ENABLE_TEST_SUITE


# Bison flags.
# ################################################################

# Recent versions of Bison generate a lot of Yacc-compatibility warnings by
# default, which are just a distraction for Jitter.
BISONFLAGS = -Wno-yacc


# jitterc frontend.
# ################################################################

# The jitterc scanner and parser generated by Flex and Bison.  We generate one C
# file and one header for each one, as typical.
FRONTEND_NON_GENERATED_FILES += jitterc/jitterc.l jitterc/jitterc.y
FRONTEND_GENERATED_FILES += \
  jitterc/jitterc-parser.c jitterc/jitterc-parser.h \
  jitterc/jitterc-scanner.c jitterc/jitterc-scanner.h
$(srcdir)/jitterc/jitterc-parser.c: jitterc/jitterc.y
	$(YACC) $(BISONFLAGS) -o "$@" "$(srcdir)/jitterc/jitterc.y"
$(srcdir)/jitterc/jitterc-parser.h: jitterc/jitterc-parser.c
# Unfortunately a file $(srcdir)/jitterc/jitterc-scanner.c generated the obvious
# way contains #include lines including standard headers before config.h , which
# breaks Gnulib.  Amend the generated file by prepending an appopriate #include
# line.  The same is true for the header file.
$(srcdir)/jitterc/jitterc-scanner.c: jitterc/jitterc.l
	$(LEX) \
	  --header-file=$(srcdir)/jitterc/jitterc-scanner.h_tmp \
	  -o"$@"_tmp \
	  "$(srcdir)/jitterc/jitterc.l" && \
	  echo '#include <config.h> /* Non-flex-generated line */' \
	  | cat - "$@"_tmp > "$@" && \
          rm "$@"_tmp
$(srcdir)/jitterc/jitterc-scanner.h: jitterc/jitterc-scanner.c
	echo '#include <config.h> /* Non-flex-generated line */' \
	  | cat - "$@"_tmp > "$@" && \
          rm "$@"_tmp


# jitter C-code-generator program.
# ################################################################

# This is the program generating C files from a ".jitter" VM specification.  Its
# C sources and this Makefile.am always use the "jitterc" prefix, to distinguish
# it from a generic user program embedding a Jittery VM.
# Anyway the user does not need this distinction: the C-generating executable is
# simply called "jitter".
bin_PROGRAMS += bin/jitter
bin_jitter_SOURCES = \
  jitterc/jitterc-generate.c jitterc/jitterc-generate.h \
  jitterc/jitterc-main.c \
  jitterc/jitterc-utility.c jitterc/jitterc-utility.h \
  jitterc/jitterc-mangle.c jitterc/jitterc-mangle.h \
  jitterc/jitterc-rewrite.c jitterc/jitterc-rewrite.h \
  jitterc/jitterc-vm.c jitterc/jitterc-vm.h \
  jitterc/jitterc-parser.c jitterc/jitterc-parser.h \
  jitterc/jitterc-scanner.c jitterc/jitterc-scanner.h
bin_jitter_LDADD = \
  lib/libjitterutility--partial.la \
  $(LDADD_FOR_GNULIB)
bin_jitter_CPPFLAGS = \
  $(CPPFLAGS_FOR_GNULIB) $(AM_CPPFLAGS) $(CPPFLAGS_FOR_PATHS) \
  $(CPPFLAGS_FOR_INTERNAL_USE)


# jitter configuration utility.
# ################################################################

# The jitter-config shell script (preprocessed by config.status ).
bin_SCRIPTS += bin/jitter-config


# M4sh machinery.
# ################################################################

# Add complex suffixes for implicit rules.  Even if there were a default rule
# about how to split something like ".in.m4sh.in" I'd prefer not to rely on it.
SUFFIXES += .in.m4sh .in

# Implicit rule to turn an M4sh m4sh script ( .in.m4sh ) into a portable shell
# script ( .in ) ready to be processed by aclocal for @-substitutions.
# This implicit rule is meant to be always used with sources and targets both
# in the source directory.
# This cannot work on a machine without autom4te installed (autom4te comes with
# Autoconf), so we intentionally fail in that case.
if JITTER_HAVE_AUTOM4TE
# Unfortunately (thanks to Bruno Haible for noticing) Make does not allow
# dependencies on suffix rules and GNU Make in fact ignores such dependencies:
# therefore writing
#    .in.m4sh.in: $(top_srcdir)/m4-utility/jitter-script.m4sh
# , as I was previously doing, was pointless.
# Therefore, this is the best we can do:
# First part: generic suffix rule.
.in.m4sh.in:
	$(AUTOM4TE) \
          --language=M4sh --prepend-include="$(top_srcdir)/m4-utility" \
          --output="$@" "$^" \
          && "$(top_srcdir)/amend-autom4te-output" "$@"
# Second part: explicit dependencies.  At least the body can be factored thanks
# to the suffix rule.
bin/jitter-config.in: $(top_srcdir)/m4-utility/jitter-script.m4sh
scripts/generate-fast-branches.in: $(top_srcdir)/m4-utility/jitter-script.m4sh
else
.in.m4sh.in:
	@echo "cannot rebuild $@ without autom4te"
	@echo "you need to install autoconf and reconfigure"
	@exit 1
endif # if JITTER_HAVE_AUTOM4TE

# The final scripts with @-substitution performed are all rebuilt automatically
# when needed, again thanks to AC_CONFIG_FILES in configure.ac .

# The built target files ( .in ) will be trivially distributed as they occur in
# configure.ac's AC_CONFIG_FILES , but their source files ( .in.m4sh ) must be
# explicitly listed here.
EXTRA_DIST += \
  bin/jitter-config.in.m4sh \
  scripts/generate-fast-branches.in.m4sh

# Also include the common M4sh library, included by the scripts above with
# m4_include (and by Autoconf macros).  Notice that this is not processed by
# aclocal.
EXTRA_DIST += \
  m4-utility/jitter-script.m4sh


# VM routine code frontend based on flex and Bison.
# ################################################################

# The generated C files mentioned here are part of libjitter, for every
# dispatching model; see the SOURCES_FOR_LIBJITTER definition below.

# The frontend sources, machine-generated with flex and Bison, are
# VM-independent, which may be slightly surprising: each VM instruction is
# simply recognized by the scanner as one occurrence of the INSTRUCTION_NAME
# token, and appended to the current program by the parser; checking for
# instruction names and argument types is done out of the parser but still in
# VM-independent code, using VM-dependent data structures.
FRONTEND_NON_GENERATED_FILES += jitter/jitter-routine.l jitter/jitter-routine.y
FRONTEND_GENERATED_FILES += \
   jitter/jitter-routine-parser.c jitter/jitter-routine-parser.h \
   jitter/jitter-routine-scanner.c jitter/jitter-routine-scanner.h
$(srcdir)/jitter/jitter-routine-parser.c: jitter/jitter-routine.y
	$(YACC) $(BISONFLAGS) -o "$@" "$(srcdir)/jitter/jitter-routine.y"
$(srcdir)/jitter/jitter-routine-parser.h: jitter/jitter-routine-parser.c
$(srcdir)/jitter/jitter-routine-scanner.c: jitter/jitter-routine.l
	$(LEX) --header-file=$(srcdir)/jitter/jitter-routine-scanner.h -o"$@" \
	  "$(srcdir)/jitter/jitter-routine.l"
$(srcdir)/jitter/jitter-routine-scanner.h: jitter/jitter-routine-scanner.c


# Jitter template files.
# ################################################################

# Template files are C sources used as data when running jitterc.  They are
# copied into the generated code, systematically replacing any occurrence of the
# text "vmprefix_" and "VMPREFIX_" with the appropriate VM prefix, in lower or
# upper case.  jitterc may further append machine-generated code to template
# copies.
JITTER_TEMPLATES = \
  templates/vm1.c templates/vm.h templates/vm-main.c

# Templates need to be distributed and installed as well, since they are used at
# generation time.  They are machine-independent.
dist_template_DATA += $(JITTER_TEMPLATES)
EXTRA_DIST += $(JITTER_TEMPLATES)


# Jitter flag files.
# ################################################################

# Flag files are short text files containing command-line options for
# preprocessing, compiling and linking C code generated by Jitter, associated to
# each dispatching model.  Flags are architecture-dependent.


# Jitter runtime libraries for every enabled dispatching model.
# ################################################################

# A separate Jitter runtime library will be compiled for each enabled
# dispatching model; the sources are always the same, compiled with different
# CPP definitions.  We use a variable *starting* with rather than ending with
# "SOURCES" in order not to let Automake deduce the existence of any build
# target just from the definition.  The variable SOURCES_FOR_LIBJITTER will
# explicitly occur in defintions of other definitions following the usual
# Automake conventions but this variable serves just for specifying the list of
# source files, once and for all.
# Notice that this also includes the libjitterutility sources; see the comments
# libjitterutility comments above.
# A few architecture-dependent files may be added later to the definition of
# variable; see the comments in "Native-code libjitter sources for supported
# architectures" below.
SOURCES_FOR_LIBJITTER = \
  jitter/jitter.c jitter/jitter.h \
  jitter/jitter-c-compiler-version.h \
  jitter/jitter-instruction.c jitter/jitter-instruction.h \
  jitter/jitter-mutable-routine.c jitter/jitter-mutable-routine.h \
  jitter/jitter-profile.c jitter/jitter-profile.h \
  jitter/jitter-routine.c jitter/jitter-routine.h \
  jitter/jitter-rewrite.c jitter/jitter-rewrite.h \
  jitter/jitter-specialize.c jitter/jitter-specialize.h \
  jitter/jitter-replicate.c jitter/jitter-replicate.h \
  jitter/jitter-mmap.c jitter/jitter-mmap.h \
  jitter/jitter-signals.c jitter/jitter-signals.h \
  jitter/jitter-disassemble.c jitter/jitter-disassemble.h \
  jitter/jitter-fast-branch.c jitter/jitter-fast-branch.h \
  jitter/jitter-fast-branch-machine-generated.h \
  jitter/jitter-executor.h \
  jitter/jitter-routine-parser.c jitter/jitter-routine-parser.h \
  jitter/jitter-routine-scanner.c jitter/jitter-routine-scanner.h \
  jitter/jitter-tagging.c jitter/jitter-tagging.h \
  jitter/jitter-vm.c jitter/jitter-vm.h \
  jitter/jitter-machine-common.S jitter/jitter-machine-common.h \
  jitter/jitter-patch.c jitter/jitter-patch.h \
  jitter/jitter-sections.c jitter/jitter-sections.h \
  jitter/jitter-data-locations.c jitter/jitter-data-locations.h \
  jitter/jitter-patch-in.c jitter/jitter-patch-in.h \
  jitter/jitter-defect.c jitter/jitter-defect.h \
  jitter/jitter-stack.c jitter/jitter-stack.h

# The libjitter headers meant for public use (and all the headers they include
# in their turn, transitively closed) also need to be installed.  Some
# architecture-dependent files may be added later to the definition of this
# variable; see the comments in "Native-code libjitter sources for supported
# architectures" below.
jitterinclude_HEADERS += \
  jitter/jitter.h \
  jitter/jitter-c-compiler-version.h \
  jitter/jitter-data-locations.h \
  jitter/jitter-defect.h \
  jitter/jitter-fast-branch.h \
  jitter/jitter-fast-branch-machine-generated.h \
  jitter/jitter-instruction.h \
  jitter/jitter-machine-common.h \
  jitter/jitter-mmap.h \
  jitter/jitter-profile.h \
  jitter/jitter-signals.h \
  jitter/jitter-mutable-routine.h \
  jitter/jitter-routine.h \
  jitter/jitter-rewrite.h \
  jitter/jitter-specialize.h \
  jitter/jitter-disassemble.h \
  jitter/jitter-executor.h \
  jitter/jitter-patch-in.h \
  jitter/jitter-routine-parser.h \
  jitter/jitter-routine-scanner.h \
  jitter/jitter-sections.h \
  jitter/jitter-tagging.h \
  jitter/jitter-stack.h \
  jitter/jitter-vm.h

# The file jitter/jitter-config.h , generated in the build directory, is
# slightly special, as it needs to be installed but not distributed.
# Conceptually it would belong with SOURCES_FOR_LIBJITTER_NODIST and
# jitterinclude_HEADERS , where it not for the nodist property.
SOURCES_FOR_LIBJITTER_NODIST = \
  jitter/jitter-config.h
nodist_jitterinclude_HEADERS += \
  jitter/jitter-config.h
# See dist-hook below.
dist-hook-remove-nodist-sources:
	chmod u+w "$(distdir)"/jitter
	rm -f "$(distdir)"/jitter/jitter-config.h

# Unconditionally define Jitter runtimes as one partial library per *possible*
# dispatch, including the ones which are disabled and cannot even compile on the
# current configuration.  In order for them not to be built automatically,
# define them in EXTRA_LTLIBRARY.
#
# Each partial runtime library for an *enabled* dispatch will be LIBADDed to one
# of the non-partial libjitter libraries; moreover the library for the best
# dispatching model will be LIBADDed to the "default" libjitter library; this
# avoids one duplicate compilation.
#
# Non-partial libraries are still convenience libraries.  Each of those is also
# embedded in a non-convenience library containing the exact same objects, for
# installation in non-sub-package mode and in order to have a dependency on the
# correct convenience libraries in sub-package mode.

# Switch-dispatch libjitter partial library.
EXTRA_LTLIBRARIES += lib/libjitter--switch--partial.la
lib_libjitter__switch__partial_la_SOURCES = \
  $(SOURCES_FOR_LIBJITTER) $(SOURCES_FOR_LIBJITTER_NODIST)
lib_libjitter__switch__partial_la_CPPFLAGS = \
  $(AM_CPPFLAGS) -DJITTER_DISPATCH_SWITCH

# Direct-threading-dispatch libjitter partial library.
EXTRA_LTLIBRARIES += lib/libjitter--direct-threading--partial.la
lib_libjitter__direct_threading__partial_la_SOURCES = \
  $(SOURCES_FOR_LIBJITTER) $(SOURCES_FOR_LIBJITTER_NODIST)
lib_libjitter__direct_threading__partial_la_CPPFLAGS = \
  $(AM_CPPFLAGS) -DJITTER_DISPATCH_DIRECT_THREADING

# Minimal-threading--partial-dispatch libjitter partial library.
EXTRA_LTLIBRARIES += lib/libjitter--minimal-threading--partial.la
lib_libjitter__minimal_threading__partial_la_SOURCES = \
  $(SOURCES_FOR_LIBJITTER) $(SOURCES_FOR_LIBJITTER_NODIST)
lib_libjitter__minimal_threading__partial_la_CPPFLAGS = \
  $(AM_CPPFLAGS) -DJITTER_DISPATCH_MINIMAL_THREADING

# No-threading--partial-dispatch libjitter partial library.
EXTRA_LTLIBRARIES += lib/libjitter--no-threading--partial.la
lib_libjitter__no_threading__partial_la_SOURCES = \
  $(SOURCES_FOR_LIBJITTER) $(SOURCES_FOR_LIBJITTER_NODIST)
lib_libjitter__no_threading__partial_la_CPPFLAGS = \
  $(AM_CPPFLAGS) -DJITTER_DISPATCH_NO_THREADING

# switch-dispatch libjitter non-partial and non-convenience libraries.
if JITTER_ENABLE_DISPATCH_SWITCH
  EXTRA_LTLIBRARIES += lib/libjitter--switch.la
  lib_libjitter__switch_la_SOURCES =
  lib_libjitter__switch_la_LIBADD = \
    lib/libjitter--switch--partial.la \
    lib/libjitterutility--partial.la
  lib_LTLIBRARIES += non-convenience-lib/libjitter--switch.la
  non_convenience_lib_libjitter__switch_la_SOURCES =
  non_convenience_lib_libjitter__switch_la_DEPENDENCIES = \
    lib/libjitter--switch.la
  non_convenience_lib_libjitter__switch_la_LIBADD = \
    lib/libjitter--switch.la
endif # JITTER_ENABLE_DISPATCH_SWITCH

# Direct-threading-dispatch libjitter non-partial and non-convenience libraries.
if JITTER_ENABLE_DISPATCH_DIRECT_THREADING
  EXTRA_LTLIBRARIES += lib/libjitter--direct-threading.la
  lib_libjitter__direct_threading_la_SOURCES =
  lib_libjitter__direct_threading_la_LIBADD = \
    lib/libjitter--direct-threading--partial.la \
    lib/libjitterutility--partial.la
  lib_LTLIBRARIES += non-convenience-lib/libjitter--direct-threading.la
  non_convenience_lib_libjitter__direct_threading_la_SOURCES =
  non_convenience_lib_libjitter__direct_threading_la_DEPENDENCIES = \
    lib/libjitter--direct-threading.la
  non_convenience_lib_libjitter__direct_threading_la_LIBADD = \
    lib/libjitter--direct-threading.la
endif # JITTER_ENABLE_DISPATCH_DIRECT_THREADING

# Minimal-threading-dispatch libjitter non-partial and non-convenience libraries.
if JITTER_ENABLE_DISPATCH_MINIMAL_THREADING
  EXTRA_LTLIBRARIES += lib/libjitter--minimal-threading.la
  lib_libjitter__minimal_threading_la_SOURCES =
  lib_libjitter__minimal_threading_la_LIBADD = \
    lib/libjitter--minimal-threading--partial.la \
    lib/libjitterutility--partial.la
  lib_LTLIBRARIES += non-convenience-lib/libjitter--minimal-threading.la
  non_convenience_lib_libjitter__minimal_threading_la_SOURCES =
  non_convenience_lib_libjitter__minimal_threading_la_DEPENDENCIES = \
    lib/libjitter--minimal-threading.la
  non_convenience_lib_libjitter__minimal_threading_la_LIBADD = \
    lib/libjitter--minimal-threading.la
endif # JITTER_ENABLE_DISPATCH_MINIMAL_THREADING

# No-threading-dispatch libjitter non-partial and non-convenience libraries.
if JITTER_ENABLE_DISPATCH_NO_THREADING
  EXTRA_LTLIBRARIES += lib/libjitter--no-threading.la
  lib_libjitter__no_threading_la_SOURCES =
  lib_libjitter__no_threading_la_LIBADD = \
    lib/libjitter--no-threading--partial.la \
    lib/libjitterutility--partial.la
  lib_LTLIBRARIES += non-convenience-lib/libjitter--no-threading.la
  non_convenience_lib_libjitter__no_threading_la_SOURCES =
  non_convenience_lib_libjitter__no_threading_la_DEPENDENCIES = \
    lib/libjitter--no-threading.la
  non_convenience_lib_libjitter__no_threading_la_LIBADD = \
    lib/libjitter--no-threading.la
endif # JITTER_ENABLE_DISPATCH_NO_THREADING

# Best-dispatch libjitter non-partial and non-convenience libraries.
EXTRA_LTLIBRARIES += lib/libjitter.la
lib_libjitter_la_SOURCES =
lib_libjitter_la_LIBADD = \
  lib/libjitter--$(JITTER_BEST_DISPATCHING_MODEL)--partial.la \
  lib/libjitterutility--partial.la
lib_LTLIBRARIES += non-convenience-lib/libjitter.la
non_convenience_lib_libjitter_la_SOURCES =
non_convenience_lib_libjitter_la_DEPENDENCIES = \
  lib/libjitter.la
non_convenience_lib_libjitter_la_LIBADD = \
  lib/libjitter.la


# Wrapper library: GNU Libtextstyle print context library.
# ################################################################

# This wrapper library, separate from the Jitter runtime library, is
# optional: it is only built when GNU Libtextstyle is available.
if JITTER_HAVE_LIBTEXTSTYLE
  EXTRA_LTLIBRARIES += lib/libjitter-libtextstyle.la
  lib_libjitter_libtextstyle_la_SOURCES = \
    jitter/jitter-print.h \
    jitter/jitter-print-libtextstyle.c
  lib_LTLIBRARIES += non-convenience-lib/libjitter-libtextstyle.la
  non_convenience_lib_libjitter_libtextstyle_la_SOURCES =
  non_convenience_lib_libjitter_libtextstyle_la_DEPENDENCIES = \
    lib/libjitter-libtextstyle.la
  non_convenience_lib_libjitter_libtextstyle_la_LIBADD = \
    lib/libjitter-libtextstyle.la

  # Actually use libjitter-libtextstyle in the examples.  It is easier to define
  # the CPP macro and to link the library for all of them, including those not
  # not needing the wrapper.
  AM_CPPFLAGS += @JITTER_LIBTEXTSTYLE_CPPFLAGS@
  LDADD_FOR_EXAMPLES += @JITTER_LIBTEXTSTYLE_LDADD@
endif # if JITTER_HAVE_LIBTEXTSTYLE


# Wrapper library: GNU Readline print context library.
# ################################################################

# This wrapper library, separate from the Jitter runtime library, is always
# built.  Whether it uses (and therefore depends on) GNU Readline depends on
# the configuration.
EXTRA_LTLIBRARIES += lib/libjitter-readline.la
lib_libjitter_readline_la_SOURCES = \
  jitter/jitter-readline.h \
  jitter/jitter-readline.c
lib_LTLIBRARIES += non-convenience-lib/libjitter-readline.la
non_convenience_lib_libjitter_readline_la_SOURCES =
non_convenience_lib_libjitter_readline_la_DEPENDENCIES = \
  lib/libjitter-readline.la
non_convenience_lib_libjitter_readline_la_LIBADD = \
  lib/libjitter-readline.la
# Use libjitter-readline for the examples.  It is easier to do this for every
# example; only some will make use of libjitter-readline .
LDADD_FOR_EXAMPLES += @JITTER_READLINE_LDADD@


# Experimental: ELF symbol visibility.
# ################################################################

# In case we are to use -fvisibility-hidden, use it everywhere: notice that
# AM_CFLAGS is used in the definition of target-specific target_CFLAGS, so its
# content is always taken into account.
if JITTER_ENABLE_VISIBILITY_HIDDEN
  AM_CFLAGS += -fvisibility=hidden
  AM_LDFLAGS += -fvisibility=hidden
endif # if JITTER_ENABLE_VISIBILITY_HIDDEN


# Non-Libtool static libraries for sub-package mode.
# ################################################################

# In sub-package mode it is convenient to build ordinary non-Libtool static
# libraries as well, in case the super-package does not use Libtool.

# This is easy: each library to be built as an ordinary static library will
# depend on its Libtool counterpart, and use Libtool to be extracted
# into $(top_builddir)/lib , beside its Libtool version.
if JITTER_SUBPACKAGE
# A generic way of extracting a non-Libtool static library file from its Libtool
# counterpart.
# It would be nice to call libtool with -Wnone , but that is not supported by
# older versions, such as the ones shipping with OpenBSD as of early 2020
# (reported by Alfred M. Szmidt).
.la.a:
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) -o $(abs_top_builddir)/$@ $<
	$(LIBTOOL) --finish $(abs_top_builddir)/lib

# For every enabled dispatch add one library.
if JITTER_ENABLE_DISPATCH_SWITCH
noinst_DATA += lib/libjitter--switch.a
endif # JITTER_ENABLE_DISPATCH_SWITCH
if JITTER_ENABLE_DISPATCH_DIRECT_THREADING
noinst_DATA += lib/libjitter--direct-threading.a
endif # JITTER_ENABLE_DISPATCH_DIRECT_THREADING
if JITTER_ENABLE_DISPATCH_MINIMAL_THREADING
noinst_DATA += lib/libjitter--minimal-threading.a
endif # JITTER_ENABLE_DISPATCH_MINIMAL_THREADING
if JITTER_ENABLE_DISPATCH_NO_THREADING
noinst_DATA += lib/libjitter--no-threading.a
endif # JITTER_ENABLE_DISPATCH_NO_THREADING

# Add one more library for the default dispatch, unconditionally.
noinst_DATA += lib/libjitter.a

if JITTER_HAVE_LIBTEXTSTYLE
# Add the GNU Libtextstyle wrapper library.
noinst_DATA += lib/libjitter-libtextstyle.a
endif # if JITTER_HAVE_LIBTEXTSTYLE

# Add the Readline wrapper library.
noinst_DATA += lib/libjitter-readline.a

endif # JITTER_SUBPACKAGE


# Native-code libjitter sources for supported architectures.
# ################################################################

# Only if architecture-specific files are actually enabled (the question is
# about being enabled, not being supported) we add architecture-dependent
# sources from the appropriate directory to libjitter and to the installed
# headers.
if JITTER_ENABLE_ASSEMBLY
  # Search included file in "machine/@JITTER_ASSEMBLY_SUBDIRECTORY@" , so that
  # including <jitter/machine/FILE> from a source will find
  # machine/@JITTER_ASSEMBLY_SUBDIRECTORY@/jitter/machine/FILE .  This is only
  # needed when compiling without installed headers: after installation the
  # installed header will look into <jitter/machine/FILE> and find it in the
  # default include path, without any -I option.
  AM_CPPFLAGS += \
    -I "$(top_srcdir)/machine/@JITTER_ASSEMBLY_SUBDIRECTORY@"

  # Add architecture-dependent sources to libjitter.
  SOURCES_FOR_LIBJITTER += \
     machine/@JITTER_ASSEMBLY_SUBDIRECTORY@/jitter/machine/jitter-machine.h \
     machine/@JITTER_ASSEMBLY_SUBDIRECTORY@/jitter/machine/jitter-machine-assembly.S \
     machine/@JITTER_ASSEMBLY_SUBDIRECTORY@/jitter/machine/jitter-machine-c.c

  # One architecture-specific header is used when compiling generated code, even
  # if it's never included directly by user code.  This is needed in particular
  # for reserving registers.
  jittermachineinclude_HEADERS += \
    machine/@JITTER_ASSEMBLY_SUBDIRECTORY@/jitter/machine/jitter-machine.h
endif # if JITTER_ENABLE_ASSEMBLY

# All the source files for every architecture have to be distributed,
# independently from which ones we use, if any, in the current configuration.
EXTRA_DIST += \
  machine/aarch64/jitter/machine/jitter-machine.h \
  machine/aarch64/jitter/machine/jitter-machine-assembly.S \
  machine/aarch64/jitter/machine/jitter-machine-c.c \
  machine/alpha/jitter/machine/jitter-machine.h \
  machine/alpha/jitter/machine/jitter-machine-assembly.S \
  machine/alpha/jitter/machine/jitter-machine-c.c \
  machine/arm/jitter/machine/jitter-machine.h \
  machine/arm/jitter/machine/jitter-machine-assembly.S \
  machine/arm/jitter/machine/jitter-machine-c.c \
  machine/m68k/jitter/machine/jitter-machine.h \
  machine/m68k/jitter/machine/jitter-machine-assembly.S \
  machine/m68k/jitter/machine/jitter-machine-c.c \
  machine/mips/jitter/machine/jitter-machine.h \
  machine/mips/jitter/machine/jitter-machine-assembly.S \
  machine/mips/jitter/machine/jitter-machine-c.c \
  machine/powerpc/jitter/machine/jitter-machine.h \
  machine/powerpc/jitter/machine/jitter-machine-assembly.S \
  machine/powerpc/jitter/machine/jitter-machine-c.c \
  machine/riscv/jitter/machine/jitter-machine.h \
  machine/riscv/jitter/machine/jitter-machine-assembly.S \
  machine/riscv/jitter/machine/jitter-machine-c.c \
  machine/sh/jitter/machine/jitter-machine.h \
  machine/sh/jitter/machine/jitter-machine-assembly.S \
  machine/sh/jitter/machine/jitter-machine-c.c \
  machine/sparc/jitter/machine/jitter-machine.h \
  machine/sparc/jitter/machine/jitter-machine-assembly.S \
  machine/sparc/jitter/machine/jitter-machine-c.c \
  machine/x86_64/jitter/machine/jitter-machine.h \
  machine/x86_64/jitter/machine/jitter-machine-assembly.S \
  machine/x86_64/jitter/machine/jitter-machine-c.c

# Also distribute the dummy architecture-specific files for unsupported
# architectures.  These help with make dist: see the comment in configure.ac .
EXTRA_DIST += \
  machine/dummy--no-threading/jitter/machine/jitter-machine.h \
  machine/dummy--no-threading/jitter/machine/jitter-machine-assembly.S \
  machine/dummy--no-threading/jitter/machine/jitter-machine-c.c \
  machine/dummy--minimal-threading/jitter/machine/jitter-machine.h \
  machine/dummy--minimal-threading/jitter/machine/jitter-machine-assembly.S \
  machine/dummy--minimal-threading/jitter/machine/jitter-machine-c.c


# A remark about building examples and their library dependencies.
# ################################################################

# All the example executables depend on the main Jitter library (in the variant
# for every relevant dispatching model) but these dependencies are not enforced.
# Automake doesn't automatically track these dependencies, because we link
# libraries in *_LDADD , thru $(JITTER_*LDADD) , with -lLIBRARY rather than
# libLIBRARY.la .  The practice here is more useful as an example for Jitter
# users.
#
# The dependencies could be enforced by defining EXTRA_target_DEPENDENCIES for
# each example target, if I wanted the thing to be completely proper.  But
# Jitter users never need to think about this, and it's better not frighten them
# with the added complexity.
#
# In practice building the target "all" before building any specific example
# program is enough.


# Examples and benchmarks: "examples" taget for the user.
# ################################################################

# Add examples conceived to compare performance of a Jittery VM against C.
# About the conditional see the comment below related to the examples target.
EXTRA_PROGRAMS += \
  example-vms/structured/examples/count-primes-in-c \
  example-vms/structured/examples/primes-in-c \
  example-vms/jitterlisp/examples/fibo-in-c
if ! JITTER_SUBPACKAGE
benchmarking-examples: \
  example-vms/structured/examples/count-primes-in-c$(EXEEXT) \
  example-vms/structured/examples/primes-in-c$(EXEEXT) \
  example-vms/jitterlisp/examples/fibo-in-c$(EXEEXT)
endif # ! JITTER_SUBPACKAGE
example_vms_structured_examples_count_primes_in_c_SOURCES = \
  example-vms/structured/examples/count-primes.c
example_vms_structured_examples_primes_in_c_SOURCES = \
  example-vms/structured/examples/primes.c
example_vms_jitterlisp_examples_fibo_in_c_SOURCES = \
  example-vms/jitterlisp/examples/fibo.c

# Again out of convenience, I make examples depend on all , so that a user can
# build the examples before having built the rest, which would normally fail
# because of BUILT_SOURCES.  This target is convenient for the user to build
# manually, and doesn't occur as a dependency of any other target.
#
# In order to discover build dependencies the C files generated by both
# Flex/Bison and Jitter must be available before building any program target;
# this is why make is called recursively, which I normally wouldn't do.
#
# Do not add an examples target when building in subpackage mode: I do not want
# Jitter's examples, which are heavyweight, to be built as a side effect of
# building the examples in a super-package using Jitter.
if ! JITTER_SUBPACKAGE
examples: all benchmarking-examples
	$(MAKE) $(EXTRA_PROGRAMS)
endif # ! JITTER_SUBPACKAGE


# Structured benchmarking examples for the user, not to be compiled.
################################################################

# The user is free to play with those translations of the primes.structured
# example into other languages, but they are not automatically compiled.
EXTRA_DIST += \
  example-vms/structured/examples/primes.java \
  example-vms/structured/examples/primes.py


# Uninspired example VM.
# ################################################################

# The Uninspired VM specification it the actual source...
JITTER_NON_GENERATED_FILES += example-vms/uninspired/uninspired.jitter

# ...And from there we generate C files.
GENERATED_C_FILES_FOR_UNINSPIRED = \
  example-vms/uninspired/uninspired-vm.h \
  example-vms/uninspired/uninspired-vm1.c \
  example-vms/uninspired/uninspired-vm2.c \
  example-vms/uninspired/uninspired-vm-main.c
JITTER_GENERATED_FILES += $(GENERATED_C_FILES_FOR_UNINSPIRED)

# Default settings for the uninspired VM, easy to override from the make command
# line -- but already built targets must be deleted manually.
# FIXME: This is mostly for myself, as the jitterc command line interface is
# going to change.
UNINSPIRED_MAX_FAST_REGISTER_NO = 1
UNINSPIRED_MAX_NONRESIDUAL_NO = -1

# PROGRAMS_UNINSPIRED is a list of example programs to build based on the
# Uninspired VM.  Which programs will be built exactly depends on the
# configuration, so this variable will be added to incrementally.
PROGRAMS_UNINSPIRED =

# Every Uninspired program will be a program, not built by default.
EXTRA_PROGRAMS += $(PROGRAMS_UNINSPIRED)

# # The Jitter-generated C files for Uninspired have to be built early for Make to
# # find source-to-source dependencies.
# BUILT_SOURCES += $(GENERATED_C_FILES_FOR_UNINSPIRED)

# How to generate the first generated file -- this actually generates all of
# them.  The dependency on Makefile.am lets me play with generation options here
# without touching the source files.  Shall I remove the dependency?  Probably.
# By convention all the generated files but the first depend on the first.
# See the comment above in "Files machine-generated by Jitter" about why
# there is no dependency on bin/jitter$(EXEEXT) .
$(srcdir)/example-vms/uninspired/uninspired-vm.h: example-vms/uninspired/uninspired.jitter $(JITTER_TEMPLATES)
	$(JITTER_EMULATOR) bin/jitter$(EXEEXT) \
          "$(srcdir)/example-vms/uninspired/uninspired.jitter" \
          --output "$(srcdir)/example-vms/uninspired/" \
          --frontend \
          --no-line \
          --max-fast-register-no=$(UNINSPIRED_MAX_FAST_REGISTER_NO) \
          --max-nonresidual-no=$(UNINSPIRED_MAX_NONRESIDUAL_NO) \
          --template-directory="$(srcdir)/templates/"
$(srcdir)/example-vms/uninspired/uninspired-vm1.c: example-vms/uninspired/uninspired-vm.h
$(srcdir)/example-vms/uninspired/uninspired-vm2.c: example-vms/uninspired/uninspired-vm.h
$(srcdir)/example-vms/uninspired/uninspired-vm-main.c: example-vms/uninspired/uninspired-vm.h

# Uninspired VM program: switch.
if JITTER_ENABLE_DISPATCH_SWITCH
  PROGRAMS_UNINSPIRED += bin/uninspired--switch
  bin_uninspired__switch_SOURCES = \
    $(GENERATED_C_FILES_FOR_UNINSPIRED)
  bin_uninspired__switch_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(top_srcdir)/example-vms/uninspired/" \
    $(JITTER_SWITCH_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_uninspired__switch_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_SWITCH_CFLAGS)
  bin_uninspired__switch_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_SWITCH_LDFLAGS)
  bin_uninspired__switch_LDADD = \
    $(JITTER_SWITCH_LDADD) $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
endif # JITTER_ENABLE_DISPATCH_SWITCH

# Uninspired VM program: direct threading.
if JITTER_ENABLE_DISPATCH_DIRECT_THREADING
  PROGRAMS_UNINSPIRED += bin/uninspired--direct-threading
  bin_uninspired__direct_threading_SOURCES = \
    $(GENERATED_C_FILES_FOR_UNINSPIRED)
  bin_uninspired__direct_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(top_srcdir)/example-vms/uninspired/" \
    $(JITTER_DIRECT_THREADING_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_uninspired__direct_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_DIRECT_THREADING_CFLAGS)
  bin_uninspired__direct_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_DIRECT_THREADING_LDFLAGS)
  bin_uninspired__direct_threading_LDADD = \
    $(JITTER_DIRECT_THREADING_LDADD) $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
endif # JITTER_ENABLE_DISPATCH_DIRECT_THREADING

# Uninspired VM program: minimal threading.
if JITTER_ENABLE_DISPATCH_MINIMAL_THREADING
  PROGRAMS_UNINSPIRED += bin/uninspired--minimal-threading
  bin_uninspired__minimal_threading_SOURCES = \
    $(GENERATED_C_FILES_FOR_UNINSPIRED)
  bin_uninspired__minimal_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(top_srcdir)/example-vms/uninspired/" \
    $(JITTER_MINIMAL_THREADING_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_uninspired__minimal_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_MINIMAL_THREADING_CFLAGS)
  bin_uninspired__minimal_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_MINIMAL_THREADING_LDFLAGS)
  bin_uninspired__minimal_threading_LDADD = \
    $(JITTER_MINIMAL_THREADING_LDADD) $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
endif # JITTER_ENABLE_DISPATCH_MINIMAL_THREADING

# Uninspired VM program: no threading.
if JITTER_ENABLE_DISPATCH_NO_THREADING
  PROGRAMS_UNINSPIRED += bin/uninspired--no-threading
  bin_uninspired__no_threading_SOURCES = \
    $(GENERATED_C_FILES_FOR_UNINSPIRED)
  bin_uninspired__no_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(top_srcdir)/example-vms/uninspired/" \
    $(JITTER_NO_THREADING_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_uninspired__no_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_NO_THREADING_CFLAGS)
  bin_uninspired__no_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_NO_THREADING_LDFLAGS)
  bin_uninspired__no_threading_LDADD = \
    $(JITTER_NO_THREADING_LDADD) $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
endif # JITTER_ENABLE_DISPATCH_NO_THREADING

# Examples VM routines for the Uninspired VM example, to be distributed.
EXTRA_DIST += \
  example-vms/uninspired/examples/euclid.vm \
  example-vms/uninspired/examples/euclid-faster.vm \
  example-vms/uninspired/examples/euclid-super.vm \
  example-vms/uninspired/examples/euclid-x86_64.s \
  example-vms/uninspired/examples/euclid.pir \
  example-vms/uninspired/examples/euclid.py \
  example-vms/uninspired/examples/euclid-symmetric.py \
  example-vms/uninspired/examples/euclid-simple-assignments.py \
  example-vms/uninspired/examples/counter.vm \
  example-vms/uninspired/examples/counter-mips.s \
  example-vms/uninspired/examples/counter-mips-using-delay-slot.s \
  example-vms/uninspired/examples/counter-sparc.s \
  example-vms/uninspired/examples/counter-x86_64.s \
  example-vms/uninspired/examples/counter.pir \
  example-vms/uninspired/examples/counter.py \
  example-vms/uninspired/examples/counter.scm \
  example-vms/uninspired/examples/counter-iterator.py \
  example-vms/uninspired/examples/bubble-sort.vm \
  example-vms/uninspired/examples/bubble-sort-random.vm \
  example-vms/uninspired/examples/signals.vm \
  example-vms/uninspired/examples/sqrt-2.vm \
  example-vms/uninspired/examples/threading-benchmark.vm \
  example-vms/uninspired/examples/vector-sum.vm

# Distribute the GNU Libtextstyle default style for the Uninspired VM.  FIXME: install.
EXTRA_DIST += example-vms/uninspired/uninspired-style.css


# Structured language example.  The VM isn't really "structured".
# ################################################################

# The Uninspired VM specification it the actual source...
JITTER_NON_GENERATED_FILES += example-vms/structured/structured.jitter

# ...And from there we generate C files.  The main driver is not always used, to
# we define a convenience variable with it, and another without.
GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED = \
  example-vms/structured/structuredvm-vm.h \
  example-vms/structured/structuredvm-vm1.c \
  example-vms/structured/structuredvm-vm2.c
GENERATED_C_FILES_FOR_STRUCTURED_MAIN_INCLUDED = \
  $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED) \
  example-vms/structured/structuredvm-vm-main.c
JITTER_GENERATED_FILES += $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_INCLUDED)

# Default settings for the structured-language VM, easy to override from the
# make command line -- but already built targets must be deleted manually.
# FIXME: This is mostly for myself, as the jitterc command line interface is
# going to change.
STRUCTUREDVM_MAX_FAST_REGISTER_NO = 5
STRUCTUREDVM_MAX_NONRESIDUAL_NO = -1


# # The Jitter-generated C files for the structured-language VM have to be built
# # early for Make to find source-to-source dependencies.
# BUILT_SOURCES += $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_INCLUDED)

# Defined a variable for the structured-language example sources.  It will be
# reused several times.
SOURCES_FOR_STRUCTURED = \
  example-vms/structured/structured-syntax.c \
  example-vms/structured/structured-syntax.h \
  example-vms/structured/structured-scanner.c \
  example-vms/structured/structured-scanner.h \
  example-vms/structured/structured-parser.c \
  example-vms/structured/structured-parser.h \
  example-vms/structured/structured-code-generator.h \
  example-vms/structured/structured-code-generator.c \
  example-vms/structured/structured-code-generator-stack.h \
  example-vms/structured/structured-code-generator-stack.c \
  example-vms/structured/structured-code-generator-register.h \
  example-vms/structured/structured-code-generator-register.c \
  $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED) \
  example-vms/structured/structured-main.c

# Before we can use these sources in any way some machine-generated C files have
# to exist already, or make won't be able to discover the correct dependencies.
#
# These forced dependencies are meant for users building from git: distributed
# tarball already contain machine-generated C files.
example-vms/structured/structured-code-generator.c: $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED)
example-vms/structured/structured-code-generator.h: $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED)
example-vms/structured/structured-code-generator-stack.c: $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED)
example-vms/structured/structured-code-generator-stack.h: $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED)
example-vms/structured/structured-code-generator-register.c: $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED)
example-vms/structured/structured-code-generator-register.h: $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED)
example-vms/structured/structured-main.c: $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_EXCLUDED)

# Like above, define a variable for the sources of the structured-VM standalone
# program.  These are particularly simple: everything generated by Jitter
# including the main driver file, and nothing else.
SOURCES_FOR_STRUCTUREDVM = $(GENERATED_C_FILES_FOR_STRUCTURED_MAIN_INCLUDED)

# PROGRAMS_STRUCTURED is a list of example programs to build based on the
# "Structured" VM.  Which programs will be built exactly depends on the
# configuration, so this variable will be added to incrementally.
PROGRAMS_STRUCTURED =

# Every "Structured-VM" program will be a program, not built by default.
EXTRA_PROGRAMS += $(PROGRAMS_STRUCTURED)

# The structured-language example and the matching standalone "structured" VM,
# with direct-threading dispatch.
PROGRAMS_STRUCTURED += bin/structured
bin_structured_SOURCES = $(SOURCES_FOR_STRUCTURED)
bin_structured_CPPFLAGS = \
  $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/structured" $(JITTER_CPPFLAGS)
bin_structured_CFLAGS = \
  $(AM_CFLAGS) $(JITTER_CFLAGS)
bin_structured_LDADD = \
  $(AM_LDADD) $(JITTER_LDADD) $(LDADD_FOR_EXAMPLES)
bin_structured_LDFLAGS = \
  $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_LDFLAGS)
PROGRAMS_STRUCTURED += bin/structuredvm
bin_structuredvm_SOURCES = $(SOURCES_FOR_STRUCTUREDVM)
bin_structuredvm_CPPFLAGS = $(bin_structured_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
bin_structuredvm_CFLAGS = $(bin_structured_CFLAGS)
bin_structuredvm_LDADD = \
  $(bin_structured_LDADD) $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
bin_structuredvm_LDFLAGS = $(bin_structured_LDFLAGS)

# The structured example , with switch dispatch.
if JITTER_ENABLE_DISPATCH_SWITCH
  PROGRAMS_STRUCTURED += bin/structured--switch
  bin_structured__switch_SOURCES = $(SOURCES_FOR_STRUCTURED)
  bin_structured__switch_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/structured" \
    $(JITTER_SWITCH_CPPFLAGS)
  bin_structured__switch_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_SWITCH_CFLAGS)
  bin_structured__switch_LDADD = \
    $(AM_LDADD) $(JITTER_SWITCH_LDADD) $(LDADD_FOR_EXAMPLES)
  bin_structured__switch_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_SWITCH_LDFLAGS)
  PROGRAMS_STRUCTURED += bin/structuredvm--switch
  bin_structuredvm__switch_SOURCES = $(SOURCES_FOR_STRUCTUREDVM)
  bin_structuredvm__switch_CPPFLAGS = \
    $(bin_structured__switch_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_structuredvm__switch_CFLAGS = \
    $(bin_structured__switch_CFLAGS)
  bin_structuredvm__switch_LDADD = \
    $(bin_structured__switch_LDADD) $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_structuredvm__switch_LDFLAGS = \
    $(bin_structured__switch_LDFLAGS)
endif # JITTER_ENABLE_DISPATCH_SWITCH

# The structured-language example and the matching standalone "structured" VM,
# with direct-threading dispatch.
if JITTER_ENABLE_DISPATCH_DIRECT_THREADING
  PROGRAMS_STRUCTURED += bin/structured--direct-threading
  bin_structured__direct_threading_SOURCES = $(SOURCES_FOR_STRUCTURED)
  bin_structured__direct_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/structured" \
    $(JITTER_DIRECT_THREADING_CPPFLAGS)
  bin_structured__direct_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_DIRECT_THREADING_CFLAGS)
  bin_structured__direct_threading_LDADD = \
    $(AM_LDADD) $(JITTER_DIRECT_THREADING_LDADD) $(LDADD_FOR_EXAMPLES)
  bin_structured__direct_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_DIRECT_THREADING_LDFLAGS)
  PROGRAMS_STRUCTURED += bin/structuredvm--direct-threading
  bin_structuredvm__direct_threading_SOURCES = $(SOURCES_FOR_STRUCTUREDVM)
  bin_structuredvm__direct_threading_CPPFLAGS = \
    $(bin_structured__direct_threading_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_structuredvm__direct_threading_CFLAGS = \
    $(bin_structured__direct_threading_CFLAGS)
  bin_structuredvm__direct_threading_LDADD = \
    $(bin_structured__direct_threading_LDADD) \
    $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_structuredvm__direct_threading_LDFLAGS = \
    $(bin_structured__direct_threading_LDFLAGS)
endif # JITTER_ENABLE_DISPATCH_DIRECT_THREADING

# The structured-language example and the matching standalone "structured" VM,
# with minimal-threading dispatch.
if JITTER_ENABLE_DISPATCH_MINIMAL_THREADING
  PROGRAMS_STRUCTURED += bin/structured--minimal-threading
  bin_structured__minimal_threading_SOURCES = $(SOURCES_FOR_STRUCTURED)
  bin_structured__minimal_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/structured" \
    $(JITTER_MINIMAL_THREADING_CPPFLAGS)
  bin_structured__minimal_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_MINIMAL_THREADING_CFLAGS)
  bin_structured__minimal_threading_LDADD = \
    $(AM_LDADD) $(JITTER_MINIMAL_THREADING_LDADD) $(LDADD_FOR_EXAMPLES)
  bin_structured__minimal_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_MINIMAL_THREADING_LDFLAGS)
  PROGRAMS_STRUCTURED += bin/structuredvm--minimal-threading
  bin_structuredvm__minimal_threading_SOURCES = $(SOURCES_FOR_STRUCTUREDVM)
  bin_structuredvm__minimal_threading_CPPFLAGS = \
    $(bin_structured__minimal_threading_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_structuredvm__minimal_threading_CFLAGS = \
    $(bin_structured__minimal_threading_CFLAGS)
  bin_structuredvm__minimal_threading_LDADD = \
    $(bin_structured__minimal_threading_LDADD) \
    $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_structuredvm__minimal_threading_LDFLAGS = \
    $(bin_structured__minimal_threading_LDFLAGS)
endif # JITTER_ENABLE_DISPATCH_MINIMAL_THREADING

# The structured-language example and the matching standalone "structured" VM,
# with no-threading dispatch.
if JITTER_ENABLE_DISPATCH_NO_THREADING
  PROGRAMS_STRUCTURED += bin/structured--no-threading
  bin_structured__no_threading_SOURCES = $(SOURCES_FOR_STRUCTURED)
  bin_structured__no_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/structured" \
    $(JITTER_NO_THREADING_CPPFLAGS)
  bin_structured__no_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_NO_THREADING_CFLAGS)
  bin_structured__no_threading_LDADD = \
    $(AM_LDADD) $(JITTER_NO_THREADING_LDADD) $(LDADD_FOR_EXAMPLES)
  bin_structured__no_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_NO_THREADING_LDFLAGS)
  PROGRAMS_STRUCTURED += bin/structuredvm--no-threading
  bin_structuredvm__no_threading_SOURCES = $(SOURCES_FOR_STRUCTUREDVM)
  bin_structuredvm__no_threading_CPPFLAGS = \
    $(bin_structured__no_threading_CPPFLAGS) $(CPPFLAGS_FOR_GNULIB)
  bin_structuredvm__no_threading_CFLAGS = \
    $(bin_structured__no_threading_CFLAGS)
  bin_structuredvm__no_threading_LDADD = \
    $(bin_structured__no_threading_LDADD) \
    $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_structuredvm__no_threading_LDFLAGS = \
    $(bin_structured__no_threading_LDFLAGS)
endif # JITTER_ENABLE_DISPATCH_NO_THREADING

# Structured-language scanner and parser generated by Flex and Bison.
FRONTEND_NON_GENERATED_FILES += \
  example-vms/structured/structured.l example-vms/structured/structured.y
FRONTEND_GENERATED_FILES += \
  example-vms/structured/structured-scanner.c \
  example-vms/structured/structured-scanner.h \
  example-vms/structured/structured-parser.c \
  example-vms/structured/structured-parser.h
$(srcdir)/example-vms/structured/structured-parser.c: example-vms/structured/structured.y
	$(YACC) $(BISONFLAGS) -o "$@" "$(srcdir)/example-vms/structured/structured.y"
$(srcdir)/example-vms/structured/structured-parser.h: example-vms/structured/structured-parser.c
$(srcdir)/example-vms/structured/structured-scanner.c: example-vms/structured/structured.l
	$(LEX) --header-file="$(srcdir)/example-vms/structured/structured-scanner.h" -o"$@" \
          "$(srcdir)/example-vms/structured/structured.l"
$(srcdir)/example-vms/structured/structured-scanner.h: example-vms/structured/structured-scanner.c

# Jitter-generated files for the structured-language VM, used for the structured
# language.  By convention all the generated files but the first depend on the
# first; notice that the build system in example-vms/structured/ doesn't generate
# the frontend file structuredvm-vm-main.c, so it would be dangerous to rely on
# "the same" first file to imply the presence of the others; for this reason in this
# Makefile.am the "first" file is structuredvm-vm-main.c .
# See the comment above in "Files machine-generated by Jitter" about why
# there is no dependency on bin/jitter$(EXEEXT) .
$(srcdir)/example-vms/structured/structuredvm-vm-main.c: example-vms/structured/structured.jitter $(JITTER_TEMPLATES)
	$(JITTER_EMULATOR) bin/jitter$(EXEEXT) \
          "$(srcdir)/example-vms/structured/structured.jitter" \
          --output "$(srcdir)/example-vms/structured/" \
          --frontend \
          --no-line \
          --max-fast-register-no=$(STRUCTUREDVM_MAX_FAST_REGISTER_NO) \
          --max-nonresidual-no=$(STRUCTUREDVM_MAX_NONRESIDUAL_NO) \
          --template-directory="$(srcdir)/templates/"
$(srcdir)/example-vms/structured/structuredvm-vm.h: example-vms/structured/structuredvm-vm-main.c
$(srcdir)/example-vms/structured/structuredvm-vm1.c: example-vms/structured/structuredvm-vm-main.c
$(srcdir)/example-vms/structured/structuredvm-vm2.c: example-vms/structured/structuredvm-vm-main.c

# Example high-level routines for the Structured example, to be distributed.
EXTRA_DIST += \
  example-vms/structured/examples/count-down.structured \
  example-vms/structured/examples/count-primes.structured \
  example-vms/structured/examples/count-up-down.structured \
  example-vms/structured/examples/euclid-iterative.structured \
  example-vms/structured/examples/euclid-recursive.structured \
  example-vms/structured/examples/even-odd.structured \
  example-vms/structured/examples/fibo.structured \
  example-vms/structured/examples/primes.structured \
  example-vms/structured/examples/count-primes.c \
  example-vms/structured/examples/primes.c

# Example VM-assembly routines for the Structured (VM) example, to be
# distributed.
EXTRA_DIST += \
  example-vms/structured/examples/count-down-opt.structuredvm \
  example-vms/structured/examples/count-down-nonopt.structuredvm \
  example-vms/structured/examples/euclid-optimized.structuredvm \
  example-vms/structured/examples/euclid-unoptimized.structuredvm

# Distribute the GNU Libtextstyle default style for Structured and the Structured VM.  FIXME: install.
EXTRA_DIST += example-vms/structured/structured-style.css


# Jittery Lisp example.
# ################################################################

# The JitterLisp VM specification is the actual source.  This ensures it's
# distributed.
JITTER_NON_GENERATED_FILES += example-vms/jitterlisp/jitterlisp.jitter

# Start with an empty list of JitterLisp programs.  I will add to this
# incrementally, checking if every variant is to be built.
PROGRAMS_JITTERLISP =

# Every JitterLisp system will be a program, not built by default.
EXTRA_PROGRAMS += $(PROGRAMS_JITTERLISP)

# This variable will be useful later, when I compile different JitterLisp
# programs, each with its own dispatching model and other configuration
# parameters.  Right now there isn't even Jitter-generated code.
SOURCES_FOR_JITTERLISP = \
  example-vms/jitterlisp/jitterlisp-sexpression.c \
  example-vms/jitterlisp/jitterlisp-sexpression.h \
  example-vms/jitterlisp/jitterlisp-allocator.h \
  example-vms/jitterlisp/jitterlisp-allocator.c \
  example-vms/jitterlisp/jitterlisp-ast.c \
  example-vms/jitterlisp/jitterlisp-ast.h \
  example-vms/jitterlisp/jitterlisp-banner.h \
  example-vms/jitterlisp/jitterlisp-banner.c \
  example-vms/jitterlisp/jitterlisp-code-generator.c \
  example-vms/jitterlisp/jitterlisp-code-generator.h \
  example-vms/jitterlisp/jitterlisp-constants.h \
  example-vms/jitterlisp/jitterlisp-constants.c \
  example-vms/jitterlisp/jitterlisp-error.h \
  example-vms/jitterlisp/jitterlisp-error.c \
  example-vms/jitterlisp/jitterlisp-eval-interpreter.h \
  example-vms/jitterlisp/jitterlisp-eval-interpreter.c \
  example-vms/jitterlisp/jitterlisp-eval-vm.h \
  example-vms/jitterlisp/jitterlisp-eval-vm.c \
  example-vms/jitterlisp/jitterlisp-eval.h \
  example-vms/jitterlisp/jitterlisp-eval.c \
  example-vms/jitterlisp/jitterlisp-macros.h \
  example-vms/jitterlisp/jitterlisp-macros.c \
  example-vms/jitterlisp/jitterlisp-operations.h \
  example-vms/jitterlisp/jitterlisp-operations.c \
  example-vms/jitterlisp/jitterlisp-primitives.h \
  example-vms/jitterlisp/jitterlisp-primitives.c \
  example-vms/jitterlisp/jitterlisp-printer.h \
  example-vms/jitterlisp/jitterlisp-printer.c \
  example-vms/jitterlisp/jitterlisp-reader.h \
  example-vms/jitterlisp/jitterlisp-reader.c \
  example-vms/jitterlisp/jitterlisp-run-input.h \
  example-vms/jitterlisp/jitterlisp-run-input.c \
  example-vms/jitterlisp/jitterlisp-settings.h \
  example-vms/jitterlisp/jitterlisp-settings.c \
  example-vms/jitterlisp/jitterlisp-utility.h \
  example-vms/jitterlisp/jitterlisp-utility.c \
  example-vms/jitterlisp/jitterlisp.c \
  example-vms/jitterlisp/jitterlisp.h \
  example-vms/jitterlisp/jitterlisp-config.h \
  example-vms/jitterlisp/jitterlisp-constant-strings.h \
  example-vms/jitterlisp/jitterlisp-constant-strings.c \
  example-vms/jitterlisp/jitterlisp-main.c \
  $(GENERATED_C_FILES_FOR_JITTERLISP)

# See the comment before the dependency for
# example-vms/structured/structured-code-generator.c .
example-vms/jitterlisp/jitterlisp-eval-vm.c: $(GENERATED_C_FILES_FOR_JITTERLISP)
example-vms/jitterlisp/jitterlisp-sexpression.c: $(GENERATED_C_FILES_FOR_JITTERLISP)

# How to generate JitterLisp C sources with Jitter.  By convention every file
# but the first depends on the first, but they are actually all generated by
# the same command.
$(srcdir)/example-vms/jitterlisp/jitterlispvm-vm.h: example-vms/jitterlisp/jitterlisp.jitter $(JITTER_TEMPLATES)
	$(JITTER_EMULATOR) bin/jitter$(EXEEXT) \
          "$(srcdir)/example-vms/jitterlisp/jitterlisp.jitter" \
          --output "$(srcdir)/example-vms/jitterlisp/" \
          --no-line \
          --template-directory="$(srcdir)/templates/"
$(srcdir)/example-vms/jitterlisp/jitterlispvm-vm1.c: example-vms/jitterlisp/jitterlispvm-vm.h
$(srcdir)/example-vms/jitterlisp/jitterlispvm-vm2.c: example-vms/jitterlisp/jitterlispvm-vm.h

GENERATED_C_FILES_FOR_JITTERLISP = \
  example-vms/jitterlisp/jitterlispvm-vm.h \
  example-vms/jitterlisp/jitterlispvm-vm1.c \
  example-vms/jitterlisp/jitterlispvm-vm2.c
JITTER_GENERATED_FILES += $(GENERATED_C_FILES_FOR_JITTERLISP)

# A sed script turning an arbitrary text into a C string literal with proper
# delimiters and escapes..
JITTERLISP_SED_STRINGIFY = \
  's/\\/\\\\/g;s/"/\\"/g;s/^/  "/g;s/$$/\\n"/g;s/?/?""/g'

$(srcdir)/example-vms/jitterlisp/jitterlisp-constant-strings.c: example-vms/jitterlisp/jitterlisp-library.lisp NO-WARRANTY COPYING
	(echo "/* This C file is machine-generated, and is not the actual source. */"; \
         echo; \
         echo "const char *jitterlisp_library_string ="; \
         cat "$(srcdir)/example-vms/jitterlisp/jitterlisp-library.lisp" \
          | @SED@ $(JITTERLISP_SED_STRINGIFY); \
         echo ";"; \
         echo "const char *jitterlisp_no_warranty ="; \
         cat "$(srcdir)/NO-WARRANTY" \
          | @SED@ $(JITTERLISP_SED_STRINGIFY); \
         echo ";"; \
         echo "const char *jitterlisp_gpl ="; \
         cat "$(srcdir)/COPYING" \
          | @SED@ $(JITTERLISP_SED_STRINGIFY); \
         echo ";") \
          > "$@"

# The safe and unsafe JitterLisp programss, with the best dispatching model...
PROGRAMS_JITTERLISP += bin/jitterlisp
bin_jitterlisp_SOURCES = $(SOURCES_FOR_JITTERLISP)
bin_jitterlisp_CPPFLAGS = \
  $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/jitterlisp" \
  $(CPPFLAGS_FOR_GNULIB) $(JITTER_CPPFLAGS) \
  -DJITTERLISP_LITTER=1
bin_jitterlisp_CFLAGS = \
  $(AM_CFLAGS) $(JITTER_CFLAGS)
bin_jitterlisp_LDADD = \
  $(AM_LDADD) $(JITTER_LDADD) $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
bin_jitterlisp_LDFLAGS = \
  $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) $(JITTER_LDFLAGS)
EXTRA_bin_jitterlisp_DEPENDENCIES = $(lib_LTLIBRARIES)
PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe
bin_jitterlisp__unsafe_SOURCES = $(SOURCES_FOR_JITTERLISP)
bin_jitterlisp__unsafe_CPPFLAGS = \
  $(bin_jitterlisp_CPPFLAGS) -DJITTERLISP_UNSAFE
bin_jitterlisp__unsafe_CFLAGS = $(bin_jitterlisp_CFLAGS)
bin_jitterlisp__unsafe_LDADD = $(bin_jitterlisp_LDADD)
bin_jitterlisp__unsafe_LDFLAGS = $(bin_jitterlisp_LDFLAGS)
EXTRA_bin_jitterlisp__unsafe_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
# ...And their safe and unsafe best-dispatch Boehm-GC versions, if enabled.
if JITTER_HAVE_BOEHM_GC
  PROGRAMS_JITTERLISP += bin/jitterlisp--boehm
  bin_jitterlisp__boehm_SOURCES = $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__boehm_CPPFLAGS = $(bin_jitterlisp_CPPFLAGS) \
    -UJITTERLISP_LITTER -DJITTERLISP_BOEHM_GC=1
  bin_jitterlisp__boehm_CFLAGS = $(bin_jitterlisp_CFLAGS)
  bin_jitterlisp__boehm_LDADD = -lgc $(bin_jitterlisp_LDADD)
  bin_jitterlisp__boehm_LDFLAGS = $(bin_jitterlisp_LDFLAGS)
  EXTRA_bin_jitterlisp__boehm_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--boehm
  bin_jitterlisp__unsafe__boehm_SOURCES = $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__boehm_CPPFLAGS = \
    $(bin_jitterlisp__boehm_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__boehm_CFLAGS = $(bin_jitterlisp__boehm_CFLAGS)
  bin_jitterlisp__unsafe__boehm_LDADD = $(bin_jitterlisp__boehm_LDADD)
  bin_jitterlisp__unsafe__boehm_LDFLAGS = $(bin_jitterlisp__boehm_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__boehm_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
endif # JITTER_HAVE_BOEHM_GC

# Man page for the main JitterLisp executable.  We currently generate only
# one man page instead of one per each of the up to sixteen functionally
# indentical executables.
if JITTER_CAN_REBUILD_MAN_PAGES
  # See the comment about doc/jitter.1 and its conditional generation.
  $(srcdir)/doc/jitterlisp.1: bin/jitterlisp$(EXEEXT)
	$(HELP2MAN_WITH_OPTIONS) --output="$@" \
          --info-page='jitterlisp'\
	  "$(JITTER_EMULATOR) bin/jitterlisp$(EXEEXT)" || true
else # ! JITTER_CAN_REBUILD_MAN_PAGES
  $(srcdir)/doc/jitterlisp.1: doc/missing-man-page.1
	cp "$(srcdir)/doc/missing-man-page.1" "$@" || true
endif
dist_man1_MANS += $(srcdir)/doc/jitterlisp.1


if JITTER_ENABLE_DISPATCH_SWITCH
  # The safe and unsafe JitterLisp programs, with switch dispatch...
  PROGRAMS_JITTERLISP += bin/jitterlisp--switch
  bin_jitterlisp__switch_SOURCES = $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__switch_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/jitterlisp" \
    $(CPPFLAGS_FOR_GNULIB) $(JITTER_SWITCH_CPPFLAGS) \
    -DJITTERLISP_LITTER=1
  bin_jitterlisp__switch_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_SWITCH_CFLAGS)
  bin_jitterlisp__switch_LDADD = \
    $(AM_LDADD) $(JITTER_SWITCH_LDADD) \
    $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_jitterlisp__switch_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) \
    $(JITTER_SWITCH_LDFLAGS)
  EXTRA_bin_jitterlisp__switch_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--switch
  bin_jitterlisp__unsafe__switch_SOURCES = $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__switch_CPPFLAGS = \
    $(bin_jitterlisp__switch_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__switch_CFLAGS = \
    $(bin_jitterlisp__switch_CFLAGS)
  bin_jitterlisp__unsafe__switch_LDADD = \
    $(bin_jitterlisp__switch_LDADD)
  bin_jitterlisp__unsafe__switch_LDFLAGS = \
    $(bin_jitterlisp__switch_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__switch_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
# ...And their safe and unsafe switch-dispatch Boehm-GC versions, if enabled.
if JITTER_HAVE_BOEHM_GC
    PROGRAMS_JITTERLISP += bin/jitterlisp--boehm--switch
    bin_jitterlisp__boehm__switch_SOURCES = $(SOURCES_FOR_JITTERLISP)
    bin_jitterlisp__boehm__switch_CPPFLAGS = \
      $(bin_jitterlisp__switch_CPPFLAGS) \
      -UJITTERLISP_LITTER -DJITTERLISP_BOEHM_GC=1
    bin_jitterlisp__boehm__switch_CFLAGS = \
      $(bin_jitterlisp__switch_CFLAGS)
    bin_jitterlisp__boehm__switch_LDADD = \
      -lgc $(bin_jitterlisp__switch_LDADD)
    bin_jitterlisp__boehm__switch_LDFLAGS = \
      $(bin_jitterlisp__switch_LDFLAGS)
    EXTRA_bin_jitterlisp__boehm__switch_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--boehm--switch
  bin_jitterlisp__unsafe__boehm__switch_SOURCES = \
    $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__boehm__switch_CPPFLAGS = \
    $(bin_jitterlisp__boehm__switch_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__boehm__switch_CFLAGS = \
    $(bin_jitterlisp__boehm__switch_CFLAGS)
  bin_jitterlisp__unsafe__boehm__switch_LDADD = \
    $(bin_jitterlisp__boehm__switch_LDADD)
  bin_jitterlisp__unsafe__boehm__switch_LDFLAGS = \
    $(bin_jitterlisp__boehm__switch_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__boehm__switch_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
endif # JITTER_HAVE_BOEHM_GC
endif # JITTER_ENABLE_DISPATCH_SWITCH

if JITTER_ENABLE_DISPATCH_DIRECT_THREADING
  # The safe and unsafe JitterLisp programs, with direct-threading dispatch...
  PROGRAMS_JITTERLISP += bin/jitterlisp--direct-threading
  bin_jitterlisp__direct_threading_SOURCES = $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__direct_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/jitterlisp" \
    $(CPPFLAGS_FOR_GNULIB) $(JITTER_DIRECT_THREADING_CPPFLAGS) \
    -DJITTERLISP_LITTER=1
  bin_jitterlisp__direct_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_DIRECT_THREADING_CFLAGS)
  bin_jitterlisp__direct_threading_LDADD = \
    $(AM_LDADD) $(JITTER_DIRECT_THREADING_LDADD) \
    $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_jitterlisp__direct_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) \
    $(JITTER_DIRECT_THREADING_LDFLAGS)
  EXTRA_bin_jitterlisp__direct_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--direct-threading
  bin_jitterlisp__unsafe__direct_threading_SOURCES = \
    $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__direct_threading_CPPFLAGS = \
    $(bin_jitterlisp__direct_threading_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__direct_threading_CFLAGS = \
    $(bin_jitterlisp__direct_threading_CFLAGS)
  bin_jitterlisp__unsafe__direct_threading_LDADD = \
    $(bin_jitterlisp__direct_threading_LDADD)
  bin_jitterlisp__unsafe__direct_threading_LDFLAGS = \
    $(bin_jitterlisp__direct_threading_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__direct_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  # ...And their safe and unsafe direct-threading Boehm-GC versions, if enabled.
if JITTER_HAVE_BOEHM_GC
    PROGRAMS_JITTERLISP += bin/jitterlisp--boehm--direct-threading
    bin_jitterlisp__boehm__direct_threading_SOURCES = $(SOURCES_FOR_JITTERLISP)
    bin_jitterlisp__boehm__direct_threading_CPPFLAGS = \
      $(bin_jitterlisp__direct_threading_CPPFLAGS) \
      -UJITTERLISP_LITTER -DJITTERLISP_BOEHM_GC=1
    bin_jitterlisp__boehm__direct_threading_CFLAGS = \
      $(bin_jitterlisp__direct_threading_CFLAGS)
    bin_jitterlisp__boehm__direct_threading_LDADD = \
      -lgc $(bin_jitterlisp__direct_threading_LDADD)
    bin_jitterlisp__boehm__direct_threading_LDFLAGS = \
      $(bin_jitterlisp__direct_threading_LDFLAGS)
    EXTRA_bin_jitterlisp__boehm__direct_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--boehm--direct-threading
  bin_jitterlisp__unsafe__boehm__direct_threading_SOURCES = \
    $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__boehm__direct_threading_CPPFLAGS = \
    $(bin_jitterlisp__boehm__direct_threading_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__boehm__direct_threading_CFLAGS = \
    $(bin_jitterlisp__boehm__direct_threading_CFLAGS)
  bin_jitterlisp__unsafe__boehm__direct_threading_LDADD = \
    $(bin_jitterlisp__boehm__direct_threading_LDADD)
  bin_jitterlisp__unsafe__boehm__direct_threading_LDFLAGS = \
    $(bin_jitterlisp__boehm__direct_threading_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__boehm__direct_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
endif # JITTER_HAVE_BOEHM_GC
endif # JITTER_ENABLE_DISPATCH_DIRECT_THREADING

if JITTER_ENABLE_DISPATCH_MINIMAL_THREADING
  # The safe and unsafe JitterLisp programs, with minimal-threading dispatch...
  PROGRAMS_JITTERLISP += bin/jitterlisp--minimal-threading
  bin_jitterlisp__minimal_threading_SOURCES = $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__minimal_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/jitterlisp" \
    $(CPPFLAGS_FOR_GNULIB) $(JITTER_MINIMAL_THREADING_CPPFLAGS) \
    -DJITTERLISP_LITTER=1
  bin_jitterlisp__minimal_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_MINIMAL_THREADING_CFLAGS)
  bin_jitterlisp__minimal_threading_LDADD = \
    $(AM_LDADD) $(JITTER_MINIMAL_THREADING_LDADD) \
    $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_jitterlisp__minimal_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) \
    $(JITTER_MINIMAL_THREADING_LDFLAGS)
  EXTRA_bin_jitterlisp__minimal_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--minimal-threading
  bin_jitterlisp__unsafe__minimal_threading_SOURCES = \
    $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__minimal_threading_CPPFLAGS = \
    $(bin_jitterlisp__minimal_threading_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__minimal_threading_CFLAGS = \
    $(bin_jitterlisp__minimal_threading_CFLAGS)
  bin_jitterlisp__unsafe__minimal_threading_LDADD = \
    $(bin_jitterlisp__minimal_threading_LDADD)
  bin_jitterlisp__unsafe__minimal_threading_LDFLAGS = \
    $(bin_jitterlisp__minimal_threading_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__minimal_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  # ...And their safe and unsafe minimal-threading Boehm-GC versions, if enabled.
if JITTER_HAVE_BOEHM_GC
    PROGRAMS_JITTERLISP += bin/jitterlisp--boehm--minimal-threading
    bin_jitterlisp__boehm__minimal_threading_SOURCES = $(SOURCES_FOR_JITTERLISP)
    bin_jitterlisp__boehm__minimal_threading_CPPFLAGS = \
      $(bin_jitterlisp__minimal_threading_CPPFLAGS) \
      -UJITTERLISP_LITTER -DJITTERLISP_BOEHM_GC=1
    bin_jitterlisp__boehm__minimal_threading_CFLAGS = \
      $(bin_jitterlisp__minimal_threading_CFLAGS)
    bin_jitterlisp__boehm__minimal_threading_LDADD = \
      -lgc $(bin_jitterlisp__minimal_threading_LDADD)
    bin_jitterlisp__boehm__minimal_threading_LDFLAGS = \
      $(bin_jitterlisp__minimal_threading_LDFLAGS)
    EXTRA_bin_jitterlisp__boehm__minimal_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--boehm--minimal-threading
  bin_jitterlisp__unsafe__boehm__minimal_threading_SOURCES = \
    $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__boehm__minimal_threading_CPPFLAGS = \
    $(bin_jitterlisp__boehm__minimal_threading_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__boehm__minimal_threading_CFLAGS = \
    $(bin_jitterlisp__boehm__minimal_threading_CFLAGS)
  bin_jitterlisp__unsafe__boehm__minimal_threading_LDADD = \
    $(bin_jitterlisp__boehm__minimal_threading_LDADD)
  bin_jitterlisp__unsafe__boehm__minimal_threading_LDFLAGS = \
    $(bin_jitterlisp__boehm__minimal_threading_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__boehm__minimal_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
endif # JITTER_HAVE_BOEHM_GC
endif # JITTER_ENABLE_DISPATCH_MINIMAL_THREADING

if JITTER_ENABLE_DISPATCH_NO_THREADING
  # The safe and unsafe JitterLisp programs, with no-threading dispatch...
  PROGRAMS_JITTERLISP += bin/jitterlisp--no-threading
  bin_jitterlisp__no_threading_SOURCES = $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__no_threading_CPPFLAGS = \
    $(AM_CPPFLAGS) -I "$(srcdir)/example-vms/jitterlisp" \
    $(CPPFLAGS_FOR_GNULIB) $(JITTER_NO_THREADING_CPPFLAGS) \
    -DJITTERLISP_LITTER=1
  bin_jitterlisp__no_threading_CFLAGS = \
    $(AM_CFLAGS) $(JITTER_NO_THREADING_CFLAGS)
  bin_jitterlisp__no_threading_LDADD = \
    $(AM_LDADD) $(JITTER_NO_THREADING_LDADD) \
    $(LDADD_FOR_EXAMPLES) $(LDADD_FOR_GNULIB)
  bin_jitterlisp__no_threading_LDFLAGS = \
    $(AM_LDFLAGS) $(JITTER_NON_INSTALLED_LDFLAGS) \
    $(JITTER_NO_THREADING_LDFLAGS)
  EXTRA_bin_jitterlisp__no_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--no-threading
  bin_jitterlisp__unsafe__no_threading_SOURCES = \
    $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__no_threading_CPPFLAGS = \
    $(bin_jitterlisp__no_threading_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__no_threading_CFLAGS = \
    $(bin_jitterlisp__no_threading_CFLAGS)
  bin_jitterlisp__unsafe__no_threading_LDADD = \
    $(bin_jitterlisp__no_threading_LDADD)
  bin_jitterlisp__unsafe__no_threading_LDFLAGS = \
    $(bin_jitterlisp__no_threading_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__no_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  # ...And their safe and unsafe no-threading Boehm-GC versions, if enabled.
if JITTER_HAVE_BOEHM_GC
    PROGRAMS_JITTERLISP += bin/jitterlisp--boehm--no-threading
    bin_jitterlisp__boehm__no_threading_SOURCES = $(SOURCES_FOR_JITTERLISP)
    bin_jitterlisp__boehm__no_threading_CPPFLAGS = \
      $(bin_jitterlisp__no_threading_CPPFLAGS) \
      -UJITTERLISP_LITTER -DJITTERLISP_BOEHM_GC=1
    bin_jitterlisp__boehm__no_threading_CFLAGS = \
      $(bin_jitterlisp__no_threading_CFLAGS)
    bin_jitterlisp__boehm__no_threading_LDADD = \
      -lgc $(bin_jitterlisp__no_threading_LDADD)
    bin_jitterlisp__boehm__no_threading_LDFLAGS = \
      $(bin_jitterlisp__no_threading_LDFLAGS)
    EXTRA_bin_jitterlisp__boehm__no_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
  PROGRAMS_JITTERLISP += bin/jitterlisp--unsafe--boehm--no-threading
  bin_jitterlisp__unsafe__boehm__no_threading_SOURCES = \
    $(SOURCES_FOR_JITTERLISP)
  bin_jitterlisp__unsafe__boehm__no_threading_CPPFLAGS = \
    $(bin_jitterlisp__boehm__no_threading_CPPFLAGS) -DJITTERLISP_UNSAFE
  bin_jitterlisp__unsafe__boehm__no_threading_CFLAGS = \
    $(bin_jitterlisp__boehm__no_threading_CFLAGS)
  bin_jitterlisp__unsafe__boehm__no_threading_LDADD = \
    $(bin_jitterlisp__boehm__no_threading_LDADD)
  bin_jitterlisp__unsafe__boehm__no_threading_LDFLAGS = \
    $(bin_jitterlisp__boehm__no_threading_LDFLAGS)
  EXTRA_bin_jitterlisp__unsafe__boehm__no_threading_DEPENDENCIES = $(EXTRA_bin_jitterlisp_DEPENDENCIES)
endif # JITTER_HAVE_BOEHM_GC
endif # JITTER_ENABLE_DISPATCH_NO_THREADING

# JitterLisp Lisp library.
EXTRA_DIST += \
  example-vms/jitterlisp/jitterlisp-library.lisp

# Distribute the GNU Libtextstyle default style for JitterLisp.  FIXME: install.
EXTRA_DIST += example-vms/jitterlisp/jitterlisp-style.css

# Examples for JitterLisp.
EXTRA_DIST += \
  example-vms/jitterlisp/examples/counter-iterative.lisp \
  example-vms/jitterlisp/examples/counter-recursive.lisp \
  example-vms/jitterlisp/examples/derivative.lisp \
  example-vms/jitterlisp/examples/factorial.lisp \
  example-vms/jitterlisp/examples/fibo.lisp \
  example-vms/jitterlisp/examples/peano.lisp \
  example-vms/jitterlisp/examples/fibo.c
# Compatibility files for limited JitterLisp compatibility for
# Common Lisp and Scheme.
EXTRA_DIST += \
  example-vms/jitterlisp/compat/common-lisp-compatibility.lisp \
  example-vms/jitterlisp/compat/gambit-compatibility.scm \
  example-vms/jitterlisp/compat/guile-compatibility.scm \
  example-vms/jitterlisp/compat/mit-gnu-scheme-compatibility.scm \
  example-vms/jitterlisp/compat/racket-compatibility.scm \
  example-vms/jitterlisp/compat/scm-compatibility.scm \
  example-vms/jitterlisp/compat/siod-compatibility.scm \
  example-vms/jitterlisp/compat/sisc-compatibility.scm \
  example-vms/jitterlisp/compat/tinyscheme-compatibility.scm

# Other files for JitterLisp.
EXTRA_DIST += example-vms/jitterlisp/THANKS

# JitterLisp documentation.
info_TEXINFOS += \
  example-vms/jitterlisp/doc/jitterlisp.texi
example_vms_jitterlisp_doc_jitterlisp_TEXINFOS = \
  $(srcdir)/example-vms/jitterlisp/doc/gpl.texi \
  $(srcdir)/example-vms/jitterlisp/doc/fdl.texi
# This is an easy way of not distributing two copies of the same two files.
$(srcdir)/example-vms/jitterlisp/doc/gpl.texi: doc/gpl.texi
	cp "$<" "$@"
$(srcdir)/example-vms/jitterlisp/doc/fdl.texi: doc/fdl.texi
	cp "$<" "$@"


# Build examples
# ################################################################

# These examples using Jitter have their own build system and are not used here.
# However we still have to distribute them, even if not in bootstrapped form
# for reasons of space economy.

# The structured-language example comes with its own clean build system as an
# example of an independent software using Jitter.  The sub-build system is not
# used here, but we have to distribute the files.
EXTRA_DIST += \
  example-build-systems/bootstrap \
  example-build-systems/NEWS \
  example-build-systems/ChangeLog \
  example-build-systems/README \
  example-build-systems/AUTHORS \
  example-build-systems/structured-simple/configure.ac \
  example-build-systems/structured-simple/Makefile.am \
  example-build-systems/structured-with-library/configure.ac \
  example-build-systems/structured-with-library/Makefile.am


# Generate debugging assembly and preprocessed files for humans.
# ################################################################

# FIXME: this is revoltingly ugly and must be replaced with a different
# mechanism.

# How to generate assembly and preprocessed C files.  Of course the exact list
# of files to be generated depends on which dispatching models are enabled.
assembly: $(ASSEMBLY_FILES_FOR_UNINSPIRED)
preprocessed: $(PREPROCESSED_FILES_FOR_UNINSPIRED)
ASSEMBLY_FILES_FOR_UNINSPIRED =
PREPROCESSED_FILES_FOR_UNINSPIRED =

# Compiler options to produce readable assembly.
READABLE_ASSEMBLY_FLAGS = \
  -fverbose-asm \
  $(JITTER_MACHINE_SPECIFIC_READABLE_ASSEMBLY_FLAGS)

# Preprocessor options to produce readable preprocessed C.
READABLE_PREPROCESSED_C_FLAGS =


# More files to distribute: build system and small doc files.
# ################################################################

EXTRA_DIST += \
  NO-WARRANTY \
  amend-autom4te-output \
  bootstrap bootstrap.conf \
  build-aux/git-version-gen \
  build-aux/gitlog-to-changelog \
  build-aux/trivial-source.c
# Notice that README-hacking is *not* supposed to be distributed: in fact the
# "bootstrap" script from Gnulib uses its presence to decide if the current
# source directory comes from a VCS checkout or a tarball.


# Texinfo documentation.
# ################################################################

TEXIINFO_OPTIONS = -I $(abs_top_builddir)/doc
AM_MAKEINFOHTMLFLAGS = $(AM_MAKEINFOFLAGS) --number-sections --fill-column=100

info_TEXINFOS += \
  doc/jitter.texi
doc_jitter_TEXINFOS = \
  $(srcdir)/doc/gpl.texi $(srcdir)/doc/fdl.texi

EXTRA_DIST += \
  doc/COPYING.DOC

# Fail cleanly when attempting to generate hardcopy documentation by using the
# format name as a target if configure did not find the necessary tools.
#
# It would be too messy to do this for every possible manual target, but the
# format target should cover most practical cases where users are likely to
# get surprised.
if ! JITTER_HAVE_TEXI2DVI
missinghardcopytools:
	@echo 'You are missing the tools to generate hardcopy documentation.'
	@exit 0
dvi: missinghardcopytools
ps: missinghardcopytools
pdf: missinghardcopytools
endif # ! JITTER_HAVE_TEXI2DVI

# Completely disable Jitter's documentation building in sub-package mode.
if JITTER_SUBPACKAGE
documentationdisabledinsubpackagemode:
	@echo 'The documentation for Jitter is not built in sub-package mode.'
	@exit 0
info: documentationdisabledinsubpackagemode
html: documentationdisabledinsubpackagemode
dvi: documentationdisabledinsubpackagemode
ps: documentationdisabledinsubpackagemode
pdf: documentationdisabledinsubpackagemode
endif # JITTER_SUBPACKAGE


# Texinfo documentation or lack thereof in sub-package mode.
# ################################################################

# In sub-package mode it is better not to even bother building Info
# documentation.  It is a waste of time and a source of incompatibilities with
# old Texinfo versions, particularly when downloading Jitter from git -- which
# is in fact the way sub-package mode is used.

if JITTER_SUBPACKAGE
# Unfortunately it does not seem to be possible to disable TEXINFOS from
# Automake conditionals, but this workaround is easy.
doc/jitter.info:
	$(MKDIR_P) `dirname "$@"` && touch "$@"
example-vms/jitterlisp/doc/jitterlisp.info:
	$(MKDIR_P) `dirname "$@"` && touch "$@"
info: doc/jitter.info example-vms/jitterlisp/doc/jitterlisp.info
endif # JITTER_SUBPACKAGE


# Web site manuals
# ################################################################

# This is never built as a dependency.
web-manuals:
	(cd "$(abs_builddir)/doc" && \
           mkdir -p web-manuals && \
	   cp "$(abs_srcdir)/build-aux/gendocs.sh" . && \
	   cp "$(abs_srcdir)/gnulib-local-doc/gendocs_template" . && \
           ./gendocs.sh --email "$(JITTER_PACKAGE_BUGREPORT)" \
                        -s "$(abs_srcdir)/doc/jitter.texi" \
                        -o "web-manuals" \
                        jitter "The Jitter manual")


# Man pages
# ################################################################

# Man pages are generated with help2man from executables.  They have to be
# distributed, so that the user doesn't need help2man installed just to compile
# Jitter from the sources.  The rules to rebuild man pages are disabled if the
# user doesn't have help2man.
# Here I use the environment variable ARGP_HELP_FMT to override argp's default
# look in --help , which make section headers start at the second column; that
# does not play well with help2man .
HELP2MAN_WITH_OPTIONS = \
  ARGP_HELP_FMT='header-col=0'; export ARGP_HELP_FMT; \
    $(HELP2MAN) --info-page='jitter' --source='Luca Saiu' --section=1

# If we have no help2man or we are cross-compiling without an emulator, then we
# cannot actually rebuild man pages; in this case we will make copies from a
# stub.  The stub has to be distributed even if it's not needed in the current
# configuration.
EXTRA_DIST += doc/missing-man-page.1

# The actual man pages.
dist_man1_MANS += \
  $(srcdir)/doc/jitter-config.1 \
  $(srcdir)/doc/jitter.1

# Rules for always-installed programs.
if JITTER_CAN_REBUILD_MAN_PAGES
  # Man pages are built from executables, but help2man's documentation
  # recommends to express a dependency from the *source* file instead, so as not
  # to force the user to have help2man installed.  This is sensible, but this
  # solution based on an Automake conditional is even better and allows for more
  # precise control and, importantly, parallel building.
  # The  || true  part at the end is a kludge to avoid failure on distcheck,
  # where man page are generated in a directory which is artificially made
  # read-only; in theory the source directory could be on a read-only filesystem
  # and this might support that use case as well.
  $(srcdir)/doc/jitter.1: bin/jitter$(EXEEXT)
	$(HELP2MAN_WITH_OPTIONS) --output="$@" \
	  "$(JITTER_EMULATOR) bin/jitter$(EXEEXT)" || true
  $(srcdir)/doc/jitter-config.1: bin/jitter-config
	$(HELP2MAN_WITH_OPTIONS) --output="$@" \
	  bin/jitter-config || true
else # ! JITTER_CAN_REBUILD_MAN_PAGES
  # We can't build real man pages on this system, so we just distribute copies
  # of a single stub page.
  # Notice that these rule don't fire for users building from a distributed
  # source tarball, unless they modify the stub page: source tarballs already
  # contain pre-generated man pages (presumably not copied from the stub), and
  # an actual man pages never becomes older than the stub unless the user
  # touches the stub.
  # About  || true  see the comment in the other conditional branch.
  $(srcdir)/doc/jitter.1: doc/missing-man-page.1
	cp "$(srcdir)/doc/missing-man-page.1" "$@" || true
  $(srcdir)/doc/jitter-config.1: doc/missing-man-page.1
	cp "$(srcdir)/doc/missing-man-page.1" "$@" || true
endif # JITTER_CAN_REBUILD_MAN_PAGES

# A user target to build man pages.
mans: $(dist_man1_MANS)


# Emacs Lisp support.
# ################################################################

# The Emacs Lisp major mode for Jitter is to be installed and distributed;
# byte-compilation is automatically enabled.
#
# It is necessary to use dist_lisp_DATA rather than dist_lisp_LISP (recent
# versions of the Automake documentation recommend dist_lisp_DATA anyway)
# in order to work around this Automake bug, still present in Automake master
# as of November 2019:
#   https://debbugs.gnu.org/cgi/bugreport.cgi?bug=38139
# I described a trivial one-character fix for the Automake bug
#   https://lists.gnu.org/archive/html/automake/2019-10/msg00001.html
# , but affected versions will remain in use for some time.  It is better to
# work around the issue here.
dist_lisp_DATA += \
  emacs/jitter-mode.el


# Git versioning support.
# ################################################################

EXTRA_DIST += \
  $(top_srcdir)/.version
BUILT_SOURCES += \
  $(top_srcdir)/.version

$(top_srcdir)/.version:
	echo $(VERSION) > "$@-t" && mv "$@-t" "$@"

# Write a .tarball-version file into the dist directory (to be run before making
# a tarball).
dist-hook-git-versioning:
	chmod u+w "$(distdir)" \
	  && echo $(VERSION) > "$(distdir)"/.tarball-version


# ChangeLog generation.
# ################################################################

JITTER_GITLOG_TO_CHANGELOG_FLAGS = \
  --no-cluster \
  --format='%s%n%n%b%n'

# Replace the current ChangeLog with a text generated from the repo git log,
# but only if we are actually building from git.  If not keep ChangeLog as it
# is.
if JITTER_BUILDING_FROM_GIT
  regenerate-changelog:
	(cd "$(top_srcdir)" \
          && ./build-aux/gitlog-to-changelog \
                $(JITTER_GITLOG_TO_CHANGELOG_FLAGS) > ChangeLog)
else
  regenerate-changelog:
	@echo "Not rebuilding $(top_srcdir)/ChangeLog: no git log"
endif # if JITTER_BUILDING_FROM_GIT

# This rule is a convenient alias for the user to call interactively in order
# to manually regenerate the ChangeLog file.
changelog: regenerate-changelog

# Replace the ChangeLog file in the distribution directory with a freshly
# generated one.
dist-hook-changelog: regenerate-changelog
	chmod u+w "$(distdir)" "$(distdir)"/ChangeLog \
          && cp "$(top_srcdir)/ChangeLog" "$(distdir)"


# Installation.
# ################################################################

# Prevent installation and uninstallation if Jitter was configured in
# sub-package mode.
if JITTER_SUBPACKAGE
do-nothing-on-sub-package-mode-install:
	@echo "Cannot install or uninstall after configuring in sub-package"
	@echo "mode."
	@echo "This is by design, and not an error: succeeding, so as not to"
	@echo "break the install or distcheck rules."
	@exit 0
install: do-nothing-on-sub-package-mode-install
install-data: do-nothing-on-sub-package-mode-install
install-exec: do-nothing-on-sub-package-mode-install
install-info: do-nothing-on-sub-package-mode-install
install-ps: do-nothing-on-sub-package-mode-install
install-dvi: do-nothing-on-sub-package-mode-install
install-pdf: do-nothing-on-sub-package-mode-install
install-html: do-nothing-on-sub-package-mode-install
install-man: do-nothing-on-sub-package-mode-install
uninstall: do-nothing-on-sub-package-mode-install
uninstall-data: do-nothing-on-sub-package-mode-install
uninstall-exec: do-nothing-on-sub-package-mode-install
uninstall-info: do-nothing-on-sub-package-mode-install
uninstall-ps: do-nothing-on-sub-package-mode-install
uninstall-dvi: do-nothing-on-sub-package-mode-install
uninstall-pdf: do-nothing-on-sub-package-mode-install
uninstall-html: do-nothing-on-sub-package-mode-install
uninstall-man: do-nothing-on-sub-package-mode-install
endif # JITTER_SUBPACKAGE


# Distribution.
# ################################################################

# I like to keep different dist-hook s separate.
dist-hook: dist-hook-git-versioning \
           dist-hook-changelog \
           dist-hook-remove-nodist-sources \
           generated-c-files


# Test suite.
# ################################################################

# We use the TAP driver.
TEST_LOG_DRIVER = \
  env AM_TAP_AWK="$(JITTER_AWK)" \
    $(SHELL) $(top_srcdir)/build-aux/tap-driver.sh

# The test scripts.  These are processed by aclocal, so Automake already knows
# their dependencies.  In order to completely disable the test suite if not
# enabled at configure time, we define TESTS within an Automake conditional.
if JITTER_ENABLE_TEST_SUITE
  # FIXME: tests/elementary/init-fini.test is currently disabled
  # TESTS += \
  #   tests/elementary/init-fini.test \
  #   tests/uninspired/interpreter.test
  TESTS += \
    tests/uninspired/fundamental.test \
    tests/uninspired/programs.test \
    tests/structured/programs.test
  # Programs used by test scripts.  FIXME: remove tests/elementary/uninspired-init-fini .
  check_PROGRAMS += \
     tests/filter-test-case-output \
     $(PROGRAMS_UNINSPIRED) $(PROGRAMS_STRUCTURED)

  # # Programs used by test scripts.  FIXME: remove tests/elementary/uninspired-init-fini .
  # check_PROGRAMS += \
  #    tests/elementary/uninspired-init-fini \
  #    $(PROGRAMS_UNINSPIRED)

  # tests_elementary_uninspired_init_fini_SOURCES = \
  #   tests/elementary/uninspired-init-fini.c \
  #   $(GENERATED_C_FILES_FOR_UNINSPIRED)
  # # See the [no longer existing] comment for example-vms/uninspired/uninspired-main.c .
  # $(srcdir)/tests/elementary/uninspired-init-fini.c: \
  #   example-vms/uninspired/uninspired-vm.h
  # tests_elementary_uninspired_init_fini_CPPFLAGS = \
  #   $(AM_CPPFLAGS) -I example-vms/uninspired/
  # tests_elementary_uninspired_init_fini_LDADD = \
  #   lib/libjitter.la $(LDADD_FOR_GNULIB)
  # Express inter- and intra-dependencies for test programs.
  # tests/elementary/init-fini.log: tests/elementary/uninspired-init-fini
  tests/uninspired/fundamental.log: $(check_PROGRAMS)
  tests/uninspired/programs.log: $(check_PROGRAMS)
  tests/structured/programs.log: $(check_PROGRAMS)
else # JITTER_ENABLE_TEST_SUITE is false
  # This causes failure when a disabled test suite is run, which is not
  # desirable in sub-package mode; however in sub-package mode with a
  # disabled test suite making "check" or "check-recursive" has no effect
  # at all, since in that case TEST_SUITE_LOG is AC_SUBSTed to an empty
  # value instead of the default test-suite.log: making all the test
  # log files, which would normally have the effect of running all tests,
  # will just make nothing.
  TESTS += \
    test-suite-disabled
  test-suite-disabled:
	  @echo 'The test suite has been disabled at configure time.'
	  @exit 1
endif # if JITTER_ENABLE_TEST_SUITE

# Test suite output filter.
tests_filter_test_case_output_SOURCES = tests/filter-test-case-output.c
tests_filter_test_case_output_LDADD = $(LDADD_FOR_GNULIB)
tests_filter_test_case_output_CPPFLAGS = \
  $(CPPFLAGS_FOR_GNULIB) $(AM_CPPFLAGS) $(CPPFLAGS_FOR_PATHS) \
  $(CPPFLAGS_FOR_INTERNAL_USE)

# JitterLisp test cases.
if JITTER_ENABLE_TEST_SUITE
  TESTS += tests/jitterlisp/jitterlisp.test
  check_PROGRAMS += $(PROGRAMS_JITTERLISP)
  tests/jitterlisp/jitterlisp.log: $(check_PROGRAMS)
  EXTRA_DIST += tests/jitterlisp/test-definitions.scm
endif # if JITTER_ENABLE_TEST_SUITE

# Tests have to be distributed, but it's not needed to add them to EXTRA_DIST in
# this case since all my tests are script processed by aclocal, and
# AC_CONFIG_FILES inputs are automatically distributed.  However test suite data
# have to be mentioned here, as they are not preprocessed and don't occur as
# dependencies of other things.
EXTRA_DIST += \
  tests/elementary/empty.vm \
  tests/structured/begin-end.structured \
  tests/structured/skip.structured \
  tests/structured/conditional.structured \
  tests/structured/euclid.structured \
  tests/structured/primes.structured \
  tests/structured/even-odd.structured \
  tests/uninspired/load-3-bit.vm \
  tests/uninspired/load-4-bit.vm \
  tests/uninspired/load-5-bit.vm \
  tests/uninspired/load-6-bit.vm \
  tests/uninspired/load-7-bit.vm \
  tests/uninspired/load-8-bit.vm \
  tests/uninspired/load-9-bit.vm \
  tests/uninspired/load-10-bit.vm \
  tests/uninspired/load-11-bit.vm \
  tests/uninspired/load-12-bit.vm \
  tests/uninspired/load-13-bit.vm \
  tests/uninspired/load-16-bit.vm \
  tests/uninspired/load-17-bit.vm \
  tests/uninspired/load-21-bit.vm \
  tests/uninspired/load-22-bit.vm \
  tests/uninspired/load-23-bit.vm \
  tests/uninspired/load-24-bit.vm \
  tests/uninspired/load-25-bit.vm \
  tests/uninspired/load-32-bit.vm \
  tests/uninspired/load-64-bit.vm \
  tests/uninspired/load-594116.vm \
  tests/uninspired/load-823944.vm \
  tests/uninspired/load-143010319.vm \
  tests/uninspired/load-256167675.vm \
  tests/uninspired/load-680185852.vm \
  tests/uninspired/load-1450166639.vm \
  tests/uninspired/load-1625250854.vm \
  tests/uninspired/load-3761490317.vm \
  tests/uninspired/load-0.vm \
  tests/uninspired/load-0xffff.vm \
  tests/uninspired/load-minus-one.vm \
  tests/uninspired/load-minus-two.vm \
  tests/uninspired/add.vm \
  tests/uninspired/sub.vm \
  tests/uninspired/mul.vm \
  tests/uninspired/div.vm \
  tests/uninspired/mod.vm \
  tests/uninspired/unconditional-branch.vm \
  tests/uninspired/conditional-branch-not-taken.vm \
  tests/uninspired/conditional-branch.vm \
  tests/uninspired/count-down.vm \
  tests/uninspired/count-down-slow-register.vm \
  tests/uninspired/count-up-down.vm \
  tests/uninspired/count-up-down-slow-registers.vm \
  tests/uninspired/branch-thru-register.vm \
  tests/uninspired/branch-thru-slow-register.vm \
  tests/uninspired/call-thru-register-leaf.vm \
  tests/uninspired/call-thru-register-non-leaf.vm \
  tests/uninspired/call-leaf.vm \
  tests/uninspired/call-non-leaf.vm

# Every test script depends on tests/utility (an aclocal output, which Automake
# knows how to build).  The generated script is not installed or distributed,
# which is what I want.
check_SCRIPTS += \
  tests/utility


# Cleanup.
# ################################################################

# Remove generated files for the examples.  This is useful to test changes
# in jitterc or in the templates, without touching VM specifications.
clean-generated-examples:
	(cd "$(srcdir)"; rm -f $(JITTER_GENERATED_FILES))

clean-local:
	rm -r -f "$(builddir)/example-vms/*/*.$(OBJEXT)"
	rm -r -f $(EXTRA_PROGRAMS) $(EXTRA_LTLIBRARIES) $(noinst_DATA)
	rm -f uninspired--*.[si]
	rm -f tests/*/*.log
	rm -f tests/*/*.tmp
	rm -f tests/*/*.expected
	rm -f tests/*/*.stdout
	rm -f tests/*/*.stderr
	rm -f tests/*/*.command-line
	rm -f core
	rm -f vgcore.[0-9]*
	@echo 'the clean rule failing in gnulib-local is benign'
	$(MAKE) -C gnulib-local clean || true

# The flags/ subtree is generated by configure, and configuration-specific: it
# should be removed by the distclean target.
# However do not remove man pages, which are in the source directory.
distclean-local:
	rm -rf flags
	@echo 'the distclean rule failing in gnulib-local is benign'
	$(MAKE) -C gnulib-local distclean || true

# All the man pages in Jitter are machine-generated.
remove-man-pages:
	echo	rm -f $(dist_man1_MANS)

# The maintainer-clean target should remove built sources, from both the build
# and the source directories.
maintainer-clean-local: clean-generated-examples remove-man-pages
	rm -f $(DEBUG_ASSEMBLY_FILES)
	(cd "$(srcdir)"; rm -f $(BUILT_SOURCES))
	@echo 'the maintainer-clean rule failing in gnulib-local is benign'
	$(MAKE) -C gnulib-local maintainer-clean || true
