/* Jitter: word set data structure.

   Copyright (C) 2020 Luca Saiu
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


#ifndef JITTER_WORD_SET_H_
#define JITTER_WORD_SET_H_

#include <config.h>  // FIXME: I almost certainly do not want this, but now I want to be able to use Gnulib as well, for my tests.

#include <stdbool.h>
#include <stdlib.h>

#include <jitter/jitter.h>
#include <jitter/jitter-bitwise.h>




/* Introduction.
 * ************************************************************************** */

/* This is an efficient implementation of a set data structure using words as
   keys, based on open-address hashing and mostly designed for one specialised
   purpose: holding the remembered set in a generational garbage collector,
   where keys are tagged objects.
   In that application the critical operation is add-unique.
   (For a more general but heavyweight alternative please see
   jitter/jitter-hash.h ; that implementation, differently from this one,
   includes sophisticated hash functions achieving a good "randomness".)

   The keys are non-NULL non-1 unsigned words (those two values are reserved),
   which may be tagged.

   Keys have no associated data: this data structure implements a set or a
   multiset, not a map.

   This structure features very efficient access, with an intentionally simple
   and "bad" first hash function.
   The table is automatically resized, always to a power of two, when the fill
   factor reaches a fixed threshold.  Collisions are handled by double hashing.
   Key removal is supported by marking removed elements as deleted, without
   reusing space.  The table access routines have been designed never to
   require multiplication, division, remainder.

   By convention both hash functions return an *offset* in bytes instead of an
   index.  (In a previous version using untagged aligned pointers as keys this
   choice saved one shift operation or made the addressing mode simpler;
   unfortunately the garbage collector remembered set now requires tagged
   objects as keys rather than object fields, in order to support pointers from
   out-of-heap objects.)

   The offset which is the result of either hash function needs to be masked to
   make it wrap around; the mask is stored in the data structure, updated when
   the structure is resized.


   Thanks to Bruno Haible for suggesting the use of double hashing in the first
   place, after hearing of my initial more naïve version relying on linear
   probing.  Bruno also cited Knuth's analysis and suggested the idea of h2 as
   defined below.  About h2 I do not claim to have followed his suggestions
   exactly, and any remaining oversights are mine. */




/* Configuration parameters.
 * ************************************************************************** */

/* The initial allocated size of an empty table, in elements.
   This must be an even power of two. */
#define JITTER_WORD_SET_INITIAL_ELEMENT_NO  8//(1 << 10)//2//(1 << 16)//64 // 64

/* This is the reciprocal of the maximum table fill ratio a. Assuming uniformity
   (which is not a given in practice, particularly due to the distribution of
   object updates with respect to tags -- this should be taken as an optimistic
   estimate) an upper bound on the expected number of probes in a successful
   search is
     1 / a * ln (1 / (1 - a))
   .  The analysis can be found in Knuth and in Cormen-Leiserson-Rivest-Stein.
   Apart from the question of the optimal choice of a value this is allowed
   to be any number greater than one, even floating-point.  This value is only
   used for computing the used-element limit (an integer) when initialising and
   resizing a table, so the efficiency of computations using it is not really
   critical.
   Since the table is built associatively but also scanned linearly it is
   important to strike a balance between the optimisation of the number of
   probes and the density of valid elements.
   Here is a convenient way to sample possible values from Emacs:
     (dolist (a (sort (list (/ 16.0) (/ 8.0) (/ 4.0) (/ 2.0)
                            (/ 10.0) (/ 5.0) (/ 3.0)
                            .4 .6 (/ 2 3.0) (/ 3 4.0) (/ 4 5.0))
                      '<=))
       (let* ((a (float a))
              (reciprocal-a (/ a))
              (probe-no (* (/ a) (log (/ (- 1 a))))))
         (insert (format "     a:%7.3f   reciprocal-a:%7.3f   probe-no:%7.3f\n"
                  a reciprocal-a probe-no))))
     a:  0.062   reciprocal-a: 16.000   probe-no:  1.033
     a:  0.100   reciprocal-a: 10.000   probe-no:  1.054
     a:  0.125   reciprocal-a:  8.000   probe-no:  1.068
     a:  0.200   reciprocal-a:  5.000   probe-no:  1.116
     a:  0.250   reciprocal-a:  4.000   probe-no:  1.151
     a:  0.333   reciprocal-a:  3.000   probe-no:  1.216
     a:  0.400   reciprocal-a:  2.500   probe-no:  1.277
     a:  0.500   reciprocal-a:  2.000   probe-no:  1.386
     a:  0.600   reciprocal-a:  1.667   probe-no:  1.527
     a:  0.667   reciprocal-a:  1.500   probe-no:  1.648
     a:  0.750   reciprocal-a:  1.333   probe-no:  1.848
     a:  0.800   reciprocal-a:  1.250   probe-no:  2.012  */
#define JITTER_WORD_SET_RECIPROCAL_FILL_RATIO  \
  3




/* Core definitions.
 * ************************************************************************** */

/* The special element configuration reserved for unused elements. */
#define JITTER_WORD_SET_UNUSED   ((jitter_uint) NULL)

/* The special element configuration reserved for elements marked as deleted. */
#define JITTER_WORD_SET_DELETED  ((jitter_uint) 1)




/* Data structure.
 * ************************************************************************** */

/* The keys are values of type jitter_uint . */

/* The table data structure. */
struct jitter_word_set
{
  /* How many elements fit in the currently allocated memory. */
  size_t allocated_element_no;

  /* How many used elements there can be in this set before it is automatically
     resized.  Resizing happens *after* every insertion of a new element. */
  size_t used_element_limit;

  /* How many elements are used.  Deleted elements are considered in use, as
     they take space in the structure; deleted elements are not explicitly
     counted, and "deleting" an existing entry does not change the number of
     used elements. */
  size_t used_element_no;

  /* A bit mask to be combined by a bitwise and operation with an offset in
     bytes (this is not a mistake: *in bytes*) from buffer to produce an offset,
     again in bytes, respecting the valid range.  Of course the size is always a
     power of two.  The mask is automatically computed at the time of
     initialisation and resizing. */
  jitter_uint mask;

  /* A pointer to a buffer of elements, dynamically allocated.  Eech element
     may be: JITTER_WORD_SET_UNUSED , JITTER_WORD_SET_DELETED , or a
     valid pointer. */
  jitter_uint *buffer;
};




/* Hash functions.
 * ************************************************************************** */

/* Hash functions here are only functions in the mathematical sense; they are
   implemented as macros to avoid function call overhead in this
   performance-critical code.  These hash functions return an *offset* in bytes,
   not an index; see the comment in the introduction. */

/* The word set uses double hashing: the first hash h1, given a key k,
   returns an offset into the buffer; in case of collision the probe distance
   (again as an offset) from the first offset is the result of the second hash
   h2 on the key.  Each probe after the first is at distance h2 (k) modulo the
   table size M from the previous one.
   The i-th probe will be therefore at offset
     (h1 (k) + i h2 (k)) mod M
   .  Only if the first probe fails there is need to compute h2 (k), and in
   that case it can be computed just once, even if more probes are needed after
   the second.  The modulo needs to be computed at each probe, but it should be
   efficient requring as it does only one bitwise and operation.  In critical
   code, where multiple accesses occur in a loop, the mask will be loaded once
   from memory and then kept in a register. */

/* The first hash function, named h1 in the comment above. */
#define JITTER_WORD_SET_HASH1_CHAR_STAR_OFFSET(key)       \
  /* Make the low bits zero so that the offset, starting  \
     from an aligned base, gives an aligned pointer. */   \
  (((jitter_uint) (key)) << JITTER_LG_BYTES_PER_WORD)

/* This is a definition of h2.
   In order to guarantee coverage of the entire buffer, and considering that the
   buffer size is a power of two, this function must return a result which is
   odd in terms of elements -- in terms of offsets it need to be an odd multiple
   of the (always even) word size.
   In general, in terms of indices, h2 must yield a result which is coprime with
   M.  M being a power of two here, any odd number will be correct. */
#define JITTER_WORD_SET_HASH2_CHAR_STAR_OFFSET(key)  \
  ((((jitter_uint) (key))                            \
    & JITTER_POINTER_NON_MISALIGNMENT_BITS_MASK)     \
   | (jitter_uint) 1 << JITTER_LG_BYTES_PER_WORD)




/* Initialisation and finalisation.
 * ************************************************************************** */

/* Initialise the pointed structure, which must not have been previously
   initalised, or must have been first initalised and then finalised. */
void
jitter_word_set_initialize (struct jitter_word_set *ps)
  __attribute__ ((nonnull (1)));

/* Finalise the pointed structure, freeing up its memory resources. */
void
jitter_word_set_finalize (struct jitter_word_set *ps)
  __attribute__ ((nonnull (1)));




/* Emptying and rebuilding.
 * ************************************************************************** */

/* Remove every element from the pointed word set, making future insertions
   faster.  The memory buffer must have been correctly initialised.  This does
   not resize the buffer. */
void
jitter_word_set_clear (struct jitter_word_set *ps)
  __attribute__ ((nonnull (1)));

/* Remove every element from the pointed word set and reallocate it so as to
   take the minimum space in memory.  The data structure must have been
   correctly initialised. */
void
jitter_word_set_clear_and_minimize (struct jitter_word_set *ps)
  __attribute__ ((nonnull (1)));

/* Behave like jitter_word_set_clear or like
   jitter_word_set_clear_and_minimize, according to the value of minimize. */
void
jitter_word_set_clear_and_possibly_minimize (struct jitter_word_set *ps,
                                             bool minimize)
  __attribute__ ((nonnull (1)));

/* Rebuild the pointed word set, keeping the same valid elements.  This makes
   future accesses to the structure more efficient by eliminating the
   possibility of collisions with deleted entries, but of course is an expensive
   operation. */
void
jitter_word_set_rebuild (struct jitter_word_set *ps)
  __attribute__ ((nonnull (1)));

/* Like jitter_word_set_rebuild, but also make the set become as small as
   possible.  Compared to jitter_word_set_rebuild this will save memory but
   make future insertions slower. */
void
jitter_word_set_rebuild_and_minimize (struct jitter_word_set *ps)
  __attribute__ ((nonnull (1)));

/* Behave like jitter_word_set_rebuild or like
   jitter_word_set_rebuild_and_minimize, according to the value of
   minimize. */
void
jitter_word_set_rebuild_and_possibly_minimize (struct jitter_word_set *ps,
                                               bool minimize)
  __attribute__ ((nonnull (1)));




/* Macro API.
 * ************************************************************************** */

/* Some of the macros here write into an l-value; unfortunately it is not
   possible in standard C to define them as expanding to expressions, which
   makes them slightly cumbersome to use. */

/* Add a new copy of the element to the pointed word set.  Here the "set"
   behaves in fact as a multiset. */
#define JITTER_WORD_SET_ADD_NEW(psp_expr,                      \
                                p_expr)                        \
  do                                                           \
    {                                                          \
      struct jitter_word_set *_jitter_ps_an_psp = (psp_expr);  \
      jitter_uint _jitter_ps_an_p = (p_expr);                  \
      jitter_uint *_jitter_ps_an_item;                         \
      size_t _jitter_ps_au_probeno __attribute__ ((unused));   \
      JITTER_WORD_SET_SEARCH (_jitter_ps_an_psp,               \
                              _jitter_ps_an_p,                 \
                              true,                            \
                              _jitter_ps_au_probeno,           \
                              _jitter_ps_an_item);             \
      * _jitter_ps_an_item = _jitter_ps_an_p;                  \
      _jitter_ps_an_psp->used_element_no ++;                   \
      JITTER_WORD_SET_RESIZE_IF_NEEDED (_jitter_ps_an_psp);    \
    }                                                          \
  while (false)

/* Like JITTER_WORD_SET_ADD_NEW, except that this does nothing if the key is
   already present. */
#define JITTER_WORD_SET_ADD_UNIQUE(psp_expr,                     \
                                   p_expr)                       \
  do                                                             \
    {                                                            \
      struct jitter_word_set *_jitter_ps_au_psp = (psp_expr);    \
      jitter_uint _jitter_ps_au_p = (p_expr);                    \
      jitter_uint *_jitter_ps_au_item;                           \
      size_t _jitter_ps_au_probeno __attribute__ ((unused));     \
      JITTER_WORD_SET_SEARCH (_jitter_ps_au_psp,                 \
                              _jitter_ps_au_p,                   \
                              false,                             \
                              _jitter_ps_au_probeno,             \
                              _jitter_ps_au_item);               \
      if (* _jitter_ps_au_item == JITTER_WORD_SET_UNUSED)        \
        {                                                        \
          * _jitter_ps_au_item = _jitter_ps_au_p;                \
          _jitter_ps_au_psp->used_element_no ++;                 \
          JITTER_WORD_SET_RESIZE_IF_NEEDED (_jitter_ps_au_psp);  \
        }                                                        \
    }                                                            \
  while (false)

/* Set the given result lvalue to a non-false result if the given key belongs to
   the pointed word set. */
#define JITTER_WORD_SET_SET_HAS(psp_expr,                      \
                                p_expr,                        \
                                res_lvalue)                    \
  do                                                           \
    {                                                          \
      struct jitter_word_set *_jitter_ps_sh_psp = (psp_expr);  \
      jitter_uint _jitter_ps_sh_p = (p_expr);                  \
      jitter_uint *_jitter_ps_sh_item;                         \
      size_t _jitter_ps_au_probeno __attribute__ ((unused));   \
      JITTER_WORD_SET_SEARCH (_jitter_ps_sh_psp,               \
                                 _jitter_ps_sh_p,              \
                                 false,                        \
                                 _jitter_ps_au_probeno,        \
                                 _jitter_ps_sh_item);          \
      (res_lvalue)                                             \
        = (* _jitter_ps_sh_item == _jitter_ps_sh_p);           \
    }                                                          \
  while (false)

/* Remove one entry; if any more equal entries are present, all of them except
   the first one remain. */
#define JITTER_WORD_SET_REMOVE(psp_expr,                      \
                               p_expr)                        \
  do                                                          \
    {                                                         \
      struct jitter_word_set *_jitter_ps_r_psp = (psp_expr);  \
      jitter_uint _jitter_ps_r_p = (p_expr);                  \
      jitter_uint *_jitter_ps_r_item;                         \
      size_t _jitter_ps_au_probeno __attribute__ ((unused));  \
      JITTER_WORD_SET_SEARCH (_jitter_ps_r_psp,               \
                                 _jitter_ps_r_p,              \
                                 false,                       \
                                 _jitter_ps_au_probeno,       \
                                 _jitter_ps_r_item);          \
      if (* _jitter_ps_r_item == _jitter_ps_r_p)              \
        * _jitter_ps_r_item = JITTER_WORD_SET_DELETED;        \
    }                                                         \
  while (false)

/* Set the given l-value to the number of probes necessary to find the given
   element, or to find that it is not present. */
#define JITTER_WORD_SET_SET_PROBE_NO(ps_expr,                     \
                                     p_expr,                      \
                                     res_lvalue)                  \
  do                                                              \
    {                                                             \
      struct jitter_word_set *_jitter_ps_sh_psp = (ps_expr);      \
      jitter_uint _jitter_ps_sh_p = (p_expr);                     \
      jitter_uint *_jitter_ps_sh_item __attribute__ ((unused));   \
      JITTER_WORD_SET_SEARCH (_jitter_ps_sh_psp,                  \
                              _jitter_ps_sh_p,                    \
                              false,                              \
                              (res_lvalue),                       \
                              _jitter_ps_sh_item);                \
    }                                                             \
  while (false)




/* Debugging.
 * ************************************************************************** */

/* Print the pointed word set. */
void
jitter_word_set_print (struct jitter_word_set *rsp)
  __attribute__((nonnull (1)));

/* Print debugging statistics about the pointed word set.  This is useful to
   experiment with different tuning parameters. */
void
jitter_word_set_print_statistics (struct jitter_word_set *rsp)
  __attribute__((nonnull (1)));




/* Internal accessor macros.
 * ************************************************************************** */

/* Expand to an expression evaluating to non-false if the given element is a
   "used" element: either a valid key or the "deleted" special element. */
#define JITTER_WORD_SET_IS_USED(_jitter_ps_p_expr)  \
  ((_jitter_ps_p_expr) > JITTER_WORD_SET_UNUSED)

/* Expand to an expression evaluating to non-false if the given element is an
   "unused" element: notice that the "deleted" special element does not count
   as unused. */
#define JITTER_WORD_SET_IS_UNUSED(_jitter_ps_p_expr)  \
  ((_jitter_ps_p_expr) == JITTER_WORD_SET_UNUSED)

/* Expand to an expression evaluating to non-false if the given element is a
   valid non-reserved element: not unused, not deleted. */
#define JITTER_WORD_SET_IS_VALID(_jitter_ps_p_expr)  \
  ((_jitter_ps_p_expr) > JITTER_WORD_SET_DELETED)

/* Given the number of elements of a table expand to an expression evaluating to
   its resize threshold. */
#define JITTER_WORD_SET_ELEMENT_NO_TO_LIMIT(_jitter_buffer_element_no)     \
  ((size_t)                                                                \
   ((_jitter_buffer_element_no) / JITTER_WORD_SET_RECIPROCAL_FILL_RATIO))




/* Internal macros operating on buffers only.
 * ************************************************************************** */

/* Expand to an expression of type jitter_uint * evaluating to the address of
   the given buffer at the given offset.  The buffer is an array of jitter_uint
   objects but the offset is in bytes, as computed by the hash functions
   above.
   This macro serves to conveniently do pointer arithmetic with an offset
   incompatible with the pointer. */
#define JITTER_WORD_SET_ACCESS_BUFFER(_jitter_ps_buffer_p_expr,       \
                                      _jitter_ps_offset_expr)         \
  ((jitter_uint *)                                                    \
   ((char *) (_jitter_ps_buffer_p_expr) + (_jitter_ps_offset_expr)))

/* There are two kind of match criteria:
   If search_first_unused is:
   - non-false: starting at h1 (key) search for the first entry which is unused 
   - false:     starting at h1 (key) search for the first entry which is either
                                     unused or matching p */

/* Expand to an expression evaluating to true if the result of the evaluation
   of the first argument "matches", as per the definition above, the result
   of the evaluation of the second according to the criterion which is the
   result of the evaluation of search_first_unused.
   Use __builtin_expect to compile more efficiently in case of match.
   This macro may evaluate some arguments zero, one or multiple times. */
#define JITTER_WORD_SET_MATCHES(some_object,                                 \
                                key,                                         \
                                search_first_unused)                         \
  ((search_first_unused)                                                     \
   ? (__builtin_expect (JITTER_WORD_SET_IS_UNUSED (some_object), true))      \
   : (__builtin_expect ((some_object) == (key), true)                        \
      || __builtin_expect (JITTER_WORD_SET_IS_UNUSED (some_object), true)))

/* This is the fundamental search facility used by every access macros, working
   on buffers.
   The exapansion is a statement assinging to the given l-values the number of
   required probes and the address of the found entry in the table; usually only
   one of the two will be needed, but the compiler will easily optimise away the
   unnecessary part of the computation.  Use the given search criterion. */
#define JITTER_WORD_SET_BUFFER_SEARCH(buffer_p_expr,                           \
                                      mask_expr,                               \
                                      key_expr,                                \
                                      search_first_unused,                     \
                                      probeno_lvalue,                          \
                                      entryp_lvalue)                           \
  do                                                                           \
    {                                                                          \
      /* Compute the expresisons given as arguments, once. */                  \
      jitter_uint *_jitter_ps_bs_buffer_p = (buffer_p_expr);                   \
      jitter_uint _jitter_ps_bs_key = (key_expr);                              \
      int /* bool */ _jitter_ps_bs_search_first_unused                         \
        = (search_first_unused);                                               \
      jitter_uint _jitter_ps_bs_mask = (mask_expr);                            \
      /* Compute the first hash, giving the initial offset. */                 \
      jitter_uint _jitter_ps_bs_offset                                         \
        = (JITTER_WORD_SET_HASH1_CHAR_STAR_OFFSET (_jitter_ps_bs_key)          \
           & _jitter_ps_bs_mask);                                              \
      jitter_uint * _jitter_ps_bs_some_p                                       \
        = JITTER_WORD_SET_ACCESS_BUFFER (_jitter_ps_bs_buffer_p,               \
                                            _jitter_ps_bs_offset);             \
      /* This could have been written in a single loop, but I find that GCC    \
         generates better code on some architectures if I unroll the first     \
         iteration and give __builtin_expected hints.  This is normal: most    \
         loops iterate either zero or many times, and it is uncommon for a     \
         loop to roll exactly once; hash table accesses follow this rare       \
         pattern.                                                              \
         A do..while loop would not work well here because of the computation  \
         of h2 on the key, which must come after the first iteration: see the  \
         comment below. */                                                     \
      jitter_int _jitter_ps_bs_probeno = 1;                                    \
      if (__builtin_expect (! JITTER_WORD_SET_MATCHES                          \
                                 (* _jitter_ps_bs_some_p,                      \
                                  _jitter_ps_bs_key,                           \
                                  _jitter_ps_bs_search_first_unused),          \
                            false))                                            \
        {                                                                      \
          /* The first probe failed; only now it is worth computing h2 on the  \
             key.  If I put this variable definition above GCC would not move  \
             it down after the first probe, even on architectures whose        \
             implementations are typically weaker at exploiting ILP.  If I do  \
             this the result appears to be good on every configuration. */     \
          jitter_uint _jitter_ps_bs_step                                       \
            = JITTER_WORD_SET_HASH2_CHAR_STAR_OFFSET (_jitter_ps_bs_key);      \
          while (_jitter_ps_bs_probeno ++,                                     \
                 (! JITTER_WORD_SET_MATCHES                                    \
                       (* _jitter_ps_bs_some_p,                                \
                        _jitter_ps_bs_key,                                     \
                        _jitter_ps_bs_search_first_unused)))                   \
            {                                                                  \
              _jitter_ps_bs_offset                                             \
                = ((_jitter_ps_bs_offset + _jitter_ps_bs_step)                 \
                   & _jitter_ps_bs_mask);                                      \
              _jitter_ps_bs_some_p                                             \
                = JITTER_WORD_SET_ACCESS_BUFFER                                \
                     (_jitter_ps_bs_buffer_p, _jitter_ps_bs_offset);           \
            }                                                                  \
        }                                                                      \
      /* Either the first probe succeeded and control fell here immediately,   \
         or the loop is over.  In any case _jitter_ps_bs_some_p contains the   \
         interesting element: either what the user searched for, or an unused  \
         slot.  GCC has no problem duplicating this final basic block, which   \
         is good. */                                                           \
      (entryp_lvalue) = _jitter_ps_bs_some_p;                                  \
      (probeno_lvalue) = _jitter_ps_bs_probeno;                                \
    }                                                                          \
  while (false)




/* Internal macros operating on struct jitter_word_set .
 * ************************************************************************** */

/* Expand to a statement resizing the pointed word set if the threshold has
   been reached. */
#define JITTER_WORD_SET_RESIZE_IF_NEEDED(_jitter_ps_psp_expr)              \
  do                                                                       \
    {                                                                      \
      struct jitter_word_set *_jitter_ps_rin_psp = (_jitter_ps_psp_expr);  \
      if (__builtin_expect (_jitter_ps_rin_psp->used_element_no            \
                            >= _jitter_ps_rin_psp->used_element_limit,     \
                            false))                                        \
        jitter_word_set_double (_jitter_ps_rin_psp);                       \
    }                                                                      \
  while (false)
  
/* This is an obvious extension to word sets of the fundamental macro
   JITTER_WORD_SET_BUFFER_SEARCH which works on buffers.  Values for the
   arguments to JITTER_WORD_SET_BUFFER_SEARCH not given here are found
   as fields within the pointed word buffer. */
#define JITTER_WORD_SET_SEARCH(psp_expr,                                   \
                               key_expr,                                   \
                               search_first_unused,                        \
                               probe_no_lvalue,                            \
                               res_lvalue)                                 \
  do                                                                       \
    {                                                                      \
      struct jitter_word_set *_jitter_ps_s_psp = (psp_expr);               \
      jitter_uint _jitter_ps_s_key = (key_expr);                           \
      int /*bool*/ _jitter_ps_s_search_first_unused                        \
        = (search_first_unused);                                           \
      jitter_uint _jitter_ps_s_mask = _jitter_ps_s_psp->mask;              \
      JITTER_WORD_SET_BUFFER_SEARCH (_jitter_ps_s_psp->buffer,             \
                                        _jitter_ps_s_mask,                 \
                                        _jitter_ps_s_key,                  \
                                        _jitter_ps_s_search_first_unused,  \
                                        (probe_no_lvalue),                 \
                                        (res_lvalue));                     \
    }                                                                      \
  while (false)




/* Internal functions.
 * ************************************************************************** */

/* Double the allocated size of the pointed word set.  Only used
   internally. */
void
jitter_word_set_double (struct jitter_word_set *ps)
  __attribute__ ((cold, nonnull (1)));


#endif // #ifndef JITTER_WORD_SET_H_
