/* Jitter: custom contextual printing.

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


#ifndef JITTER_PRINT_H_
#define JITTER_PRINT_H_

#include <stdio.h>  /* For the definition of FILE . */

#include <jitter/jitter.h>
#include <jitter/jitter-dynamic-buffer.h>

#if defined (JITTER_HAVE_LIBTEXTSTYLE)
#include <textstyle.h>  /* For the definition of styled_ostream_t . */
#endif // #if defined (JITTER_HAVE_LIBTEXTSTYLE)




/* Introduction.
 * ************************************************************************** */

/* This header defines an extensible output abstraction called "print context",
   allowing the user to print data along with user-specified "decorations" to an
   underlying channel such as a file descriptor, libc stream or socket, or even
   a memory buffer.  A "print context kind" describes the behaviour of
   operations, including the way decorations are handled; every print context
   has a print context kind, plus an underlying channel on which to write.  A
   few print context kinds are provided here; other use cases are possible for
   the user to define using this documented API (pretty-printing comes to mind),
   and might be directly provided here in the future.

   "Decorations" may express semantic attributes such as the nesting level of a
   program form or encode cosmetic features such as fonts, colours or
   hyperlinks, for example as implemented by GNU libtextstyle .  Some (or all)
   decorations will have no effect in some context kinds; a user is still free
   to write any decoration to any print context, and to reuse the same code for
   printing to different contexts -- for example on a terminal supporting
   colours and fonts, to a file, into a buffer in memory.

   The API is meant to be expressive enough for wrapping the functionality of
   GNU libtextstyle for printed colour or font attributes and hyperlinks where
   the library is available, while ignoring such functionality otherwise.
   If the underlying channel abstraction does not support such facilities then
   the additional styling and hyperlinking operations will have no effect, but
   the data output operations will still behave as intended.
   A libtextstyle print context kind is provided in a separate library, as
   explained below, in order not to have libtextstyle as a dependency for
   libjitter.

   It is possible in general to interleave other input/output operations on the
   underlying channel (for example file seeking, or reading) with print context
   operations, as long as the print context is explicitly flushed before each
   explicit access to the underlying channel.

   This API is not by itself thread-safe: in a concurrent environment it is the
   user's responsibility to ensure that critical sections are executed
   atomically.

   The Jitter runtime library prints VM routines and native code disassemblies
   through this facility, generating output to a print context provided by the
   user. */

/* Every function with return type int in this API returns 0 to signify success,
   while any other result signifies failure.
   In case of error these functions may print an unspecified number of
   characters before failing; there is currently no support for error recovery.
   Depending on the definition of the context kind, some of the functions
   defined below may have no effect on the output, and simply return 0.  When
   the context kind in question defines the operation as having any effect, any
   operation is subject to failure.
   Notice that beginning or ending a decoration may well be output operations,
   subject to failure. */


/* A print context is a pointer to an opaque structure. */
typedef struct jitter_print_context_private *
jitter_print_context;




/* Context initialisation.
 * ************************************************************************** */

/* Functions making new contexts are specific to each context kind, and a few
   can be found below.  Any context returned by a context-making function will
   have initially no active decorations. */




/* Printing with contexts.
 * ************************************************************************** */

/* A formatted output function in the style of printf , while easy to define on
   GNU and a few other platforms thanks to aprintf , is unfortunately
   impractical to provide using standard C only and a generic libc, and is
   therefore omitted in the interest of portability.  Of course the user is free
   to employ aprintf , sprintf or any other facility in order to compose a
   string in memory to be then printed by calling jitter_print_char_star . */

/* Print a char object in the given context. */
int
jitter_print_char (jitter_print_context ct, char x)
  __attribute__ ((nonnull (1)));

/* Print char_no char objects read from memory starting at the address p,
   including any '\0' chars.  This function is intended for binary output. */
int
jitter_print_chars (jitter_print_context ct, const char *p, size_t char_no)
  __attribute__ ((nonnull (1, 2)));

/* Print the pointed string in the given context, up to and not including its
   first '\0' character.  Undefined behaviour is the string is NULL or not
   '\0'-terminated. */
int
jitter_print_char_star (jitter_print_context ct, const char *x)
  __attribute__ ((nonnull (1, 2)));

/* Print a pointer in the given context, using the same syntax as printf's
   "%p" output format. */
int
jitter_print_pointer (jitter_print_context ct, void *x)
  __attribute__ ((nonnull (1)));

/* Print an integer in the given context, using the given radix.  The minimum
   supported radix is 2, and the maximum is 36.  Undefined behaviour for
   unsupported radixes. */
int
jitter_print_short (jitter_print_context ct, int radix, short x)
  __attribute__ ((nonnull (1)));
int
jitter_print_int (jitter_print_context ct, int radix, int x)
  __attribute__ ((nonnull (1)));
int
jitter_print_long (jitter_print_context ct, int radix, long x)
  __attribute__ ((nonnull (1)));
int
jitter_print_long_long (jitter_print_context ct, int radix, jitter_long_long x)
  __attribute__ ((nonnull (1)));
int
jitter_print_ushort (jitter_print_context ct, int radix, unsigned short x)
  __attribute__ ((nonnull (1)));
int
jitter_print_uint (jitter_print_context ct, int radix, unsigned int x)
  __attribute__ ((nonnull (1)));
int
jitter_print_ulong (jitter_print_context ct, int radix, unsigned long x)
  __attribute__ ((nonnull (1)));
int
jitter_print_ulong_long (jitter_print_context ct, int radix,
                         jitter_ulong_long x)
  __attribute__ ((nonnull (1)));

/* Print an floating-point number in the given context.  Right now only the
   equivalent of printf's "%f" output format is currently supported; for
   printing in other formats the user will have to resort to sprintf or
   custom alternatives. */
int
jitter_print_float (jitter_print_context ct, float x)
  __attribute__ ((nonnull (1)));
int
jitter_print_double (jitter_print_context ct, double x)
  __attribute__ ((nonnull (1)));
#if defined (JITTER_HAVE_LONG_DOUBLE)
int
jitter_print_long_double (jitter_print_context ct, long double x)
  __attribute__ ((nonnull (1)));
#endif // #if defined (JITTER_HAVE_LONG_DOUBLE)

/* Convenience aliases for printing integer or floating-point data having Jitter
   types.  Each of these definition is just a macro resolving to one of the
   previous integer-printing function names, according to the configuration. */
#if   JITTER_SIZEOF_VOID_P == JITTER_SIZEOF_SHORT
  /* This will not happen on GNU, but one check costs almost nothing. */
# define  jitter_print_jitter_int jitter_print_short
# define  jitter_print_jitter_uint jitter_print_ushort
#elif JITTER_SIZEOF_VOID_P == JITTER_SIZEOF_INT
# define  jitter_print_jitter_int jitter_print_int
# define  jitter_print_jitter_uint jitter_print_uint
#elif JITTER_SIZEOF_VOID_P == JITTER_SIZEOF_LONG
# define  jitter_print_jitter_int jitter_print_long
# define  jitter_print_jitter_uint jitter_print_ulong
#elif defined (JITTER_HAVE_LONG_LONG_INT) \
      && (JITTER_SIZEOF_VOID_P == JITTER_SIZEOF_LONG_LONG)
# define  jitter_print_jitter_int jitter_print_long_long
# define  jitter_print_jitter_uint jitter_print_ulong_long
#else
# error "this should never happen: sizeof (void *) has an unexpected value"
#endif
#if    JITTER_SIZEOF_VOID_P == JITTER_SIZEOF_FLOAT
# define  jitter_print_jitter_float jitter_print_float
#elif  JITTER_SIZEOF_VOID_P == JITTER_SIZEOF_DOUBLE
# define  jitter_print_jitter_float jitter_print_double
#elif  defined (JITTER_HAVE_LONG_DOUBLE) \
       && (JITTER_SIZEOF_VOID_P == JITTER_SIZEOF_LONG_DOUBLE)
# define  jitter_print_jitter_float jitter_print_long_double
#else
# error "this should never happen: no floating-point type is defined with the"
# error "same size as void *."
#endif




/* Decorations.
 * ************************************************************************** */

/* Each decoration has a name, a type and a value.  Decoration may be nested
   inside each other, and have to be "begun" or "ended" in a strictly LIFO
   order.  The topmost decoration with a given name, if one exists, is the
   "active" one.
   Decoration values are dynamically typed. */

/* The name of a decoration is a string.  User functions may pass temporary
   strings to this API, for example always reusing the same memory; decoration
   names (and string values: see below) are copied into internally managed
   memory as needed. */
typedef char *
jitter_print_decoration_name;

/* An indicator of which supported type a decoration currently has. */
enum jitter_print_decoration_type
  {
    /* The decoration value is integer. */
    jitter_print_decoration_type_integer,

    /* The decoration value is floating-point. */
    jitter_print_decoration_type_floating_point,

    /* The decoration value is a text string (copied into local memory) by
       the print context subsystem. */
    jitter_print_decoration_type_string,

    /* The decoration value is a pointer to arbitrary user memory. */
    jitter_print_decoration_type_pointer
  };

/* The value of a particular decoration.  Each field is associated to one
   value of enum jitter_print_decoration_type , defined above. */
union jitter_print_decoration_value
{
  /* A signed integer value. */
  jitter_int integer;

  /* A floating-point value. */
  double floating_point;

  /* A '\0'-terminated string.  Any value given by the user is copied into
     locally managed memory, and the user should not attempt to free the copy
     pointed by this field. */
  char *string;

  /* A pointer to arbitrary data supplied by the user.  Differently from the
     string field this pointer refers user data, which is the user's
     responsibility to keep alive until the decoration remains in use. */
  void *pointer;
};




/* Decoration beginning.
 * ************************************************************************** */

/* "Beginning" a decoration in a print context makes the decoration "active" and
   (according to the print context kind) may affect how text printed after the
   "begin" operation looks like, until the decoration is "ended".  Decorations
   can be nested, but must be begun and ended in a strictly LIFO order. */

/* Begin an integer-type decoration in the pointed context, having the given
   name and the given value. */
int
jitter_print_begin_decoration_integer (jitter_print_context ct,
                                       jitter_print_decoration_name name,
                                       jitter_int value)
  __attribute__ ((nonnull (1, 2)));

/* Begin a floating-point-type decoration in the pointed context, having the
   given name and the given value. */
int
jitter_print_begin_decoration_floating_point (jitter_print_context ct,
                                              jitter_print_decoration_name name,
                                              double value)
  __attribute__ ((nonnull (1, 2)));

/* Begin a string-type decoration in the pointed context, having the given name
   and the given value.  The value is copied into internally managed memory, and
   it is the user's responsibility to dispose of her original copy. */
int
jitter_print_begin_decoration_string (jitter_print_context ct,
                                      jitter_print_decoration_name name,
                                      char *value)
  __attribute__ ((nonnull (1, 2, 3)));

/* Begin a pointer-type decoration in the pointed context, having the given name
   and the given value.  This function, differently from
   jitter_print_begin_decoration_string , never derefenreces the value pointer
   but simply copies it; it is the user's responsibility to keep the pointed
   data alive until the decoration is ended by a matching
   jitter_print_end_decoration call. */
int
jitter_print_begin_decoration_pointer (jitter_print_context ct,
                                       jitter_print_decoration_name name,
                                       void *value)
  __attribute__ ((nonnull (1, 2)));




/* Decoration ending.
 * ************************************************************************** */

/* End the last started decoration, which must have the given name.  Undefined
   behaviour if no matching decoration was begun, or if beginning and end
   of multiple decorations do not follow a LIFO order. */
int
jitter_print_end_decoration (jitter_print_context ct,
                             jitter_print_decoration_name name)
  __attribute__ ((nonnull (1, 2)));




/* Decoration introspection.
 * ************************************************************************** */

/* Check the currently active decoration in the given context.  If any exists
   then set the memory pointed from name, type and value to pointers to relevant
   decoration attributes in internal memory, which will remain valid as long as
   no further decoration is begun or ended.  Set the memory pointed by the three
   pointers to NULL otherwise. */
void
jitter_print_get_decoration (jitter_print_context ct,
                             jitter_print_decoration_name *name_p,
                             enum jitter_print_decoration_type **type_pp,
                             union jitter_print_decoration_value **value_pp)
  __attribute__ ((nonnull (1, 2, 3, 4)));

/* Like jitter_print_get_decoration , but only consider decorations with the
   given name.  There is no user pointer name to set in this case. */
void
jitter_print_get_decoration_named (jitter_print_context ct,
                                   jitter_print_decoration_name name,
                                   enum jitter_print_decoration_type **type_pp,
                                   union jitter_print_decoration_value
                                   **value_pp)
  __attribute__ ((nonnull (1, 2, 3, 4)));




/* Convenience functions for known decorations: classes.
 * ************************************************************************** */

/* "Classes" describe the cosmetic look of text output.  Libtextstyle uses
   string values for classes. */

/* The name of class decorations. */
#define JITTER_PRINT_DECORATION_NAME_CLASS "class"

/* The name of a class, as used by libtextstyle.  It is a simple
   '\0'-terminated text string, which is copied into internally managed
   memory as needed. */
typedef char *
jitter_print_class;

/* Switch to using the given class for the forthcoming output, until the
   next call to jitter_print_begin_class or to jitter_print_end_class.  Class
   uses nest in a LIFO style: after ending a class use the previously "begun"
   class, if any, comes back into effect. */
int
jitter_print_begin_class (jitter_print_context ct, jitter_print_class c)
  __attribute__ ((nonnull (1, 2)));

/* Stop using the last class given in jitter_print_begin_class, and switch back
   to the class defined by the previoys jitter_print_begin_class call (or no
   class, if the outermost call was matched) for the following.  Undefined
   behaviour if there is no previous jitter_print_begin_class call to match. */
int
jitter_print_end_class (jitter_print_context ct)
  __attribute__ ((nonnull (1)));

/* If any class is currently active in the pointed context return its value,
   using internal memory managed internally by the print context system; return
   NULL otherwise.  Undefined behaviour if the current class is set to a
   non-string value. */
char *
jitter_print_get_class (jitter_print_context ct)
  __attribute__ ((returns_nonnull,
                  nonnull (1)));




/* Convenience functions for known decorations: hyperlinks.
 * ************************************************************************** */

/* In Libtextstyle a "hyperlink" is a text string formatted like a URL
   associated to output text; some recent terminal emulator actually render URLs
   as interactive clickable links.
   Libtextstyle hyperlinks are not nestable and always have a string type.
   Jitter print contexts only enforce this non-nestability and the decoration
   type when using the convenience functions in this section; otherwise generic
   decorations are dynamically typed, and can nest. */
#define JITTER_PRINT_DECORATION_NAME_HYPERLINK "url"

/* A hyperlink destination, as used by libtextstyle.  It is a simple
   '\0'-terminated string, which gets copied into internally handled memory as
   needed. */
typedef char *
jitter_print_url;

/* Mark the current output as the beginning of a hyperlink to the given URL,
   which must be a valid '\0'-terminated string.
   Fail fatally if a hyperlink is already active at the time of the call. */
int
jitter_print_begin_hyperlink (jitter_print_context ct, jitter_print_url url)
  __attribute__ ((nonnull (1, 2)));

/* Mark the current output as the end of the hyperlink which was started before
   with jitter_print_begin_hyperlink.  Undefined behaviour if there is no
   previous jitter_print_begin_hyperlink call to match.  At the end of the
   call the context has no longer any current hyperlink. */
int
jitter_print_end_hyperlink (jitter_print_context ct)
  __attribute__ ((nonnull (1)));

/* If any hyperlink is currently active in the pointed context return its value,
   using internal memory managed internally by the print context system; return
   NULL otherwise.  Undefined behaviour if the current hyperlink is set to a
   non-string value. */
char *
jitter_print_get_hyperlink (jitter_print_context ct)
  __attribute__ ((returns_nonnull,
                  nonnull (1)));




/* Other functionality available in every context.
 * ************************************************************************** */

/* Functions making new contexts are specific to each context kind, and
   are not defined here.  Any context returned by a context-making function
   will have initially no class and no hyperlink destination. */

/* Flush the output.  Return 0 on success, a non-zero value on error. */
int
jitter_print_flush (jitter_print_context ct)
  __attribute__ ((nonnull (1)));

/* End any still open decoration, flush the output then destroy the given
   context.  Undefined behaviour if the context is not valid or has already
   been destroyed.  Return 0 if all of the close and flush operations succeed,
   a non-zero result otherwise; destroy the data structure in either case.
   Notice that, for every reasonable print context type, destroying the
   context should *not* close the underlying channel: it may be useful
   for the user to perform additional input/output even after the print
   context has been finalised. */
int
jitter_print_context_destroy (jitter_print_context ct)
  __attribute__ ((nonnull (1)));




/* Predefined print context kinds: FILE * and file descriptor.
 * ************************************************************************** */

/* Return a fresh print context printing to the pointed libc buffered stream,
   ignoring decorations. */
jitter_print_context
jitter_print_context_make_file_star (FILE *f)
  __attribute__ ((warn_unused_result, returns_nonnull,
                  nonnull (1)));

/* Return a fresh print context printing to the given libc unbuffered file
   descriptor, ignoring flushing and decorations. */
jitter_print_context
jitter_print_context_make_fd (int fd)
  __attribute__ ((warn_unused_result, returns_nonnull));




/* Predefined print context kind: string.
 * ************************************************************************** */

/* Return a fresh print context "printing" into a memory buffer which
   automatically grows to fit the output.  Such a context ignores flushing and
   decorations. */
jitter_print_context
jitter_print_context_make_memory (void)
  __attribute__ ((warn_unused_result, returns_nonnull));

/* Given a print context made by jitter_print_context_make_memory, return
   a malloc-allocated copy of its current buffer, also including any '\0'
   chars previously printed by the user, terminated by a further '\0' char.
   It is the user's responsibility to eventually dispose of the copied buffer
   by calling free .
   If the supplied length pointer is not NULL write the length of the printed
   data (not counting the one added trailing '\0') into its pointed memory.

   Notice that the returned buffer will contain '\0' chars *before* the trailing
   one which is appended by this function, if any were printed as single chars
   (printing using print_char_star functions does not copy any '\0' char to the
   output) or binary data (using print_chars functions).  This feature is
   intended for binary formats, in which case the user should pass a non-NULL
   value for length_p . */
char *
jitter_print_context_get_memory (jitter_print_context c, size_t *length_p)
  __attribute__ ((warn_unused_result, returns_nonnull,
                  nonnull (1)));




/* Predefined print context kind: GNU libtextstyle.
 * ************************************************************************** */

/* The functionality in this section is only available if Jitter was configured
   with GNU libtextstyle support.
   User code is easy to conditionalise by using either a Libtextstyle context,
   or a different FILE * or file descriptor context. */
#if defined (JITTER_HAVE_LIBTEXTSTYLE)

/* GNU Libtextstyle is a large library which should not be unconditionally
   linked in Jittery VMs.  For this reason the Libtextstyle print context kind
   is distributed in a separate optional library, libjitter-libtextstyle .  Like
   the rest of the Jitter runtime it is built as a Libtool library, and also
   available as a static library when Jitter is configured in sub-package mode.

   libjitter-libtextstyle requires its own initialisation and finalisation.  The
   sources are in jitter/jitter-print-libtextstyle.c .  There is no separate
   header file. */

/* Initialise the Jitter print context wrapper for GNU libtextstyle.  This
   needs to be called once before using the facility. */
void
jitter_print_libtextstyle_initialize (void);

/* Finalise the Jitter print context wrapper for GNU libtextstyle.  This may
   be called after the facility is no longer needed, to free up resources --
   and, maybe mainly, in order to avoid spurious valgrind warnings.
   It is possible to initialise the subsystem again after calling this. */
void
jitter_print_libtextstyle_finalize (void);

/* Return a fresh print context using the given Libtextstyle ostream.  The
   returned print context supports classes and hyperlinks.
   This function can only called after the Jitter print context wrapper for GNU
   libtextstyle has been initialised by jitter_print_libtextstyle_initialize . */
jitter_print_context
jitter_print_context_make_libtextstyle (styled_ostream_t ostream)
  __attribute__ ((warn_unused_result, returns_nonnull,
                  nonnull (1)));

#endif /* #if defined (JITTER_HAVE_LIBTEXTSTYLE) */




/* Defining new print context kinds.
 * ************************************************************************** */

/* The user may define a custom context kind by providing functions associated
   to fundamental operations and storing pointers to them in memory as fields
   of a structure of type struct jitter_print_context_kind_struct .
   Each print context contains a pointer to user-provided data, which the
   functions within the struct jitter_print_context_kind_struct object may
   access.
   Since more fields within struct jitter_print_context_kind_struct may be
   added in the future the user is strongly advised to dynamically initialise
   such structures using jitter_print_context_kind_make_trivial , and then only
   set the fields associated to implemented operations. */
struct jitter_print_context_kind_struct;
typedef struct jitter_print_context_kind_struct *
jitter_print_context_kind;

/* A pointer to any data needed for the context.  The user is free to cast this
   pointer to any other kind of applicable pointer; an example is the file_star
   context defined in jitter-print.c , whose functions cast
   jitter_print_context_data to FILE * in order to keep track of the underlying
   libc stream. */
typedef void *
jitter_print_context_data;

/* Return a new context kind, the function pointers fields all set to NULL.  The
   caller is supposed to call this function and then set the fields she is
   interested in within the struct pointed by the result, leaving the rest as
   NULL so that the undefined operations do nothing. */
jitter_print_context_kind
jitter_print_context_kind_make_trivial (void)
  __attribute__ ((warn_unused_result, returns_nonnull));

/* Destroy the pointed context kind.  This is mostly provided for symmetry's
   sake, and to silence valgrind: in practice context kinds will be allocated
   globally and there is no real need to destroy them at finalisation.  After
   calling this it becomes invalid to use any context of the destroyed kind. */
void
jitter_print_context_kind_destroy (jitter_print_context_kind k)
  __attribute__ ((nonnull (1)));

/* It is the user's responsibility to provide a convenient function for making a
   context of her new custom kind.  The context kinds defined in the section
   above may serve as examples: see the definition of jitter_print_initialize,
   with the caveat that the user should use
   jitter_print_context_kind_make_trivial rather than
   jitter_print_context_kind_initialize_trivial .
   See jitter/jitter-print-libtextstyle.c for an example of a context kind
   definition which does not ignore decorations.

   Every context-making function should use this function and return its result;
   the final user working on context, however, does not need to ever see this,
   and should instead allocate each context with a function appropriate for its
   kind such as jitter_print_context_make_file_star for file_star contexts. */
jitter_print_context
jitter_print_context_make (jitter_print_context_kind k,
                           jitter_print_context_data d)
  __attribute__ ((warn_unused_result, returns_nonnull,
                  nonnull (1)));

/* The fields within the struct below, along with its total size, may change in
   future versions and the user should treat the structure as containing unknown
   fields, by first allocating an instance through
   jitter_print_context_kind_make_trivial and then setting its known non-NULL
   fields. */
struct jitter_print_context_kind_struct
{
  /* All the functions here return 0 on success and a non-zero result on
     failure.

     If a field is NULL within a specific context kind then the corresponding
     operation does nothing and always returns 0, with one exception: either one
     of print_char and print_chars, or both, may be non-NULL.  If only one is
     defined then the missing operation is emulated via the other as described
     below. */

  /* Print the given character to the pointed underlying channel.  If the
     underlying primitive is interrupted by a recoverable error (for example
     of the kind where errno is set to EAGAIN) retry until success.
     If this field is NULL but print_chars is not then this functionality is
     emulated via the function pointed by the print_chars field, reading one
     char's worth of memory from a temporary copy. */
  int (* print_char) (jitter_print_context_data d, char c);

  /* Print char_no char objects read from memory starting at the address p and
     including any '\0' characters found in the interval to the underlying
     channel.  If the underlying primitive is interrupted by a recoverable error
     (for example of the kind where errno is set to EAGAIN) retry until success.
     If this field is NULL but print_char is not then this functionality is
     emulated by a loop performing multiple calls to the function pointed by
     the print_char field. */
  int (* print_chars) (jitter_print_context_data d, const char *p,
                       size_t char_no);

  /* Begin a decoration with the name, value type and value, in the pointed
     underlying channel.  This function is called when the new decoration is
     not active yet.
     The pointers received here point to internally managed memory, that the
     user should not alter. */
  int (* begin_decoration) (jitter_print_context_data d,
                            const jitter_print_decoration_name name,
                            enum jitter_print_decoration_type type,
                            const union jitter_print_decoration_value *value);

  /* End a decoration with the given name, value type and value, in the pointed
     underlying channel.  This is always called on the last begun decoration
     in LIFO order, with the decoration no longer active.
     The pointers received here point to internally managed memory, that the
     user should not alter. */
  int (* end_decoration) (jitter_print_context_data d,
                          const jitter_print_decoration_name name,
                          enum jitter_print_decoration_type type,
                          const union jitter_print_decoration_value *value);

  /* Flush the pointed underlying channel. */
  int (* flush) (jitter_print_context_data d);

  /* Destroy the pointed print context data, without closing the underlying
     channel.  No flushing is necessary here; the function pointed by the
     flush field, if any, will be called at destruction time when appropriate. */
  int (* destroy_without_flushing) (jitter_print_context_data d);
};




/* Global initialisation.
 * ************************************************************************** */

/* This function, which makes the predefined context kinds available, is
   automatically called the first time any VM is initialised.  There is no
   matching finalisation.  Not for the user. */
void
jitter_print_initialize (void);

#endif // #ifndef JITTER_PRINT_H_
