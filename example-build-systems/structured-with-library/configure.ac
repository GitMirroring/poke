## This is an -*- Autoconf -*- file to be processed with autoconf
## to obtain a configure script.

## Copyright (C) 2016, 2017, 2021 Luca Saiu
## Updated in 2019, 2020 by Luca Saiu
## Written by Luca Saiu

## This file is part of the Jitter structured-language example,
## distributed along with GNU Jitter under the same license.

## GNU Jitter is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## GNU Jitter is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>.


# Global initialization.
################################################################

# There is no real need to keep an independent version number for the
# structured-language example.
AC_INIT([Structured],
        [1.0],
        [bug-epsilon@gnu.org],
        [structured-with-library],
        [http://ageinghacker.net/jitter])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])


# Autoconf macro inclusion.
################################################################

# Make the file jitter.m4 visible from here, to get access to JITTER .
AC_CONFIG_MACRO_DIR([build-aux])


# Automake initialization.
################################################################

# The tar-pax option lets make dist work with GNU tar, even if path names are
# very long.
AM_INIT_AUTOMAKE([tar-pax])


# Libtool initialization.
################################################################

LT_INIT


# Check for the C compiler and its features.
################################################################

# We need a C compiler.  This call would not even be needed, because
# JITTER_JITTER or JITTER_JITTER_SUBPACKAGE already require it.
AC_PROG_CC


# Configure Jitter as a sub-package.
################################################################

# Configure Jitter as a sub-package, without depending on it being installed
# anywere.  This assumes that the Jitter top source directory has been copied
# or symbolically linked as "jitter" within this directory.
JITTER_JITTER_SUBPACKAGE([jitter])


# Check for frontend C-code-generation tools.
################################################################

# Check for flex.
AC_PROG_LEX([noyywrap])
if test "x$LEX" != "xflex"; then
  if test -e "$srcdir/structured-scanner.c"; then
    AC_MSG_WARN([you will need flex (instead of $LEX) if you modify the
                 distributed C scanners])
  else
    AC_MSG_ERROR([you need flex (instead of $LEX) to generate scanners,
                  if you compile from git])
  fi
fi

# Check for Bison.
AC_PROG_YACC
if ! (echo "x$YACC" | grep bison > /dev/null); then
  if test -e "$srcdir/structured-parser.c"; then
    AC_MSG_WARN([you will need GNU Bison (instead of $YACC) if you modify
                 the distributed C parsers])
  else
    AC_MSG_ERROR([you need GNU Bison (instead of $YACC) to generate parsers,
                  if you compile from git])
  fi
fi


# Output.
################################################################

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
