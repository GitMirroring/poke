dnl configure.ac for GNU poke
dnl
dnl Please process this file with autoconf to get a 'configure'
dnl script.

dnl Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026 Jose E.
dnl Marchesi

dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl Branch-specific suffix to apply to the version, to disambiguate it from
dnl other branches.  master and maint/poke-* are treated special in that they
dnl get a short description, where all other branches get emitted in full.
m4_define([poke_branch],
	  [m4_esyscmd_s([(git tag --points-at HEAD | grep .) >/dev/null 2>&1  \
                            || { git branch --show-current                    \
                                     | sed 's|^maint/poke-.*|maint|;
                                            s|/|_|g;
                                            s|^master$|dev|'; }])])


dnl Version generated from a Git tag via git-version-gen.  To understand the
dnl following mess, it is integral to know that ``:'' is special in Git and
dnl will never be emitted.  We (ab)use this fact to escape the ``-g'', which is
dnl another special construct in Git - used by git-describe output to delimit
dnl the object reference from data for human consumption.
dnl We also rely on this fact and choose ``:'' as a safe sed delimiter.
dnl After we escape -g, and sufficiently mangle the describe output in order to
dnl shove the branch name into the middle of it, we convert the escape ``:''
dnl back into a ``g'', so that we end up with a git-showable reference (unless
dnl the tree is -dirty).

m4_define([poke_version], [m4_esyscmd_s([
    build-aux/git-version-gen .tarball-version                            \
        --prefix 'releases\/poke-'                                        \
        's:^\(releases/poke-\)\([^-]*\)-\([^-]*\)\(-g.*\):\1\2-]poke_branch()-[\3\4:;
         s:g\([[:xdigit:]]\+\)$:\:\1:'                                    \
      | sed 's/:/g/'
])])

AC_INIT([GNU poke],
        [poke_version()],
	[poke-devel@gnu.org], [poke],
        [http://www.jemarch.net/poke.html])

AC_CONFIG_AUX_DIR([build-aux])
dnl In order to have a custom INSTALL file, we need Automake option 'foreign'.
AM_INIT_AUTOMAKE([foreign tar-ustar])
AC_CONFIG_HEADERS(poke/config.h)
AC_CONFIG_MACRO_DIRS([m4])

# Include the Autoconf macros from Jitter.
m4_include([m4/jitter.m4])


dnl Autobuild
dnl AB_INIT

AC_DEFINE([PACKAGE_BUGZILLA],["https://sourceware.org/bugzilla/describecomponents.cgi?product=poke"],[URL for entering bugs in the project's bugzilla])

AC_PROG_CC
gl_EARLY
libpoke_EARLY

LT_INIT
AC_PROG_CC
dnl AC_PROG_CC_C99 is to be used with autoconf < 2.70.
m4_version_prereq([2.70], [:], [AC_PROG_CC_C99])
AM_PROG_CC_C_O

AC_ARG_VAR([LEX], [The flex implementation to use.])
if test -n "$LEX"; then
  AC_MSG_CHECKING([for flex])
  AC_MSG_RESULT([$LEX])
else
  AC_PATH_PROG([LEX], [flex], [:])
fi

gl_PROG_BISON([POKE_BISON],[3.6])

dnl System
AC_CANONICAL_HOST
canonical=$host

dnl The proc IOD is currently Linux-specific, since it relies on the
dnl availability of the /proc file-system.

case "${host_os}" in
  linux*)
    have_proc=yes ;;
  *)
    have_proc=no ;;
esac

if test "x$have_proc" = "xyes"; then
  AC_DEFINE([HAVE_PROC], 1,
            [Defined if the host system features a proc directory.])
fi
AM_CONDITIONAL([HAVE_PROC], [test "x$have_proc" = "xyes"])

dnl Support --{enable,disable}-poked, but note it is not supported on
dnl certain operating systems.

AC_ARG_ENABLE([poked],
              AS_HELP_STRING([--enable-poked],
                             [Enable building the poked component (default is YES)]),
              [enable_poked=$enableval], [enable_poked=yes])

case "${host_os}" in
  mingw* | windows* )
    enable_poked=no ;;
  *)
    ;;
esac

AM_CONDITIONAL([ENABLE_POKED], [test "x$enable_poked" = "xyes"])

dnl Support --{enable,disable}-pokefmt.

AC_ARG_ENABLE([pokefmt],
              AS_HELP_STRING([--enable-pokefmt],
                             [Enable building the pokefmt component (default is YES)]),
              [enable_pokefmt=$enableval], [enable_pokefmt=yes])

AM_CONDITIONAL([ENABLE_POKEFMT], [test "x$enable_pokefmt" = "xyes"])

gl_INIT
libpoke_INIT

# Try to find a libtextstyle with hyperlinks support.
gl_LIBTEXTSTYLE_OPTIONAL([0.20.5])

# GNU help2man creates man pages from --help output; in many cases, this
# is sufficient, and obviates the need to maintain man pages separately.
# However, this means invoking executables, which we generally cannot do
# when cross-compiling, so we test to avoid that (the variable
# "cross_compiling" is set by AC_PROG_CC).
if test $cross_compiling = no; then
  AM_MISSING_PROG(HELP2MAN, help2man)
else
  HELP2MAN=:
fi

dnl The following Automake conditional is used in some Makefile.am files
dnl in order to avoid actions that wouln't work while cross compiling.

AM_CONDITIONAL([NATIVE_BUILD], [test "x$cross_compiling" = "xno"])

dnl i18n with gettext
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

dnl RAS requires a version of awk that supports gensub().  However,
dnl since we are including the RAS-generated .pkc files in the
dnl tarball, this should not be a hard dependency here: people
dnl building poke from released tarballs are not required to have any
dnl particular version of awk installed.  We therefore set RAS_AWK
dnl here, using the value ':' for when no suitable awk is found.  Then
dnl in the `.pkc : .pks' rule in Makefile.am an error message is
dnl emitted in case no proper AWK is present.

error_on_awk_gensub=no
AX_CHECK_AWK_GENSUB([RAS_AWK=$AWK],
  [if test -d $srcdir/.git ; then
     error_on_awk_gensub=yes
   fi
   RAS_AWK=:])
AC_SUBST([RAS_AWK])

dnl The Boehm-Weiser garbage collector

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(BDW_GC,[bdw-gc],[have_gc=yes],[have_gc=no])

dnl Jitter

JITTER_JITTER_SUBPACKAGE([jitter])

dnl Profiling in the PVM

AC_ARG_ENABLE([pvm-profiling],
              AS_HELP_STRING([--enable-pvm-profiling],
                             [Enable profiling in the PVM (default is NO)]),
              [profiling_enabled=$enableval], [profiling_enabled=no])

if test "x$profiling_enabled" = "xyes"; then
  AC_DEFINE([PVM_PROFILE_COUNT], [1], [use count-based profiling in the PVM])
  AC_DEFINE([PVM_PROFILE_SAMPLE], [1], [use sample-based profiling in the PVM])
fi

dnl libnbd for nbd:// io spaces (optional). Testing it also requires
dnl nbdkit

AC_ARG_ENABLE([libnbd],
              AS_HELP_STRING([--enable-libnbd],
                             [Enable building with NBD support (default is YES)]),
              [libnbd_enabled=$enableval], [libnbd_enabled=yes])
NBDKIT=no
if test "x$libnbd_enabled" = "xyes"; then
  PKG_CHECK_MODULES([LIBNBD], [libnbd],
    [dnl Test whether libnbd is really present. This is needed for bi-arch
     dnl systems: libnbd may be installed only in 64-bit mode, and the
     dnl libnbd.pc file will then also be found in 32-bit mode, for which no
     dnl library is installed.
     saved_CFLAGS=$CFLAGS
     saved_LIBS=$LIBS
     CFLAGS="$CFLAGS $LIBNBD_CFLAGS"
     LIBS="$LIBS $LIBNBD_LIBS"
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <libnbd.h>]],
          [[return nbd_get_errno ();]])
       ],
       [dnl Yes it is present.
        AC_DEFINE([HAVE_LIBNBD], [1], [libnbd found at compile time])
        AC_PATH_PROGS([NBDKIT], [nbdkit], [no], [$PATH:/usr/local/sbin:/usr/sbin])
       ],
       [dnl Nope, pkg-config fooled us.
        libnbd_enabled=no
        LIBNBD_CFLAGS=
        LIBNBD_LIBS=
       ])
     LIBS=$saved_LIBS
     CFLAGS=$saved_CFLAGS
    ],
    [libnbd_enabled=no
     LIBNBD_CFLAGS=
     LIBNBD_LIBS=
    ])
fi
AM_CONDITIONAL([NBD], [test "x$libnbd_enabled" = "xyes"])
AC_SUBST([LIBNBD_CFLAGS])
AC_SUBST([LIBNBD_LIBS])

dnl Check for mmap
AC_FUNC_MMAP

AM_CONDITIONAL([HAVE_MMAP], [test "x$ac_cv_func_mmap_fixed_mapped" = "xyes"])

dnl Used in Makefile.am.  See the note there.
WITH_JITTER=$with_jitter
AC_SUBST([WITH_JITTER])

dnl We need to determine the endianness of the host system.  The
dnl following macro is also supposed to work when cross-compiling.

AC_C_BIGENDIAN

dnl The following M4 macro from gnulib sets HOST_CPU_C_ABI_32BIT to
dnl 'yes' if the C language ABI is a 32-bit one, to 'no' if it is
dnl 64-bit, or to 'unknown'.
dnl
dnl We set POKE_HOST_32BIT accordingly, and warn if the 32bitness of
dnl the host cannot be determined.

gl_HOST_CPU_C_ABI_32BIT
case "$HOST_CPU_C_ABI_32BIT" in
  yes)
    AC_DEFINE([POKE_HOST_32BIT],[1],[whether the host C ABI is 32-bit]);;
  unknown)
    AC_MSG_ERROR([unable to determine whether the host C ABI is 32-bit]);;
  *) ;;
esac

dnl Use libtextstyle if available.  Otherwise, use the dummy header
dnl file provided by gnulib's libtextstyle-optional module.

gl_LIBTEXTSTYLE_OPTIONAL
AX_LIBTEXTSTYLE_HYPERLINK

if test $HAVE_LIBTEXTSTYLE = yes; then
    AC_CHECK_LIB([textstyle], [term_styled_ostream_get_destination], [
      AC_DEFINE([HAVE_TEXTSTYLE_ACCESSORS_SUPPORT], 1,
                [Defined if libtextstyle has support for terminal hyperlinks])
    ])
fi

dnl The hyperlinks server is optional.  It is enabled by default if
dnl there is a libtextstyle with hyperlinks support available.

AC_ARG_ENABLE([hserver],
              AS_HELP_STRING([--enable-hserver],
                             [Enable the terminal hyperlinks server (default is YES)]),
              [hserver_enabled=$enableval], [hserver_enabled=$has_hyper_support])

if test "x$has_hyper_support" = "xno"; then
  hserver_enabled=no
fi

AM_CONDITIONAL([HSERVER], [test "x$hserver_enabled" = "xyes"])
if test "x$hserver_enabled" = "xyes"; then
  AC_DEFINE([HAVE_HSERVER], 1,
            [Defined if the hyperlinks server is available])
fi

dnl The test framework is optional.
AC_CHECK_PROGS([RUNTESTPROG],[runtest],[:])
AM_CONDITIONAL([HAVE_DEJAGNU], [test "$RUNTESTPROG" != ":"])

dnl editor syntax definitions
AC_ARG_WITH([vimdir],
 [AS_HELP_STRING([--with-vimdir],
                 [override the default vim directory])],
 [ vimdir="$withval"
   AC_MSG_CHECKING([where vim files should go])
   AC_MSG_RESULT([$vimdir])],
 [ vimdir='${datadir}/vim/vimfiles' ]
)
AC_SUBST([vimdir])

AM_PATH_LISPDIR

dnl Generate output files
AC_CONFIG_FILES(Makefile
                gl/Makefile
                gl-libpoke/Makefile
                libpoke/Makefile
                libpoke/pkl-config.pk
                poke/Makefile
                poked/Makefile
                pokefmt/Makefile
                utils/Makefile
                pickles/Makefile
                doc/Makefile
                man/Makefile
                po/Makefile.in
                etc/Makefile
                etc/vim/Makefile
                testsuite/Makefile
                testsuite/poke.libpoke/Makefile
                autoconf/Makefile)
AC_CONFIG_FILES([run],
                [chmod +x,-w run])

dnl pkg-config description
AX_CREATE_PKGCONFIG_INFO('libpoke/poke.pc', , , 'libpoke')

AC_OUTPUT

dnl Summary

AS_ECHO(["Summary of some of the main features enabled by configure."])

AS_ECHO(["  Build libpoke?                                yes
  Build poke?                                   yes
  Build poked?                                  ${enable_poked}
  Build pokefmt?                                ${enable_pokefmt}
  Build support for NBD IO spaces?              ${libnbd_enabled}
"])

dnl Report warnings

if test "x$HAVE_LIBTEXTSTYLE" = "xno"; then
   AC_MSG_WARN([libtextstyle was not found in the system. Poke's output won't be styled.])
fi

if test "x$hserver_enabled" = "xno"; then
   AC_MSG_WARN([building poke without terminal hyperlinks server support.
     Install a hyperlinks-capable libtextstyle and use --enable-hserver to activate it.])
fi

if test "x$libnbd_enabled" != "xyes"; then
   AC_MSG_WARN([building poke without NBD io space support.
     Install libnbd to use it.])
fi

dnl Report errors

if test "x$error_on_awk_gensub" = "xyes"; then
   AC_MSG_ERROR([you need a gensub-capable awk when building from VCS.
     Please install gawk.])
fi

if test "x$have_gc" = "xno"; then
   AC_MSG_ERROR([can't find the Boehm GC library.  Please install it.])
fi

if test "x$gl_cv_lib_readline" = "xno"; then
   AC_MSG_ERROR([can't find an usable libreadline.  Please install one.])
fi

dnl End of configure.ac
