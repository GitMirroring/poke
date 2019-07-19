/* Jitter: VM-independent library.

   Copyright (C) 2016, 2017, 2019 Luca Saiu
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


#ifndef JITTER_H_
#define JITTER_H_

#include <stdlib.h> // for size_t .
#include <limits.h> // for CHAR_BIT .
#include <stdint.h>
#include <inttypes.h> // format strings for standard integer types.
#include <string.h> // JITTER_ARCHITECTURE_IS relies on strcmp .

/* Include the host-dependent header , and make sure that it actually contains
   some definitions. */
#include <jitter/jitter-config.h>
#ifndef SIZEOF_VOID_P
#  error "jitter/jitter-config.h is probably incorrect"
#endif // #ifndef SIZEOF_VOID_P

/* Include macros emulating missing GNU C features. */
#include <jitter/jitter-missing.h>

/* We need some CPP machinery here, for conveninient stringification and token
   concatenation. */
#include <jitter/jitter-cpp.h>

/* Include the machine-specific header, if one exists for the host machine. */
#ifdef JITTER_ASSEMBLY_SUBDIRECTORY
# include <jitter/machine/jitter-machine.h>
#endif // #ifdef JITTER_ASSEMBLY_SUBDIRECTORY




/* Fundamental type definitions.
 * ************************************************************************** */

/* Configurations where a char is not 8 bits have never been tested. */
#if CHAR_BIT != 8
# warning "The char type of a size different from 8 bits: untested."
#endif // #if CHAR_BIT != 8

/* Define jitter_int and jitter_uint as word-sized integer type, respectively
   signed and unsigned; also define the format strings, JITTER_PRIi ,
   JITTER_PRIu , JITTER_PRIo and JITTER_PRIx to be used with printf and scanf
   for those types, in the style of PRIi64 , PRIu64 , PRIo64 and PRIx64 . */
#if   (SIZEOF_VOID_P * CHAR_BIT == 16)
  /* This will not happen on GNU, but one check costs almost nothing. */
  typedef int16_t  jitter_int;
  typedef uint16_t jitter_uint;
# define JITTER_PRIi PRIi16
# define JITTER_PRIu PRIu16
# define JITTER_PRIo PRIo16
# define JITTER_PRIx PRIx16
#elif (SIZEOF_VOID_P * CHAR_BIT == 32)
  typedef int32_t  jitter_int;
  typedef uint32_t jitter_uint;
# define JITTER_PRIi PRIi32
# define JITTER_PRIu PRIu32
# define JITTER_PRIo PRIo32
# define JITTER_PRIx PRIx32
#elif (SIZEOF_VOID_P * CHAR_BIT == 64)
  typedef int64_t  jitter_int;
  typedef uint64_t jitter_uint;
# define JITTER_PRIi PRIi64
# define JITTER_PRIu PRIu64
# define JITTER_PRIo PRIo64
# define JITTER_PRIx PRIx64
#else
# error "can't find a word-sized integer type."
#endif // #if   (SIZEOF_VOID_P == ...)

/* Define two more format strings for convenience, JITTER_INT_FORMAT and
   JITTER_UINT_FORMAT; they can be used like "%li" and "%lu" . */
#define JITTER_INT_FORMAT  "%" JITTER_PRIi
#define JITTER_UINT_FORMAT "%" JITTER_PRIu

/* Define a word-sized floating-point type. */
#if   (SIZEOF_VOID_P == SIZEOF_FLOAT)
  typedef float jitter_float;
#elif (SIZEOF_VOID_P == SIZEOF_DOUBLE)
  typedef double jitter_float;
#elif (SIZEOF_VOID_P == SIZEOF_LONG_DOUBLE)
/* This should not happen anywhere, I guess -- on PowerPC long double is 64-bit
   but so is double which is checked before.  Anyway, it costs nothing. */
  typedef long double jitter_float;
#else
# error "can't find a word-sized floating-point type"
#endif // #if   (SIZEOF_VOID_P == ...)


/* A thread, which is to say a label-as-value using the GNU C extension.  From
   the executor point of view the memory is constant, so restrict here is
   correct and might possibly enable some optimization. */
typedef const void * restrict jitter_thread;

/* A register index as occurring in an instruction.  A pair <class, index>
   uniquely identifies a register. */
typedef jitter_int jitter_register_index;

/* The index of an unspecialized instruction, 0-based. */
typedef jitter_int jitter_label_as_index;

/* Define a word-sized union holding an integer (signed or unsigned) or a
   pointer value.  This fits in a general register, by design; in order to allow
   this we prefer not to include a floating-point case as well in the same
   union.
   The integer fields are word-sized, and therefore they can also hold any enum
   value without loss of information, given the appropriate signedness; in this
   project enum values are always non-negative.

   FIXME: is this design a problem on m68k, which has different registers for
   integers and addresses?  Not very high-priority, but I'm curious. */
union jitter_word
{
  /* A signed word-sized integer. */
  jitter_int fixnum;

  /* An unsigned word-sized integer. */
  jitter_uint ufixnum;

  /* A word pointer.  The restrict qualifier may enable some optimization and is
     correct here: when memory is changed thru the pointer the program being
     interpreted will explicitly reload when needed; it's useless to let GCC be
     pessimistic here and assume that everything may be changed by a store thru
     this. */
  union jitter_word * restrict pointer;

  /* A label is in practice a pointer, with the difference that the pointed
     memory is constant. */
  const void *label;

  /* FIXME: are these useful?  Pointers to different types, just as a
     convenience. */
  void * restrict pointer_to_void;
  char * restrict pointer_to_char;

  /* A thread. */
  jitter_thread thread;
};




/* The long long type or some approximation of it.
 * ************************************************************************** */

/* Jitter uses integer types as wide as pointers in most cases, but in a few
   circumstances, particularly for textual I/O, it is convenient to use the
   widest integer type available. */

/* Provide some replacement for long long if it's not available.  In either
   case define:
   - the type jitter_long_long , defined as either long long or long ;
   - the type jitter_ulong_long , defined as the unsigned version of
     jitter_long_long ;
   - the format strings JITTER_PRIill and JITTER_PRIull (not including the "%"
     prefix, for signed and unsigned types, printed only in radix 10), expanding
     to either "lli" and "llu" or "li" and "lu";
   - the macros jitter_strtoll and jitter_strtoull, expanding to either strtoll
     and strtoull or strtol and strtoul . */
#ifdef HAVE_LONG_LONG_INT
  /* We have a real long long type.  Define the type and macros above as trivial
     wrappers. */
  typedef long long jitter_long_long;
  typedef unsigned long long jitter_ulong_long;
# define JITTER_PRIill "lli"
# define JITTER_PRIull "llu"
# define jitter_strtoll strtoll
# define jitter_strtoull strtoull
#else
  /* The type long long is not available in this configuration.  Use long in its
     place. */
  typedef long jitter_long_long;
  typedef unsigned long jitter_ulong_long;
# define JITTER_PRIill "li"
# define JITTER_PRIull "lu"
# define jitter_strtoll strtol
# define jitter_strtoull strtoul
#endif // #ifdef HAVE_LONG_LONG_INT




/* Word size in bytes and its binary logarithm.
 * ************************************************************************** */

/* Word size in bytes (actually in chars). */
#define JITTER_BYTES_PER_WORD \
  SIZEOF_VOID_P

/* Define the binary logarithm of the word size in bits (a common parameter to
   use for shifting operations), and the name for a Gas pseudo-op generating a
   word-size object.  These definitions need a dispatch on the word size. */
#if   JITTER_BYTES_PER_WORD == 8
#  define JITTER_LG_BYTES_PER_WORD 3
#  define JITTER_ASM_WORD ".quad"
#elif JITTER_BYTES_PER_WORD == 4
#  define JITTER_LG_BYTES_PER_WORD 2
#  define JITTER_ASM_WORD ".long"
#elif JITTER_BYTES_PER_WORD == 2
#  define JITTER_LG_BYTES_PER_WORD 1
#  define JITTER_ASM_WORD ".word"
#  warning "Weird: running Jitter on a 16-bit machine; this is untested."
#else
#  error "Weird: this machine's word size is not 8, 4 or 2 bytes"
#endif // #if JITTER_BYTES_PER_WORD == ...




/* Definitions depending on the dispatch model.
 * ************************************************************************** */

/* If there is no CPP definition for a dispatching model use the one identified
   as the best by configure. */
#if    ! defined(JITTER_DISPATCH_SWITCH)             \
    && ! defined(JITTER_DISPATCH_DIRECT_THREADING)   \
    && ! defined(JITTER_DISPATCH_MINIMAL_THREADING)  \
    && ! defined(JITTER_DISPATCH_NO_THREADING)
# if   defined(JITTER_BEST_DISPATCHING_MODEL_IS_SWITCH)
#   define JITTER_DISPATCH_SWITCH 1
# elif defined(JITTER_BEST_DISPATCHING_MODEL_IS_DIRECT_THREADING)
#   define JITTER_DISPATCH_DIRECT_THREADING 1
# elif defined(JITTER_BEST_DISPATCHING_MODEL_IS_MINIMAL_THREADING)
#   define JITTER_DISPATCH_MINIMAL_THREADING 1
# elif defined(JITTER_BEST_DISPATCHING_MODEL_IS_NO_THREADING)
#   define JITTER_DISPATCH_NO_THREADING 1
# else
#   error "no best dispatching model is defined.  This should never happen."
# endif // #if defined(JITTER_BEST_DISPATCHING_MODEL_IS_...)
#endif // no CPP definition for the dispatch model

/* Check that one dispatching model is defined with a CPP macro, and define
   JITTER_REPLICATE if needed.  Also define the JITTER_DISPATCH_NAME macro
   as the dispatching model name, usable as a C identifier. */
#if   defined(JITTER_DISPATCH_SWITCH)
# define JITTER_DISPATCH_NAME switch
#elif defined(JITTER_DISPATCH_DIRECT_THREADING)
# define JITTER_DISPATCH_NAME direct_threading
#elif defined(JITTER_DISPATCH_MINIMAL_THREADING)
# define JITTER_DISPATCH_NAME minimal_threading
  /* Minimal threading requires code replication. */
# define JITTER_REPLICATE 1
#elif defined(JITTER_DISPATCH_NO_THREADING)
# define JITTER_DISPATCH_NAME no_threading
  /* No-threading requires code replication. */
# define JITTER_REPLICATE 1
#else
# error "unknown dispatching model.  This should never happen."
#endif // #if defined(JITTER_DISPATCH_...)




/* The selected dispatching model name as a string or a C identifier.
 * ************************************************************************** */

/* Define the dispatching model name as a string literal. */
#define JITTER_DISPATCH_NAME_STRING       \
  JITTER_STRINGIFY(JITTER_DISPATCH_NAME)

/* Compute the name of a C global whose name depends on the dispatching model.
   Only the one for the selected model is defined, and this serves to prevent
   mistakes when linking jitterc-generated code to a runtime library; the
   definition is in jitter.c and the code using the global is in vm1.c . */
#define JITTER_DISPATCH_DEPENDENT_GLOBAL_NAME                \
  JITTER_CONCATENATE_THREE(jitter_this_is_the_runtime_for_,  \
                           JITTER_DISPATCH_NAME,             \
                           _dispatch)




/* Feature macros derived on other macros.
 * ************************************************************************** */

/* The following macros are useful from many places in C code, in particular in
   order to know whether some feature should be enabled, depending on the
   dispatching mode or the architecture.
   It is convenient to define them here in a centralized way, rather than in
   individual headers which the user may forget to include before testing
   whether some feature macro is defined.

   Naming convention:
   - Feature macros defining whether a functionality could be used have names
     starting with "JITTER_HAVE_" .
   - Feature macros defining whether a functionality is actually used (in the
     current dispatching mode, for the program which is being compiled) have
     names starting with "JITTER_USE_" .
   Notice that the feature macros defined in jitter/jitter-config.h are also
   visible from here. */


/* This is not a feature macro in the sense of the other macros defined in
   this section, but it still belongs here because of its practical role.
   Expand to a (currently non-constant) Boolean expression evaluating to
   non-false iff the architecture name, as per JITTER_ASSEMBLY_SUBDIRECTORY,
   is the given one.
   If the architecture name is unknown then the expansion evaluates to false
   with any argument.
   The macro argument must evaluate to a C string. */
#ifdef JITTER_ASSEMBLY_SUBDIRECTORY
#define JITTER_ARCHITECTURE_IS(jitter_architecture_name_as_string)  \
  (! strcmp ((jitter_architecture_name_as_string),                  \
             JITTER_ASSEMBLY_SUBDIRECTORY))
#else // unknown architecture
#define JITTER_ARCHITECTURE_IS(jitter_architecture_name_as_string)  \
  false
#endif // #ifdef JITTER_ASSEMBLY_SUBDIRECTORY


#endif // #ifndef JITTER_CONFIG_H_
