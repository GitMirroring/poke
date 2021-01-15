/* Jitter: fast-branch header.

   Copyright (C) 2017, 2019, 2020 Luca Saiu
   Written by Luca Saiu

   This file is part of Jitter.

   Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTER_FAST_BRANCH_H_
#define JITTER_FAST_BRANCH_H_

#include <jitter/jitter-arithmetic.h>


/* Check whether patch-ins are needed, and supported on this machine.
 * ************************************************************************** */

/* This among the rest includes <jitter/machine/jitter-machine.h> , which is
   enough to see the definition of JITTER_MACHINE_SUPPORTS_PROCEDURE used below,
   and all the machine-specific branch definitions, which are optional; in case
   patch-ins are used what is missing will be defined here. */
#include <jitter/jitter-patch-in.h>




/* Low-level versus high-level fast-branches.
 * ************************************************************************** */

/* The low-level machine-specific definition for conditional fast branches, when
   available, "assume the worst case" in terms of constantness.  This means that
   low-level macros expand to inline assembly code actually performing a
   conditional, even when there is no need to because the condition is actually
   a compile-time constant.

   Low-level macros can assume they receive trivial arguments with no side
   effects, always either constants or variables, even if not necessarily known
   at compile time.  Argument evaluation happens *out* of low-level macros.

   Low-level macros are conceived for easy portability and make machine-specific
   code more declarative than algorithmic, but are not intended for the user.
   The user should always use high-level fast branches, defined in a
   machine-generated header as explained below (using machine-specific low-level
   fast-branches when available), which evaluate every comparison argument
   exactly once, rely on statements containing conditionals based on complex
   constant expression in order to eventually generate optimized code containing
   only a translation of the appropriate expansion of low-level macros.

   It is not necessary to protect low-level fast-branch operators with do..while
   (false).  The high-level definitions already wrap their uses.  Like elsewhere
   in Jitter, a macro whose name ends with "_" is unsafe in this respect, but
   is only used internally. */

/* This one low-level fast branch has a machine-independent definition, only
   relying on JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL and
   JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL , which of course are to
   be supplied in the machine-specific header.
   Low-level fast-branch unconditionally, going to tgt. */
#ifdef JITTER_HAVE_PATCH_IN
# define _JITTER_LOW_LEVEL_BRANCH_FAST_(tgt)             \
    JITTER_PATCH_IN_PLACEHOLDER_GOTO_                    \
       (JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL,  \
        JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL,  \
        tgt,                                             \
        0, 0, 0); /* Unused for this case. */            \
    __builtin_unreachable ()
#else /* not using fast branches */
# define _JITTER_LOW_LEVEL_BRANCH_FAST_(tgt)                      \
    /* If fast branches are not used this expands to an ordinary  \
       branch.  The arguments of fast and non-fast branches are   \
       not at all compatible, but the user will not see the       \
       difference. */                                             \
    JITTER_BRANCH(tgt)
#endif // #ifdef JITTER_HAVE_PATCH_IN

/* This is a good default solution for overflow-checking on sum and subtraction,
   which appears to yield optimal code on machines without special support for
   overflow checks such as RISC-V and pre-r6 MIPS, provided that the
   fast-branch-on-negative operation is efficient.  Instead of using the builtin
   plus inline asm (which would need to generate a Boolean and then test that
   same Boolean in another conditional) I can generate better code by computing
   the condition as a signed word in simple C and then using a low-level
   fast-branch primitive, presumably written in assembly, only for
   fast-branching on its sign. */
#if (! defined (_JITTER_LOW_LEVEL_BRANCH_FAST_IF_PLUS_OVERFLOWS_)     \
     && ! defined (_JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_))
# define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_PLUS_OVERFLOWS_(opd0, opd1, tgt)  \
    _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_                              \
       (JITTER_WOULD_PLUS_OVERFLOW_SIGNED_WORD (jitter_uint,                \
                                                jitter_int,                 \
                                                (jitter_int) (opd0),        \
                                                (jitter_int) (opd1),        \
                                                JITTER_BITS_PER_WORD),      \
        (tgt))
#endif // no machine-specific support for sum overflow
#if (! defined (_JITTER_LOW_LEVEL_BRANCH_FAST_IF_MINUS_OVERFLOWS_)     \
     && ! defined (_JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_))
# define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_MINUS_OVERFLOWS_(opd0, opd1, tgt)  \
    _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_                               \
       (JITTER_WOULD_MINUS_OVERFLOW_SIGNED_WORD (jitter_uint,                \
                                                 jitter_int,                 \
                                                 (jitter_int) (opd0),        \
                                                 (jitter_int) (opd1),        \
                                                 JITTER_BITS_PER_WORD),      \
        (tgt))
#endif // no machine-specific support for subtraction overflow

/* No equally good solution for multiplication overflow is obvious.  By default
   the machine-generated header will use GCC's builtin when available, or a C
   macro otherwise. */




/* Middle-level operate-and-branch-on-overflow.
 * ************************************************************************** */

/* The following definitions rewrite low-level operate-and-branch-on-overflow
   branches into other low-level operate-and-branch-on-overflow which are
   cheaper to compute, where possible.  These constitute a thin abstraction over
   low-level operate-and-branch-on-overflow, and are used internally in the
   machine-generated header descrbied below in order to implement high-level
   branches, of both the operate-and-branch-on-overflow and branch-on-overflow
   kinds.  Middle-level primitives do not optimize away overflow checks: that is
   done by high-level primitives, which will not resort to middle-level
   primitives (as there is be no need to: GCC will be able to optimize the
   operations written in C, in those cases) when the overflow checks yield
   results known at compile time, either always or never branching.
   Middle-level primitives are used in the "worst" case by high-level
   primitives, where the overflow condition is not known, but the operation
   might still be optimizable; the overflow check will remain in the rewrite.
   Of course nothing of this is intended for the user, who should only ever see
   high-level operations. */

/* These macros may all evaluate their arguments more than once.  They do not
   even protect their arguments with parentheses in the expansion: they are
   meant to be only used in machine-generated code, with variables or constants
   as arguments. */
#define _JITTER_MIDDLE_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_(res, a, b, tgt)  \
  /* Nothing to do here. */                                                 \
  _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_ (res, a, b, tgt)
#define _JITTER_MIDDLE_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_(res, a, b, tgt)  \
  /* Nothing to do here. */                                                  \
  _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_ (res, a, b, tgt)
#define _JITTER_MIDDLE_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_(res, a, b, tgt)  \
  if (JITTER_KNOWN_TO_BE (a, -1))                                            \
    {                                                                        \
      /* -1 * b  ==>  0 - b */                                               \
      _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_ (res, 0, b, tgt);     \
    }                                                                        \
  else if (JITTER_KNOWN_TO_BE (b, -1))                                       \
    {                                                                        \
      /* a * -1  ==>  0 - a */                                               \
      _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_ (res, 0, a, tgt);     \
    }                                                                        \
  else if (JITTER_KNOWN_TO_BE (a, 2))                                        \
    {                                                                        \
      /* 2 * b  ==>  b + b */                                                \
      _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_ (res, b, b, tgt);      \
    }                                                                        \
  else if (JITTER_KNOWN_TO_BE (b, 2))                                        \
    {                                                                        \
      /* a * 2  ==>  a + a */                                                \
      _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_ (res, a, a, tgt);      \
    }                                                                        \
  else                                                                       \
    {                                                                        \
      /* Worst case: a * b is not rewritten. */                              \
      _JITTER_LOW_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_ (res, a, b, tgt);     \
    }
#define _JITTER_MIDDLE_LEVEL_DIVIDED_BRANCH_FAST_IF_OVERFLOW_(res, a, b, tgt)  \
  if (JITTER_KNOWN_TO_BE (b, -1))                                              \
    {                                                                          \
      /* a / -1  ==>  0 - a */                                                 \
      _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_ (res, 0, a, tgt);       \
    }                                                                          \
  /* No need to treat the a / 2 case here: it cannot overflow, and the         \
     high-level primitive can handle that. */                                  \
  else if (JITTER_KNOWN_TO_BE_EQUAL (a, b))                                    \
    {                                                                          \
      /* a / a  ==>  if a == 0 then overflow else 1 */                         \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_ (a, tgt);                         \
      (res) = 1;                                                               \
    }                                                                          \
  else if (JITTER_KNOWN_TO_BE (a, 1))                                          \
    {                                                                          \
      /* 1 / b  ==>  case b of 0: overflow, 1: 1, -1: -1 else 0 */             \
      /* The benefit of this case may be somewhat questionable.  This code     \
         yields a couple of conditionals or conditional moves, but course no   \
         divisions.  Without this case I get possibly a little fewer           \
         instructions, still with conditionals, but including one division.    \
         Notice that b is not known and therefore the tests against b will     \
         survive optimization into run time, or otherwise we would not be here \
         in the middle-level primitive. */                                     \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_ (b, tgt);                         \
      if (b == 1)                                                              \
        (res) = 1;                                                             \
      else if (b == -1)                                                        \
        (res) = -1;                                                            \
      else                                                                     \
        (res) = 0;                                                             \
    }                                                                          \
  else if (JITTER_KNOWN_TO_BE (a, -1))                                         \
    {                                                                          \
      /* -1 / b  ==>  case b of 0: overflow, -1: 1, 1: -1 else 0 */            \
      /* The comment about efficiency in the previous case applies here as     \
         well. */                                                              \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_ (b, tgt);                         \
      if (b == -1)                                                             \
        (res) = 1;                                                             \
      else if (b == 1)                                                         \
        (res) = -1;                                                            \
      else                                                                     \
        (res) = 0;                                                             \
    }                                                                          \
  else                                                                         \
    {                                                                          \
      /* Worst case: a / b is not rewritten. */                                \
      _JITTER_LOW_LEVEL_DIVIDED_BRANCH_FAST_IF_OVERFLOW_ (res, a, b, tgt);     \
    }
#define _JITTER_MIDDLE_LEVEL_REMAINDER_BRANCH_FAST_IF_OVERFLOW_(res, a, b, tgt) \
  /* No rewrite here. */                                                        \
  _JITTER_LOW_LEVEL_REMAINDER_BRANCH_FAST_IF_OVERFLOW_ (res, a, b, tgt)
  



/* High-level negation overflow primitives.
 * ************************************************************************** */

/* The integer negation operation is only a user convenience hiding a first zero
   operand in a subtraction.  In this case it is enough to define high-level
   primitives, based on the subtraction which have been defined already. */
#define _JITTER_NEGATE_BRANCH_FAST_IF_OVERFLOW(res, opd0, tgt)  \
  _JITTER_MINUS_BRANCH_FAST_IF_OVERFLOW((res), 0, (opd0), tgt)
#define _JITTER_BRANCH_FAST_IF_NEGATE_OVERFLOWS(opd0, tgt)  \
  _JITTER_BRANCH_FAST_IF_MINUS_OVERFLOWS(0, (opd0), tgt)




/* High-level unconditional fast branch.
 * ************************************************************************** */

/* High-level unconditional branch.  This definition is machine-independent,
   even if of course it relies on a low-level machine-specific primitive (itself
   having a fallback definition in case it is missing). */
#define _JITTER_BRANCH_FAST(tgt)             \
  do                                         \
    {                                        \
      _JITTER_LOW_LEVEL_BRANCH_FAST_ (tgt);  \
    }                                        \
  while (false)




/* The machine-generated header for fast branches.
 * ************************************************************************** */

/* In order to make porting easier a machine specification does not need to
   include a definition for every possible low-level fast branch; moreover, in
   the case of overflow checking, a machine specification is free to either
   define operate-and-branch-on-overflow or branch-on-overflow primitives; the
   missing kind will be defined based on the other.

   This machine-generated header completes the missing definitions for low-level
   fast branches and defines high-level fast branches. */
# include <jitter/jitter-fast-branch-machine-generated.h>




/* Fast-branch-and-link definition.
 * ************************************************************************** */

/* Provide a definition of _JITTER_BRANCH_FAST_AND_LINK , based on what is
   available in the current configuration.
   The definition will choose the best among the following alternatives:
   - the slow JITTER_BRANCH_AND_LINK_FALLBACK if fast labels are not used in
     this configuration;
   - a generic branch-and-link implementation doing the actual branch as a fast
     branch;
   - a native machine-specific version, only available with no-threading
     dispatch.

   The user should never call _JITTER_BRANCH_FAST_AND_LINK nor the macros used
   in its definition .  Suitable user macros, without the initial underscore
   prefix and without the _INTERNAL suffix, are automatically defined to be only
   visible in caller instructions .  This forces the user to correctly declare
   callers so that return labels can be handled in every case. */

/* Provide a definition of _JITTER_BRANCH_FAST_AND_LINK . */
#if ! defined(JITTER_DISPATCH_NO_THREADING)
  /* With any dispatching model different from no-threading fast branches revert
     to generic slow branches; in the same way fast branch-and-link operations
     revert to generic slow branch-and-link operations. */
# define _JITTER_BRANCH_FAST_AND_LINK(target) \
  _JITTER_BRANCH_AND_LINK_FALLBACK (target)
#elif ! defined(JITTER_HAVE_PATCH_IN)
# if defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)
    /* Currently machine-specific support must also provide patch-ins if it
       provides procedures. */
#   error "the machine supports procedures but not patch-ins"
# else
    /* The machine supports neither patch-ins, nor procedures.  We can use the
       generic implementation of branch-and-link with slow branches. */
#   define _JITTER_BRANCH_FAST_AND_LINK(target) \
      _JITTER_BRANCH_AND_LINK_FALLBACK (target)
# endif // if defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)
#elif ! defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)
  /* The machine supports patch-ins, but not procedures.  The best we can do is
     using a generic branch-and-link implementation (relying on a hidden
     residual parameter for caller instructions) which performs the actual
     branch with a fast branch. */
# define _JITTER_BRANCH_FAST_AND_LINK(target)                                \
    do                                                                       \
      {                                                                      \
        /* Use the return address from the implicit specialized argument */  \
        jitter_state_runtime._jitter_link.label                              \
          = _JITTER_RETURN_ADDRESS;                                          \
        _JITTER_BRANCH_FAST (target);                                        \
        /* FIXME: __builtin_unreachable should not be needed, if             \
           _JITTER_BRANCH_FAST is defined correctly. */                      \
        /* __builtin_unreachable (); */                                      \
      }                                                                      \
    while (false)
#elif ! defined(_JITTER_BRANCH_FAST_AND_LINK_NATIVE)
# error "The machine claims to support procedures but _JITTER_BRANCH_FAST_AND_LINK_NATIVE is not defined."
#else
  /* The efficient case: no-threading dispatch, the machine supports patch-ins
     and procedures. */
# define _JITTER_BRANCH_FAST_AND_LINK(target)   \
  _JITTER_BRANCH_FAST_AND_LINK_NATIVE (target)
#endif




/* Ignore the rest of this header if not using patch-ins.
 * ************************************************************************** */

/* The rest of this header expands to nothing if fast branches are not supported
   in this configuration.  The previous CPP includes suffices to make the
   JITTER_HAVE_PATCH_IN visible, if any. */

#ifdef JITTER_HAVE_PATCH_IN




/* Include headers.
 * ************************************************************************** */

#include <stdio.h>
#include <stdbool.h>

#include <jitter/jitter.h>
#include <jitter/jitter-cpp.h>




/* Fast branch types.
 * ************************************************************************** */

/* The type of a fast branch as a patch-in case, each corresponding to different
   assembly non-zero natural code to insert in assembly code.

   These are CPP macros rather than enum cases because the case values are used
   in inline assembly code (actually data), where a known value is used.  An
   assembly "immediate" constraint is not usable, as in some platforms immediate
   literals are always prefixed by a character such as $ or #, which is not
   needed or correct in data definitions.

   This complication is a pity: GCC documents the "x86 Operand Modifiers" as
   non-portable; having the equivalent of the 'c' modifier on every platform
   would allow for cleaner code.

   It is not necessary to use every case on any given machine, or to use only
   these.  Still it is convenient to have a centralized definition to reuse. */
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL                    1
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ZERO                 2
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NONZERO              3

#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_POSITIVE             4
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NONPOSITIVE          5
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NEGATIVE             6
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NONNEGATIVE          7

#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_EQUAL                8
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NOTEQUAL             9
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_LESS_UNSIGNED        10
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_LESS_SIGNED          11
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NOTGREATER_UNSIGNED  12
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NOTGREATER_SIGNED    13
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_GREATER_UNSIGNED     14
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_GREATER_SIGNED       15
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NOTLESS_UNSIGNED     16
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NOTLESS_SIGNED       17
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_AND                  18
#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_NOTAND               19

#define JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY                  20

#define JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK                  21
/* These should be extended in the future with floating-point conditionals. */

/* The other values up to 999 included are reserved.  Fast branch cases starting
   at 1000 are available for architecture-specific code to define. */

#endif // #ifdef JITTER_HAVE_PATCH_IN
#endif // #ifndef JITTER_FAST_BRANCH_H_
