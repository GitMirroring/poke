/* Jitter: general-purpose CPP macros for stringification and concatenation.

   Copyright (C) 2017, 2021 Luca Saiu
   Updated in 2019 by Luca Saiu
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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#ifndef JITTER_CPP_H_
#define JITTER_CPP_H_

/* This entire header has no dependencies and expands to nothing, by design.  It
   is safe to include from assembly sources. */

/* Remark on multiple-level expansion.
 * ************************************************************************** */

/* Yes, these two-levels of macros are needed; see the (excellent) GNU CPP
   manual.  The resulting clumsiness is the real reason why the macros in this
   header are useful. */




/* Convenient stringification.
 * ************************************************************************** */

/* Expand to the stringification of the macro parameter, itself unexpanded. 
   This is not intended for the user. */
#define JITTER_STRINGIFY_UNEXPANDED(whatever)  \
  # whatever

/* Expand to the stringification of the macro parameter, after it is expanded
   itself. */
#define JITTER_STRINGIFY(whatever)       \
  JITTER_STRINGIFY_UNEXPANDED(whatever)




/* Convenient token concatenation.
 * ************************************************************************** */

/* Expand to the token concatentation of the given macro parameters, unexpanded.
   These are not intended for the user. */
#define JITTER_CONCATENATE_TWO_UNEXPANDED(a, b)  \
  a ## b
#define JITTER_CONCATENATE_THREE_UNEXPANDED(a, b, c)  \
  a ## b ## c
#define JITTER_CONCATENATE_FOUR_UNEXPANDED(a, b, c, d)  \
  a ## b ## c ## d
#define JITTER_CONCATENATE_FIVE_UNEXPANDED(a, b, c, d, e)  \
  a ## b ## c ## d ## e

/* Expand to the token concatentation of the given macro parameters, after they
   are expanded themselves. */
#define JITTER_CONCATENATE_TWO(a, b)  \
  JITTER_CONCATENATE_TWO_UNEXPANDED(a, b)
#define JITTER_CONCATENATE_THREE(a, b, c)  \
  JITTER_CONCATENATE_THREE_UNEXPANDED(a, b, c)
#define JITTER_CONCATENATE_FOUR(a, b, c, d)  \
  JITTER_CONCATENATE_FOUR_UNEXPANDED(a, b, c, d)
#define JITTER_CONCATENATE_FIVE(a, b, c, d, e)  \
  JITTER_CONCATENATE_FIVE_UNEXPANDED(a, b, c, d, e)




/* Local identifier poisoning.
 * ************************************************************************** */

/* This feature serves to provide helpful error messages in case a code block
   contains a use of a locally forbidden identifier.  The identifier is poisoned
   at the beginning of the block and unpoisoned at the end.
   It is possible to poison functions, non-function globals, macros, and even C
   keywords.

   The implementation relies on non-standard GNU C features.  It is used
   conditionally in generated code.  Where the feature is not supported the
   system will fail in a less friendly way in case of incorrect code, but
   correctness will not be compromised. */

/* This is how to poison an indentifier foo, for example with the error message
   about a missing "threading" attribute:
#    pragma push_macro ("foo")
#    undef foo
#    define foo JITTER_POISONED_WITHOUT_ATTRIBUTE (foo, "threading")

   And this is how to unpoison it:
#    pragma pop_macro ("foo")

   In order to avoid spurious warning messages at the beginning of a sequence
   of poisonings, this should be generated (once):
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpragmas"
#    pragma GCC diagnostic ignored "-Wunknown-warning-option"
#    pragma GCC diagnostic ignored "-Wbuiltin-macro-redefined"
#    pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
   At the end, after unpoisoning, this restores the previous diagnostics state:
#    pragma GCC diagnostic pop  */

/* The canonic use of _Pragma with a stringized argument. */
#define JITTER_PRAGMA(x)  \
  _Pragma (#x)

/* Expand to a sequence of two _Pragma uses, the first one printing a nice error
   message involving the given identifier and the given message (provided by the
   caller as a string), the second one causing the compiler to fail. */
#define JITTER_PRAGMA_WARN_WITH_REASON(identifier, reason_string_literal)      \
  /* This is printed in a nicer way than #pragma GCC error. */                 \
  JITTER_PRAGMA (message ("\nYou cannot use \"" # identifier "\" in "          \
                          JITTER_SPECIALIZED_INSTRUCTION_NAME_AS_STRING " \n"  \
                          "(a specialisation of "                              \
                          JITTER_INSTRUCTION_NAME_AS_STRING ")\n"              \
                          reason_string_literal ". \n"                         \
                          "Any type error you may see after this message \n"   \
                          "is probably an effect of this macro which \n"       \
                          "serves to prevent the use of \"" #identifier        \
                          "\"." ));                                            \
  /* And now fail, as well.  This literal string is printed nicely. */         \
  JITTER_PRAGMA (GCC error "Refusing to compile.");

/* Expand to a GNU C statement-expresison evaluating to zero (which is
   compatible with most types in C) and a use of JITTER_PRAGMA_WARN_WITH_REASON.
   The fact that the identifier expands to an expression makes it likely that
   the resulting expression remains well-formed syntactically, which avoids
   distracting syntax errors in many cases. */
#define JITTER_POISONED_EXPRESSION(identifier, reason_string_literal)    \
  ({ JITTER_PRAGMA_WARN_WITH_REASON(identifier, reason_string_literal);  \
     0; })

/* Expand to a use of JITTER_PRAGMA_WARN_WITH_REASON with the appropriate error
   message. */
#define JITTER_POISONED_EVERYWHERE(identifier)                                \
  JITTER_POISONED_EXPRESSION (identifier,                                 \
                              "-- this is forbidden everywhere in user "  \
                              "code")

/* This factors the common code in JITTER_POISONED_WITH_ATTRIBUTE and
   JITTER_POISONED_WITHOUT_ATTRIBUTE . */
#define JITTER_POISONED_WITH_OR_WITHOUT_ATTRIBUTE(identifier, participle,     \
                                                  attribute)                  \
  JITTER_POISONED_EXPRESSION (identifier,                                     \
                              participle " the attribute \"" attribute "\"")

#define JITTER_POISONED_WITH_ATTRIBUTE(identifier, attribute)       \
  JITTER_POISONED_WITH_OR_WITHOUT_ATTRIBUTE (identifier, "which has",  \
                                             attribute)
#define JITTER_POISONED_WITHOUT_ATTRIBUTE(identifier, attribute)        \
  JITTER_POISONED_WITH_OR_WITHOUT_ATTRIBUTE (identifier, "which lacks",  \
                                             attribute)

#endif // #ifndef JITTER_CPP_H_
