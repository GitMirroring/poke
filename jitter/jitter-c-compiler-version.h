/* Jitter: C compiler version check.

   Copyright (C) 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of GNU Jitter.

   GNU Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   GNU Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTER_C_COMPILER_VERSION_H_
#define JITTER_C_COMPILER_VERSION_H_

#include <jitter/jitter-cpp.h>


/* Introduction.
 * ************************************************************************** */

/* Jitter's advanced dispatches rely critically on compiler features, and in
   particular on using GCC rather than some other compiler.  If advanced
   dispatches are enabled at all then we want to verify that the compiler being
   used for compiling generated code in the same Jitter was configured for.

   We can be afford to be more lax with simple dispatches. */




/* Conditionalise on dispatch.
 * ************************************************************************** */

/* Sanity check: the checks perfomed here rely on other Jitter headers being
   included already. */
#if ! defined (JITTER_SIZEOF_VOID_P)
# error "You have included jitter/jitter-c-compiler-version.h without first"
# error "including jitter/jitter-config.h"
#endif
#if ! defined (JITTER_INTERNAL) && ! defined (JITTER_DISPATCH_NAME_STRING)
# error "You have included jitter/jitter-c-compiler-version.h yourself, instead"
# error "of including jitter/jitter.h"
#endif


/* Expand to nothing and perform no checks if the dispatch being used is simple,
   or if the current compilation is part of a dispatch-independent library,
   internal to Jitter.
   In the case of direct-threading, which still requires GNU C features, there
   will be some compiler error if the compiler being used is different from the
   one Jitter was configured for and the one being used does not support GNU C.
   This situation is tolerable, since the user will see the problem anyhow.  It
   is not feasible to recognise that the (unknown) compiler is the same, in
   general. */
#if ! defined (JITTER_INTERNAL)                      \
    && ! defined (JITTER_DISPATCH_SWITCH)            \
    && ! defined (JITTER_DISPATCH_DIRECT_THREADING)




/* Compiler identity and version check.
 * ************************************************************************** */

/* Sanity check: if we arrived here then a complex dispatch is being used, and
   therefore GCC must have been detected at configure time. */
#if ! defined (JITTER_HAVE_ACTUAL_GCC)
# error "An advanced dispatch is being used, but GCC was not detected at"
# error "configure time.  This should never happen."
#endif

/* Check the same macros which were checked at configure time to determine if
   GCC is being used, and which version it is.  These macros are defined
   automatically. */
#define JITTER_CURRENT_GCC_MAJOR_VERSION       __GNUC__
#define JITTER_CURRENT_GCC_MINOR_VERSION       __GNUC_MINOR__
#define JITTER_CURRENT_GCC_PATCHLEVEL_VERSION  __GNUC_PATCHLEVEL__
#define JITTER_CURRENT_GCC_COMBINED_VERSION  \
  (JITTER_CURRENT_GCC_MAJOR_VERSION * 10000  \
   + JITTER_CURRENT_GCC_MINOR_VERSION * 100  \
   + JITTER_CURRENT_GCC_PATCHLEVEL_VERSION)

/* Check if GCC is being used now, and if the same version was used when
   configuring Jitter.  In order to generate a clean error message every check
   must be in a different CPP conditional case. */
#if defined (__clang_major__)
  /* clang lies pretending to be GCC 4, as per its definition of __GNUC__.  We
     cannot trust the value of __GNUC__ in this case. */
  JITTER_PRAGMA                                                               \
     (message ("\n"                                                           \
               "Jitter was configured with GCC but now clang is being used "  \
               "to compile VM code.  This will not work with advanced "       \
               "dispatches such as "                                          \
               JITTER_DISPATCH_NAME_STRING                                    \
               ", being used now.\n"                                          \
               "We recommend using GCC."))
# error "Jitter configured with GCC, but clang being used for VM code"
/* FIXME: there are probably other compilers which, like clang, lie pretending
   to be GCC.  I could check for them as well. */
#elif ! defined (__GNUC__)
  /* This compiler does not lie, but is not GCC. */
  JITTER_PRAGMA                                                                \
     (message ("\n"                                                            \
               "Jitter was configured with GCC but now a different compiler "  \
               "is being used to compile VM code.  This will not work with "   \
               "advanced dispatches such as "                                  \
               JITTER_DISPATCH_NAME_STRING                                     \
               ", being used now.\n"                                           \
               "We recommend using GCC."))
# error "Jitter configured with GCC, but another compiler used for VM code"
/* If we arrived here then GCC is being used.  But it is not necessarily the
   same version used for configuring Jitter. */
#elif JITTER_GCC_COMBINED_VERSION != JITTER_CURRENT_GCC_COMBINED_VERSION
  JITTER_PRAGMA                                                              \
     (message ("\n"                                                          \
               "Jitter was configured with GCC "                             \
               JITTER_STRINGIFY (JITTER_GCC_MAJOR_VERSION)                   \
               "."                                                           \
               JITTER_STRINGIFY (JITTER_GCC_MINOR_VERSION)                   \
               "."                                                           \
               JITTER_STRINGIFY (JITTER_GCC_PATCHLEVEL_VERSION)              \
               " but now GCC "                                               \
               JITTER_STRINGIFY (JITTER_CURRENT_GCC_MAJOR_VERSION)           \
               "."                                                           \
               JITTER_STRINGIFY (JITTER_CURRENT_GCC_MINOR_VERSION)           \
               "."                                                           \
               JITTER_STRINGIFY (JITTER_CURRENT_GCC_PATCHLEVEL_VERSION)      \
               " is being used.  With advanced dispatches such as "          \
               JITTER_DISPATCH_NAME_STRING                                   \
               ", being used now, there may be subtle incompatibilities.\n"  \
               "We recommend rebuilding Jitter for this GCC version.\n"))
# error "Jitter configured for a different GCC version"
#else
  /* Good.  The same GCC version used for configuring Jitter is being used
     now. */
#endif




/* Conditionalise on dispatch: end.
 * ************************************************************************** */

#endif /* simple dispatch conditional. */

#endif // #ifndef JITTER_C_COMPILER_VERSION_H_
