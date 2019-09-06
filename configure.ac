## This is an -*- Autoconf -*- file to be processed with autoconf
## to obtain a configure script.

## Copyright (C) 2016, 2017, 2018, 2019 Luca Saiu
## Written by Luca Saiu

## This file is part of Jitter.

## Jitter is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## Jitter is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with Jitter.  If not, see <http://www.gnu.org/licenses/>.


# Global initialization.
################################################################

AC_PREREQ([2.69])
AC_INIT([Jitter],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [bug-epsilon@gnu.org],
        [jitter],
        [http://ageinghacker.net/jitter])
AC_CONFIG_SRCDIR([NO-WARRANTY])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([build-aux])
AC_REQUIRE_AUX_FILE([trivial-source.c])
AC_CONFIG_HEADERS([config.h jitter/jitter-config.h])


# Autoconf macros for Jitter.
################################################################

# We will not use AC_JITTER here, since we are building Jitter itself rather
# than checking for an installed Jitter; however some of our Autoconf macros,
# which of course are not installed yet, will come in handy.
m4_include([autoconf/jitter.m4])


# Automake initialization.
################################################################

AM_INIT_AUTOMAKE


# Define prefixed aliases for Autoconf macros.
################################################################

# A few macro definitions will end up in an installed header, and their names
# have to be prefixed with JITTER_ so as not to conflict with macros from other
# software.
AC_DEFINE_UNQUOTED([JITTER_PACKAGE_NAME], ["$PACKAGE_NAME"],
                   [The name of the Jitter package])
AC_DEFINE_UNQUOTED([JITTER_PACKAGE_VERSION], ["$PACKAGE_VERSION"],
                   [The Jitter version as a string])
AC_DEFINE_UNQUOTED([JITTER_PACKAGE_BUGREPORT], ["$PACKAGE_BUGREPORT"],
                   [An email address where to report Jitter bugs])


# Make the source and build directory names visible here.
################################################################

# Define jitter_abs_top_builddir and jitter_abs_top_srcdir as ordinary
# shell variables, for internal use here in configure.
#
# The equivalent variables ac_abs_top_srcddir and ac_abs_top_builddir
# are defined very late in the generated configure script, and not
# documented anyway.
jitter_abs_top_builddir="$(pwd)"
cd "${srcdir}"
jitter_abs_top_srcdir="$(pwd)"
cd "${jitter_abs_top_builddir}"


# Sub-package mode support.
################################################################

# Check whether we should configure Jitter in "sub-package mode".  In
# sub-package mode a copy of the Jitter source is distributed in the style of
# Gnulib, as a subdirectory along with another software package using it, so
# that the two are built together.
# Sub-package mode is meant for users of Jitter, and not for developing Jitter
# itself: in order to speed up Jitter's compilation and its test suite, only the
# best available dispatch is enabled.

# In sub-package mode installation does nothing, and dynamic libraries are
# always disabled.  Libraries meant for the user are built as both Libtool
# libraries and ordinary static libraries; the user may choose whether to use
# Libtool in her own super-package.

# Sub-package mode is enabled if the environment variable JITTER_SUBPACKAGE is
# defined to any non-empty value.  This is meant to be defined, and exported, by
# the configure script of the super-package using Jitter as a subpackage; the
# value is meant to be the Jitter subdirectory name relative to the
# super-package source directory, but the actual value is only useful for the
# super-package configuration: here any non-empty value will do, and the source
# and build directories are known in any case.

# Define the feature macro and change the kind of libraries to build by default
# if sub-package mode is enabled.
# In case the command line contradicts the default, I will still force static
# libraries to be enabled and shared libraries to be disabled, just giving a
# warning: see "Libtool support" below.

# Define JITTER_SUBPACKAGE as a precious variable, so that it is described by
# --help and, more importantly, its value at the time of the first configuration
# is kept around in case of an automatic re-configuration.  This is very useful
# in case some Autoconf or Automake source in a sub-package Jitter source
# directory is modified.
AC_ARG_VAR([JITTER_SUBPACKAGE],
           [define to a non-empty value for configuring Jitter as a
            sub-package])

# Change library defaults.
if test "x$JITTER_SUBPACKAGE" != 'x'; then
    AC_DEFINE_UNQUOTED([JITTER_SUBPACKAGE], [1],
                       [Jitter is configured in sub-package mode])
    AC_ENABLE_STATIC
    AC_DISABLE_SHARED
fi

# Define an Automake conditional.
AM_CONDITIONAL([JITTER_SUBPACKAGE],
               [test "x$JITTER_SUBPACKAGE" != 'x'])

# Provide feedback.
AC_MSG_CHECKING([if we are configuring in sub-package mode])
if test "x$JITTER_SUBPACKAGE" != 'x'; then
  AC_MSG_RESULT([yes: "$JITTER_SUBPACKAGE"])
else
  AC_MSG_RESULT([no])
fi


# Check for the C compiler and its features.
################################################################

# Indeed, we rely on a C compiler.
AC_PROG_CC

# This package contains libraries.  When enabling LTO with static libraries (for
# which setting CFLAGS/LDFLAGS is enough) we need to use the specialized library
# tools which come with GCC.  Autoconf doesn't check for them by default, but
# it's easy to do.  If the specialized tools aren't available we fall back on the
# traditional ones.
#
# I noticed that moving the check for RANLIB to this early point prevents the
# default (HOSTTRIPLET-ranlib) from being chosen despite the order of the tools
# chosen here.  Some default might come from gl_EARLY.
AC_CHECK_TOOLS([RANLIB], [gcc-ranlib ranlib])
AC_CHECK_TOOLS([AR], [gcc-ar ar])

# Run the early part of the Gnulib initialization.  This is recommended
# right after AC_PROG_CC .
gl_EARLY

# Make sure the compiler supports a recent version of C.  Without this I
# has problem on OpenBSD, which ships with a very old GCC.
AC_PROG_CC_C99

# Check that we can use -c and -o together with the compiler.  I currently don't
# bother supporting compilers lacking this basic feature, but I could if it were
# actually needed.
AC_PROG_CC_C_O
if test "x$NO_MINUS_C_MINUS_O" != "x"; then
  AC_MSG_NOTICE([your C compiler doesn't accept -c and -o together.
                 Such a compiler is currently not supported, but support would
                 be easy to add -- in case this were the only problem])
  AC_MSG_ERROR([please write to $PACKAGE_BUGREPORT if you want to help])
fi

# We do support cross-compiling.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# Define a feature CPP macro if we are cross-compiling; also set a shell
# variable holding "yes" or "no" to use in this script.
AC_MSG_CHECKING([if we are cross-compiling])
if test "x$build" = "x$host"; then
   AC_MSG_RESULT([no, native compiling on $host .])
   jitter_cross_compiling=no
else
   AC_DEFINE_UNQUOTED([JITTER_CROSS_COMPILING], [1],
                      [Defined if Jitter was cross-compiled.])
   AC_MSG_RESULT([yes, cross-compiling from $build to $host .])
   jitter_cross_compiling=yes
fi

# Installed shell scripts will have to work with a Bourne shell on the host
# system.  In case of native compilation that is simply @SHELL@ as detected
# here, but for cross-compiled installations we can only guess.
if test "x$jitter_cross_compiling" = "xno"; then
  AC_SUBST([SHELL_OR_BIN_SH], [$SHELL])
else
  AC_SUBST([SHELL_OR_BIN_SH], [/bin/sh])
fi

# We have preprocessed assembly files (for some architectures).
AM_PROG_AS

# Can we actually run on something other than GCC?  Probably, as long as the
# compiler supports a recent version of the C standard.
if test "$GCC" = ""; then
   AC_MSG_WARN([not using GCC.  Trying to go on anyway])
fi

# Checks for C type sizes.
AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([float])
AC_CHECK_SIZEOF([double])
AC_CHECK_SIZEOF([long double])
AC_TYPE_SIZE_T

# By definition we will take the size of a pointer as the host machine "word
# size".  This information is also used in the test suite, so define a
# substitution as well.
AC_SUBST([SIZEOF_VOID_P], [$ac_cv_sizeof_void_p])


# Check for the GNU assembler.
################################################################

# Jitter can use Gas features, and ideally I would like to disable them when
# some other assembler is being used, even if this is low-priority.
AC_CHECK_TOOL([JITTER_GNU_ASSEMBLER], [as], [no])
if test "x$JITTER_GNU_ASSEMBLER" != "xno"; then
  if $JITTER_GNU_ASSEMBLER --version > /dev/null \
     && $JITTER_GNU_ASSEMBLER --version \
        | grep 'GNU [aA]ssembler' > /dev/null; then
    # We found an asembler, but it either doesn't accept --version or the output
    # of --version does not say anything similar to "GNU assembler".  I am
    # accepting a capitalized variant as an alternative and not requiring the
    # string to occur at the beginning of a line just to account for some
    # possible future changes in the format of gas --version .
    JITTER_GNU_ASSEMBLER=no
  fi
fi
if test "x$JITTER_GNU_ASSEMBLER" = "xno"; then
   AC_MSG_WARN([the host assembler is not the GNU assembler.  Going on anyway.])
else
   AC_DEFINE_UNQUOTED([JITTER_HOST_ASSEMBLER_IS_GNU], [1],
                      [Defined if the host assembler is the GNU assembler.])
   AC_MSG_NOTICE([the host assembler is the GNU assembler.  Good.])
fi


# Check for system characteristics.
################################################################

# I never test on weird systems not supporting shebangs.
AC_SYS_INTERPRETER
if test "x$interpval" != "xyes"; then
   AC_MSG_WARN([your system doesn't support shebang.  Trying to go on anyway])
fi

# I never test on systems limiting file name length to 14 characters.
AC_SYS_LONG_FILE_NAMES
if test "x$ac_cv_sys_long_file_names" != "xyes"; then
   AC_MSG_WARN([file names are limited to 14 characters.  Things may break])
fi


# Check for programs.
################################################################

# mkdir -p is convenient.  It is currently used in the makefile and in
# vm/generate-instructions.in .
AC_PROG_MKDIR_P

# We call objdump at runtime when disassembling.  Even not having it installed
# is not fatal, and the problem is easy to fix anyway after compiling Jitter.
AC_PATH_PROG([JITTER_OBJDUMP], [objdump])
if test "x$jitter_cross_compiling" = "xyes"; then
  AC_DEFINE_UNQUOTED([JITTER_OBJDUMP],
                     ["objdump"],
                     [name of the native objdump utility on the host system])
elif test "$JITTER_OBJDUMP" = ""; then
  AC_MSG_WARN([no native objdump found. \
Assuming it will be installed and called "objdump", if disassembling is \
desired])
  AC_DEFINE_UNQUOTED([JITTER_OBJDUMP],
                     ["objdump"],
                     [name of the native objdump utility (not actually found)])
else
  AC_DEFINE_UNQUOTED([JITTER_OBJDUMP],
                     ["$JITTER_OBJDUMP"],
                     [name of the native objdump utility])
fi

# We also check for a cross-objdump, which is very convenient when debugging and
# can be invoked from cross-compiled programs run on the build machine thru
# qemu-user.
AC_CHECK_TOOLS([JITTER_CROSS_OBJDUMP], [objdump])
if test "x$JITTER_CROSS_OBJDUMP" = "x"; then
  AC_MSG_WARN([no cross-objdump found])
else
  AC_DEFINE_UNQUOTED([JITTER_CROSS_OBJDUMP],
                     ["$JITTER_CROSS_OBJDUMP"],
                     [name of the cross-objdump utility from GNU binutils])
fi

# Check for tr, using by configure.
AC_PATH_PROG([JITTER_TR], [tr])
if test "x$JITTER_TR" = "x"; then
  AC_MSG_ERROR([you need the Unix tr utility])
fi

# Check for flex.
AM_PROG_LEX
if test "x$LEX" != "xflex"; then
  if test -e "$srcdir/jitterc/jitterc-scanner.c"; then
    AC_MSG_WARN([you will need flex (instead of $LEX) if you modify the
                 distributed C scanners])
  else
    AC_MSG_ERROR([you need flex (instead of $LEX) to generate scanners,
                  if you compile from git])
  fi
fi

# Check for Bison.
AC_PROG_YACC
if ! (echo "x$YACC" | grep -q bison); then
  if test -e "$srcdir/jitterc/jitterc-parser.c"; then
    AC_MSG_WARN([you will need GNU Bison (instead of $YACC) if you modify
                 the distributed C parsers])
  else
    AC_MSG_ERROR([you need GNU Bison (instead of $YACC) to generate parsers,
                  if you compile from git])
  fi
fi

# Check for Texinfo commands.  In practice makeinfo is more important than
# texi2dvi, as Info is the default format.
AC_PATH_PROG([JITTER_MAKEINFO], [makeinfo])
if test "x$JITTER_MAKEINFO" = "x"; then
  if test -e "$srcdir/doc/jitter.info"; then
    AC_MSG_WARN([you will need GNU Texinfo to regenerate the Info \
documentation, in case you change the Texinfo source])
  else
    AC_MSG_ERROR([you need GNU Texinfo to generate the Info documentation, if \
you compile from git; release tarballs contain the documentation \
pre-generated])
  fi
fi
AC_PATH_PROG([JITTER_TEXI2DVI], [texi2dvi])
if test "x$JITTER_TEXI2DVI" = "x"; then
  AC_MSG_WARN([you will need texi2dvi, from GNU Texinfo, if you want to \
generate DVI, PostScript or PDF documentation])
fi
# Define an Automake conditional saying whether it is possible to generate
# hardcopy documentation.  This way failures will be cleaner.
AM_CONDITIONAL([JITTER_HAVE_TEXI2DVI],
               [test "x$JITTER_TEXI2DVI" != "x"])

# Check for GNU help2man.  That is not required for the user: pre-generated man
# pages are included in the distribution.
AC_CHECK_PROG(HAS_HELP2MAN, help2man, yes, no)
if test "x$HAS_HELP2MAN" != "xyes"; then
  jitter_has_help2man="no"
  if test -e "$srcdir/jitterc/jitterc.1"; then
    AC_MSG_WARN([You need GNU help2man if you want to rebuild manual pages])
  else
    AC_MSG_WARN([You need GNU help2man to build manual pages if you compile
                 from git and you are not content with just man page stubs])
  fi
else
  jitter_has_help2man="yes"
fi
AM_MISSING_PROG(HELP2MAN, help2man)


# Check for programs only used in the test suite.
################################################################

# JITTER_CHECK_TESTSUITE_PROGS
# ----------------------------
# Check the availability of a program only used for the test suite in the
# predefined binary paths.  The first program is the name of the substitution to
# define (empty if the program is not found, otherwise its pathname); the second
# argument is a list of possible basenames.  If the program is not found then
# the first argument is added to the shell variable , and a warning message is
# printed.
# jitter_test_suite_missing_progs.
# Example:
# JITTER_CHECK_TESTSUITE_PROG([JITTER_CMP], [some-other-weird-cmp cmp])
AC_DEFUN([JITTER_CHECK_TESTSUITE_PROGS],
  [AC_PATH_PROGS([$1], [$2])
   if test "x$$1" = "x"; then
     AC_MSG_WARN([none of {$2} found.  Cannot run the test suite])
     jitter_test_suite_missing_progs="$jitter_test_suite_missing_progs $1"
   fi])

# Check for optional programs only used by the test suite.
JITTER_CHECK_TESTSUITE_PROGS([JITTER_AWK], [gawk mawk nawk awk])
JITTER_CHECK_TESTSUITE_PROGS([JITTER_CMP], [cmp])
JITTER_CHECK_TESTSUITE_PROGS([JITTER_BASENAME], [basename])
JITTER_CHECK_TESTSUITE_PROGS([JITTER_DIRNAME], [dirname])
#JITTER_CHECK_TESTSUITE_PROGS([JITTER_SEQ], [seq]) # not currently used.

# Some other programs we can live without, still useful (but not mandatory) for
# the test suite:
AC_PATH_PROGS([JITTER_TIMEOUT], [timeout])


# Utility functions for configure.
################################################################

# Output stdin, translating every lower-case ASCII character into its upper-case
# version, and - into _; every other character is left unchanged.
jitter_lowercase_characters='abcdefghijklmnopqrstuvwxyz-'
jitter_uppercase_characters='ABCDEFGHIJKLMNOPQRSTUVWXYZ_'
jitter_to_upper ()
{
  $JITTER_TR "$jitter_lowercase_characters" "$jitter_uppercase_characters"
}

# Perform a translation which is the reverse of the one in jitter_to_upper .
jitter_to_lower ()
{
  $JITTER_TR "$jitter_uppercase_characters" "$jitter_lowercase_characters"
}

# If the given $CC command-line option is supported (its actual effect is not
# tested) then append the option, after a space, to the named shell veriable.
# Two arguments: the name of a shell variable to possibly modify, and a
# candidate $CC option.
jitter_check_cc_option ()
{
  jitter_flag_variable_name="$1"
  jitter_compiler_option="$2"

  # We cache the result of an option being supported (as "yes" or "no") in a
  # shell variable named after the option.  Read the appropriate variable for
  # $jitter_compiler_option into jitter_cached_value .
  jitter_variable_name="jitter_cc_option_supported_"AS_TR_SH($jitter_compiler_option)
  AS_VAR_COPY([jitter_cached_value], [$jitter_variable_name])

  # If we lack a cached value then perform the test, and set jitter_result ;
  # otherwise set jitter_result from the cached value.
  if test "x$jitter_cached_value" = "x"; then
    AC_MSG_CHECKING([if $CC supports the option $jitter_compiler_option])
    if $CC \
         -c -o test.o \
         "$jitter_compiler_option" \
         "$srcdir/build-aux/trivial-source.c" \
           > /dev/null \
           2> /dev/null; then
      jitter_result=yes
    else
      jitter_result=no
    fi
    AS_VAR_SET([$jitter_variable_name], [$jitter_result])
    AC_MSG_RESULT([$jitter_result])
  else
    jitter_result="$jitter_cached_value"
  fi

  # Now $jitter_result is either "yes" or "no".  Use it to decide whether to
  # append the option to the flag variable.
  if test "x$jitter_result" = "xyes"; then
    AS_VAR_COPY([jitter_flag_variable_old_value], [$jitter_flag_variable_name])
    AS_VAR_SET([$jitter_flag_variable_name],
               "$jitter_flag_variable_old_value $jitter_compiler_option")
  fi
}

# Like jitter_check_cc_option , but working on zero or more $CC candidate options.
# One or more arguments: the first is the name of a shell variable, as in the
# first argument of jitter_check_cc_option ; all the other arguments are individual
# candidate $CC options.
jitter_check_cc_options ()
{
  jitter_flag_variable_name="$1"
  shift

  for jitter_compiler_option in $@; do
    jitter_check_cc_option "$jitter_flag_variable_name" "$jitter_compiler_option"
  done
}


# Libtool support.
################################################################

# In case this is a sub-package build force static libraries to be enabled and
# shared libraries to be disabled, even if this is different from what the user
# asked.
# This is reasonable in a sub-package: of course the super-package remains free
# to choose differently.
# REMARK: enable_static and enable_shared are not explicitly documented.
if test "x$JITTER_SUBPACKAGE" != 'x'; then
  if test "x${enable_static}" = 'xno'; then
    AC_MSG_WARN([forcing static libraries to be enabled in sub-package mode])
    enable_static=yes
  fi
  if test "x${enable_shared}" = 'xyes'; then
    AC_MSG_WARN([forcing shared libraries to be disabled in sub-package mode])
    enable_shared=no
  fi
fi

# Initialize Libtool.
LT_INIT

# Update the libtool script if it becomes out of date.
AC_SUBST([LIBTOOL_DEPS])


# Initialize Gnulib.
################################################################

gl_INIT


# Emacs Lisp support.
################################################################

# Check for the Emacs Lisp installation path.
AM_PATH_LISPDIR


# Check for libraries: GNU Readline
################################################################

# Jitter can use GNU Readline, even if there is a crude emulation facility
# for when it's not installed.  It is currently only used in JitterLisp but
# I plan to use it more in the future, so it's linked to every program.
#
# Anyway, don't check for Readline and refuse to use it if we got
# --without-readline or --with-readline=no as a configure option.
AC_ARG_WITH([readline],
            AS_HELP_STRING([--with-readline],
                           [use GNU ReadLine (default: detected)]),
            jitter_check_readline="$withval",
            jitter_check_readline="yes")
if test "x$jitter_check_readline" != "xno"; then
  # These seem to be readline dependencies.
  AC_CHECK_LIB([ncurses], [main])
  AC_CHECK_LIB([termcap], [main])
  AC_CHECK_LIB([tinfo], [main])

  # Check for the actual Readline library.
  AC_CHECK_LIB([readline], [readline], , [did_we_find_readline=no])

  # If we found it, LDADD has already been updated.  In that case also define a
  # CPP feature macro, so as to use the actual Readline instead of my crude
  # emulation.
  if test "x$did_we_find_readline" != "xno"; then
    AC_DEFINE_UNQUOTED([JITTER_HAVE_GNU_READLINE], [1],
                       [Use the actual GNU Readline library])
  fi
else
  # We were requested to do without Readline.
  AC_MSG_NOTICE([not using GNU Readline because of configure option])
fi # if test "x$jitter_check_readline" = "xyes"


# Check for headers.
################################################################


# Check for GCC attributes.
################################################################

# Check if the GNU C attribute syntax is recognized at all.
AC_CACHE_CHECK([for GNU C attributes], [ac_cv_have_gnu_c_attribute],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
        [[__attribute__ (())
          int
          f (int x)
          {
            return x;
          }]],
        [[return f (0);]])],
     [ac_cv_have_gnu_c_attribute=yes],
     [ac_cv_have_gnu_c_attribute=no])])
if test "x$ac_cv_have_gnu_c_attribute" = "xyes"; then
  AC_DEFINE([JITTER_HAVE_GNU_C_ATTRIBUTE], [1],
            [Define if the compiler supports __attribute__.])
fi

# Check for specific attributes not supported by older versions of GCC;
# and possibly by other compilers.  Notice that we have to temporarily
# change CFLAGS (in a subshell, so as not to affect the rest of this
# script) to force -Werror.  If -Werror is not supported than the tests
# will all fail, which is the correct conservative behavior.

# Check for attribute returns_nonnull .
(CFLAGS="$CFLAGS -Werror"; export CFLAGS
AC_CACHE_CHECK([for the returns_nonnull attribute],
  [ac_cv_have_returns_nonnull],
  [AC_COMPILE_IFELSE(
     [AC_LANG_PROGRAM(
        [[long n = 7;
          __attribute__ ((returns_nonnull))
          long *
          f (void)
          {
            return & n;
          }]],
        [[return * f () != 0;]])],
     [ac_cv_have_returns_nonnull=yes],
     [ac_cv_have_returns_nonnull=no])])
if test "x$ac_cv_have_returns_nonnull" = "xyes"; then
  AC_DEFINE([JITTER_HAVE_ATTRIBUTE_RETURNS_NONNULL], [1],
            [Define if the returns_nonnull attribute works])
fi)


# Many people like to compile with -Werror.  This is a way of redefining
# attributes not supported by the compiler so that they expand to nothing.



# Check for GCC builtins.
################################################################

# Checking for builtins with AC_CHECK_FUNCS does not work for me, which is
# reasonable since they are not usually implemented as C functions.

# Overflow-checking builtins are fairly recent, and it is better to check their
# presence rather than assuming that they are always available with GCC.
AC_CACHE_CHECK([for GCC overflow-checking builtins], [ac_cv_have_gcc_overflow],
  [AC_LINK_IFELSE(
     [AC_LANG_PROGRAM(
        [[#include <stdio.h>]],
        [[int foo = __builtin_add_overflow_p (1, 2, 42);
          printf ("%i\n", foo); /* -Werror: avoid unused variable warnings. */
        ]])],
     [ac_cv_have_gcc_overflow=yes],
     [ac_cv_have_gcc_overflow=no])])
if test "x$ac_cv_have_gcc_overflow" = "xyes"; then
  AC_DEFINE([JITTER_HAVE_GCC_OVERFLOW_CHECKING], [1],
            [Define if __builtin_add_overflow_p and friends are usable.])
fi


# Check for specific functions in the C library.
################################################################

# Check for functions letting me know the page size.
AC_CHECK_FUNCS([sysconf getpagesize])

# Check for functions letting me allocate executable space.
AC_CHECK_FUNCS([mmap64 mmap mremap munmap])

# Check for getrlimit/setrlimit and alarm.
AC_CHECK_FUNCS([getrlimit setrlimit alarm])


# System-dependent configuration.
################################################################

# Define a few macros describing the host and the build systems.
AC_DEFINE_UNQUOTED([JITTER_HOST_CPU], ["$host_cpu"],
                   [The host CPU architecture, as per the GNU convention.])
AC_DEFINE_UNQUOTED([JITTER_HOST_VENDOR], ["$host_vendor"],
                   [The host vendor, as per the GNU convention.])
AC_DEFINE_UNQUOTED([JITTER_HOST_OS], ["$host_os"],
                   [The host operating system, as per the GNU convention.])
AC_DEFINE_UNQUOTED([JITTER_HOST_TRIPLET], ["$host"],
                   [The host system triplet, as per the GNU convention.])
AC_DEFINE_UNQUOTED([JITTER_BUILD_CPU], ["$build_cpu"],
                   [The build CPU architecture, as per the GNU convention.])
AC_DEFINE_UNQUOTED([JITTER_BUILD_VENDOR], ["$build_vendor"],
                   [The build vendor, as per the GNU convention.])
AC_DEFINE_UNQUOTED([JITTER_BUILD_OS], ["$build_os"],
                   [The build operating system, as per the GNU convention.])
AC_DEFINE_UNQUOTED([JITTER_BUILD_TRIPLET], ["$build"],
                   [The build system triplet, as per the GNU convention.])

# Define a feature macro if the host system is GNU.  This is useful for
# conditionalizing on GNU libc features.
if echo "$host_os" | grep -q gnu; then
  AC_DEFINE_UNQUOTED([JITTER_HOST_OS_IS_GNU], [1],
                     [Is the host operating system GNU?])
fi

# Check endianness.
AC_C_BIGENDIAN

# Use a command-line option to explicitly disable assembly support.  I've needed
# this for a version of FreeBSD, which shipped with a very old Gas.
# This is used to compute $jitter_enable_host_assembly ; a check below
# will decide if support for the host machine is actually available.
AC_MSG_CHECKING([if assembly support is (potentially) enabled])
AC_ARG_ENABLE([assembly],
              AS_HELP_STRING([--enable-assembly@<:@=CPU@:>@],
                 [force the use of the named assembly (as in the first part of
a GNU-style target triplet), even if not detected.
With --disable-assembly or --enable-assembly=no, disable assembly support even
if available for the host machine.
With --enable-assembly or --enable-assembly=auto, enable assembly support if
automatically found. (default: auto)]),
              jitter_enable_host_assembly="$enableval",
              jitter_enable_host_assembly="auto")
AC_MSG_RESULT([$jitter_enable_host_assembly])

# Check whether we have assembly support for the host architecture.
# For the architectures we know, also define sensible options for objdump --
# some of these are necessary, others a matter of preference.  When the
# architecture is unknown some default is guessed.
#
# At this point jitter_enable_host_assembly is defined to one of "no", "auto",
# or an architecture name.  If assembly support is not found "auto" will turn
# into "no" after this check, but "no" will not turn into "auto": the option
# above is conceived to *disable* assembly support when available.
#
# An architecture name given by the user will be used in place of the
# detected $host_cpu .
# At the end of the case statement jitter_has_host_assembly will be defined.
if test "x$jitter_enable_host_assembly" = "xno"; then \
  default_host_cpu="no-assembly"
elif test "x$jitter_enable_host_assembly" = "xauto"; then
  default_host_cpu="$host_cpu"
else
  default_host_cpu="$jitter_enable_host_assembly"
fi
AC_MSG_CHECKING([if we have assembly support for \"$default_host_cpu\"])
AS_CASE([$default_host_cpu],
        [aarch64*],    [#JITTER_ASSEMBLY_SUBDIRECTORY=aarch64
                        jitter_has_host_assembly=no;
                        jitter_objdump_options="--architecture=aarch64 --disassembler-options=reg-names-raw"],
        [arm*],        [#JITTER_ASSEMBLY_SUBDIRECTORY=arm
                        jitter_has_host_assembly=no;
                        jitter_objdump_options="--architecture=arm --disassembler-options=reg-names-raw"],
        [i?86],        [#JITTER_ASSEMBLY_SUBDIRECTORY=i386
                        jitter_has_host_assembly=no;
                        jitter_objdump_options="--architecture=i386 --disassembler-options=att,suffix"],
        [mips|mipsel], [JITTER_ASSEMBLY_SUBDIRECTORY=mips;
                        jitter_has_host_assembly=yes;
                        jitter_objdump_options="--architecture=mips:isa32 --disassembler-options=no-aliases,reg-names=numeric"],
        [mips64|mips64el],
                       [#JITTER_ASSEMBLY_SUBDIRECTORY=mips;
                        jitter_has_host_assembly=no;
                        jitter_objdump_options="--architecture= --disassembler-options=no-aliases,reg-names=numeric"],
        [powerpc64|powerpc64le],
                       [#JITTER_ASSEMBLY_SUBDIRECTORY=powerpc
                        jitter_has_host_assembly=no;
                        jitter_objdump_options="--architecture=powerpc:common64"],
        [ppc|powerpc|powerpcle],
                        jitter_has_host_assembly=yes;
                       [JITTER_ASSEMBLY_SUBDIRECTORY=powerpc],
        [riscv*],      [#JITTER_ASSEMBLY_SUBDIRECTORY=riscv;
                        jitter_has_host_assembly=no;
                        jitter_objdump_options="--architecture=riscv:rv64 --disassembler-options=no-aliases,numeric"],
        [sh4*],        [JITTER_ASSEMBLY_SUBDIRECTORY=sh
                        jitter_has_host_assembly=yes;
                        jitter_objdump_options="--architecture=sh4a --disassembler-options=no-aliases"],
        [sparc*],      [JITTER_ASSEMBLY_SUBDIRECTORY=sparc
                        jitter_has_host_assembly=yes;
                        jitter_objdump_options="--architecture=sparc:v9b --disassembler-options=no-aliases"],
        [x86_64],      [JITTER_ASSEMBLY_SUBDIRECTORY=x86_64;
                        jitter_has_host_assembly=yes;
                        jitter_objdump_options="--architecture=i386:x86-64 --disassembler-options=x86-64,att,suffix"],
                       # Default case.  Notice that, as a fallback case, we call
                       # objdump with "--architecture=" , without actually
                       # specifying an architecture name.
                       # Using a default architecture might not be completely
                       # reliable in the case of ISA extensions, but appears to
                       # work well in practice; in any case this is a fallback
                       # case: it is easy to provide a sensible default for
                       # supported architectures by adding cases above.
                       # There is no need for objdump to guess endianness, as
                       # the information is always provided as a separate option
                       # by the Jittery program, which knows the correct value
                       # from configure.
                       [jitter_has_host_assembly=no;
                        jitter_objdump_options="--architecture= --disassembler-options=no-aliases"])
if test "x$jitter_objdump_options" = "x"; then
  jitter_objdump_options="--architecture=$host_cpu"
fi

# Define objdump options unconditionally, even if the user disabled host
# assembly.  This is harmless enough, as the code is defensive and will
# revert to a sensible fallback solution when objdump fails.
AC_DEFINE_UNQUOTED([JITTER_OBJDUMP_OPTIONS],
                   ["$jitter_objdump_options"],
                   [architecture-specific options for objdump])
if test "x$jitter_has_host_assembly" = "xno"; then
  AC_MSG_RESULT([no])
else
  AC_MSG_RESULT([yes: $JITTER_ASSEMBLY_SUBDIRECTORY])
fi

# If the user explicitly requested support for an architecture but that
# was not found, fail.
if test "x$jitter_enable_host_assembly" != "xno" \
   && test "x$jitter_enable_host_assembly" != "xauto" \
   && test "x$JITTER_ASSEMBLY_SUBDIRECTORY" = "x"; then
   AC_MSG_ERROR([could not find assembly for $jitter_enable_host_assembly])
fi


# Check if assembly support exists for the host machine.
################################################################

# The Automake conditional JITTER_HAVE_ASSEMBLY will be true iff there is support for
# the host machine.  If so JITTER_ASSEMBLY_SUBDIRECTORY will be AC_SUBST'ed to be
# the subdirectory holding configuration-specific files.
if test "x$jitter_has_host_assembly" = "xyes"; then
   AC_SUBST([JITTER_ASSEMBLY_SUBDIRECTORY])
   AC_DEFINE_UNQUOTED([JITTER_HAVE_ASSEMBLY], [yes],
                      [Define if we have assembly for the host architecture.])
   AC_DEFINE_UNQUOTED([JITTER_ASSEMBLY_SUBDIRECTORY], ["$JITTER_ASSEMBLY_SUBDIRECTORY"],
                      [the subdirectory containing assembly files.])
fi
AM_CONDITIONAL([JITTER_HAVE_ASSEMBLY],
               [test "x$jitter_has_host_assembly" = "xyes"])


# Configuration-dependent features not directly relying on assembly support.
############################################################################

# According to the architecture some operations (defined in C, and still
# portable to any archtiecture) may be faster or slower.  Jitter contains
# conditional code to support different solutions.
#
# This is independent from architecture-specific assembly support.

# Some architectures support a good way of computing a conditional expression
# based on the negative sign of a disciminand without using branches; other
# architectures do not.
AC_MSG_CHECKING([if straight-line negativity tests are fast])
# By default, let us say that the straight-line version is better.
jitter_have_fast_straight_line_negativity=yes
AS_CASE([$default_host_cpu],
        # Only on these few archtiectures my hand-optimized straight-line
        # version is slower than the naïve alternative.
        [x86_64|i?86|sh4*],
        [jitter_have_fast_straight_line_negativity=no])
AC_MSG_RESULT([$jitter_have_fast_straight_line_negativity])
if test "x$jitter_have_fast_straight_line_negativity" = "xyes"; then
  AC_DEFINE_UNQUOTED([JITTER_HAVE_FAST_STRAIGHT_LINE_NEGATIVITY], [1],
                     [Define to 1 iff negativity tests are faster straight-line])
fi

# On some architectures it is efficient to "mask off" a tag in the low-order
# bits, in the sense of bitwise and-ing with a literal constant having zeroes
# only in the significant bits.  This in practice require an and instruction
# with a sign-extended immediate, large immediate operands, an and instruction
# with a negated zero-extended immedate, or something equivalent.
AC_MSG_CHECKING([if and-ing with a sign-extended negative immediate is fast])
# By default, let us say that the operation is efficient.
jitter_have_fast_mask_off=yes
AS_CASE([$default_host_cpu],
        # On these archtiectures the "masking off" operation has a faster
        # alternative, because the bitwise and instruction with an immediate
        # zero-extends its argument instead of sign-extending it, or there is
        # no immediate version at all working on an arbitrary register, or
        # because of some other restriction.
        #
        # Notes on specific architectures:
        # SH, while not ideal in this sense, has particularly limited shifting
        # instructions, making the mask alternative better even if it requires
        # a separate instruction to load an immediate into a temporary before
        # and-ing; the immediate, at least, is sign-extended.
        [mips*],
        [jitter_have_fast_mask_off=no])
AC_MSG_RESULT([$jitter_have_fast_mask_off])
if test "x$jitter_have_fast_mask_off" = "xyes"; then
  AC_DEFINE_UNQUOTED([JITTER_HAVE_FAST_MASK_OFF], [1],
                     [Define to 1 iff masking off low-order bits is efficient])
fi


# Binary format.
################################################################

# Determine whether the host OS uses the ELF binary format.

# This is crude but probably sufficient for the currently supported
# configurations.  This check should guess correctly in the case of modern GNU
# systems, all using ELF, and conservatively assume that ELF is not used in most
# other cases.
AC_MSG_CHECKING([if the host OS uses the ELF binary format])
AS_CASE([$host_os],
        [*aout*|*coff*],
          [jitter_host_os_is_elf=no],
        [gnu*|linux-gnu*|*elf*],
          [jitter_host_os_is_elf=yes],
        [*uclibc*],
          [jitter_host_os_is_elf=yes],
          [jitter_host_os_is_elf=no])
AC_MSG_RESULT([$jitter_host_os_is_elf])

# Determine whether the host OS uses the COFF binary format.

# Again, this is crude but should suffice.
AC_MSG_CHECKING([if the host OS uses the COFF binary format])
AS_CASE([$host_os],
        [*coff*],
          [jitter_host_os_is_coff=yes],
        [*mingw*],
          [jitter_host_os_is_coff=yes],
          [jitter_host_os_is_coff=no])
AC_MSG_RESULT([$jitter_host_os_is_coff])

# Define C feature macros telling whether we can rely on the binary format
# being ELF, or COFF.
if test "x$jitter_host_os_is_elf" = "xyes"; then
  AC_DEFINE_UNQUOTED([JITTER_HOST_OS_IS_ELF], [1],
                     [Define to 1 iff the host OS uses the ELF format])
fi
if test "x$jitter_host_os_is_coff" = "xyes"; then
  AC_DEFINE_UNQUOTED([JITTER_HOST_OS_IS_COFF], [1],
                     [Define to 1 iff the host OS uses the COFF format])
fi


# M4sh machinery.
################################################################

# For portability and for better factoring I generate shell scripts with M4sh.
# The actual source files are ".m4sh.in" scripts, to be preprocessed by autom4te
# into ".in" scripts, for aclocal to perform @-substitutions into ordinary
# scripts (no file extension).
# The first step of this double translation relies on autom4te, which is not
# required on the user's machine; therefore I distribute ".in" scripts, but
# the user needs autom4te if she wants to regenerate them.
AC_CHECK_PROG(HAS_AUTOM4TE, autom4te, yes, no)
if test "x$HAS_AUTOM4TE" != "xyes"; then
  AC_MSG_WARN([You need autom4te, coming from GNU Autoconf, if you want to
               rebuild scripts from their M4sh source])
fi
AM_MISSING_PROG(AUTOM4TE, autom4te)
AM_CONDITIONAL([JITTER_HAVE_AUTOM4TE],
               [test "x$HAS_AUTOM4TE" = "xyes"])


# Emulator support for running cross-compilied binaries.
################################################################

# We optionally support qemu-user emulation for running cross-compiled binaries
# for the host on the build system; other user binary emulators might work as
# well.
# This is particularly useful for running the test suite when cross-compiling.
# The option argument is the emulator command line included options, to be
# followed by the emulated program with its own options: for example
# "qemu-ppc -L /my-cross-path/powerpc-unknown-linux-gnu/sysroot" .
AC_MSG_CHECKING([if host emulator support is used])
AC_ARG_WITH([emulator],
            AS_HELP_STRING([--with-emulator="EMULATOR WITH OPTIONS"],
            [use an emulator running cross-compiled binaries,
             including the test suite, on the build platform: default no]),
            jitter_with_emulator="$withval",
            jitter_with_emulator="")
if    test "x$jitter_with_emulator" = "x" \
   || test "x$jitter_with_emulator" = "xno"; then
  jitter_with_emulator=""
  AC_MSG_RESULT([no])
else
  AC_MSG_RESULT([yes, $jitter_with_emulator])
fi
AC_SUBST([JITTER_EMULATOR], $jitter_with_emulator)


# Can we actually rebuild man pages?
################################################################

# GNU help2man could be made to work even when cross-compiling, as long as there
# is a usable emulator.  Still, there is very little gain in doing this and lots
# of gratuitous headaches when using the missing script with an emulator.  I
# will keep things simple for now.
AC_MSG_CHECKING([if we should use help2man to rebuild man pages])
if    test "x$jitter_cross_compiling" != "xyes" \
   && test "x$jitter_has_help2man"    =  "xyes"; then
   jitter_can_rebuild_man_pages="yes"
   AC_MSG_RESULT([yes])
else
   jitter_can_rebuild_man_pages="no"
   AC_MSG_RESULT([no (cross-compiling or no help2man)])
fi
AM_CONDITIONAL([JITTER_CAN_REBUILD_MAN_PAGES],
               [test "x$jitter_can_rebuild_man_pages" = "xyes"])


# Valgrind support.
################################################################

# We optionally support valgrind in the test suite.  First use a configure
# option to override the default.
AC_MSG_CHECKING([if Valgrind is supported])
AC_ARG_WITH([valgrind],
            AS_HELP_STRING([--with-valgrind@<:@=VALGRINDNAME@:>@],
                           [use Valgrind for the test suite (default:
                            detected)]),
            jitter_with_valgrind="$withval",
            jitter_with_valgrind="detect")
# At this point $jitter_with_valgrind may be defined as "yes" , "no" ,
# "detect" , or the program name.

# Decide whether to actually enable valgrind.  Right now we disable
# valgrind when cross-compilation or an emulator is enabled at the
# same time.
if test "x$jitter_cross_compiling" = "xyes"; then
  AC_MSG_RESULT([no, disabled because we are cross-compiling])
  AC_SUBST([JITTER_VALGRIND], [])
elif ! test "x$JITTER_EMULATOR" = "x"; then
  AC_MSG_RESULT([no, disabled because an emulator is also used])
  AC_SUBST([JITTER_VALGRIND], [])
elif test "x$jitter_with_valgrind" = "xno"; then
  AC_MSG_RESULT([no, disabled by configure option])
  AC_SUBST([JITTER_VALGRIND], [])
elif    test "x$jitter_with_valgrind" = "xdetect" \
     || test "x$jitter_with_valgrind" = "xyes"; then
  AC_PATH_PROG([JITTER_VALGRIND], [valgrind])
  if test "x$JITTER_VALGRIND" = "x"; then
    AC_MSG_RESULT([no, not found])
    if test "x$jitter_with_valgrind" = "xyes"; then
      AC_MSG_ERROR([Valgrind requested but not found])
    fi
  else
    AC_MSG_RESULT([yes, found as $JITTER_VALGRIND])
  fi
else
  AC_MSG_RESULT([yes, defined by configure option as $jitter_with_valgrind])
  AC_SUBST([JITTER_VALGRIND], [$jitter_with_valgrind])
fi


# Test suite support.
################################################################

# Decide whether we can run the test suite...
AC_MSG_CHECKING([if we can run the test suite])
# By default we can run the test suite if we have all the needed utilites
# *and* either we are not cross-compiling, or if an emulator is enabled...
if    test "x$jitter_test_suite_missing_progs" = "x" \
   && (   test "x$jitter_cross_compiling" = "xno" \
       || ! test "x$JITTER_EMULATOR" = "x"); then
  default=yes
else
  default=no
fi
# ... But this default can be overridden with a configure option.
AC_ARG_ENABLE([test-suite],
              AS_HELP_STRING([--enable-test-suite],
                             [enable the test suite (default: detected)]),
              jitter_enable_test_suite="$enableval",
              jitter_enable_test_suite="$default")
if    ! test "x$jitter_enable_test_suite" = "xyes" \
   && ! test "x$jitter_enable_test_suite" = "xno"; then
   AC_MSG_ERROR([invalid option argument for --enable-test-suite])
fi

# Now we know if the test suite is enabled or not.  Define a substitution and
# an Automake conditional about it.
AC_SUBST([JITTER_ENABLE_TEST_SUITE], [$jitter_enable_test_suite])
AC_MSG_RESULT([$JITTER_ENABLE_TEST_SUITE])
AM_CONDITIONAL([JITTER_ENABLE_TEST_SUITE],
               [test "x$JITTER_ENABLE_TEST_SUITE" = "xyes"])

# If the test suite was enabled despite a missing dependency, print a warning.
if    test "x$jitter_enable_test_suite" = "xyes" \
   && ! test "x$jitter_test_suite_missing_progs" = "x"; then
   AC_MSG_WARN([enabling the test suite despite missing programs, as \
requested.  You can expect some spurious failures])
fi

# Print a warning about skipped test cases if we can run the test suite but
# Valgrind is disabled.
if    test "x$JITTER_CAN_RUN_TESTSUITE" = "xyes" \
   && test "x$JITTER_VALGRIND" = "x"; then
  AC_MSG_WARN([the test suite will skip Valgrind test cases])
fi


# Dispatching models to be enabled.
################################################################

# Check whether each dispatching model is available and enabled, in order of
# increasing preference.  The last found to be enabled is also considered the
# best.
jitter_enabled_dispatch_models=""
jitter_best_dispatch_model=""

# Define a disclaimer to be printed to the user when checking which dispatches
# are enabled, showing that the decision is not final in sub-package mode.
if test "x$JITTER_SUBPACKAGE" != 'x'; then
  jitter_subpackage_disclaimer=', provisionally (sub-package mode)'
else
  jitter_subpackage_disclaimer=''
fi

# Is the switch dispatch enabled?  Check the default and the configure
# command-line option.
AC_MSG_CHECKING([if switch dispatching is enabled])
default=yes
AC_ARG_ENABLE([dispatch-switch],
              AS_HELP_STRING([--enable-dispatch-switch],
              [enable switch dispatching: default no (unimplemented)]),
              jitter_enable_dispatch_switch="$enableval",
              jitter_enable_dispatch_switch="$default")
if test "x$jitter_enable_dispatch_switch" = "xyes"; then
  jitter_best_dispatch_model="switch"
  jitter_enabled_dispatch_models="$jitter_enabled_dispatch_models switch"
fi
AC_MSG_RESULT([$jitter_enable_dispatch_switch$jitter_subpackage_disclaimer])

# Is the direct-threading dispatch enabled?  Check the default and the configure
# command-line option.
AC_MSG_CHECKING([if direct-threading dispatch is enabled])
default=$(test "x$GCC" = "xyes" && echo yes || echo no)
AC_ARG_ENABLE([dispatch-direct-threading],
              AS_HELP_STRING([--enable-dispatch-direct-threading],
              [enable direct-threading dispatch: default yes if and
only if GCC or a compiler claiming to be GCC is used]),
              jitter_enable_dispatch_direct_threading="$enableval",
              jitter_enable_dispatch_direct_threading="$default")
if test "x$jitter_enable_dispatch_direct_threading" = "xyes"; then
  jitter_best_dispatch_model="direct-threading"
  jitter_enabled_dispatch_models="$jitter_enabled_dispatch_models direct-threading"
fi
AC_MSG_RESULT([$jitter_enable_dispatch_direct_threading$jitter_subpackage_disclaimer])

# Is the minimal-threading dispatch enabled?  Check the default and the
# configure command-line option.
AC_MSG_CHECKING([if minimal-threading dispatch is enabled])
# FIXME: reenable this when I make defect handling reliable.
#default=$(test "x$GCC" = "xyes" && echo yes || echo no)
default=no
AC_ARG_ENABLE([dispatch-minimal-threading],
              AS_HELP_STRING([--enable-dispatch-minimal-threading],
              [enable minimal threading dispatch: default yes if and
only if GCC or a compiler claiming to be GCC is used]),
              jitter_enable_dispatch_minimal_threading="$enableval",
              jitter_enable_dispatch_minimal_threading="$default")
if test "x$jitter_enable_dispatch_minimal_threading" = "xyes"; then
  jitter_best_dispatch_model="minimal-threading"
  jitter_enabled_dispatch_models="$jitter_enabled_dispatch_models minimal-threading"
fi
AC_MSG_RESULT([$jitter_enable_dispatch_minimal_threading$jitter_subpackage_disclaimer])
# FIXME: remove this warning message when I make defect handling reliable.
if test "x$jitter_enable_dispatch_minimal_threading" = "xyes"; then
  AC_MSG_WARN([minimal-threading dispatch is currently unreliable, and should
  not be used in production until defective instruction replacement is fully
  implemented])
fi

# Is the no-threading dispatch enabled?  Check the default and the configure
# command-line option.
AC_MSG_CHECKING([if no-threading dispatch is enabled])
if    test "x$GCC" = "xyes" \
   && test "x$jitter_has_host_assembly" = "xyes" \
   && test "x$jitter_host_os_is_elf" = "xyes"; then
  # FIXME: reenable this when I make defect handling reliable.
  #default=yes
  default=no
else
  default=no
fi
AC_ARG_ENABLE([dispatch-no-threading],
              AS_HELP_STRING([--enable-dispatch-no-threading],
              [enable no-threading dispatch: default yes if and
only if GCC or a compiler claiming to be GCC is used, the host
architecture has assembly support, and the binary format is ELF]),
              jitter_enable_dispatch_no_threading="$enableval",
              jitter_enable_dispatch_no_threading="$default")
if test "x$jitter_enable_dispatch_no_threading" = "xyes"; then
  jitter_best_dispatch_model="no-threading"
  jitter_enabled_dispatch_models="$jitter_enabled_dispatch_models no-threading"
fi
AC_MSG_RESULT([$jitter_enable_dispatch_no_threading$jitter_subpackage_disclaimer])
# Fail if no-threading dispatch was enabled without assembly support.
if test "x$jitter_has_host_assembly" = "xno" \
   && test "x$jitter_enable_dispatch_no_threading" = "xyes"; then
   AC_MSG_ERROR([no-threading dispatch requires assembly support])
fi
# FIXME: remove this warning message when I make defect handling reliable.
if test "x$jitter_enable_dispatch_no_threading" = "xyes"; then
  AC_MSG_WARN([no-threading dispatch is currently unreliable, and should not be
  used in production until defective instruction replacement is fully
  implemented])
fi

# In sub-package mode disable every dispatch except for the best one, in order
# to speed up compilation and testing.
if test "x$JITTER_SUBPACKAGE" != 'x'; then
  AC_MSG_NOTICE([sub-package mode: disabling every dispatch except]
[$jitter_best_dispatch_model])
  AC_MSG_NOTICE([  (the following dispatches could have been enabled:]
[ $jitter_enabled_dispatch_models)])
  jitter_enabled_dispatch_models=" $jitter_best_dispatch_model"
  jitter_enable_dispatch_switch=no
  jitter_enable_dispatch_direct_threading=no
  jitter_enable_dispatch_minimal_threading=no
  jitter_enable_dispatch_no_threading=no
  case "$jitter_best_dispatch_model" in
    switch)
       jitter_enable_dispatch_switch=yes;;
    direct-threading)
       jitter_enable_dispatch_direct_threading=yes;;
    minimal-threading)
       jitter_enable_dispatch_minimal_threading=yes;;
    no-threading)
       jitter_enable_dispatch_no_threading=yes;;
    *) AC_MSG_ERROR([this should never happen]);;
  esac
fi

# Now that we have finally decided which dispatches will be enabled, define a
# substitution, a CPP feature macro and an Automake conditional for each of them.
AC_SUBST([JITTER_ENABLE_DISPATCH_SWITCH],
         [$(test "x$jitter_enable_dispatch_switch" = 'xyes' && echo '1')])
AC_SUBST([JITTER_ENABLE_DISPATCH_DIRECT_THREADING],
         [$(test "x$jitter_enable_dispatch_direct_threading" = 'xyes' && echo '1')])
AC_SUBST([JITTER_ENABLE_DISPATCH_MINIMAL_THREADING],
         [$(test "x$jitter_enable_dispatch_minimal_threading" = 'xyes' && echo '1')])
AC_SUBST([JITTER_ENABLE_DISPATCH_NO_THREADING],
         [$(test "x$jitter_enable_dispatch_no_threading" = 'xyes' && echo '1')])
if test "x$jitter_enable_dispatch_switch" = 'xyes'; then
  AC_DEFINE_UNQUOTED([JITTER_ENABLE_DISPATCH_SWITCH], [1],
                     [enable switch dispatch])
fi
if test "x$jitter_enable_dispatch_direct_threading" = 'xyes'; then
  AC_DEFINE_UNQUOTED([JITTER_ENABLE_DISPATCH_DIRECT_THREADING], [1],
                     [enable direct-threading dispatch])
fi
if test "x$jitter_enable_dispatch_minimal_threading" = 'xyes'; then
  AC_DEFINE_UNQUOTED([JITTER_ENABLE_DISPATCH_MINIMAL_THREADING], [1],
                     [enable minimal-threading dispatch])
fi
if test "x$jitter_enable_dispatch_no_threading" = 'xyes'; then
  AC_DEFINE_UNQUOTED([JITTER_ENABLE_DISPATCH_NO_THREADING], [1],
                     [enable no-threading dispatch])
fi
AM_CONDITIONAL([JITTER_ENABLE_DISPATCH_SWITCH],
               [test "x$jitter_enable_dispatch_switch" = 'xyes'])
AM_CONDITIONAL([JITTER_ENABLE_DISPATCH_DIRECT_THREADING],
               [test "x$jitter_enable_dispatch_direct_threading" = 'xyes'])
AM_CONDITIONAL([JITTER_ENABLE_DISPATCH_MINIMAL_THREADING],
               [test "x$jitter_enable_dispatch_minimal_threading" = 'xyes'])
AM_CONDITIONAL([JITTER_ENABLE_DISPATCH_NO_THREADING],
               [test "x$jitter_enable_dispatch_no_threading" = 'xyes'])


# AC_SUBST the list of all the enabled dispatching models, and the best one;
# these don't use quotes as they are meant for jitter-config and shell scripts,
# not for C code.
AC_SUBST([JITTER_ENABLED_DISPATCHES],
         [$jitter_enabled_dispatch_models])
AC_SUBST([JITTER_BEST_DISPATCH],
         [$jitter_best_dispatch_model])

# Define a substitution holding the best dispatching model name, and a a CPP
# feature macro named after the best dispatching model.
# We also need an Autoheader template for every possible CPP "best-distpatching"
# macro name, as the actual name is computed with a shell expansion and
# autoheader cannot discover every possibility.
AH_TEMPLATE([JITTER_BEST_DISPATCHING_MODEL_IS_SWITCH],
            [Defined if switch is the best enabled dispatch.])
AH_TEMPLATE([JITTER_BEST_DISPATCHING_MODEL_IS_DIRECT_THREADING],
            [Defined if direct-threading is the best enabled dispatch.])
AH_TEMPLATE([JITTER_BEST_DISPATCHING_MODEL_IS_MINIMAL_THREADING],
            [Defined if minimal-threading is the best enabled dispatch.])
AH_TEMPLATE([JITTER_BEST_DISPATCHING_MODEL_IS_NO_THREADING],
            [Defined if no-threading is the best enabled dispatch.])
if test "x$jitter_best_dispatch_model" = "x"; then
  AC_MSG_ERROR([no dispatchnig model is enabled])
else
  AC_MSG_NOTICE([enabled dispatching models are:]
[ $jitter_enabled_dispatch_models])
  AC_MSG_NOTICE([the best enabled dispatching model is:]
[  $jitter_best_dispatch_model])
  AC_SUBST([JITTER_BEST_DISPATCHING_MODEL], [$jitter_best_dispatch_model])
  JITTER_BEST_DISPATCH_MODEL=$(echo $jitter_best_dispatch_model | jitter_to_upper)
  AC_DEFINE_UNQUOTED([JITTER_BEST_DISPATCHING_MODEL_NAME_LOWER],
                     ["$jitter_best_dispatch_model"],
                     [The name of the best dispatching model, lower-case.])
  AC_DEFINE_UNQUOTED([JITTER_BEST_DISPATCHING_MODEL_NAME_UPPER],
                     ["$JITTER_BEST_DISPATCH_MODEL"],
                     [The name of the best dispatching model, upper-case with
                      '-' replaced by '_'.])
  AC_DEFINE_UNQUOTED([JITTER_BEST_DISPATCHING_MODEL_IS_$JITTER_BEST_DISPATCH_MODEL],
                     [1],
                     [Defined if $jitter_best_dispatch_model is the best
                      enabled dispatching model.])
fi


# Installation paths, relative to installation prefixes.
################################################################

# According to the GNU Coding Standards installation prefixes can be decided
# very late, at make time; therefore there are no AC_SUBST or AC_DEFINE calls
# fixing complete paths here.  However we can establish once and for all some
# subdirectories, relative to prefixes we don't know yet.
AC_SUBST([JITTER_FLAG_SUBDIRECTORY],
         [lib/$PACKAGE_TARNAME/$PACKAGE_VERSION/flags])
AC_SUBST([JITTER_TEMPLATE_SUBDIRECTORY],
         [$PACKAGE_TARNAME/$PACKAGE_VERSION/templates])


# Flags.
################################################################

# Generate flag files in $builddir/flags/$dispatch_model , for every enabled
# dispatch_model .  Cleanup at the beginning, cd back to $builddir at the end.
rm -rf flags
mkdir flags
for dispatch_model in $jitter_enabled_dispatch_models; do
  DISPATCH_MODEL="$(echo $dispatch_model | jitter_to_upper)"

  # Generate flags for $dispatch_model
  mkdir "flags/$dispatch_model"
  AC_MSG_NOTICE([generating flags for dispatching model \"$dispatch_model\"...])
  JITTER_FLAGS_CFLAGS=""
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -O2 -fomit-frame-pointer
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-reorder-blocks
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-reorder-blocks-and-partition
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-crossjumping
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-thread-jumps
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-tree-tail-merge
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-isolate-erroneous-paths-dereference
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-split-paths

  # LTO, impressive as it is in general, is a bad match for Jitter, for
  # correctness reasons on complex dispatches and for performance reasons with
  # switch and direct-threading dispatches.
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-lto

  # Experimental: disable debugging options which appear to be expensive at
  # compile time.
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -g0
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-var-tracking
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-var-tracking-assignments

# I like register names to be visually distinct from numeric immediates --
  # this is particularly important for data locations, which are text strings
  # visible to the user containing assembly operands in the syntax generated by
  # GCC.  I know this option to exist on PowerPC, but it does no harm to try it
  # on other architectures as well.
  jitter_check_cc_options JITTER_FLAGS_CFLAGS -mregnames

  if test "x$jitter_host_os_is_elf" != "xyes" \
     && test "x$JITTER_GNU_ASSEMBLER" != "xno"; then
    # Jitter's section-changing mechanism on non-ELF systems relies on changing
    # subsections using Gas ".text SUBSECTION" directives, which don't play well
    # with CFI directives.  In this case we also need all the code to be
    # generated in .text, and not in ".text.hot" or ".text.unlikely", as
    # -freorder-functions does.
    jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-dwarf2-cfi-asm
    jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-reorder-functions
  fi

#  jitter_check_cc_options JITTER_FLAGS_CFLAGS -frename-registers
#  jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-plt
  if test "$dispatch_model" != "switch"; then
    jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-gcse
  fi
  case "$dispatch_model" in
    minimal-threading|no-threading)
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fPIC
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-align-loops
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-align-jumps
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-align-labels
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-jump-tables
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-tree-switch-conversion
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -flive-range-shrinkage
#      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fsched-pressure
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-ipa-icf
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-ipa-cp
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-ipa-cp-clone
      jitter_check_cc_options JITTER_FLAGS_CFLAGS -fno-gcse
      ;;
  esac
  case "$host_cpu" in
    x86_64)
      if    test "$dispatch_model" = "minimal-threading" \
         || test "$dispatch_model" = "no-threading" ; then
        jitter_check_cc_options JITTER_FLAGS_CFLAGS -mcmodel=large
      fi
      ;;
    *)
      case "$dispatch_model" in
        minimal-threading|no-threading)
          # GCC generates calls to intrinsics/libc functions such as memcpy to
          # pass parameters.  At least on PowerPC GNU/Linux these resolve to
          # calls via the GOT or PLT which then get optimized into PC-relative
          # branch-and-link instructions, I suppose by ld.so .  That is all very
          # good in normal circumastances, but the last optimization breaks my
          # relocatable code.  Generate inline code for such calls instead, for
          # the kind of smallish buffer lengths one might find in VM
          # instructions.
          jitter_check_cc_options JITTER_FLAGS_CFLAGS \
             -mblock-move-inline-limit=8192
        ;;
      esac
      ;;
  esac

  JITTER_FLAGS_CPPFLAGS="-DJITTER_DISPATCH_$DISPATCH_MODEL=1"

  # In sub-package mode Jitter's C headers are not installed in any particular
  # include path, so I need to let the compiler find them via -I options in the
  # source directory and, for files generated by config.status , in the build
  # directory as well.
  # This holds for two kinds of headers:
  # * headers following the pattern
  #     jitter/FILE.h
  #   , included in C as <jitter/FILE.h>;
  # * (where assembly support exists) headers following the more complex pattern
  #     machine/${JITTER_ASSEMBLY_SUBDIRECTORY}/jitter/machine/FILE.h
  #   , included in C as <jitter/machine/FILE.h> .
  # Makefile.am (see section "Native-code libjitter sources for supported
  # architectures") uses this same trick to compile examples when Jitter may not
  # be installed.
  if test "x$JITTER_SUBPACKAGE" != 'x'; then
    JITTER_FLAGS_CPPFLAGS="$JITTER_FLAGS_CPPFLAGS -I ${jitter_abs_top_srcdir}"
    JITTER_FLAGS_CPPFLAGS="$JITTER_FLAGS_CPPFLAGS -I ${jitter_abs_top_builddir}"
    if test "x$JITTER_ASSEMBLY_SUBDIRECTORY" != 'x'; then
      JITTER_FLAGS_CPPFLAGS="$JITTER_FLAGS_CPPFLAGS -I ${jitter_abs_top_srcdir}/machine/${JITTER_ASSEMBLY_SUBDIRECTORY}"
    fi
  fi

  JITTER_FLAGS_LDADD="-ljitter--$dispatch_model"

  # Search for Jitter runtime libraries in the right place.
  if test "x$JITTER_SUBPACKAGE" != 'x'; then
    # See the comment above for sub-package mode and include paths.  This case
    # is simpler.
    JITTER_FLAGS_LDFLAGS="-L${jitter_abs_top_builddir}/lib"
  else
    JITTER_FLAGS_LDFLAGS="-L${libdir}"
  fi
  export JITTER_FLAGS_LDFLAGS

  echo "$CC"                    > flags/$dispatch_model/CC
  echo "$JITTER_FLAGS_CPPFLAGS" > flags/$dispatch_model/CPPFLAGS
  echo "$JITTER_FLAGS_CFLAGS"   > flags/$dispatch_model/CFLAGS
  echo "$JITTER_FLAGS_LDADD"    > flags/$dispatch_model/LDADD
  echo "$JITTER_FLAGS_LDFLAGS"  > flags/$dispatch_model/LDFLAGS

  # Also AC_SUBST flag variables for the best dispatching model, if the best
  # happens to be the one we're handling now.
  if test "x$jitter_best_dispatch_model" = "x$dispatch_model"; then
    AC_SUBST([JITTER_CFLAGS],
             [$(cat flags/$dispatch_model/CFLAGS)])
    AC_SUBST([JITTER_CPPFLAGS],
             [$(cat flags/$dispatch_model/CPPFLAGS)])
    AC_SUBST([JITTER_LDADD],
             [$(cat flags/$dispatch_model/LDADD)])
    AC_SUBST([JITTER_LDFLAGS],
             [$(cat flags/$dispatch_model/LDFLAGS)])
#    AC_SUBST([JITTER_MAKE_FLAGS],
#             '$(MAKEFLAGS) CFLAGS="$(AM_CFLAGS) $(JITTER_CFLAGS)" CPPFLAGS="$(AM_CPPFLAGS) $(JITTER_CPPFLAGS)" LDFLAGS="$(AM_LDFLAGS) $(JITTER_LDFLAGS)" LDADD="$(JITTER_LDADD)"') # There is no AM_LDADD
  fi
done

# AC_SUBST flag variables for every dispatching model.  FIXME: the first
# argument of AC_SUBST must be literal , therefore a shell loop doesn't work
# here, but it shouldn't be hard with m4 macros.
if test "x$jitter_enable_dispatch_switch" = "xyes"; then
  AC_SUBST([JITTER_SWITCH_CFLAGS],
           [$(cat flags/switch/CFLAGS)])
  AC_SUBST([JITTER_SWITCH_CPPFLAGS],
           [$(cat flags/switch/CPPFLAGS)])
  AC_SUBST([JITTER_SWITCH_LDADD],
           [$(cat flags/switch/LDADD)])
  AC_SUBST([JITTER_SWITCH_LDFLAGS],
           [$(cat flags/switch/LDFLAGS)])
#  AC_SUBST([JITTER_SWITCH_MAKE_FLAGS],
#           '$(MAKEFLAGS) CFLAGS="$(AM_CFLAGS) $(JITTER_SWITCH_CFLAGS)" CPPFLAGS="$(AM_CPPFLAGS) $(JITTER_SWITCH_CPPFLAGS)" LDFLAGS="$(AM_LDFLAGS) $(JITTER_SWITCH_LDFLAGS)" LDADD="$(JITTER_SWITCH_LDADD)"') # There is no AM_LDADD
fi
if test "x$jitter_enable_dispatch_direct_threading" = "xyes"; then
  AC_SUBST([JITTER_DIRECT_THREADING_CFLAGS],
           [$(cat flags/direct-threading/CFLAGS)])
  AC_SUBST([JITTER_DIRECT_THREADING_CPPFLAGS],
           [$(cat flags/direct-threading/CPPFLAGS)])
  AC_SUBST([JITTER_DIRECT_THREADING_LDADD],
           [$(cat flags/direct-threading/LDADD)])
  AC_SUBST([JITTER_DIRECT_THREADING_LDFLAGS],
           [$(cat flags/direct-threading/LDFLAGS)])
#  AC_SUBST([JITTER_DIRECT_THREADING_MAKE_FLAGS],
#           '$(MAKEFLAGS) CFLAGS="$(AM_CFLAGS) $(JITTER_DIRECT_THREADING_CFLAGS)" CPPFLAGS="$(AM_CPPFLAGS) $(JITTER_DIRECT_THREADING_CPPFLAGS)" LDFLAGS="$(AM_LDFLAGS) $(JITTER_DIRECT_THREADING_LDFLAGS)" LDADD="$(JITTER_DIRECT_THREADING_LDADD)"') # There is no AM_LDADD
fi
if test "x$jitter_enable_dispatch_minimal_threading" = "xyes"; then
  AC_SUBST([JITTER_MINIMAL_THREADING_CFLAGS],
           [$(cat flags/minimal-threading/CFLAGS)])
  AC_SUBST([JITTER_MINIMAL_THREADING_CPPFLAGS],
           [$(cat flags/minimal-threading/CPPFLAGS)])
  AC_SUBST([JITTER_MINIMAL_THREADING_LDADD],
           [$(cat flags/minimal-threading/LDADD)])
  AC_SUBST([JITTER_MINIMAL_THREADING_LDFLAGS],
           [$(cat flags/minimal-threading/LDFLAGS)])
#  AC_SUBST([JITTER_MINIMAL_THREADING_MAKE_FLAGS],
#           '$(MAKEFLAGS) CFLAGS="$(AM_CFLAGS) $(JITTER_MINIMAL_THREADING_CFLAGS)" CPPFLAGS="$(AM_CPPFLAGS) $(JITTER_MINIMAL_THREADING_CPPFLAGS)" LDFLAGS="$(AM_LDFLAGS) $(JITTER_MINIMAL_THREADING_LDFLAGS)" LDADD="$(JITTER_MINIMAL_THREADING_LDADD)"') # There is no AM_LDADD
fi
if test "x$jitter_enable_dispatch_no_threading" = "xyes"; then
  AC_SUBST([JITTER_NO_THREADING_CFLAGS],
           [$(cat flags/no-threading/CFLAGS)])
  AC_SUBST([JITTER_NO_THREADING_CPPFLAGS],
           [$(cat flags/no-threading/CPPFLAGS)])
  AC_SUBST([JITTER_NO_THREADING_LDADD],
           [$(cat flags/no-threading/LDADD)])
  AC_SUBST([JITTER_NO_THREADING_LDFLAGS],
           [$(cat flags/no-threading/LDFLAGS)])
#  AC_SUBST([JITTER_NO_THREADING_MAKE_FLAGS],
#           '$(MAKEFLAGS) CFLAGS="$(AM_CFLAGS) $(JITTER_NO_THREADING_CFLAGS)" CPPFLAGS="$(AM_CPPFLAGS) $(JITTER_NO_THREADING_CPPFLAGS)" LDFLAGS="$(AM_LDFLAGS) $(JITTER_NO_THREADING_LDFLAGS)" LDADD="$(JITTER_NO_THREADING_LDADD)"') # There is no AM_LDADD
fi

# AC_SUBST a set of LDFLAGS to be prepended to the ordinary ones, only useful
# for linking from an uninstlaled build directory.  @JITTER_UNINSTALLED_LDFLAGS@
# must always come before other *_LDFLAGS variables, since the first -L option
# takes precedence.
AC_SUBST([JITTER_UNINSTALLED_LDFLAGS],
         [-L$jitter_abs_top_builddir/lib])


# Debugging support.
################################################################

# We generate readable assembly files for debugging; on some architectures I
# prefer an alternative non-default format, which I find more readable.  These
# options are passed to the compiler only when generating textual assembly as
# a make target.
case "$host_cpu" in
  powerpc*|ppc*)
    # I like register names to be visually distinct from numeric immediates.
    AC_SUBST([JITTER_MACHINE_SPECIFIC_READABLE_ASSEMBLY_FLAGS], [-mregnames]);;
esac

# Look for the indent utility, and remember whether it is available in an
# Automake conditional.  This is only used for debugging targets.
AC_PATH_PROG([JITTER_INDENT], [indent])
if test "x$JITTER_INDENT" = "x"; then
  AC_MSG_WARN([no indent utility found. \
You will not be able to build nice-looking preprocessed C files for \
debugging.  You can still generate preprocessed files, but they will \
not look as nice])
fi
# FIXME: use AC_PATH_PROGS_FEATURE_CHECK to check for indent command line
# argument support.
AM_CONDITIONAL([JITTER_HAVE_INDENT], ! test "x$JITTER_INDENT" = "x")


# Test suite.
################################################################

# Use the TAP interface to exchange data with tests.  This requires a script,
# distributed with Automake.
AC_REQUIRE_AUX_FILE([tap-driver.sh])

# Generate test suite scripts.
AC_CONFIG_FILES([tests/utility]) # This should not be made executable.
AC_CONFIG_FILES([tests/elementary/init-fini.test],
                [chmod +x tests/elementary/init-fini.test])
AC_CONFIG_FILES([tests/uninspired/fundamental.test],
                [chmod +x tests/uninspired/fundamental.test])
AC_CONFIG_FILES([tests/uninspired/programs.test],
                [chmod +x tests/uninspired/programs.test])
AC_CONFIG_FILES([tests/structured/programs.test],
                [chmod +x tests/structured/programs.test])
AC_CONFIG_FILES([tests/jitterlisp/jitterlisp.test],
                [chmod +x tests/jitterlisp/jitterlisp.test])

# Generate convenience emulator script.
AC_CONFIG_FILES([scripts/emulator],
                [chmod +x scripts/emulator])

# Generate convenience disassembly highlighter for ANSI terminals.
AC_CONFIG_FILES([scripts/highlight-disassembly],
                [chmod +x scripts/highlight-disassembly])

# Generate the jitter-config script.
AC_CONFIG_FILES([bin/jitter-config],
                [chmod +x bin/jitter-config])

# Generate the shell script generating high-level fast-branch macros.
AC_CONFIG_FILES([scripts/generate-fast-branches],
                [chmod +x scripts/generate-fast-branches])


# Optional dependencies, for examples only.
################################################################

# JitterLisp can optionally use Boehm's garbage collector.  I use AC_CHECK_LIB
# with an explicit ACTION-IF-FOUND rather than AC_SEARCH_LIBS , because I don't
# want to modify the LIBS variable.  This is specific to some examples.
AC_CHECK_LIB([gc], [GC_malloc], [JITTER_HAVE_BOEHM_GC_LIBRARY=yes])
AC_CHECK_HEADERS([gc/gc.h], [JITTER_HAVE_BOEHM_GC_HEADER=yes])
if    test "x$JITTER_HAVE_BOEHM_GC_LIBRARY" = "xyes" \
   && test "x$JITTER_HAVE_BOEHM_GC_HEADER" = "xyes"; then
  JITTER_HAVE_BOEHM_GC=yes
else
  JITTER_HAVE_BOEHM_GC=no
fi
AC_MSG_CHECKING([if Boehm's GC is usable for JitterLisp])
AC_MSG_RESULT([$JITTER_HAVE_BOEHM_GC])
AM_CONDITIONAL([JITTER_HAVE_BOEHM_GC],
               [test "x$JITTER_HAVE_BOEHM_GC" = "xyes"])

# Also set a substitution telling whether Boehm's GC is used, which is useful
# to check from the test suite.
if test "x$JITTER_HAVE_BOEHM_GC" = "xyes"; then
  AC_SUBST([JITTER_HAVE_BOEHM_GC_SUBST], [1])
else
  AC_SUBST([JITTER_HAVE_BOEHM_GC_SUBST], [])
  true
fi


# ChangeLog generation.
################################################################

# Define an Automake conditional telling whether we're building from git.
# This is useful to decide whether to rebuild the ChangeLog file on dist.
AM_CONDITIONAL([JITTER_BUILDING_FROM_GIT],
               [test -d "$srcdir/.git"])


# Output.
################################################################

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([gnulib-local/Makefile])

AC_OUTPUT
