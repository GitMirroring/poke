/* JitterLisp: reader.

   Copyright (C) 2017, 2018 Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with GNU Jitter under the same license.

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


#include "jitterlisp-reader.h"

#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-parse-int.h>
#include <jitter/jitter-readline.h>
#include <jitter/jitter-string.h>

#include "jitterlisp-error.h"
#include "jitterlisp-sexpression.h"
#include "jitterlisp-allocator.h"
#include "jitterlisp-printer.h"




/* Char-reading.
 * ************************************************************************** */

int
jitterlisp_string_char_reader_function
   (jitterlisp_char_reader_state *const_char_star_star)
{
  const char **pointer_to_string_pointer = (const char **) const_char_star_star;
  char res;

  /* If we're at the end already return EOF and don't increment the pointer, out
     of defensiveness (if every function is used correctly, no reading should be
     performed after that point anyhow); otherwise read the current character
     and only after that increment the pointer. */
  if ((res = ** pointer_to_string_pointer) == '\0')
    return EOF;
  else
    {
      (* pointer_to_string_pointer) ++;
      return res;
    }
}

int
jitterlisp_stream_char_reader_function
   (jitterlisp_char_reader_state *file_star_star)
{
  FILE *f = * (FILE **) file_star_star;
  return fgetc (f);
}





/* Scanner state.
 * ************************************************************************** */

/* The internal scanner state.  The same structure instance is used for parsing
   an entire s-expression, and in particular survives scanner calls. */
struct jitterlisp_scanner_state
{
  /* The current lookahead character, or EOF if we are at the end of the
     input. */
  int lookahead;

  /* Input row and column number, mostly useful for error messages. */
  int row_no, column_no;

  /* A dynamic buffer containing the current token text, not including the
     lookahead character.  Notice that this is not automatically
     '\0'-terminated: the function jitterlisp_scanner_token_text is provided
     to do that in a convenient way. */
  struct jitter_dynamic_buffer token_text;

  /* The char-reader function. */
  jitterlisp_char_reader_function char_reader_function;

  /* The char-reader state. */
  jitterlisp_char_reader_state char_reader_state;

  /* The char-reader finalizing function, freeing resources for the char-reader
     state. */
  jitterlisp_char_reader_finalizer char_reader_finalizer;
};

/* Read the next character of the input in the pointed sstate, setting the
   lookahead field (to EOF if there are no more input characters).  This
   overwrites the previous lookahead, and doesn't append it to the current token
   text. */
static void
jitterlisp_scanner_advance (struct jitterlisp_scanner_state *sstate)
{
  sstate->lookahead
    = sstate->char_reader_function (& sstate->char_reader_state);

  /* Advance row and column indices. */
  if (sstate->lookahead == '\n')
    {
      sstate->row_no ++;
      sstate->column_no = 0;
    }
  else
    sstate->column_no ++;
}

/* Add the lookahead character to the current token text. */
void
jitterlisp_scanner_add_lookahead (struct jitterlisp_scanner_state *sstate)
{
  /* Convert the lookahead character into a char: this way we can point to
     it without relying on a specific endianness. */
  char lookahead_as_char = sstate->lookahead;

  /* Append the one character we converted to the token text. */
  jitter_dynamic_buffer_push (& sstate->token_text, & lookahead_as_char, 1);
}

/* Add a '\0' terminator to the current token text and return the beginning of
   the token text as a string.  The result pointer will only remain valid until
   the scanner state is modified. */
const char *
jitterlisp_scanner_token_text (struct jitterlisp_scanner_state *sstate)
{
  /* Append the terminator character. */
  char terminator = '\0';
  jitter_dynamic_buffer_push (& sstate->token_text, & terminator, 1);

  /* Return a pointer to the beginning of the string, without copying it. */
  return sstate->token_text.region;
}

/* Reset the token text to an empty (non-'\0'-terminated) string. */
static void
jitterlisp_scanner_clear_token_text (struct jitterlisp_scanner_state *sstate)
{
  /* Remove all the characters we pushed before making the used size zero. */
  jitter_dynamic_buffer_pop (& sstate->token_text,
                             sstate->token_text.used_size);
}

/* Initialize the pointed scanner state using the given char-reader.  Notice
   that the lookahead character is set immediately (to EOF if the input is
   empty), so that a scanner doesn't ever need to deal with uninitialized
   data.  The consequence of this is this function will read the first
   token before returning, which potentially makes it a blocking operation. */
static void
jitterlisp_initialize_scanner_state (struct jitterlisp_scanner_state *sstate,
                                     jitterlisp_char_reader_function crfu,
                                     jitterlisp_char_reader_state crs,
                                     jitterlisp_char_reader_finalizer crfi)
{
  /* Initialize the dynamic buffer.  It will contains zero characters at the
     beginning. */
  jitter_dynamic_buffer_initialize (& sstate->token_text);

  /* Initialize the char reader. */
  sstate->char_reader_function = crfu;
  sstate->char_reader_state = crs;
  sstate->char_reader_finalizer = crfi;

  /* Initialize input location.  Here I'm following the Emacs convention, with
     1-based row indices and 0-based column indices. */
  sstate->row_no = 1;
  sstate->column_no = 0;

  /* Read the first character (or EOF). */
  jitterlisp_scanner_advance (sstate);
}

/* Free the scanner state resources.  This does not deal with char-reader
   finalization, which is the caller's resposibility. */
static void
jitterlisp_finalize_scanner_state (struct jitterlisp_scanner_state *sstate)
{
  if (sstate->char_reader_finalizer != NULL)
    sstate->char_reader_finalizer (& sstate->char_reader_state);
  jitter_dynamic_buffer_finalize (& sstate->token_text);
}




/* Character names.
 * ************************************************************************** */

/* Return non-false iff the given text, including the #\ prefix, represents a
   valid character. */
static bool
jitterlisp_is_valid_character (const char *text)
{
  /* If there is no #\ prefix then this is not a character.  */
  if (strncmp (text, "#\\", 2))
    return false;

  /* If the character name length including the #\ prefix is just three then
     the character is ordinary, and valid. */
  if (strlen (text) == 3)
    return true;

  /* Look for every non-ordinary character name.  If we find one that matches
     we have our response. */
  int i;
  for (i = 0; i < jitterlisp_non_ordinary_character_name_binding_no; i ++)
    if (! strcmp (jitterlisp_non_ordinary_character_name_bindings [i].name,
                  text + 2))
      return true;

  /* We didn't find a matching name.  If we arrived here the token looks like
     a character but has an invalid name. */
  return false;
}

/* Given a valid character name, including the #\ prefix, return its character.
   The character name may be ordinary or non-ordinary, but it must be valid. */
static jitter_int
jitterlisp_decode_valid_character (const char *text)
{
  /* If there is no #\ prefix then this is not a character.  */
  if (strncmp (text, "#\\", 2))
    jitter_fatal ("jitterlisp_decode_valid_character: invalid prefix");

  /* If the character name length including the #\ prefix is just three then
     the character is ordinary. */
  if (strlen (text) == 3)
    return text [2];

  /* Look for every non-ordinary character name.  If we find one that matches
     we have our response. */
  int i;
  for (i = 0; i < jitterlisp_non_ordinary_character_name_binding_no; i ++)
    if (! strcmp (jitterlisp_non_ordinary_character_name_bindings [i].name,
                  text + 2))
      return jitterlisp_non_ordinary_character_name_bindings [i].character;

  /* We didn't find a matching name.  If we arrived here the token looks like
     a character but has an invalid name. */
  jitter_fatal ("jitterlisp_decode_valid_character: invalid name");
}




/* S-expression scanner.
 * ************************************************************************** */

/* The scanner is completely invisible to the user, who doesn't need to see the
   distinction between scanning and parsing and will just call a Lisp-style
   "reader" to obtain an s-expression.  Nothing of this is declared in the
   header. */

/* The scanner internal automaton state.  This is, of course, distinct from the
   scanner state which is a more complex struct.  The DFA state is only used
   within the main scanner function. */
enum jitterlisp_scanner_dfa_state
  {
    /* No part of an actual token has been recognized yet. */
    jitterlisp_scanner_dfa_state_initial,

    /* The scanner is looking inside a comment, between the opening ';' and the
       closing '\n'. */
    jitterlisp_scanner_dfa_state_comment,

    /* The scanner has recognized ',', which may be either the full prefix ","
       or the beginning of the two-character prefix ",@".  We have to look at the
       next character to know. */
    jitterlisp_scanner_dfa_state_comma,

    /* The scanner is looking inside a more complex token: it may be a symbol, a
       number, a prefix or some keyword such as "#t".  It would be a little
       laborious to handle the different kind of tokens manually in a DFA (we
       would need states such as jitterlisp_scanner_dfa_state_after_sign ,
       jitterlisp_scanner_dfa_state_after_hash , and so on), so we use a the
       function jitterlisp_complicated_text_to_token to differentiate among
       different kinds of complicated tokens. */
    jitterlisp_scanner_dfa_state_complicated
  };

/* A token identifier as recognized by the scanner. */
enum jitterlisp_token
  {
    /* This case is used as an intentionally invalid value at initialization, to
       make sure that we advance instead of using a non-existent lookahead.  It
       is also used for ill-formed tokens to make parsing fail. */
    jitterlisp_token_invalid,

    jitterlisp_token_open,
    jitterlisp_token_close,
    jitterlisp_token_dot,
    jitterlisp_token_character,
    jitterlisp_token_fixnum,
    jitterlisp_token_symbol,
    jitterlisp_token_prefix,
    jitterlisp_token_true,
    jitterlisp_token_false,
    jitterlisp_token_eof /* End of the input. */
  };

/* Return the token corresponding to the pointed text.  The text must contain no
   whitespace around, and must match a single token.  The scanner isolates
   strings suitable for this function from the input. */
static enum jitterlisp_token
jitterlisp_complicated_text_to_token (const char *text)
{
  /* First check if the text is a key word. */
  if (! strcmp (text, "("))
    return jitterlisp_token_open;
  else if (! strcmp (text, ")"))
    return jitterlisp_token_close;
  else if (! strcmp (text, "."))
    return jitterlisp_token_dot;
  else if (! strcmp (text, "#f"))
    return jitterlisp_token_false;
  else if (! strcmp (text, "#t"))
    return jitterlisp_token_true;
  else if (! strcmp (text, "'"))
    return jitterlisp_token_prefix;
  else if (! strcmp (text, "`"))
    return jitterlisp_token_prefix;
  else if (! strcmp (text, ","))
    return jitterlisp_token_prefix;
  else if (! strcmp (text, ",@"))
    return jitterlisp_token_prefix;
  else if (! strcmp (text, "#"))
    return jitterlisp_token_prefix;

  /* The text is not a key word.  Check if it's a character. */
  if (! strncmp (text, "#\\", 2))
    {
      /* The text looks like a character.  It is either a valid character or an
         invalid token: we don't accept an ill-formed character as a symbol. */
      if (jitterlisp_is_valid_character (text))
        return jitterlisp_token_character;
      else
        return jitterlisp_token_invalid;
    }

  /* The text is not a key word or a character.  Check if it matches a number;
     if not we say it's a symbol. */
  jitter_long_long useless;
  if (jitter_string_to_long_long_inconvenient (text, & useless) == 0)
    return jitterlisp_token_fixnum; // FIXME: this ignores overflow and doesn't support floats or bignums.
  else
    return jitterlisp_token_symbol;
}

/* Recognize the next token using the given scanner state.  Return the token
   identifier, and '\0'-terminate the current token text to make it convenient
   to access for the parser. */
static enum jitterlisp_token
jitterlisp_scan (struct jitterlisp_scanner_state *sstate)
{
  /* Clear the previous token text. */
  jitterlisp_scanner_clear_token_text (sstate);

  /* We are in the initial state, ready to read characters. */
  enum jitterlisp_scanner_dfa_state s = jitterlisp_scanner_dfa_state_initial;

/* A clean way of exiting the scanning loop in advance when we recognize the end
   of the current token. */
#define JITTERLISP_END_OF_TOKEN            \
  do                                       \
    {                                      \
      goto jitterlisp_scanner_after_loop;  \
    }                                      \
  while (false)

  /* Read characters starting from the current lookahead.  We advance the
     lookahead at the end of the loop body and not here, since we don't want to
     miss the first character of the token text. */
  while (sstate->lookahead != EOF)
    {
      /* If we arrived here the lookahead is an actual character, not EOF. */
      char c = sstate->lookahead;

      /* Appropriately handle the current lookahead character according to the
         current state. */
      switch (s)
        {
        case jitterlisp_scanner_dfa_state_initial:
          switch (c)
            {
            case ' ': case '\t': case '\n': case '\r': case '\f':
              /* A whitespace character in the initial state: ignore it and keep
                 scanning. */
              break;
            case ';':
              /* Here starts a comment: change to comment state and keep
                 scanning. */
              s = jitterlisp_scanner_dfa_state_comment;
              break;
            case '(': case ')': case '\'': case '`':
              /* We recognized a single-character token.  End the token, but
                 advance so as not to see this same character the next time we
                 scan. */
              jitterlisp_scanner_add_lookahead (sstate);
              jitterlisp_scanner_advance (sstate);
              s = jitterlisp_scanner_dfa_state_complicated;
              JITTERLISP_END_OF_TOKEN;
            case ',':
              /* This can be a prefix on its own, or the beginning of a
                 two-character prefix.  Change state and keep scanning to check
                 which case it is.  We need this slight complication to let the
                 user write "," or ",@" immediately followed by another
                 s-expression, without spaces after the prefix. */
              jitterlisp_scanner_add_lookahead (sstate);
              s = jitterlisp_scanner_dfa_state_comma;
              break;
            default:
              /* We found the begininng of a complicated token.  Append the
                 lookahead to the token text, change state and keep scanning. */
              jitterlisp_scanner_add_lookahead (sstate);
              s = jitterlisp_scanner_dfa_state_complicated;
            } /* inner switch for jitterlisp_scanner_dfa_state_initial . */
          break;

        case jitterlisp_scanner_dfa_state_comment:
          switch (c)
            {
            case '\n':
              /* We reached the comment end: go back to the initial state to
                 scan for a token. */
              s = jitterlisp_scanner_dfa_state_initial;
              break;
            default:
              /* We're still inside the comment.  Keep scanning. */;
            } /* inner switch for jitterlisp_scanner_dfa_state_comment . */
          break;

        case jitterlisp_scanner_dfa_state_comma:
          switch (c)
            {
            case '@':
              /* We recognized the prefix ",@" .  Stop, but first advance so as
                 not to see '@' at the next scan. */
              jitterlisp_scanner_add_lookahead (sstate);
              jitterlisp_scanner_advance (sstate);
              s = jitterlisp_scanner_dfa_state_complicated;
              JITTERLISP_END_OF_TOKEN;
            default:
              /* We recognized the prefix ",".  Stop, and don't advance: the
                 current lookahead will be part of the next token. */
              s = jitterlisp_scanner_dfa_state_complicated;
              JITTERLISP_END_OF_TOKEN;
            } /* inner switch for jitterlisp_scanner_dfa_state_comma . */
          break;

        case jitterlisp_scanner_dfa_state_complicated:
          switch (c)
            {
            case ' ': case '\t': case '\n': case '\r': case '\f':
              /* We found whitespace while scanning a complicated token: the
                 token has ended. */
              JITTERLISP_END_OF_TOKEN;
            case ';':
              /* We found a comment beginning while scanning a complicated
                 token: the token has ended. */
              JITTERLISP_END_OF_TOKEN;
            case '(': case ')': case '#': case '\'': case '`': case ',':
              /* We found a reserved character while scanning a complicated
                 token: the token has ended. */
              JITTERLISP_END_OF_TOKEN;
            default:
              /* The current character belongs to the complicated token.  Append
                 it to the text and keep scanning, without changing state. */
              jitterlisp_scanner_add_lookahead (sstate);
            } /* inner switch for jitterlisp_scanner_dfa_state_comment . */
          break;

        default:
          jitter_fatal ("scanner: invalid state");
        } /* End of the scanner switch on s . */

      /* Read the next input character (or EOF). */
      jitterlisp_scanner_advance (sstate);
    } /* End of the scanner while loop. */

/* This will no longer be used below. */
#undef JITTERLISP_END_OF_TOKEN

  const char *token_text;
 jitterlisp_scanner_after_loop:
  /* If we arrived here we reached the end of a token, of the end of the
     input. */

  /* '\0'-terminate the current token text so that the parser can use it it, and
     keep a pointer to its beginning for our own convenience here. */
  token_text = jitterlisp_scanner_token_text (sstate);

  /* Decide what kind of token this is. */
  switch (s)
    {
    case jitterlisp_scanner_dfa_state_initial:
      /* We arrived at the end without finding a token: the input must have
         ended. */
      return jitterlisp_token_eof;

    case jitterlisp_scanner_dfa_state_complicated:
      /* We isolated a token text.  To understand what kind of token it is use
         an auxiliary function. */
      return jitterlisp_complicated_text_to_token (token_text);

    case jitterlisp_scanner_dfa_state_comment:
      /* We reached the end of the input inside a comment. */
      return jitterlisp_token_eof;

    default:
      jitter_fatal ("scanner: impossible");
    }
}




/* Parser state.
 * ************************************************************************** */

struct jitterlisp_parser_state
{
  /* The scanner state. */
  struct jitterlisp_scanner_state scanner_state;

  /* The lookahead token.  Its text is the token_text field in scanner_state,
     always '\0'-terminated from the point of view of the parser.  Notice that
     lookahead_token (and scanner_state.token_text) is already initialized with
     the first token (which may be jitterlisp_token_eof) by
     jitterlisp_initialize_parser_state , so the parser never needs to deal with
     part of its state not being initialized. */
  enum jitterlisp_token lookahead_token;
};

/* Scan the next token using the given parser state, return it, and update the
   lookahead. */
static enum jitterlisp_token
jitterlisp_parser_advance (struct jitterlisp_parser_state *pstate)
{
  pstate->lookahead_token = jitterlisp_scan (& pstate->scanner_state);
  return pstate->lookahead_token;
}

/* Initialize the pointed parser state, using the given char reader.  This also
   initializes the scanner state contained in the parser state.
   Notice that this function calls jitterlisp_initialize_scanner_state , which
   doesn't terminate until the first character is read, making this function
   blocking. */
static void
jitterlisp_initialize_parser_state (struct jitterlisp_parser_state *pstate,
                                    jitterlisp_char_reader_function crfi,
                                    jitterlisp_char_reader_state crs,
                                    jitterlisp_char_reader_finalizer crfu)
{
  /* Initialize the scanner state.  Notice that this reads the first *character*
     (not token) as the scanner lookahead, which may be a blocking operation. */
  jitterlisp_initialize_scanner_state (& pstate->scanner_state,
                                       crfi, crs, crfu);

  /* Calling jitterlisp_parser_advance (pstate) here would make the parser
     slightly more intuitive, and also ensure that pstate->lookahead_token
     is always set: see the comments below about non-advancing parsing
     functions.
     Unfortunately that alternative is unacceptable for an interactive REPL
     where we want to recognize a nonterminal as soon as it ends, with one
     lookahead *character* instead of one lookahead token. */
  pstate->lookahead_token = jitterlisp_token_invalid;
}

/* Finalize the pointed parser state, using the given char reader.  This also
   finalizes the scanner state contained in the parser state, but not the
   char reader. */
static void
jitterlisp_finalize_parser_state (struct jitterlisp_parser_state *pstate)
{
  jitterlisp_finalize_scanner_state (& pstate->scanner_state);
}

/* Return a pointer to the current token text as a '\0'-terminated C string, in
   the pointed parser state.  Do not advance.  The returned pointer is only
   valid until the parser state is advanced or finalized. */
static const char *
jitterlisp_parser_token_text (const struct jitterlisp_parser_state *pstate)
{
  return pstate->scanner_state.token_text.region;
}

/* Fail from the pointed parser state printing the given message. */
__attribute__ ((noreturn))
static void
jitterlisp_parse_error (struct jitterlisp_parser_state *pstate,
                        const char *user_message)
{
  /* Prepare a malloc-allocated string for jitterlisp_error. */
  size_t message_length = 1000 + strlen (user_message);
  char *message = jitter_xmalloc (message_length);
  sprintf (message,
           "<INPUT>:%i:%i: parse error near %s: %s",
           (int) pstate->scanner_state.row_no,
           (int) pstate->scanner_state.column_no,
           jitterlisp_parser_token_text (pstate),
           user_message);

  /* Call jitterlisp_error, which will longjmp away. */
  jitterlisp_error (message);
}




/* Prefixes.
 * ************************************************************************** */

/* Given a prefix name (such as "'"), return the name of the symbol which will
   be the car of the s-expression made of the prefix and the s-expression
   following it (such as "quote"). */
static const char *
jitterlisp_prefix_name_to_symbol_name (const char *prefix_name)
{
  if (! strcmp (prefix_name, "'"))
    return "quote";
  else if (! strcmp (prefix_name, "`"))
    return "quasiquote";
  else if (! strcmp (prefix_name, ","))
    return "unquote";
  else if (! strcmp (prefix_name, ",@"))
    return "unquote-splicing";
  else if (! strcmp (prefix_name, "#"))
    return "vector-literal";
  else
    jitter_fatal ("jitterlisp_prefix_name_to_symbol_name: invalid prefix "
                  "name \"%s\"", prefix_name);
}

/* Given a prefix symbol name (such as "quote" or "unquote") and an s-expression
   (such as foo) return the s-expression obtained by prefixing the named prefix
   to the given s-expression (such as (quote foo) or (unquote foo)).

   Rationale: it would be more convenient to directly receive the prefix name
   (such as "'" or ",") instead of the prefix symbol name, but the prefix name
   is held in a temporary scanner state buffer which gets overwritten when
   parsing sexp .  Instead the prefix symbol name as returned by
   jitterlisp_prefix_name_to_symbol_name points to static memory, which we don't
   need to copy or destroy. */
static jitterlisp_object
jitterlisp_prefix_sexpression (const char *prefix_symbol_name,
                               jitterlisp_object sexp)
{
  jitterlisp_object prefix_symbol
    = JITTERLISP_SYMBOL_ENCODE(jitterlisp_symbol_make_interned
                                  (prefix_symbol_name));
  /* Return (PREFIX-SYMBOL sexp), which is just another way of writing
     (PREFIX-SYMBOL . (sexp . ())) . */
  return jitterlisp_cons(prefix_symbol,
                         jitterlisp_cons (sexp,
                                          JITTERLISP_EMPTY_LIST));
}




/* Advancing and non-advancing parsing functions.
 * ************************************************************************** */

/* It would be more intuitive not to have both "advancing" and "non-advancing"
   parser functions; each parsing function could advance just after recognizing
   each token.
   That would work, except for one big flaw: in order to recognize the end of a
   nonterminal we would always need to have the *next* token available.
   Unfortunately that alternative would break the REPL, making it react to the
   each s-expression in a delayed fashion, only when the next one begins.

   The first parsing function to be called must be advancing.  The lookahead
   token is initialized as invalid in each parser state out of defensiveness,
   to make parsing fail if a non-advancing function is used first. */

/* Parse the next s-expression without advancing first: the current lookahead
   will be the first token of the result. */
static jitterlisp_object
jitterlisp_parse_sexp_non_advancing (struct jitterlisp_parser_state *pstate);

/* Parse the next cdr without advancing first: the current lookahead will be the
   first token of the result. */
static jitterlisp_object
jitterlisp_parse_cdr_non_advancing (struct jitterlisp_parser_state *pstate);

/* Advance the parser (to have the next token in the input as the lookahead) and
   then parse the next s-expression. */
static jitterlisp_object
jitterlisp_parse_sexp (struct jitterlisp_parser_state *pstate)
{
  jitterlisp_parser_advance (pstate);
  return jitterlisp_parse_sexp_non_advancing (pstate);
}

/* Advance the parser and then parse the next cdr. */
static jitterlisp_object
jitterlisp_parse_cdr (struct jitterlisp_parser_state *pstate)
{
  jitterlisp_parser_advance (pstate);
  return jitterlisp_parse_cdr_non_advancing (pstate);
}




/* S-expression parser.
 * ************************************************************************** */

/* The mutually recursive functions below are a hand-translation of the
   following attributed grammar:

   <sexp> ::= #<eof>         { $$ = eof; }
            | atom           { $$ = $1; }
            | prefix <sexp>  { if is_eof ($2) then
                                 error ();
                               else
                                 $$ = with-prefix ($1, $2); }
            | ( <cdr>        { $$ = $1; }

   <cdr>  ::= #<eof>         { error (); }
            | )              { $$ = empty_list; }
            | . <sexp> )     { $$ = $2; }
            | <sexp> <cdr>   { $$ = cons ($1, $2); }

   Notice that "( . <sexp> )" is recognized as an alternative degenerate form of
   "<sexp>".  Several Scheme systems do that as well.  Forbidding this would
   make the grammar, and therefore the parser, slightly more complicated. */

static jitterlisp_object
jitterlisp_parse_sexp_non_advancing (struct jitterlisp_parser_state *pstate)
{
  switch (pstate->lookahead_token)
    {
    case jitterlisp_token_eof:
      return JITTERLISP_EOF;
    case jitterlisp_token_false:
      return JITTERLISP_FALSE;
    case jitterlisp_token_true:
      return JITTERLISP_TRUE;
    case jitterlisp_token_character:
      {
        jitter_int character = jitterlisp_decode_valid_character
                                  (jitterlisp_parser_token_text (pstate));
        return JITTERLISP_CHARACTER_ENCODE(character);
      }
    case jitterlisp_token_fixnum:
      {
        jitter_long_long i = jitter_string_to_long_long_unsafe
                               (jitterlisp_parser_token_text (pstate));
        return JITTERLISP_FIXNUM_ENCODE(i);
      }
    case jitterlisp_token_symbol:
      {
        const char *name = jitterlisp_parser_token_text (pstate);
        struct jitterlisp_symbol *s = jitterlisp_symbol_make_interned (name);
        return JITTERLISP_SYMBOL_ENCODE(s);
      }

    case jitterlisp_token_prefix:
      {
        const char *prefix_name = jitterlisp_parser_token_text (pstate);
        const char *prefix_symbol_name
          = jitterlisp_prefix_name_to_symbol_name (prefix_name);
        jitterlisp_object se = jitterlisp_parse_sexp (pstate);
        if (JITTERLISP_IS_EOF(se))
          jitterlisp_parse_error (pstate, "prefix at EOF");
        else
          return jitterlisp_prefix_sexpression (prefix_symbol_name, se);
      }

    case jitterlisp_token_open:
      return jitterlisp_parse_cdr (pstate);

    default:
      jitterlisp_parse_error (pstate, "invalid s-expression beginning");
    }
}

static jitterlisp_object
jitterlisp_parse_cdr_non_advancing (struct jitterlisp_parser_state *pstate)
{
  switch (pstate->lookahead_token)
    {
    case jitterlisp_token_eof:
      jitterlisp_parse_error (pstate, "EOF after open parens");

    case jitterlisp_token_close:
      return JITTERLISP_EMPTY_LIST;

    case jitterlisp_token_dot:
      {
        jitterlisp_object res = jitterlisp_parse_sexp (pstate);
        jitterlisp_parser_advance (pstate); /* Check for ) */
        if (pstate->lookahead_token == jitterlisp_token_close)
          return res;
        else
          jitterlisp_parse_error (pstate, "expected )");
      }

    default:
      {
        jitterlisp_object car = jitterlisp_parse_sexp_non_advancing (pstate);
        jitterlisp_object cdr = jitterlisp_parse_cdr (pstate);
        return jitterlisp_cons(car, cdr);
      }
    }
}




/* Reader state: user API.
 * ************************************************************************** */

/* We export to the user a struct called struct jitterlisp_reader_state , as an
   abstract type; the user doesn't need to see the distinction between scanner
   and parser, and even less the lookahead field. */
struct jitterlisp_reader_state
{
  /* The parser state, which contains the scanner state as well. */
  struct jitterlisp_parser_state pstate;

  /* The hook to run after each toplevel s-expression parsing. */
  jitterlisp_post_parsing_hook post_parsing_hook;
};

struct jitterlisp_reader_state*
jitterlisp_make_reader_state (jitterlisp_char_reader_function crfi,
                              jitterlisp_char_reader_state crs,
                              jitterlisp_char_reader_finalizer crfu,
                              jitterlisp_post_parsing_hook pph)
{
  struct jitterlisp_reader_state *res
    = jitter_xmalloc (sizeof (struct jitterlisp_reader_state));
  jitterlisp_initialize_parser_state (& res->pstate, crfi, crs, crfu);
  res->post_parsing_hook = pph;
  return res;
}

void
jitterlisp_destroy_reader_state (struct jitterlisp_reader_state *rs)
{
  jitterlisp_finalize_parser_state (& rs->pstate);
  free (rs);
}




/* Reader state convenience function: stream reader.
 * ************************************************************************** */

struct jitterlisp_reader_state*
jitterlisp_make_stream_reader_state (FILE *input)
{
  return jitterlisp_make_reader_state (jitterlisp_stream_char_reader_function,
                                       ((jitterlisp_char_reader_state) input),
                                       NULL, NULL);
}




/* Reader state convenience function: string reader.
 * ************************************************************************** */

struct jitterlisp_reader_state*
jitterlisp_make_string_reader_state (const char *string)
{
  return jitterlisp_make_reader_state (jitterlisp_string_char_reader_function,
                                       ((jitterlisp_char_reader_state) string),
                                       NULL, NULL);
}




/* Reader state convenience function: readline and readline-one readers.
 * ************************************************************************** */

/* The struct implementing the reader state of readline and readline-one
   readers.  Notice that this struct is malloc-allocated as part of the reader
   state, and destroyed at reader state destruction: the user doesn't need
   to keep any data structure alive for the lifetime of the reader including
   the prompt string, which is cloned. */
struct jitterlisp_readline_char_reader_state
{
  /* The prompt string to show at every jitter_readline call.  This is a
     malloc-allocated copy. */
  char *prompt;

  /* A flag telling whether we saw an EOF result, which is to say if
     jitter_readline has returned NULL. */
  bool got_EOF;

  /* The last entire line we read as returned by jitter_readline, or NULL. */
  char *last_line_or_NULL;

  /* The next character to be sent to the scanner within last_line_or_NULL when
     last_line_or_NULL is non-NULL; unspecified otherwise. */
  char *next_char_p;

  /* A boolean flag preventing further jitter_readline calls; this is used
     in the hook to implement the "one" semantics. */
  bool no_more_lines;
};

/* Return the next character (or NULL if we found EOF as detected by
   jitter_readline) of the input, automatically calling jitter_readline if we
   are at the end of the string in memory and advancing next_char_p as
   needed. */
static int
jitterlisp_readline_char_reader_function (jitterlisp_char_reader_state *crspp)
{
  struct jitterlisp_readline_char_reader_state *crsp
    = * (struct jitterlisp_readline_char_reader_state **) crspp;

  /* If we already saw EOF refuse to read any more lines, and return EOF. */
  if (crsp->got_EOF)
    return EOF;

  /* If we haven't got a line read one... */
  if (crsp->last_line_or_NULL == NULL)
    {
      /* ...Unless we've been told to stop.  If after reading one more line
         we receive NULL we've found EOF. */
      if (crsp->no_more_lines
          || ((crsp->last_line_or_NULL = jitter_readline (crsp->prompt))
              == NULL))
        {
          crsp->got_EOF = true;
          return EOF;
        }

      /* If we haven't returned yet then we have a non-NULL line: set the next
         character pointer to its beginning, and go on. */
      crsp->next_char_p = crsp->last_line_or_NULL;
    }

  /* If we arrived here then we have an actual line to read from, and
     crsp->next_char_p points within it. */

  /* Does crsp->next_char_p point to a '\0' character?  If so, we have to
     interpret that as a '\n' character (which readline strips off), and prepare
     to read a new entire line at the next call. */
  if (* crsp->next_char_p == '\0')
    {
      free (crsp->last_line_or_NULL);
      crsp->last_line_or_NULL = NULL;
      return '\n';
    }

  /* If we arrived here then the next character is ordinary. */
  return * (crsp->next_char_p ++);
}

static void
jitterlisp_readline_char_reader_finalizer (jitterlisp_char_reader_state *crspp)
{
  struct jitterlisp_readline_char_reader_state *crsp
    = * (struct jitterlisp_readline_char_reader_state **) crspp;

  free (crsp->prompt);
  free (crsp);
}

/* A hook preventing a readline reader state from getting further lines, and
   then checking that the next parsed s-expression is #<eof> -- meaning that
   there is nothing more after what we parsed.  This is where the "one" part of
   readline-once is implemented. */
static void
jitterlisp_readline_one_post_parsing_hook (jitterlisp_char_reader_state *crspp,
                                           struct jitterlisp_reader_state *rsp,
                                           jitterlisp_object o)
{
  struct jitterlisp_readline_char_reader_state *crsp
    = * (struct jitterlisp_readline_char_reader_state **) crspp;

  crsp->no_more_lines = true;
  if (jitterlisp_parse_sexp (& rsp->pstate) != JITTERLISP_EOF)
    jitterlisp_parse_error (& rsp->pstate,
                            "trailing garbage after the one s-expression");

}

/* The common implementation of jitterlisp_make_readline_reader_state and
   jitterlisp_make_readline_one_reader_state . */
static struct jitterlisp_reader_state*
jitterlisp_make_readline_possibly_one_reader_state (const char *prompt,
                                                    bool one_only)
{
  /* Make a readline state data structure, allocated with malloc.  The structure
     will be destroyed by the char-state finalization function. */
  struct jitterlisp_readline_char_reader_state *crstate
    = jitter_xmalloc (sizeof (struct jitterlisp_readline_char_reader_state));;
  crstate->prompt = jitter_clone_string (prompt);
  crstate->last_line_or_NULL = NULL;
  crstate->got_EOF = false;
  crstate->next_char_p = NULL;

  crstate->no_more_lines = false;

  /* Make a new reader state*/
  return jitterlisp_make_reader_state
     (jitterlisp_readline_char_reader_function,
      crstate,
      jitterlisp_readline_char_reader_finalizer,
      one_only ? jitterlisp_readline_one_post_parsing_hook : NULL);
}

struct jitterlisp_reader_state*
jitterlisp_make_readline_reader_state (const char *prompt)
{
  return jitterlisp_make_readline_possibly_one_reader_state (prompt, false);
}

struct jitterlisp_reader_state*
jitterlisp_make_readline_one_reader_state (const char *prompt)
{
  return jitterlisp_make_readline_possibly_one_reader_state (prompt, true);
}




/* S-expression reader: user API.
 * ************************************************************************** */

/* The non-static function for the user. */
jitterlisp_object
jitterlisp_read (struct jitterlisp_reader_state *rsp)
{
  jitterlisp_object res = jitterlisp_parse_sexp (& rsp->pstate);
  if (rsp->post_parsing_hook != NULL)
    rsp->post_parsing_hook (& rsp->pstate.scanner_state.char_reader_state,
                            rsp,
                            res);
  return res;
}




/* S-expression readline convenience reader.
 * ************************************************************************** */

jitterlisp_object
jitterlisp_read_readline_one (const char *prompt)
{
  /* Make a readline-one reader state, read from it once and destroy it.  In
     case of error still destroy the context, to avoid leaks, and propagate the
     error outside. */
  struct jitterlisp_reader_state *rstate
    = jitterlisp_make_readline_one_reader_state (prompt);
  jitterlisp_object res;
  bool success = true;
  JITTERLISP_HANDLE_ERRORS({ res = jitterlisp_read (rstate); },
                           { success = false; });
  jitterlisp_destroy_reader_state (rstate);
  if (! success)
    jitterlisp_reerror ();

  /* Return what we read. */
  return res;
}
