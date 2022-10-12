/* Jitter: custom contextual printing.

   Copyright (C) 2020, 2022 Luca Saiu
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


#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <jitter/jitter.h>
#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-string.h>

#include <jitter/jitter-print.h>




/* Internal functionality.
 * ************************************************************************** */

/* A decoration <name, type, value> tuple.  This is what the stack within
   each context structs contains. */
struct jitter_print_decoration
{
  /* The name for this decoration.  This is a malloc-allocated copy, not
     shared with data provided by the user.  */
  char *name;

  /* The decoration type, whose value indicates which of the value fields is
     significant. */
  enum jitter_print_decoration_type type;

  /* The value associated to the decoration. */
  union jitter_print_decoration_value value;
};

/* Free the resources associated to the pointed decoration. */
static void
jitter_print_decoration_finalize (struct jitter_print_decoration *d)
{
  /* Free the local copy of the name. */
  free (d->name);

  /* Free the local copy of the string value, if that is the union significant
     field. */
  if (d->type == jitter_print_decoration_type_string)
    free (d->value.string);
}

/* Initialise the pointed decoration stack, meant to be the field stack of
   a struct jitter_print_context_struct object. */
static void
jitter_print_decoration_stack_initialize (struct jitter_dynamic_buffer *s)
{
  jitter_dynamic_buffer_initialize (s);
}

/* Free the resources associated to the pointed decoration stack, meant to be
   the field stack of a struct jitter_print_context_struct object. */
static void
jitter_print_decoration_stack_finalize (struct jitter_dynamic_buffer *s)
{
  struct jitter_print_decoration *elements
    = ((struct jitter_print_decoration *)
       JITTER_DYNAMIC_BUFFER_TO_CONST_POINTER (s));
  struct jitter_print_decoration *limit
    = ((struct jitter_print_decoration *)
       JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST (s));
  struct jitter_print_decoration *element;
  for (element = elements; element < limit; element ++)
    jitter_print_decoration_finalize (element);
  jitter_dynamic_buffer_finalize (s);
}

/* Return a pointer to the bottom of the decoration stack in the pointed
   context.  The bottom may contain a valid decoration, or not. */
static struct jitter_print_decoration *
jitter_print_decoration_stack_bottom (struct jitter_print_context_struct *ct)
{
  return ((struct jitter_print_decoration *)
          JITTER_DYNAMIC_BUFFER_TO_CONST_POINTER (& ct->stack));
}

/* Return a pointer to the top decoration of the pointed context, or NULL
   if the stack is currently empty. */
static struct jitter_print_decoration *
jitter_print_decoration_stack_top (struct jitter_print_context_struct *ct)
{
  struct jitter_print_decoration *bottom
    = jitter_print_decoration_stack_bottom (ct);
  struct jitter_print_decoration *top
    = ((struct jitter_print_decoration *)
       JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST (& ct->stack)) - 1;
  if (top < bottom)
    return NULL;
  else
    return top;
}




/* Decorations.
 * ************************************************************************** */

/* Common code factoring the logic in jitter_print_begin_decoration_integer ,
   jitter_print_begin_decoration_floating_point ,
   jitter_print_begin_decoration_string , jitter_print_begin_decoration_pointer
   .  Strings, both in the decoration name and in the decoration value, if
   applicable, are cloned here: the caller should not make clones out of this
   function. */
static int
jitter_print_begin_decoration (jitter_print_context ct,
                               jitter_print_decoration_name name_orig,
                               enum jitter_print_decoration_type type,
                               union jitter_print_decoration_value value_orig)
{
  /* Make a new decoration for the context decoration stack, cloning the fields
     that need to be cloned.  Do not push it to the stack yet, so that user code
     has access to the previously active decorations. */
  struct jitter_print_decoration d;
  d.name = jitter_clone_string (name_orig);
  d.type = type;
  if (type == jitter_print_decoration_type_string)
    d.value.string = jitter_clone_string (value_orig.string);
  else
    d.value = value_orig;

  /* Perform the user action, with the declration ready but not yet active
     on the stack. */
  int res = 0;
  if (ct->kind->begin_decoration != NULL)
    res = ct->kind->begin_decoration (ct->data, d.name, d.type, & d.value);

  /* Make the new decoration active. */
  JITTER_DYNAMIC_BUFFER_PUSH (& ct->stack, struct jitter_print_decoration, & d);

  return res;
}

int
jitter_print_begin_decoration_integer (jitter_print_context ct,
                                       jitter_print_decoration_name name,
                                       jitter_int value)
{
  union jitter_print_decoration_value v;
  v.integer = value;
  return jitter_print_begin_decoration (ct, name,
                                        jitter_print_decoration_type_integer,
                                        v);
}

int
jitter_print_begin_decoration_floating_point (jitter_print_context ct,
                                              jitter_print_decoration_name name,
                                              double value)
{
  union jitter_print_decoration_value v;
  v.floating_point = value;
  return jitter_print_begin_decoration
            (ct, name, jitter_print_decoration_type_floating_point, v);
}

int
jitter_print_begin_decoration_string (jitter_print_context ct,
                                      jitter_print_decoration_name name,
                                      char *value)
{
  union jitter_print_decoration_value v;
  v.string = value;
  return jitter_print_begin_decoration (ct, name,
                                        jitter_print_decoration_type_string,
                                        v);
}

int
jitter_print_begin_decoration_pointer (jitter_print_context ct,
                                       jitter_print_decoration_name name,
                                       void *value)
{
  union jitter_print_decoration_value v;
  v.pointer = value;
  return jitter_print_begin_decoration (ct, name,
                                        jitter_print_decoration_type_pointer,
                                        v);
}

int
jitter_print_end_decoration (jitter_print_context ct,
                             jitter_print_decoration_name name)
{
  /* Check what the topmost decoration is; fail fatally if it is not what
     it is supposed to be in LIFO order. */
  struct jitter_print_decoration *d = jitter_print_decoration_stack_top (ct);
  if (d == NULL)
    jitter_fatal ("jitter_print_end_decoration: no current decoration "
                  "(expecting \"%s\")", name);
  else if (strcmp (d->name, name))
    jitter_fatal ("jitter_print_end_decoration: current decoration is \"%s\" "
                  "instead of \"%s\"", d->name, name);

  /* Pop the deoration from the stack, but do not finalise it yet: the
     user function still needs it. */
  jitter_dynamic_buffer_pop (& ct->stack,
                             sizeof (struct jitter_print_decoration));

  /* Perform the user action.  The decoration is no longer active while the
     user function runs, but the user still receives its data. */
  int res = 0;
  if (ct->kind->end_decoration != NULL)
    res = ct->kind->end_decoration (ct->data, d->name, d->type, & d->value);

  /* Now we can destroy the decoration. */
  jitter_print_decoration_finalize (d);

  return res;
}

/* End every decoration which has been begun and not ended yet, following the
   correct LIFO order.  Return 0 iff every close operation succeeds.  This is
   intended for context finalisation, in cases where the stream is closed early
   before some decorations are explicitly ended. */
static int
jitter_print_end_all_decorations (jitter_print_context ct)
{
  int res = 0;
  struct jitter_print_decoration *top;
  while ((top = jitter_print_decoration_stack_top (ct)) != NULL)
    res = res || jitter_print_end_decoration (ct, top->name);
  return res;
}


/* Decoration introspection.
 * ************************************************************************** */

/* Factor the common code of jitter_print_get_decoration and
   jitter_print_get_decoration_named , behaving like
   jitter_print_get_decoration_named iff name is not NULL. */
static void
jitter_print_get_decoration_possibly_named
   (jitter_print_context ct,
    jitter_print_decoration_name name,
    jitter_print_decoration_name *name_pp,
    enum jitter_print_decoration_type **type_pp,
    union jitter_print_decoration_value **value_pp)
{
  struct jitter_print_decoration *bottom
    = jitter_print_decoration_stack_bottom (ct);
  struct jitter_print_decoration *top
    = jitter_print_decoration_stack_top (ct);

  /* Scan the stack top-to-bottom, and stop at the first match. */
  struct jitter_print_decoration *d;
  if (top != NULL)
    for (d = top; d >= bottom; d --)
      if (name == NULL || ! strcmp (d->name, name))
        {
          * name_pp = d->name;
          * type_pp = & d->type;
          * value_pp = & d->value;
          return;
        }

  /* If we arrived here no match exists. */
  * name_pp = NULL;
  * type_pp = NULL;
  * value_pp = NULL;
}

void
jitter_print_get_decoration (jitter_print_context ct,
                             jitter_print_decoration_name *name_pp,
                             enum jitter_print_decoration_type **type_pp,
                             union jitter_print_decoration_value **value_pp)
{
  jitter_print_get_decoration_possibly_named (ct, NULL, name_pp,
                                              type_pp, value_pp);
}

void
jitter_print_get_decoration_named
   (jitter_print_context ct,
    jitter_print_decoration_name name,
    enum jitter_print_decoration_type **type_pp,
    union jitter_print_decoration_value **value_pp)
{
  char *found_name_p;
  jitter_print_get_decoration_possibly_named (ct, name, & found_name_p,
                                              type_pp, value_pp);
}




/* Convenience functions for known decorations.
 * ************************************************************************** */

int
jitter_print_begin_class (jitter_print_context ct, jitter_print_class c)
{
  return jitter_print_begin_decoration_string
            (ct,
             JITTER_PRINT_DECORATION_NAME_CLASS,
             c);
}

int
jitter_print_end_class (jitter_print_context ct)
{
  return jitter_print_end_decoration (ct,
                                      JITTER_PRINT_DECORATION_NAME_CLASS);
}

int
jitter_print_begin_hyperlink (jitter_print_context ct, jitter_print_url url)
{
  /* Prevent nesting. */
  char *active_hyperlink = jitter_print_get_hyperlink (ct);
  if (active_hyperlink != NULL)
    jitter_fatal ("jitter_print_begin_hyperlink: hyperlink already active "
                  "(\"%s\")", active_hyperlink);

  return jitter_print_begin_decoration_string
            (ct,
             JITTER_PRINT_DECORATION_NAME_HYPERLINK,
             url);
}

int
jitter_print_end_hyperlink (jitter_print_context ct)
{
  return jitter_print_end_decoration (ct,
                                      JITTER_PRINT_DECORATION_NAME_HYPERLINK);
}

/* Common code factoring jitter_print_get_class and jitter_print_get_hyperlink
   . */
static char *
jitter_print_get_string_decoration (jitter_print_context ct,
                                    jitter_print_decoration_name name)
{
  enum jitter_print_decoration_type *type_p;
  union jitter_print_decoration_value *value_p;
  jitter_print_get_decoration_named (ct, name, & type_p, & value_p);
  if (type_p == NULL)
    return NULL;
  else if (* type_p != jitter_print_decoration_type_string)
    jitter_fatal ("jitter_print_get_string_decoration: non-string value for "
                  "decoration %s", name);
  else
    return value_p->string;
}

char *
jitter_print_get_class (jitter_print_context ct)
{
  return jitter_print_get_string_decoration
            (ct, JITTER_PRINT_DECORATION_NAME_CLASS);
}
char *
jitter_print_get_hyperlink (jitter_print_context ct)
{
  return jitter_print_get_string_decoration
            (ct, JITTER_PRINT_DECORATION_NAME_HYPERLINK);
}



/* Functionality available in every context.
 * ************************************************************************** */

int
jitter_print_flush (jitter_print_context ct)
{
  if (ct->kind->flush != NULL)
    return ct->kind->flush (ct->data);
  else
    return 0;
}

int
jitter_print_context_destroy (jitter_print_context ct)
{
  int res_end_decorations = jitter_print_end_all_decorations (ct);
  int res_flush = jitter_print_flush (ct);
  int res_destroy = 0;
  if (ct->kind->destroy_without_flushing)
    res_destroy = ct->kind->destroy_without_flushing (ct->data);
  jitter_print_decoration_stack_finalize (& ct->stack);
  free (ct);
  return res_end_decorations || res_flush || res_destroy;
}




/* Printing with contexts.
 * ************************************************************************** */

/* The size of a buffer large enough to hold the printed representation of
   any numeric or pointer type, in chars. */
#define JITTER_PRINT_MEMORY_BUFFER_SIZE                                       \
  /* The radix associated to the least economical space representation is 2.  \
     In binary a 64-bit word takes up to 64 digits, plus one for the '\0'     \
     terminator.  No sane configuration will print pointers in binary, so     \
     pointers will be more compact even if printed with a prefix. */          \
  65

/* Internal emulation of the print_char behaviour within struct
   jitter_print_context_kind_struct , to be used when print_char is NULL but
   print_chars is not. */
static int
jitter_print_char_as_a_size_1_chars (jitter_print_context ct, char c)
{
  return ct->kind->print_chars (ct->data, & c, 1);
}

/* Internal emulation of the print_chars behaviour within struct
   jitter_print_context_kind_struct , to be used when print_chars is NULL but
   print_char is not. */
static int
jitter_print_chars_in_a_loop (jitter_print_context ct, const char *p,
                              size_t char_no)
{
  int res = 0;
  int i;
  for (i = 0; i < char_no; i ++)
    {
      res = ct->kind->print_char (ct->data, p [i]);
      if (res != 0)
        break;
    }
  return res;
}

int
jitter_print_char (jitter_print_context ct, char c)
{
  if (ct->kind->print_char != NULL)
    return ct->kind->print_char (ct->data, c);
  else if (ct->kind->print_chars != NULL)
    return jitter_print_char_as_a_size_1_chars (ct, c);
  else
    /* Neither user function is defined.  Print nothing. */
    return 0;
}

int
jitter_print_chars (jitter_print_context ct, const char *p, size_t char_no)
{
  if (ct->kind->print_chars != NULL)
    return ct->kind->print_chars (ct->data, p, char_no);
  else if (ct->kind->print_char != NULL)
    return jitter_print_chars_in_a_loop (ct, p, char_no);
  else
    /* Neither user function is defined.  Print nothing. */
    return 0;
}

int
jitter_print_char_star (jitter_print_context ct, const char *s)
{
  return jitter_print_chars (ct, s, strlen (s));
}

/* The only function we need to print signed data. */
int
jitter_print_long_long (jitter_print_context ct, int radix, jitter_long_long x)
{
  if (x < 0)
    {
      int minus_error = jitter_print_char (ct, '-');
      if (minus_error != 0)
        return minus_error;
      jitter_ulong_long absolute_value = (jitter_ulong_long) (- x);
      return jitter_print_ulong_long (ct, radix, absolute_value);
    }
  else
    return jitter_print_ulong_long (ct, radix, x);
}

/* This is the core of every integer printing function. */
int
jitter_print_ulong_long (jitter_print_context ct, int radix, jitter_ulong_long x)
{
  /* This is the only place where we need to validate the radix.  Every other
     integer printing function, even when printing zero, ends up calling
     this function. */
  if (radix < 2 || radix > 36)
    jitter_fatal ("jitter printing: invalid radix %i", radix);

  if (x == 0)
    return jitter_print_char (ct, '0');
  else
    {
      /* Build a string containing the *reversed* digits in memory; we
         do not know how long it will be yet. */
      char reversed_digits [JITTER_PRINT_MEMORY_BUFFER_SIZE];
      int i = 0;
      while (x != 0)
        {
          const char *all_digits = "0123456789abcdefghijklmnopqrstuvwxyz";
          reversed_digits [i] = all_digits [x % radix];
          x /= radix;
          i ++;
        }
      /* Print the characters from the string, read backwards.  Notice that at
         the beginning i is the index of the leftmost element *out* of the
         string. */
      int res /* to silence a GCC warning: the initialisation is not actually
                 needed, since this loop cannot be empty: since x is not zero in
                 this else branch the string will have at least one digit.  I do
                 not particularly blame GCC for not being able to prove this
                 complex property. */
        = 0;
      for (i --; i >= 0; i --)
        {
          res = jitter_print_char (ct, reversed_digits [i]);
          if (res != 0)
            break;
        }
      return res;
    }
}

/* The functions for printing out every other integer type are based on the two
   functions above. */
int
jitter_print_short (jitter_print_context ct, int radix, short x)
{
  return jitter_print_long_long (ct, radix, x);
}
int
jitter_print_int (jitter_print_context ct, int radix, int x)
{
  return jitter_print_long_long (ct, radix, x);
}
int
jitter_print_long (jitter_print_context ct, int radix, long x)
{
  return jitter_print_long_long (ct, radix, x);
}
int
jitter_print_ushort (jitter_print_context ct, int radix, unsigned short x)
{
  return jitter_print_ulong_long (ct, radix, x);
}
int
jitter_print_uint (jitter_print_context ct, int radix, unsigned int x)
{
  return jitter_print_ulong_long (ct, radix, x);
}
int
jitter_print_ulong (jitter_print_context ct, int radix, unsigned long x)
{
  return jitter_print_ulong_long (ct, radix, x);
}

/* For pointer and floating-point types we rely on sprintf.  Float values are
   printed just like double values anyway. */
int
jitter_print_pointer (jitter_print_context ct, void *p)
{
  char buffer [JITTER_PRINT_MEMORY_BUFFER_SIZE];
  sprintf (buffer, "%p", p);
  return jitter_print_char_star (ct, buffer);
}
int
jitter_print_float (jitter_print_context ct, float x)
{
  return jitter_print_double (ct, x);
}
int
jitter_print_double (jitter_print_context ct, double x)
{
  char buffer [JITTER_PRINT_MEMORY_BUFFER_SIZE];
  sprintf (buffer, "%f", x);
  return jitter_print_char_star (ct, buffer);
}
#if defined (JITTER_HAVE_LONG_DOUBLE)
int
jitter_print_long_double (jitter_print_context ct, long double x)
{
  char buffer [JITTER_PRINT_MEMORY_BUFFER_SIZE];
  sprintf (buffer, "%Lf", x);
  return jitter_print_char_star (ct, buffer);
}
#endif // #if defined (JITTER_HAVE_LONG_DOUBLE)




/* Defining new print context kinds.
 * ************************************************************************** */

/* Initialize the pointed context kind like
   jitter_print_context_kind_make_trivial would, without allocating a new
   buffer.  The user is not supposed to use this, as she should treat the the
   context kind as an opaque data type; however this is convenient for keeping
   predefined context kinds in global variables, which can be initialised and
   never finalized, without getting distracting warnings from valgrind. */
static void
jitter_print_context_kind_initialize_trivial (jitter_print_context_kind k)
{
  k->print_char = NULL;
  k->print_chars = NULL;
  k->begin_decoration = NULL;
  k->end_decoration = NULL;
  k->flush = NULL;
  k->destroy_without_flushing = NULL;
}

jitter_print_context_kind
jitter_print_context_kind_make_trivial (void)
{
  jitter_print_context_kind res
    = jitter_xmalloc (sizeof (struct jitter_print_context_kind_struct));
  jitter_print_context_kind_initialize_trivial (res);
  return res;
}

void
jitter_print_context_kind_destroy (jitter_print_context_kind k)
{
  free (k);
}

jitter_print_context
jitter_print_context_make (jitter_print_context_kind k, jitter_print_context_data d)
{
  jitter_print_context res
    = jitter_xmalloc (sizeof (struct jitter_print_context_struct));

  jitter_print_decoration_stack_initialize (& res->stack);
  res->kind = k;
  res->data = d;

  return res;
}




/* Predefined print context kinds: file_star print context.
 * ************************************************************************** */

/* The one file_star context kind global, allocated statically rather than
   with malloc. */
static struct jitter_print_context_kind_struct
jitter_print_context_kind_file_star_struct;
static jitter_print_context_kind
jitter_print_context_kind_file_star
  = & jitter_print_context_kind_file_star_struct;

/* Non-trivial context kind operations. */
static int
jitter_print_context_file_star_print_char (jitter_print_context_data d,
                                           char c)
{
  return fputc (c, (FILE *) d) == EOF;
}
static int
jitter_print_context_file_star_flush (jitter_print_context_data d)
{
  return fflush ((FILE *) d);
}
/* There is no need for a function
   jitter_print_context_file_star_destroy_without_flushing .  There is nothing
   to destroy, since the FILE * object was given by the user, and the file is
   not to be closed here. */

/* Global initialisation for this context kind. */
static void
jitter_print_context_kind_file_star_initialize (void)
{
  jitter_print_context_kind_initialize_trivial
     (jitter_print_context_kind_file_star);
  jitter_print_context_kind_file_star->print_char
    = jitter_print_context_file_star_print_char;
  jitter_print_context_kind_file_star->flush
    = jitter_print_context_file_star_flush;
}

/* The user-visible function. */
jitter_print_context
jitter_print_context_make_file_star (FILE *f)
{
  return jitter_print_context_make (jitter_print_context_kind_file_star, f);
}




/* Predefined print context kinds: file-descriptor print context.
 * ************************************************************************** */

/* A file descriptor in memory, as the datum pointed within a file-descriptor
   print context.  This struct serves to make the code simple and clean, but
   it would also be possible to cast between pointer and int and store the
   descriptor directly in place of the pointer. */
struct jitter_print_context_fd
{
  /* The underlying file descriptor. */
  int fd;
};

/* The one fd context kind global, allocated statically rather than
   with malloc. */
static struct jitter_print_context_kind_struct
jitter_print_context_kind_fd_struct;
static jitter_print_context_kind
jitter_print_context_kind_fd
  = & jitter_print_context_kind_fd_struct;

/* Non-trivial context kind operations. */
static int
jitter_print_context_fd_print_chars (jitter_print_context_data d,
                                     const char *p, size_t char_no)
{
  int fd = ((struct jitter_print_context_fd *)d)->fd;
  const char *remaining_part = p;
  int remaining_part_length = char_no;
  int res = 0;
  while (remaining_part_length > 0)
    {
      res = write (fd, remaining_part, remaining_part_length);
      if (res == -1)
        {
          /* Break from the loop if the error is not recoverable here.  If the
             problem can be fixed by simply trying again then do nothing without
             updating remaining_part or remaining_part_length, and remain in the
             loop. */
          if (errno != EAGAIN && errno != EINTR)
            break;
        }
      else
        {
          /* The entire string may or may not have been printed at this point.
             Advance to the remaining_part pointer and keep track of how many
             characters have not been printed yet; this may or may not end the
             loop. */
          remaining_part += res;
          remaining_part_length -= res;
        }
    };

  /* Return 0 iff res does not currently contain an error result. */
  return res == -1 ? res : 0;
}
static int
jitter_print_context_fd_destroy_without_flushing
   (jitter_print_context_data d)
{
  free (d);

  /* Do *not* call close here.  The descriptor is for the user to close. */
  return 0;
}

/* Global initialisation for this context kind. */
static void
jitter_print_context_kind_fd_initialize (void)
{
  jitter_print_context_kind_initialize_trivial (jitter_print_context_kind_fd);
  jitter_print_context_kind_fd->print_chars
    = jitter_print_context_fd_print_chars;
  jitter_print_context_kind_fd->destroy_without_flushing
    = jitter_print_context_fd_destroy_without_flushing;
}

/* The user-visible function. */
jitter_print_context
jitter_print_context_make_fd (int fd)
{
  struct jitter_print_context_fd *data
    = jitter_xmalloc (sizeof (struct jitter_print_context_fd));
  data->fd = fd;
  return jitter_print_context_make (jitter_print_context_kind_fd, data);
}




/* Predefined print context kinds: memory print context.
 * ************************************************************************** */

/* The "channel" for a memory print context is in fact a dynamically
   allocated buffer. */
struct jitter_print_context_memory
{
  /* A dynamic buffer containing char elements. */
  struct jitter_dynamic_buffer db;
};

/* The one memory context kind global, allocated statically rather than
   with malloc. */
static struct jitter_print_context_kind_struct
jitter_print_context_kind_memory_struct;
static jitter_print_context_kind
jitter_print_context_kind_memory
  = & jitter_print_context_kind_memory_struct;

/* Non-trivial context kind operations. */
static int
jitter_print_context_memory_print_chars (jitter_print_context_data d,
                                         const char *p, size_t char_no)
{
  struct jitter_dynamic_buffer *db
    = & ((struct jitter_print_context_memory *) d)->db;
  jitter_dynamic_buffer_push (db, p, char_no);
  return 0;
}
static int
jitter_print_context_memory_destroy_without_flushing
   (jitter_print_context_data d)
{
  /* Destroy the dynamic buffer content holding printed data, and the memory
     held by the dyanmic buffer struct itself. */
  jitter_dynamic_buffer_finalize (& ((struct jitter_print_context_memory *)
                                     d)->db);
  free (d);
  return 0;
}

/* Global initialisation for this context kind. */
static void
jitter_print_context_kind_memory_initialize (void)
{
  jitter_print_context_kind_initialize_trivial
     (jitter_print_context_kind_memory);
  jitter_print_context_kind_memory->print_chars
    = jitter_print_context_memory_print_chars;
  jitter_print_context_kind_memory->destroy_without_flushing
    = jitter_print_context_memory_destroy_without_flushing;
}

/* Return a fresh memory context. */
jitter_print_context
jitter_print_context_make_memory (void)
{
  struct jitter_print_context_memory *data
    = jitter_xmalloc (sizeof (struct jitter_print_context_memory));
  jitter_dynamic_buffer_initialize (& data->db);
  return jitter_print_context_make (jitter_print_context_kind_memory, data);
}

char *
jitter_print_context_get_memory (jitter_print_context c,
                                 size_t *length_p)
{
  /* Fail if c is not in fact a string print context. */
  if (c->kind != jitter_print_context_kind_memory)
    jitter_fatal ("jitter_print_context_get_memory: not a memory print context");

  /* Copy any chars which have been printed including chars beyond the first
     '\0', if any -- this is why this does not use strncpy; unconditionally add
     a final '\0' at the end. */
  struct jitter_dynamic_buffer *db
    = & ((struct jitter_print_context_memory *) c->data)->db;
  size_t length = JITTER_DYNAMIC_BUFFER_USED_SIZE (db);
  char *res = jitter_xmalloc (length + 1);
  memcpy (res, JITTER_DYNAMIC_BUFFER_TO_CONST_POINTER (db), length);
  res [length] = '\0';

  /* The length is important in case of binary output, when the user printed
     '\0' chars. */
  if (length_p != NULL)
    * length_p = length;
  return res;
}




/* Global initialisation.
 * ************************************************************************** */

void
jitter_print_initialize (void)
{
  static /*jitter_bool*/bool jitter_print_already_initialized = false;

  if (jitter_print_already_initialized)
    return;

  /* Initialise predefined context types. */
  jitter_print_context_kind_file_star_initialize ();
  jitter_print_context_kind_fd_initialize ();
  jitter_print_context_kind_memory_initialize ();

  jitter_print_already_initialized = true;
}
