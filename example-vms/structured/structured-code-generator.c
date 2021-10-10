/* Jittery structured language example: code generator common machinery.

   Copyright (C) 2017, 2019, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of the Jitter structured-language example, distributed
   along with Jitter under the same license.

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


#include <string.h>
#include <jitter/jitter-fatal.h>

#include "structured-code-generator.h"


/* Compile-time environment: definitions.
 * ************************************************************************** */

/* The kind of binding. */
enum structured_binding_case
  {
    structured_binding_case_variable,
    structured_binding_case_temporary
  };

/* A binding for a datum held in a variable or a temporary, as known at compile
   time. */
struct structured_binding
{
  /* The kind of this binding. */
  enum structured_binding_case case_;

  /* The language-level datum location bound to a register. */
  union
  {
    /* The variable name.  The pointed string is shared with the AST, and
       doesn't need to be freed here.
       This field is only significant when case_ is
       structured_binding_case_variable . */
    structured_variable variable;

    /* A temporary index.
       This field is only significant when case_ is
       structured_binding_case_temporary . */
    structured_temporary temporary;
  };

  /* The 0-based index used to keep the datum.  This will be used as the
     index for an r-class register. */
  structured_register_index register_index;
};

/* Compile-time informations about a procedure. */
struct structured_procedure_binding
{
  /* A procedure name.  malloc-allocated, from the parser. */
  char *procedure_name;

  /* A program point: the beginning of the procedure, right before the
     callee instruction. */
  structuredvm_label label;

  /* A program point: the instruction where a caller should jump to in a tail
     call. */
  structuredvm_label label_tail;

  /* The number of arguments. */
  size_t arity;
};

/* A static environment structure contains a datum-to-register-index
   mapping.  This is used as an abstract type, the actual definition
   being in structured-code-generator.h */
struct structured_static_environment
{
  /* A dynamic array of struct structured_binding elements.  This
     is used as a stack, with the top on the right (high indices) at the bottom
     on the left (low indices). */
  struct jitter_dynamic_buffer bindings;

  /* The next unused temporary.  This is used to sequentially generate fresh
     temporary names.  Temporary names are not reused. */
  structured_temporary next_temporary;

  /* A dynamic array of struct structured_procedure_binding elements.  This
     is used as a stack, with the top on the right (high indices) at the bottom
     on the left (low indices). */
  struct jitter_dynamic_buffer procedure_bindings;
};




/* Compile-time environment: initialization and finalization.
 * ************************************************************************** */

/* Initialize the pointed static-environment struct. */
static void
structured_static_environment_initialize (struct structured_static_environment *e)
{
  jitter_dynamic_buffer_initialize (& e->bindings);
  e->next_temporary = 0;
  jitter_dynamic_buffer_initialize (& e->procedure_bindings);
}

/* Finalize the pointed static-environment struct, without freeing the struct
   itself. */
static void
structured_static_environment_finalize (struct structured_static_environment *e)
{
  jitter_dynamic_buffer_finalize (& e->bindings);
  jitter_dynamic_buffer_finalize (& e->procedure_bindings);
}

struct structured_static_environment*
structured_static_environment_make (void)
{
  struct structured_static_environment *res
    = jitter_xmalloc (sizeof (struct structured_static_environment));
  structured_static_environment_initialize (res);
  return res;
}

void
structured_static_environment_destroy (struct structured_static_environment *e)
{
  structured_static_environment_finalize (e);
  free (e);
}




/* Compile-time environment: binding access utility.
 * ************************************************************************** */

/* Given a pointer to static environment return an initial pointer to they
   payload in bindings, which is much more convenient for read access.
   The returned pointer remains valid until no more bindings are added or
   removed. */
static struct structured_binding *
structured_static_environment_to_bindings (struct structured_static_environment
                                           *e)
{
  return ((struct structured_binding *) e->bindings.region);
}

/* Given a pointer to a static environment return the number of bindings. */
static size_t
structured_static_environment_to_binding_no (const struct
                                             structured_static_environment *e)
{
  return (e->bindings.used_size / sizeof (struct structured_binding));
}

/* Given a pointer to a static environment return a pointer to the last binding.
   At least one binding is supposed to exist. */
static struct structured_binding *
structured_static_environment_last_binding (struct structured_static_environment
                                            *e)
{
  size_t binding_no = structured_static_environment_to_binding_no (e);
  if (binding_no < 1)
    jitter_fatal ("structured_static_environment_last_binding: "
                  "empty static environment");
  struct structured_binding *bindings
    = structured_static_environment_to_bindings (e);
  return bindings + (binding_no - 1);
}

/* Add a new binding to the binding buffer in the pointed static environment,
   and return a pointer to it.  The pointer will remain valid until other
   bindings are pushed or popped. */
static struct structured_binding*
structured_static_environment_push_binding (struct structured_static_environment
                                            *e)
{
  struct jitter_dynamic_buffer *bindings = & e->bindings;
  return jitter_dynamic_buffer_reserve
           (bindings, sizeof (struct structured_binding));
}

/* Remove the last binding from the binding buffer in the pointed static
   environment. */
static void
structured_static_environment_pop_binding (struct structured_static_environment
                                           *e)
{
  jitter_dynamic_buffer_pop (& e->bindings,
                             sizeof (struct structured_binding));
}

/* Bind a procedure with the given name in the static environment to the given
   labels. */
void
structured_static_environment_bind_procedure
   (struct structured_static_environment *e,
    char *procedure_name,
    structuredvm_label label,
    structuredvm_label label_tail,
    size_t arity)
{
  struct structured_procedure_binding pb;
  pb.procedure_name = procedure_name;
  pb.label = label;
  pb.label_tail = label_tail;
  pb.arity = arity;
  jitter_dynamic_buffer_push (& e->procedure_bindings,
                              & pb,
                              sizeof (struct structured_procedure_binding));
}




/* Compile-time environment: accessors.
 * ************************************************************************** */

/* The common logic underlying binding operations.  This makes the binding,
   writes in the register, and returns a pointer to the binding.  The other
   binding fields are not set here, as they depend on the binding case. */
static struct structured_binding *
structured_static_environment_bind (struct structured_static_environment *e)
{
  structured_register_index new_register
    = structured_static_environment_fresh_register (e);
  struct structured_binding *new_binding
    = structured_static_environment_push_binding (e);
  new_binding->register_index = new_register;
  return new_binding;
}

structured_register_index
structured_static_environment_bind_variable
   (struct structured_static_environment *e,
    const structured_variable v)
{
  struct structured_binding *new_binding
    = structured_static_environment_bind (e);
  new_binding->case_ = structured_binding_case_variable;
  new_binding->variable = v;
#ifdef DEBUG
  fprintf (stderr, "+ Binding %s to %%r%i\n", new_binding->variable,
           new_binding->register_index);
#endif // #ifdef DEBUG
  return new_binding->register_index;
}

structured_register_index
structured_static_environment_bind_temporary
   (struct structured_static_environment *e,
    const structured_temporary t)
{
  struct structured_binding *new_binding
    = structured_static_environment_bind (e);
  new_binding->case_ = structured_binding_case_temporary;
  new_binding->temporary = t;
#ifdef DEBUG
  fprintf (stderr, "+ Binding temporary %i to %%r%i\n",
           (int) new_binding->temporary, new_binding->register_index);
#endif // #ifdef DEBUG
  return new_binding->register_index;
}

void
structured_static_environment_unbind_variable
   (struct structured_static_environment *e,
    const structured_variable v)
{
  struct structured_binding *last_binding
    = structured_static_environment_last_binding (e);
  if (last_binding->case_ == structured_binding_case_temporary)
    jitter_fatal ("bug: unbinding variable %s before unbinding temporary %i",
                  v, (int) last_binding->temporary);
  if (strcmp (v, last_binding->variable))
    jitter_fatal ("bug: unbinding variable %s before unbinding variable %s",
                  v, last_binding->variable);
#ifdef DEBUG
  fprintf (stderr, "- Unbinding variable %s from %%r%i\n",
           last_binding->variable, (int) last_binding->register_index);
#endif // #ifdef DEBUG
  structured_static_environment_pop_binding (e);
}

void
structured_static_environment_unbind_temporary
   (struct structured_static_environment *e,
    const structured_temporary t)
{
  struct structured_binding *last_binding
    = structured_static_environment_last_binding (e);
  if (last_binding->case_ == structured_binding_case_variable)
    jitter_fatal ("bug: unbinding temporary %i before unbinding variable %s",
                  (int) t, last_binding->variable);
  if (t != last_binding->temporary)
    jitter_fatal ("bug: unbinding temporary %i before unbinding temporary %i",
                  (int) t, (int) last_binding->temporary);
#ifdef DEBUG
  fprintf (stderr, "- Unbinding temporary %i from %%r%i\n",
           (int) last_binding->temporary, (int) last_binding->register_index);
#endif // #ifdef DEBUG
  structured_static_environment_pop_binding (e);
}

bool
structured_static_environment_has (struct structured_static_environment *e,
                                   const structured_variable v)
{
  size_t binding_no = structured_static_environment_to_binding_no (e);
  struct structured_binding *bindings
    = structured_static_environment_to_bindings (e);
  int i;
  for (i = binding_no - 1; i >= 0; i --)
    if (! strcmp (bindings [i].variable, v))
      return true;
  return false;
}

structured_register_index
structured_static_environment_lookup_variable
   (struct structured_static_environment *e,
    const structured_variable v)
{
  /* Look for the register index associated to v in the binding array, starting
     from the end: the most recent binding shadows any previous binding. */
  size_t binding_no = structured_static_environment_to_binding_no (e);
  struct structured_binding *bindings
    = structured_static_environment_to_bindings (e);
  int i;
  for (i = binding_no - 1; i >= 0; i --)
    if (bindings [i].case_ == structured_binding_case_variable
        && ! strcmp (bindings [i].variable, v))
      return bindings [i].register_index;

  /* If we arrived here then v is unbound. */
  jitter_fatal ("unbound variable %s", v);
}

structured_register_index
structured_static_environment_lookup_temporary
   (struct structured_static_environment *e,
    const structured_temporary t)
{
  /* Look for the register index associated to t in the binding array.
     Differently from the variable case here the order doesn't affect
     correctness, as temporaries don't shadow each other; however searching from
     the end should be faster. */
  size_t binding_no = structured_static_environment_to_binding_no (e);
  struct structured_binding *bindings
    = structured_static_environment_to_bindings (e);
  int i;
  for (i = binding_no - 1; i >= 0; i --)
    if (bindings [i].case_ == structured_binding_case_temporary
        && bindings [i].temporary == t)
      return bindings [i].register_index;

  /* If we arrived here then t is unbound. */
  jitter_fatal ("unbound temporary %i", (int) t);
}

static structured_register_index
structured_static_environment_highest_register_index_in_use
  (struct structured_static_environment *e)
{
  /* Look for the highest-index currently used register.  Notice that highest
     must be signed, as I need to initialize the maximum to -1. */
  structured_register_index highest = -1;
  size_t binding_no = structured_static_environment_to_binding_no (e);
  struct structured_binding *bindings
    = structured_static_environment_to_bindings (e);
  int i;
  for (i = 0; i < binding_no; i ++)
    if (bindings [i].register_index > highest)
      highest = bindings [i].register_index;

  return highest;
}

structured_register_index
structured_static_environment_fresh_register (struct
                                              structured_static_environment *e)
{
  /* First pass: look for the highest-index currently used register. */
  structured_register_index highest
    = structured_static_environment_highest_register_index_in_use (e);

  /* Second pass: make a boolean array indexed by register indices, saying which
     register is being used, and fill it.  The result we are looking for will be
     highest + 1, or smaller; therefore highest + 2 is a safe array size.
     Notice that if no register is being used at this time then highest will be -1,
     and array_size will be 1: there's no need for a special case. */
  size_t binding_no = structured_static_environment_to_binding_no (e);
  struct structured_binding *bindings
    = structured_static_environment_to_bindings (e);
  size_t array_size = highest + 2;
  bool *used_registers = jitter_xmalloc (array_size * sizeof (bool));
  memset (used_registers, 0, array_size * sizeof (bool));
  int i;
  for (i = 0; i < binding_no; i ++)
    used_registers [bindings [i].register_index] = true;

  /* Third pass: look for the smallest unused register using the Boolean array. */
  for (i = 0; i < array_size; i ++)
    if (! used_registers [i])
      {
        free (used_registers);
        return i;
      }
  jitter_fatal ("boolean array not marking any register as unused: bug");
}

structured_register_index
structured_static_environment_fresh_temporary
   (struct structured_static_environment *e)
{
  return e->next_temporary ++;
}

size_t
structured_static_environment_used_register_no
   (struct structured_static_environment *e)
{
  return structured_static_environment_highest_register_index_in_use (e) + 1;
}


bool
structured_static_environment_has_procedure
   (struct structured_static_environment *e,
    char *procedure_name)
{
  size_t procedure_binding_no = (e->procedure_bindings.used_size
                                 / sizeof (struct structured_procedure_binding));
  struct structured_procedure_binding *bindings
    = (struct structured_procedure_binding *) e->procedure_bindings.region;
  int i;
  for (i = 0; i < procedure_binding_no; i ++)
    if (! strcmp (bindings [i].procedure_name, procedure_name))
      return true;
  return false;
}

/* The common factor of structured_static_environment_lookup_procedure and
   structured_static_environment_lookup_procedure_tail, returning a pointer to
   the internal binding. */
static struct structured_procedure_binding *
structured_static_environment_lookup_procedure_helper
   (struct structured_static_environment *e,
    char *procedure_name)
{
  size_t procedure_binding_no = (e->procedure_bindings.used_size
                                 / sizeof (struct structured_procedure_binding));
  struct structured_procedure_binding *bindings
    = (struct structured_procedure_binding *) e->procedure_bindings.region;
  int i;
  for (i = 0; i < procedure_binding_no; i ++)
    if (! strcmp (bindings [i].procedure_name, procedure_name))
      return bindings + i;
  jitter_fatal ("unbound procedure %s", procedure_name);
}

structuredvm_label
structured_static_environment_lookup_procedure
   (struct structured_static_environment *e,
    char *procedure_name)
{
  struct structured_procedure_binding *binding
    = structured_static_environment_lookup_procedure_helper (e, procedure_name);
  return binding->label;
}

structuredvm_label
structured_static_environment_lookup_procedure_tail
   (struct structured_static_environment *e,
    char *procedure_name)
{
  struct structured_procedure_binding *binding
    = structured_static_environment_lookup_procedure_helper (e, procedure_name);
  return binding->label_tail;
}

size_t
structured_static_environment_lookup_procedure_arity
   (struct structured_static_environment *e,
    char *procedure_name)
{
  struct structured_procedure_binding *binding
    = structured_static_environment_lookup_procedure_helper (e, procedure_name);
  return binding->arity;
}
