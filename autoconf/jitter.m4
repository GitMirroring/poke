# Autoconf macros for Jitter.
# Copyright (C) 2017, 2018, 2019, 2020, 2021 Luca Saiu
# Written by Luca Saiu

# This file is part of GNU Jitter.

# GNU Jitter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# GNU Jitter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>.


# The Autoconf manual says (about writing Autoconf macros):
# No Autoconf macro should ever enter the user-variable name space;
# i.e., except for the variables that are the actual result of running the
# macro, all shell variables should start with 'ac_'.  In addition, small
# macros or any macro that is likely to be embedded in other macros should
# be careful not to use obvious names.


# Generic M4 machinery.
################################################################

# jitter_tocpp([my-text])
# -----------------------
# Expand to the argument where each letter is converted to upper case, and
# each dash is replaced by an underscore.
# Example: jitter_tocpp([my-teXt_4]) --> MY_TEXT_4
# About the namespace, see the comment for jitter_using_automake.
m4_define([jitter_tocpp],
          [m4_translit([m4_toupper([$1])], ][-][, ][_][)])dnl

# jitter_define_iterator([iterator_name], [list])
# -----------------------------------------------
# Expand to the *definition* of an iterator like jitter_for_dispatch or
# jitter_for_flag , below.  Notice that there is an empty line appended to
# each repetition of the body.
# This higher-order macro is used internally to define convenient, nestable
# iterator macros.
m4_define([jitter_define_iterator],
  [m4_define([$1],
             [m4_foreach(][$][1][, [$2],
                         ][$][2][
)])])
# FIXME: no, this doesn't get quoting quite right.  Not using it.  Test case:
# jitter_for_dispatch([j],
#   [echo "quoted-[j] is unquoted-j"])
# =>
# echo "quoted-switch is unquoted-switch"
# echo "quoted-direct-threading is unquoted-direct-threading"
# echo "quoted-minimal-threading is unquoted-minimal-threading"
# echo "quoted-no-threading is unquoted-no-threading"
#
# Instead, by using directly m4_foreach , I get what I want:
# m4_foreach([i],
#            [jitter_dispatches],
#   [echo "quoted-[i] is unquoted-i"
# ])
# =>
# echo "quoted-i is unquoted-switch"
# echo "quoted-i is unquoted-direct-threading"
# echo "quoted-i is unquoted-minimal-threading"
# echo "quoted-i is unquoted-no-threading"

# jitter_for_dispatch([a_dispatch_variable], [body])
# --------------------------------------------------
# Expand to a sequence of bodies, each copy with a_dispatch_variable replaced
# by each of the dispatches in the order of jitter_dispatches; each body
# repetition has a newline at the end.
# Example:
# jitter_for_dispatch([a],
#   [jitter_for_dispatch([b],
#     [echo [Here is a dispatch pair: ]a[, ]b])])
#
# FIXME: this commented-out definition is not correct with respect to
# quoting: see the comment above.
# # jitter_define_iterator([jitter_for_dispatch],
# #                        [jitter_dispatches])
m4_define([jitter_for_dispatch],
  [m4_foreach([$1], [jitter_dispatches], [$2
])])

# jitter_for_flag([a_flag_variable], [body])
# ------------------------------------------
# Like jitter_for_dispatch, using jitter_flags instead of jitter_dispatches .
#
# FIXME: this commented-out definition is not correct with respect to
# quoting: see the comment above.
# jitter_define_iterator([jitter_for_flag],
#                        [jitter_flags])
m4_define([jitter_for_flag],
  [m4_foreach([$1], [jitter_flags], [$2
])])


# Jitter global definitions.
################################################################

# jitter_dispatches
# -----------------
# An M4 quoted list of every existing Jitter dispatch, including the ones which
# are not enabled, lower-case with dashes.
m4_define([jitter_dispatches],
          [[switch], [direct-threading], [minimal-threading], [no-threading]])

# jitter_flags
# ------------
# An M4 quoted list of flag names, lower-case with dashes.
m4_define([jitter_flags],
          [[cflags], [cppflags], [ldadd], [ldflags], [libadd]])


# Jitter internal Autoconf macros.
################################################################

# The Autoconf macros in this section are subject to change in the future, and
# should not be directly invoked by the user.

# JITTER_USING_AUTOMAKE
# ---------------------
# This is only used internally.  If Automake is being used, then define
# the shell variable jitter_using_automake to "yes".
AC_DEFUN([JITTER_USING_AUTOMAKE], [
# Define jitter_using_automake to a non-empty value iff the configuration
# system is used Automake and well, and not just Autoconf.  I have found no
# documented way of doing this, so I am relying on am__api_version being
# defined, which happens, indirectly, when AM_INIT_AUTOMAKE is called.
# This is not particularly related to Jitter but I found no predefined way
# of doing it, and I'm adding the variable to the JITTER namespace just
# to prevent conflicts.
if test "x$am__api_version" != "x"; then
  jitter_using_automake="yes"
fi
]) # JITTER_USING_AUTOMAKE


# Sub-packages to be configured immediately, not on AC_OUTPUT.
################################################################

# JITTER_CONFIG_SUBDIRS_NOW([dir ...])
# ------------------------------------
# Behave like AC_CONFIG_SUBDIRS , but call the recursive configure immediately
# rather that at the time of AC_OUTPUT.  A final explicit AC_OUTPUT call by
# the user will still be required, as usual.
#
# Rationale: If the sub-package is configured immediately, the rest of
# the outer package configure script can make its configuration depend
# on the inner package.
AC_DEFUN([JITTER_CONFIG_SUBDIRS_NOW], [
# Keep a backup of some shell variables used by configure at AC_OUTPUT time to
# handle generated files and subdirectories.  Then clean up the current values
# of these variables so that AC_OUTPUT *only* deals with the subpackage.
jitter_ac_config_files_backup="${ac_config_files}"
jitter_subdirs_backup="${subdirs}"
ac_config_files=''
subdirs=''
# Add the sub-package the ordinary way, and output.  This has the effect of
# configuring the sub-package.
AC_CONFIG_SUBDIRS([$1])
AC_MSG_NOTICE([recursively invoking configure in sub-package] [$1] [right now])
# Here we could use AC_OUTPUT and then remove config.status , which should
# not be generated here as that would introduce subtle bugs in case the user
# forgot to call AC_OUTPUT at the end.
# But instead of using AC_OUTPUT we can use its supposedly internal subroutine
# _AC_OUTPUT_SUBDIRS, which *only* deals with sub-packages.  This is exactly
# what we need.
_AC_OUTPUT_SUBDIRS
AC_MSG_NOTICE([back from the recursive configuration in] [$1])
# Restore the shell variables we saved later, so that a user call to
# AC_OUTPUT later will do its work.  The sub-package we have configured
# already will not be configured again.
ac_config_files="${jitter_ac_config_files_backup}"
subdirs="${jitter_subdirs_backup}"
]) # JITTER_CONFIG_SUBDIRS_NOW


# Jitter Autoconf macros, not meant for the user.
################################################################

# JITTER_CHECK_AUTOCONF_VERSION
# -----------------------------
# Define the following M4 macros determining whether Autoconf is recent enough
# to support a certain fetaure:
# * jitter_autoconf_version
#   Define to the Autoconf version number as suitable as an argument for
#   m4_version_compare.  Define as the three-character string old if Autoconf
#   is too old to support this feature.
# * jitter_ac_prog_cc_can_check_c_dialect
#   Define to a non-empty value if we are sure that AC_PROG_CC sets
#   ac_prog_cc_stdc.  Empty otherwise;
# * jitter_ac_prog_lex_with_arguments
#   Define to a non-empty value if we are sure that AC_LEX supports arguments.
#   Empty otherwise.
AC_DEFUN([JITTER_CHECK_AUTOCONF_VERSION], [
# Autoconf 2.71 is:
# - a version sufficiently recent for
#   JITTER_PROG_CC_AND_CHECK_C_DIALECT_NEW_STYLE, which relies on AC_PROG_CC
#   setting $ac_prog_cc_stdc.  Notice that AC_PROG_CC_C99 is considered obsolete
#   by recent versions or Autoconf;
# - a version supporting AC_PROG_LEX with the noyywrap argument.
# I would like to require that, but it may be too recent for many people.
# AC_PREREQ([2.71])
AC_PREREQ([2.62])

# Conditionalise on Autoconf version and define the feature macros mentioned in
# the comment above.  First define defaults, then change them if needed.
m4_define([jitter_autoconf_version], [old])
m4_define([jitter_ac_prog_cc_can_check_c_dialect], [])
m4_define([jitter_ac_prog_lex_with_arguments], [])
m4_ifdef([AC_AUTOCONF_VERSION],
  [# AC_AUTOCONF_VERSION is defined, so the Autoconf version is >= 2.62.
   m4_define([jitter_autoconf_version], m4_defn([AC_AUTOCONF_VERSION]))
   m4_if(m4_version_compare(m4_defn([AC_AUTOCONF_VERSION]), [2.71]), [-1],
         [AC_MSG_WARN([Autoconf not recent enough for recognising C dialect])],
         [m4_define([jitter_ac_prog_cc_can_check_c_dialect], [yes])])
   m4_if(m4_version_compare(m4_defn([AC_AUTOCONF_VERSION]), [2.71]), [-1],
         [AC_MSG_WARN([Autoconf not recent enough for AC_PROG_LEX with arguments])],
         [m4_define([jitter_ac_prog_lex_with_arguments], [yes])])
  ],
  [# AC_AUTOCONF_VERSION is not defined.  Assume Autoconf to be older than
   # version 2.62, when the macro was introduced.  Before 2.62
   # m4_version_compare was also more limited.
  ])dnl
]) # JITTER_CHECK_AUTOCONF_VERSION

# JITTER_PROG_CC_AND_CHECK_C_DIALECT_OLD_STYLE
# --------------------------------------------
# This is one of the two definitions of JITTER_PROG_CC_AND_CHECK_C_DIALECT; it
# is less precise than JITTER_PROG_CC_AND_CHECK_C_DIALECT_NEW_STYLE but works
# with older Autoconf versions as well.
AC_DEFUN([JITTER_PROG_CC_AND_CHECK_C_DIALECT_OLD_STYLE], [
# Use AC_PROG_CC_C99, which is the best we can do.
AC_REQUIRE([AC_PROG_CC_C99])

# Report.
AC_MSG_CHECKING([the C dialect supported by "$CC" (in a somewhat imprecise \
way due to the Autoconf version which is ]jitter_autoconf_version[)])
if test "x$ac_cv_prog_cc_c99" = "xno"; then
  AC_MSG_RESULT([older than C99])
  AC_MSG_WARN([this compiler is pre-C99: expect trouble (but trying to \
continue anyway)])
else
  AC_MSG_RESULT([at least C99])
fi
]) # JITTER_PROG_CC_AND_CHECK_C_DIALECT_OLD_STYLE

# JITTER_PROG_CC_AND_CHECK_C_DIALECT_NEW_STYLE
# --------------------------------------------
# This is one of the two definitions of JITTER_PROG_CC_AND_CHECK_C_DIALECT; this
# requires that the Autoconf macros are recent enough for AC_PROG_CC to set
# ac_prog_cc_stdc.
AC_DEFUN([JITTER_PROG_CC_AND_CHECK_C_DIALECT_NEW_STYLE], [
# Use AC_PROG_CC to check for a C compiler...
AC_REQUIRE([AC_PROG_CC])

# ...AC_PROG_CC also sets ac_prog_cc_stdc , which is very convenient to check
# that the supported C dialect is recent enough, even before performing specific
# tests.
AC_MSG_CHECKING([the C dialect supported by "$CC"])
AS_CASE([$ac_prog_cc_stdc],
        [no],
          [AC_MSG_RESULT([pre-standard C])
           AC_MSG_WARN([expect huge trouble])],
        [c89],
          [AC_MSG_RESULT([C89])
           AC_MSG_WARN([expect trouble unless the compiler supports a lot of \
extensions: C89 is now very old])],
        [c99],
          [AC_MSG_RESULT([C99 only])
           AC_MSG_WARN([C99 compilers are less commonly tested but should \
work])],
        [c11],
          [AC_MSG_RESULT([C11])
           AC_MSG_NOTICE([good: we routinely test Jitter with C11 compilers])],
        # Default case (for future compatibility).
          [AC_MSG_RESULT(["$ac_prog_cc_stdc", which I assume to be least C11])
           AC_MSG_NOTICE([this C compiler seems modern])])
]) # JITTER_PROG_CC_AND_CHECK_C_DIALECT_NEW_STYLE

# JITTER_PROG_CC_AND_CHECK_C_DIALECT
# ----------------------------------
# Use AC_PROG_CC and possibly other macros to check for a C compiler, and warn
# if the supported dialect is not recent enough; try to continue in any case.
#
# This is defined conditionally according to the Autoconf version; more recent
# versions of Autoconf allow for more precise checks.
AC_DEFUN([JITTER_PROG_CC_AND_CHECK_C_DIALECT], [
# First make sure we have checked the Autoconf version, and defined feature m4
# macros.
AC_REQUIRE([JITTER_CHECK_AUTOCONF_VERSION])

# Require either one macro or the other, according to the Autoconf version.
m4_if(jitter_ac_prog_cc_can_check_c_dialect, [],
      [AC_REQUIRE([JITTER_PROG_CC_AND_CHECK_C_DIALECT_OLD_STYLE])],
      [AC_REQUIRE([JITTER_PROG_CC_AND_CHECK_C_DIALECT_NEW_STYLE])])
]) # JITTER_PROG_CC_AND_CHECK_C_DIALECT

# JITTER_WITH_JITTER_COMMAND_LINE_OPTION
# --------------------------------------
# Provide a configure-time option --with-jitter="PREFIX", setting a prefix
# to search for jitter-config and jitter.
# If the option is not given then the two executables are searched for in
# $PATH; if the option is given then they are searched for *only* in the
# "bin" subdirectory of the given prefix.
#
# This macro sets the environment variable jitter_jitter_path if the option is
# given; it leaves the variable unset otherwise.
#
# This option is intended for packages using Jitter as a dependency, rather than
# as a sub-package; in sub-package mode the executables will be searched for in
# the appropriately named subdirectories of the super-package source and build
# directories.
AC_DEFUN([JITTER_WITH_JITTER_COMMAND_LINE_OPTION], [
# Fail if this is used in sub-package mode.
if test "x$JITTER_SUBPACKAGE_DIRECTORY" != 'x'; then \
  AC_MSG_ERROR([supporting --with-jitter makes no sense in sub-package mode])
fi

# Provide an option for the user to explicitly set the prefix to
# bin/jitter .  jitter_jitter_path will be defined as either the given
# path with "/bin" appended, or $PATH.
AC_ARG_WITH([jitter],
            [AS_HELP_STRING([--with-jitter="PREFIX"],
               [use the jitter and jitter-config programs from the "bin"
                subdirectory of the given prefix instead of searching for
                them in $PATH])],
            [jitter_jitter_path="$withval/bin"])
])

# JITTER_PROG_JITTER_CONFIG
# -------------------------
# Make sure that the system can compile Jittery VMs; this also tests
# for a C compiler using AC_PROG_CC.
# Look for the jitter-config script:
# * if the shell variable JITTER_SUBPACKAGE_DIRECTORY is defined as a non-empty
#   value, in ${JITTER_SUBPACKAGE_DIRECTORY}/bin relative to the super-package
#   build directory;
# * if the environment variable jitter_jitter_path is set, in
#   ${jitter_jitter_path}/bin (only);
# * otherwise, in $PATH;
#
# Choose the default dispatch, using --with-jitter-dispatch="DISPATCH" or
# the best available dispatch if the option is not given.
#
# Define the substitution JITTER_CONFIG to either the full pathname of a
# jitter-config script which appears to be working, or nothing in case of
# problems.
# When Automake is used, also define the following Automake conditional:
# * JITTER_HAVE_JITTER_CONFIG  (true iff jitter-config has been found).
#
# In case jitter-config is found, also substitute:
# * JITTER_CONFIG_VERSION      (the version of Jitter the jitter-config script
#                               comes from)
# * JITTER_DISPATCHES          (all the enabled dispatched, small caps, with
#                               dashes separating words, one space separating
#                               dispatch names);
# * JITTER_BEST_DISPATCH       (the best dispatch in JITTER_DISPATCHES);
# * JITTER_DEFAULT_DISPATCH    (the dispatch chosen by default);
# * JITTER_CFLAGS              (CFLAGS with the default dispatch);
# * JITTER_CPPFLAGS            (CPPFLAGS with the default dispatch);
# * JITTER_LDADD               (LDADD with the default dispatch);
# * JITTER_LDFLAGS             (LDFLAGS with the default dispatch);
# * JITTER_LIBADD              (LIBADD with the default dispatch);
# * for every dispatch $D (in all caps, with underscores separating words):
#   - JITTER_$D_CFLAGS         (CFLAGS with dispatch $D);
#   - JITTER_$D_CPPFLAGS       (CPPFLAGS with dispatch $D);
#   - JITTER_$D_LDADD          (LDADD with dispatch $D);
#   - JITTER_$D_LDFLAGS        (LDFLAGS with dispatch $D);
#   - JITTER_$D_LIBADD         (LIBADD with dispatch $D).
# When Automake is used, also define the following Automake conditionals
# for each dispatch $D (in all caps, with underscores separating
# words):
# * JITTER_ENABLE_DISPATCH_$D  (true iff $D is enabled).
#
# Moreover, define the following for the libjitter-libtextstyle wrapper:
# * JITTER_LIBTEXTSTYLE_CPPFLAGS  (the command line C preprocessor flags to
#                                  use for *using* the wrapper library;
#                                  without these falgs the wrapper is not
#                                  used, even if available);
# * JITTER_LIBTEXTSTYLE_LDADD     (LDADD for linking the wrapper and GNU
#                                  libtextstyle itself into an executable);
# * JITTER_LIBTEXTSTYLE_LIBADD    (LDADD for linking the wrapper and GNU
#                                  libtextstyle itself into a library).
# When Automake is used, also defined the following Automake conditional:
# * JITTER_HAVE_LIBTEXTSTYLE      (true iff GNU Libtextstyle and its wrapper
#                                  are available).
#
# Moreover, define the following for the libjitter-readline wrapper, which
# is always built:
# * JITTER_READLINE_LDADD         (LDADD for linking the wrapper (and GNU
#                                  readline itself, where used) into an
#                                  executable);
# * JITTER_READLINE_LIBADD        (LDADD for linking the wrapper (and GNU
#                                  readline itself, where used) into a
#                                  library).
AC_DEFUN([JITTER_PROG_JITTER_CONFIG], [
# I'd like to define Automake conditionals later on, but that only works if
# the project is actually using Automake.
AC_REQUIRE([JITTER_USING_AUTOMAKE])

# Every test from now on is about C.
AC_LANG_PUSH([C])

# In order to compile Jittery VMs we need a recent C compiler.
AC_REQUIRE([JITTER_PROG_CC_AND_CHECK_C_DIALECT])  # This also defines EXEEXT .

# Define jitter_jitter_path from JITTER_SUBPACKAGE_DIRECTORY, if Jitter is being
# used in sub-package mode.  From the point of view of prefixes, this is
# functionally equivalent to --with-jitter .
if test "x$JITTER_SUBPACKAGE_DIRECTORY" != 'x' \
   && test "x$JITTER_SUBPACKAGE_DIRECTORY" != 'xno'; then
  if test "x$jitter_jitter_path" != 'x'; then
    # Using --with-jitter together with JITTER_SUBPACKAGE_DIRECTORY is a
    # contradiction.
    AC_MSG_ERROR([Enabling sub-package mode with JITTER_SUBPACKAGE_DIRECTORY is
                  incompatible with the --with-jitter option])
  else
    # Using sub-package mode, when --with-jitter was not given.  Set the
    # path variable.
    # REMARK: ac_pwd is undocumented.  Is there a cleaner alternative to
    #         find the build directory at configure time?
    jitter_jitter_path="${ac_pwd}/${JITTER_SUBPACKAGE_DIRECTORY}/bin"
  fi
fi

# Search for the "jitter-config" script and perform the JITTER_CONFIG
# substitution.
if test "x$jitter_jitter_path" = 'x'; then
  # Search in $PATH (only).
  AC_PATH_PROG([JITTER_CONFIG],
               [jitter-config])
else
  # Search in $jitter_jitter_path (only).
  AC_PATH_PROG([JITTER_CONFIG],
               [jitter-config],
               ,
               [$jitter_jitter_path])
fi

# However jitter-config was found, verify that it can be used; if not, unset the
# JITTER_CONFIG variable (and its substitution).
AS_IF([test "x$JITTER_CONFIG" = "x"],
        [AC_MSG_NOTICE([can't find jitter-config])],
      [! test -r "$JITTER_CONFIG"],
        [AC_MSG_WARN([can't read jitter-config at $JITTER_CONFIG])
         JITTER_CONFIG=""],
      [! test -x "$JITTER_CONFIG"],
        [AC_MSG_WARN([can't execute jitter-config at $JITTER_CONFIG])
         JITTER_CONFIG=""],
      [! "$JITTER_CONFIG" --best-dispatch > /dev/null 2> /dev/null],
        [AC_MSG_WARN([non-working jitter-config at $JITTER_CONFIG])
         JITTER_CONFIG=""])

# Provide an option for the user to explicitly choose a default dispatch.
# Decide on the default dispatch at this point, either using the option
# or choosing the best.
# jitter_default_dispatch will be defined as the dispatch in lower
# case with dashes separating words.  Default flag variables will refer to the
# selected dispatch.
# If the requested dispatch is not avaiable the variable will be redefined later
# to use the best, with a warning.
AC_ARG_WITH([jitter-dispatch],
            [AS_HELP_STRING([--with-jitter-dispatch="DISPATCH"],
               [use the given dispatch (either one given by
                jitter-config --dispatches or "best", which is the default)
                for the default jitter flag variables; if the requested
                dispatch is not available in the current Jitter
                configuration warn and use the best])],
            [jitter_default_dispatch="$withval"],
            [jitter_default_dispatch="best"])

# Define the Automake conditional JITTER_HAVE_JITTER_CONFIG , if we are using
# Automake.
if test "x$jitter_using_automake" != "x"; then
  AM_CONDITIONAL([JITTER_HAVE_JITTER_CONFIG],
                 [test "x$JITTER_CONFIG" != "x"])
fi

# At this point $JITTER_CONFIG is either the full pathname to an apparently
# working script, or empty.  If it seems to work, use it to define the rest of
# the substitutions.
if test "x$JITTER_CONFIG" != "x"; then
  # Define the jitter-config version.
  AC_SUBST([JITTER_CONFIG_VERSION],
           [$("$JITTER_CONFIG" --dump-version)])

  # Define the list of enabled dispatches.
  AC_SUBST([JITTER_DISPATCHES],
           [$("$JITTER_CONFIG" --dispatches)])
  AC_MSG_NOTICE([the available Jitter dispatches are $JITTER_DISPATCHES])

  # Define the best available dispatch.
  AC_SUBST([JITTER_BEST_DISPATCH],
           [$("$JITTER_CONFIG" --best-dispatch)])
  AC_MSG_NOTICE([the best available Jitter dispatch is "$JITTER_BEST_DISPATCH"])

  # Define the default dispatch.  In case "best" was requested, replace
  # it with the actual name.  If the dispatch selected as default is
  # not available warn, and use the best available.
  if test "x$jitter_default_dispatch" = "xbest"; then
    jitter_default_dispatch="$JITTER_BEST_DISPATCH"
  elif ! "$JITTER_CONFIG" --has-dispatch="$jitter_default_dispatch"; then
    AC_MSG_WARN([the requested Jitter dispatch \
"$jitter_default_dispatch" is not available: choosing the best available \
"$JITTER_BEST_DISPATCH" instead])
    jitter_default_dispatch="$JITTER_BEST_DISPATCH"
  fi
  AC_SUBST([JITTER_DEFAULT_DISPATCH],
           [$jitter_default_dispatch])
  AC_MSG_NOTICE([the default Jitter dispatch used here will be \
"$JITTER_DEFAULT_DISPATCH"])

  # Define flags for the default dispatch.
  jitter_for_flag([a_flag],
    [AC_SUBST([JITTER_]jitter_tocpp(a_flag),
              [$("$JITTER_CONFIG" --dispatch="$JITTER_DEFAULT_DISPATCH" \
                 --a_flag)])])

  # For every dispatch and flag define a substitution JITTER_$dispatch_$flag .
  jitter_for_dispatch([a_dispatch],
    [if "$JITTER_CONFIG" --has-dispatch=a_dispatch; then
       jitter_for_flag([a_flag],
         [AC_SUBST([JITTER_]jitter_tocpp(a_dispatch)[_]jitter_tocpp(a_flag),
                   [$("$JITTER_CONFIG" --dispatch=a_dispatch --a_flag)])])
     fi])
fi # ...if jitter-config exists and works

# If using Automake then for every dispatch, define an Automake conditional
# telling whether it's enabled.  This has to be done even if we couldn't find a
# usable jitter-config , since Automake conditionals must be always defined:
# they do not default to false.
if test "x$jitter_using_automake" != "x"; then
  jitter_for_dispatch([a_dispatch],
    [AM_CONDITIONAL([JITTER_ENABLE_DISPATCH_]jitter_tocpp(a_dispatch),
                    [   test "x$JITTER_CONFIG" != "x" \
                     && "$JITTER_CONFIG" --has-dispatch=]a_dispatch)])
fi

# Define substitutions for the GNU Libtextstyle wrapper, if jitter-config
# was found.
if test "x$JITTER_CONFIG" != 'x'; then
  AC_SUBST([JITTER_LIBTEXTSTYLE_CPPFLAGS],
           [$("$JITTER_CONFIG" --libtextstyle-cppflags)])
  AC_SUBST([JITTER_LIBTEXTSTYLE_LDADD],
           [$("$JITTER_CONFIG" --libtextstyle-ldadd)])
  AC_SUBST([JITTER_LIBTEXTSTYLE_LIBADD],
           [$("$JITTER_CONFIG" --libtextstyle-libadd)])
fi

# Define substitutions for the GNU Readline wrapper, if jitter-config
# was found.
if test "x$JITTER_CONFIG" != 'x'; then
  AC_SUBST([JITTER_READLINE_LDADD],
           [$("$JITTER_CONFIG" --readline-ldadd)])
  AC_SUBST([JITTER_READLINE_LIBADD],
           [$("$JITTER_CONFIG" --readline-libadd)])
fi

# Define the Automake conditional for the GNU Libtextstyle wrapper, if we
# are using Automake and jitter-config was found.
if    test "x$jitter_using_automake" != "x" \
   && test "x$JITTER_CONFIG" != 'x'; then
  AM_CONDITIONAL([JITTER_HAVE_LIBTEXTSTYLE],
                 ["$JITTER_CONFIG" --have-libtextstyle])
fi

# We're done testing C features, for the time being.
AC_LANG_POP([C])
]) # JITTER_PROG_JITTER_CONFIG


# JITTER_PROG_JITTER
# ------------------
# Look for jitter, the C code generator program in $PATH, or if the option
# --with-jitter="PREFIX" is given in PREFIX/bin (only).
#
# Substitute:
# * JITTER                            (the jitter program full path, or empty
#                                      if not found)
# * JITTER_VERSION                    (the version of Jitter the C generator
#                                      program comes from)
#
# When Automake is used, also define the following Automake conditional:
# * JITTER_HAVE_JITTER_PROG_JITTER    (true iff jitter has been found).
AC_DEFUN([JITTER_PROG_JITTER], [
# Check for a C compiler, if it hasn't been done already.  This defines EXEEXT .
AC_REQUIRE([JITTER_PROG_CC_AND_CHECK_C_DIALECT])

# I'd like to define an Automake conditional later on, but that only works if
# the project is actually using Automake.
AC_REQUIRE([JITTER_USING_AUTOMAKE])

# Search for the "jitter" program and perform the JITTER substitution.
if test "x$jitter_jitter_path" = 'x'; then
  # Search in $PATH (only).
  AC_PATH_PROG([JITTER],
               [jitter])
else
  # Search in $jitter_jitter_path (only).
  AC_PATH_PROG([JITTER],
               [jitter],
               ,
               [$jitter_jitter_path])
fi

# However jitter was found, verify that it can be used; if not, unset the JITTER
# variable (and its substitution).
AS_IF([test "x$JITTER" = "x"],
        [AC_MSG_NOTICE([can't find jitter])],
      [! test -r "$JITTER"],
        [AC_MSG_WARN([can't read jitter at $JITTER])
         JITTER=""],
      [! test -x "$JITTER"],
        [AC_MSG_WARN([can't execute jitter at $JITTER])
         JITTER=""],
      [! "$JITTER" --dump-version > /dev/null],
        [AC_MSG_WARN([jitter at $JITTER seems to fail])
         JITTER=""])

# Define the Automake conditional JITTER_HAVE_JITTER_PROG_JITTER , if we are
# using Automake.
if test "x$jitter_using_automake" != "x"; then
  AM_CONDITIONAL([JITTER_HAVE_JITTER_PROG_JITTER],
                 [test "x$JITTER" != "x"])
fi

# If we found a C generator, make the other substitutions.
if test "x$JITTER" != "x"; then
  # Define the Jitter C generator version.
  AC_SUBST([JITTER_VERSION],
           [$("$JITTER" --dump-version)])
fi
]) # JITTER_PROG_JITTER


# Internal helpers for public utility macros.
################################################################

# The common factor of JITTER_PROG_FLEX_OLD_STYLE and
# JITTER_PROG_FLEX_NEW_STYLE.
AC_DEFUN([JITTER_PROG_FLEX_COMMON], [
# This must be called after AC_PROG_LEX, with or without arguments.
if echo "$LEX" | grep 'flex' > /dev/null; then
  AC_MSG_NOTICE([flex is installed as "$LEX"])
else
  # The program which was found was not flex: undefine it.
  AC_MSG_WARN([found "$LEX" when looking for flex: that is not adequate])
  LEX=''
  ac_cv_prog_LEX=''
fi
]) # JITTER_PROG_FLEX_COMMON

# One of the two implementations of JITTER_PROG_FLEX, intended for older
# Autoconf versions.
AC_DEFUN([JITTER_PROG_FLEX_OLD_STYLE], [
AC_PROG_LEX
JITTER_PROG_FLEX_COMMON
])

# One of the two implementations of JITTER_PROG_FLEX, intended for recent
# Autoconf versions.
AC_DEFUN([JITTER_PROG_FLEX_NEW_STYLE], [
AC_PROG_LEX([noyywrap])
JITTER_PROG_FLEX_COMMON
])


# Jitter Autoconf macros, intended for the user.
################################################################

# The macros in this sections are meant for other packages using Jitter as a
# dependency or as a sub-package.  They are not used in the configuration of
# Jitter.


# JITTER_JITTER
# -------------
# Check for jitter-config and jitter as by calling both
# JITTER_PROG_JITTER_CONFIG and JITTER_PROG_JITTER , performing the same
# substitutions and supporting the same command-line options.
#
# Add the configure option --with-jitter , as per
# JITTER_WITH_JITTER_COMMAND_LINE_OPTION above.
#
# Warn if jitter-config and jitter have different versions.
#
# This is the only macro the user needs to call to check for a Jitter
# installation as a dependency.
AC_DEFUN([JITTER_JITTER], [

# Provide the command-line option --with-jitter.
AC_REQUIRE([JITTER_WITH_JITTER_COMMAND_LINE_OPTION])

# Check for jitter-config .
AC_REQUIRE([JITTER_PROG_JITTER_CONFIG])

# Check for jitter .
AC_REQUIRE([JITTER_PROG_JITTER])

# In case we found both, check that their two versions match.
if test "x$JITTER_CONFIG" != "x" && test "x$JITTER" != "x"; then
  if test "x$JITTER_CONFIG_VERSION" != "x$JITTER_VERSION"; then
    AC_MSG_WARN([version mismatch between $JITTER_CONFIG (version
$JITTER_CONFIG_VERSION) and $JITTER (version $JITTER_VERSION)])
  fi
fi
]) # JITTER_JITTER


# JITTER_JITTER_SUBPACKAGE([subdirectory])
# ----------------------------------------
# Configure Jitter in sub-package mode, using the Jitter source directory
# from the given subdirectory relative to the super-package source directory.
#
# This is the only macro the user needs to call to configure Jitter as a
# sub-package included in a source directory.  It defines the same entities
# as JITTER, but differently from it does not rely on, or support, an
# already installed copy of Jitter as a dependency.
AC_DEFUN([JITTER_JITTER_SUBPACKAGE], [

# We are going to use sed below.
AC_REQUIRE([AC_PROG_SED])

# Notice that in this case it makes no sense to support the command-line option
# --with-jitter , and therefore this macro does not depend on
# JITTER_WITH_JITTER_COMMAND_LINE_OPTION .

# Print one explicit message about what is about to happen.  Even Autoconf
# experts might be surprised by the unusual configuration order, and it is good
# to be explicit in case something happens.
AC_MSG_NOTICE([configuring Jitter as a sub-package in ./$1])

# Make sure that Jitter will be configured in sub-package mode.  Jitter's
# configure script checks for this environment variable, as it is inconvenient
# to call it with an additional command-line option when invoked recursively
# from a super-package configure script.
# The variable must be set to the Jitter source subdirectory, relative to the
# super-package source directory.
JITTER_SUBPACKAGE_DIRECTORY="$1"
export JITTER_SUBPACKAGE_DIRECTORY

# Call Jitter's configure script recursively, right now.
JITTER_CONFIG_SUBDIRS_NOW([$1])

# Check for jitter-config .  Do not check for the jitter C generator: it will
# not normally be available at configuration time, but it will be built.
JITTER_PROG_JITTER_CONFIG

# Fail if we failed to find jitter-config; this should not happen in sub-package
# mode, after Jitter's subdirectory has been configured with success.
if test "x$JITTER_CONFIG" = 'x'; then
  AC_MSG_ERROR([could not find jitter-config in Jitter sub-package within
                $JITTER_SUBPACKAGE_DIRECTORY])
fi

# The "jitter" C generator may not exist yet, but it can be built and its future
# full path is known.
JITTER="$(echo $JITTER_CONFIG | $SED 's/-config$//')$EXEEXT"
AC_SUBST([JITTER], [$JITTER])
AC_MSG_NOTICE([jitter will be built at $JITTER])

# The C generator will have the same version as the jitter-config script.
JITTER_VERSION="$JITTER_CONFIG_VERSION"
AC_SUBST([JITTER_VERSION], [$JITTER_VERSION])

]) # JITTER_JITTER_SUBPACKAGE


# Jitter utility macros, possibly of interest to the user.
################################################################

# The macros in this section were prepared for internal use, but are
# sufficiently general to be of interest to external users as well.

# Check for Flex.  Set the output variable LEX and the cache variable
# ac_cv_prog_LEX to the program name, if found.
AC_DEFUN([JITTER_PROG_FLEX], [
# First make sure we have checked the Autoconf version, and defined feature m4
# macros.
AC_REQUIRE([JITTER_CHECK_AUTOCONF_VERSION])

# Older Autoconf versions define AC_PROG_LEX with no arguments; newer Autoconf
# versions strongly prefer an argument, and warn if the macro is called without
# any.
# Require either one macro or the other, according to the Autoconf version.
m4_if(jitter_ac_prog_lex_with_arguments, [],
      [AC_REQUIRE([JITTER_PROG_FLEX_OLD_STYLE])],
      [AC_REQUIRE([JITTER_PROG_FLEX_NEW_STYLE])])
]) # JITTER_PROG_FLEX

# Check for Bison.  Set the output variable YACC and the cache variable
# ac_cv_prog_YACC to the program name, if found.
AC_DEFUN([JITTER_PROG_BISON], [
AC_PROG_YACC
if echo "$YACC" | grep 'bison' > /dev/null; then
  AC_MSG_NOTICE([GNU Bison is installed as "$YACC"])
else
  # The program which was found was not Bison: undefine it.
  AC_MSG_WARN([found "$YACC" when looking for GNU Bison: that is not adequate])
  YACC=''
  ac_cv_prog_YACC=''
fi
]) # JITTER_PROG_BISON
