/* Jitter: VM generation-time data structures.

   Copyright (C) 2017, 2018, 2019, 2020, 2021 Luca Saiu
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


/* Include the Gnulib header. */
#include <config.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <c-ctype.h>

#include <xalloc.h>
#include <gl_array_list.h>
#include <gl_xlist.h>

#include "jitterc-mangle.h"
#include "jitterc-utility.h"
#include "jitterc-vm.h"
#include "jitterc-rewrite.h"

#include <jitter/jitter-malloc.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-hash.h>
#include <jitter/jitter-string.h>


/* A number of characters sufficient to hold the printed representation of any
   parameter in a specialized instruction, without counting specalized
   literals, and without counting any terminating '\0'. */
#define JITTERC_MAX_ARGUMENT_PRINTED_SIZE_PREFIX_SIZE  6

/* A number of characters sufficient to hold the printed representation of any
   residual parameter.  Since even the largest 64-bit integers printed in
   decimal fit in 20 digits this is safe. */
#define JITTERC_MAXIMUM_ARGUMENT_PRINTED_SIZE 25


struct jitterc_register*
jitterc_make_register (struct jitterc_register_class *class, unsigned index)
{
  struct jitterc_register *res
    = xmalloc (sizeof (struct jitterc_register));
  res->class = class;
  res->index = index;
  return res;
}

struct jitterc_literal*
jitterc_make_literal (enum jitterc_literal_type type,
                      union jitterc_literal_value value)
{
  struct jitterc_literal *res
    = xmalloc (sizeof (struct jitterc_literal));
  res->type = type;
  res->value = value;
  return res;
}




/* Implicit wraps.
 * ************************************************************************** */

/* Add implicitly defined wrappers for globals and functions. */
static void
jitterc_add_implicit_wraps (struct jitterc_vm *vm)
{
  gl_list_add_last (vm->wrapped_globals,
                    jitter_clone_string ("vmprefix_states"));
}




/* Implicit instructions.
 * ************************************************************************** */

/* Add a new VM instruction described by the arguments to the given VM,
   and return it. */
static struct jitterc_instruction *
jitterc_add_implicit_instruction (struct jitterc_vm *vm,
                                  const char *name,
                                  const char *c_code)
{
  struct jitterc_instruction *res;
  jitterc_vm_append_instruction (vm, res = jitterc_make_instruction ());
  res->name = jitter_clone_string (name);
  res->mangled_name = jitterc_mangle (res->name);
  res->hotness = jitterc_hotness_cold;
  res->relocatability = jitterc_relocatability_relocatable;
  res->branchingness = jitterc_branchingness_branching;
  res->callerness = jitterc_callerness_non_caller;
  res->calleeness = jitterc_calleeness_non_callee;
  res->returningness = jitterc_returningness_non_returning;
  res->has_fast_labels = false;
  res->code = jitter_clone_string (c_code);

  return res;
}

/* Add all the implicitly-defined instructions to the given VM.  This is called
   at VM initialization. */
static void
jitterc_add_implicit_instructions (struct jitterc_vm *vm)
{
  jitterc_add_implicit_instruction (vm, "exitvm", "JITTER_EXIT();");
  jitterc_add_implicit_instruction (vm, "unreachable", "");
}




/* VM initialization.
 * ************************************************************************** */

struct jitterc_vm*
jitterc_make_vm (void)
{
  struct jitterc_vm *res = xmalloc (sizeof (struct jitterc_vm));
  /* Initialize the fields with intentionally invalid values just to prevent
     distractions later. */
  res->source_file_name = NULL;
  res->directory = NULL;
  res->tmp_directory = NULL;
  res->template_directory = NULL;
  res->max_fast_register_no_per_class = -2;
  res->max_nonresidual_literal_no = -2;

  /* Use sensible defaults for fields customizable by the user. */
  res->lower_case_prefix = "vm";
  res->upper_case_prefix = "VM";
  res->name = NULL;
  res->generate_line = true;
  res->register_classes = jitterc_make_empty_list ();

  res->wrapped_functions = jitterc_make_empty_list ();
  res->wrapped_globals = jitterc_make_empty_list ();

  /* These need to be heap-allocated and distinct from one another, since we may
     concatenate an old part with something new and free the two parts. */
  res->initial_header_c_code = jitter_clone_string ("");
  res->initial_vm1_c_code = jitter_clone_string ("");
  res->initial_vm2_c_code = jitter_clone_string ("");
  res->initial_vm_main_c_code = jitter_clone_string ("");
  res->early_header_c_code = jitter_clone_string ("");
  res->late_header_c_code = jitter_clone_string ("");
  res->printer_c_code = jitter_clone_string ("");
  res->rewriter_c_code = jitter_clone_string ("");
  res->early_c_code = jitter_clone_string ("");
  res->before_main_c_code = jitter_clone_string ("");
  res->initialization_c_code = jitter_clone_string ("");
  res->finalization_c_code = jitter_clone_string ("");
  res->state_early_c_code = jitter_clone_string ("");
  res->state_backing_struct_c_code = jitter_clone_string ("");
  res->state_runtime_struct_c_code = jitter_clone_string ("");
  res->state_initialization_c_code = jitter_clone_string ("");

  /* This is NULL unless explicitly specified, differently from similar fields:
     the reason is that the default is non-empty: the default for state reset is
     the composition of state finalisation and state initialisation. */
  res->state_reset_c_code = NULL;

  res->state_finalization_c_code = jitter_clone_string ("");
  res->instruction_beginning_c_code = jitter_clone_string ("");
  res->instruction_end_c_code = jitter_clone_string ("");

  /* No files have been written yet. */
  res->written_file_names = jitterc_make_empty_list ();

  /* There are no instructions or specialized instructions yet, but we make
     emtpy lists to be able to add later. */
  res->instructions = jitterc_make_empty_list ();
  res->specialized_instructions = jitterc_make_empty_list ();
  res->specialized_instruction_forest = jitterc_make_empty_list ();

  /* The name_to_instruction hash table is empty.  It will be filled after
     sorting, when all the instructions are in place.  This, like many other
     data structures in jitterc, is never finalized. */
  jitter_hash_initialize (& res->name_to_instruction);

  /* There are no rewrite rules yet. */
  res->rewrite_rules = jitterc_make_empty_list ();

  /* There are no stacks yet. */
  res->stacks = jitterc_make_empty_list ();

  /* The maximal residual arity is still zero; every time we add a specialized
     instruction this will be compared with the residual arity of the new
     specialized instruction, and set to the higher value of the two. */
  res->max_residual_arity = 0;

  /* Add implicit wrapped globals and functions. */
  jitterc_add_implicit_wraps (res);

  /* Add the implicit instructions. */
  jitterc_add_implicit_instructions (res);

  return res;
}

void
jitterc_vm_add_setting (struct jitterc_vm *vm,
                        const char *name,
                        const char *value)
{
  if (! strcmp (name, "prefix"))
    {
      vm->lower_case_prefix = jitter_clone_string (value);
      /* Compute the upper-case prefix, and at the same time make sure that the
         lower-case version is valid. */
      vm->upper_case_prefix = jitter_clone_string (value);
      int i;
      for (i = 0; vm->upper_case_prefix [i] != '\0'; i ++)
        {
          if (! c_isalpha (vm->upper_case_prefix [i]))
            jitter_fatal ("the prefix contains the non-alphabetic character "
                          "'%c' (0x%x)", vm->upper_case_prefix [i],
                          vm->upper_case_prefix [i]);
          if (! c_islower (vm->upper_case_prefix [i]))
            jitter_fatal ("the prefix contains the upper-case character '%c' "
                          "(0x%x)", vm->upper_case_prefix [i],
                          vm->upper_case_prefix [i]);
          vm->upper_case_prefix [i] = toupper (vm->upper_case_prefix [i]);
        }
    }
  else if (! strcmp (name, "name"))
    vm->name = jitter_clone_string (value);
  else
    jitter_fatal ("unknown setting %s", name);
}

/* Return true iff the given character is valid as a register class or stack
   class character. */
static bool
jitterc_valid_register_or_class_character (char c)
{
  /* This assumes characters to be sorted alphabetically and to be contiguous in
     their representation, which should hopefully be true everywhere. */
  return c >= 'a' && c <= 'z';
}

/* Behave like jitterc_lookup_register_class in case of success, but return NULL
   in case of failure instead of failing fatally. */
static struct jitterc_register_class*
jitterc_lookup_register_class_or_NULL (const struct jitterc_vm *vm, char c)
{
  /* I expect register classes to be very few in number; it's useless to use
     elaborate data structure for this. */
  int i;
  for (i = 0; i < gl_list_size (vm->register_classes); i ++)
    {
      struct jitterc_register_class *a_class
        = ((struct jitterc_register_class *)
           gl_list_get_at (vm->register_classes, i));
      if (a_class->letter == c)
        return a_class;
    }

  /* We didn't find the register class. */
  return NULL;
}

struct jitterc_register_class*
jitterc_lookup_register_class (const struct jitterc_vm *vm, char c)
{
  struct jitterc_register_class *res
    = jitterc_lookup_register_class_or_NULL (vm, c);

  if (res != NULL)
    return res;
  else
    jitter_fatal ("no register class has the character '%c'", c);
}

struct jitterc_register_class*
jitterc_make_register_class (void)
{
  // FIXME: validate fields later.
  /* Allocate an initialize the register class descriptor.  Some fields have
     intentionally invalid defaults which must be changed, or set to a natural
     default. */
  struct jitterc_register_class *res
    = xmalloc (sizeof (struct jitterc_register_class));
  res->letter = '\0';             /* INvalid default. */
  res->long_name = NULL;          /* INvalid default. */
  res->c_type = NULL;             /* INvalid default. */
  res->c_initial_value = NULL;    /* VALID default. */
  res->fast_register_no = -1;     /* INvalid default. */
  res->use_slow_registers = -1;   /* INvalid default. */

  /* Return the new element. */
  return res;
}

void
jitterc_vm_register_class_set_letter (struct jitterc_register_class* rc,
                                      char letter)
{
  if (rc->letter != '\0')
    jitter_fatal ("register class letter '%c' set twice", letter);
  rc->letter = letter;
}
void
jitterc_vm_register_class_set_long_name (struct jitterc_register_class* rc,
                                         const char *long_name)
{
  if (rc->long_name != NULL)
    jitter_fatal ("register class: long name set twice");
  rc->long_name = jitter_clone_string (long_name);
}
void
jitterc_vm_register_class_set_c_type (struct jitterc_register_class* rc,
                                      const char *c_type)
{
  if (rc->c_type != NULL)
    jitter_fatal ("register class: c-type set twice");
  rc->c_type = jitter_clone_string (c_type);
}
void
jitterc_vm_register_class_set_c_initial_value (struct jitterc_register_class* rc,
                                               const char *c_initial_value)
{
  if (rc->c_initial_value != NULL)
    jitter_fatal ("register class: c-initial_value set twice");
  rc->c_initial_value = jitter_clone_string (c_initial_value);
}
void
jitterc_vm_register_class_set_fast_register_no (struct jitterc_register_class* rc,
                                                int fast_register_no)
{
  if (rc->fast_register_no != -1)
    jitter_fatal ("register class: fast-register-no set twice");
  if (fast_register_no < 0)
    jitter_fatal ("register class: fast-register-no must be non-negative");
  rc->fast_register_no = fast_register_no;
}
void
jitterc_vm_register_class_set_use_slow_registers (struct jitterc_register_class
                                                  * rc, int use_slow_registers)
{
  if (rc->use_slow_registers != -1)
    jitter_fatal ("register class: slow register use set twice");
  rc->use_slow_registers = use_slow_registers;
}

/* Destructively change the case of the pointed '\0'-terminated string, either
   to lowercase or to uppercase according to the value of to_lower.  Fail
   fatally if any of the following is true:
   - the pointer is NULL;
   - the string is empty;
   - the first character is not an ASCII letter;
   - any character is not an ASCII letter or number. */
static void
jitterc_change_case (char *s, bool to_lower, const char *name_for_errors)
{
  if (s == NULL)
    jitter_fatal ("%s is empty", name_for_errors);
  if (* s == '\0')
    jitter_fatal ("%s is empty", name_for_errors);
  if (! c_isalpha (* s))
    jitter_fatal ("%s begins with '%c' which is not an ASCII letter",
                  name_for_errors, * s);
  char * p;
  for (p = s; * p != '\0'; p ++)
    {
      if (* p == '-')
        * p = '_';
      if (! c_isalpha (* p) && *p != '_')
        jitter_fatal ("%s \"%s\" contains the non-alphabetic non-'_' non-'-' "
                      " character '%c' (0x%x)", name_for_errors, s,
                      * p, *p);
      if (to_lower)
        * p = tolower (* p);
      else
        * p = toupper (* p);
    }
}

/* Fail fatally if the given letter or lower-case long name is already in use
   for some existing register class or stack in the pointed VM. */
static void
jitterc_ensure_register_class_or_stack_consistency (struct jitterc_vm *vm,
                                                    char letter,
                                                    const char *
                                                    lower_case_long_name)
{
  int i;
  for (i = 0; i < gl_list_size (vm->register_classes); i ++)
    {
      const struct jitterc_register_class *other
        = gl_list_get_at (vm->register_classes, i);
      if (! strcmp (lower_case_long_name, other->lower_case_long_name))
        jitter_fatal ("duplicate name \"%s\" in use by register class",
                      lower_case_long_name);
      if (letter == other->letter)
        jitter_fatal ("duplicate letter \'%c\' in use by register class",
                      letter);
    }
  for (i = 0; i < gl_list_size (vm->stacks); i ++)
    {
      const struct jitterc_stack *other
        = gl_list_get_at (vm->stacks, i);
      if (! strcmp (lower_case_long_name, other->lower_case_long_name))
        jitter_fatal ("duplicate name \"%s\", in use by stack",
                      lower_case_long_name);
      if (letter == other->letter)
        jitter_fatal ("duplicate letter \'%c\' in use by stack", letter);
    }
}

void
jitterc_vm_add_register_class (struct jitterc_vm *vm,
                               struct jitterc_register_class *rc)
{
  /* Fail if any fields which requires initialisation was not initialised. */
  if (rc->letter == '\0')
    jitter_fatal ("register class without a letter");
  if (! jitterc_valid_register_or_class_character (rc->letter))
    jitter_fatal ("invalid register class letter '%c'", rc->letter);
  if (rc->long_name == NULL)
    {
      char long_name [100];
      sprintf (long_name, "register_class_%c", rc->letter);
      rc->long_name = jitter_clone_string (long_name);
    }

  /* Set any remaining uninitialised value to a reasonable default. */
  if (rc->fast_register_no == -1)
    rc->fast_register_no = 0;
  if (rc->c_type == NULL)
    rc->c_type = "union jitter_word";
  if (rc->use_slow_registers == -1)
    rc->use_slow_registers = 1;

  /* Set fields which are always uninitalised at this point to a reasonable
     default. */
  rc->lower_case_long_name = jitter_clone_string (rc->long_name);
  rc->upper_case_long_name = jitter_clone_string (rc->long_name);
  jitterc_change_case (rc->lower_case_long_name, true,
                       "register class long name");
  jitterc_change_case (rc->upper_case_long_name, false,
                       "register class long name");

  /* Check that there are no name clashes with other register classes or with
     stacks. */
  jitterc_ensure_register_class_or_stack_consistency (vm,
                                                      rc->letter,
                                                      rc->lower_case_long_name);

  /* Add the register class to the VM. */
  gl_list_add_last (vm->register_classes, rc);
}

struct jitterc_stack *
jitterc_vm_make_stack (void)
{
  /* Allocate a stack descriptor with default values, some of which are made
     intentionally invalid at initialisation and need to be changed. */
  struct jitterc_stack *stack
    = xmalloc (sizeof (struct jitterc_stack));
  stack->letter = '\0';                            /* INvalid. */
  stack->c_type = NULL;                            /* INvalid. */
  stack->long_name = NULL;                         /* INvalid. */
  stack->c_initial_value = NULL;                   /* VALID. */
  stack->element_no = -1;                          /* INvalid. */
  stack->implementation
    = jitterc_stack_implementation_uninitialized;  /* INvalid. */
  stack->guard_underflow = -1;                     /* INvalid. */
  stack->guard_overflow = -1;                      /* INvalid. */

  return stack;
}

void
jitterc_vm_stack_set_letter (struct jitterc_stack *s,
                             char letter)
{
  if (s->letter != '\0')
    jitter_fatal ("stack letter '%c' set twice", letter);
  s->letter = letter;
}
void
jitterc_vm_stack_set_long_name (struct jitterc_stack *s,
                                const char *long_name)
{
  if (s->long_name != NULL)
    jitter_fatal ("stack: long name set twice");
  s->long_name = jitter_clone_string (long_name);
}
void
jitterc_vm_stack_set_c_element_type (struct jitterc_stack *s,
                                     const char *c_type)
{
  if (s->c_type != NULL)
    jitter_fatal ("stack: c-type set twice");
  s->c_type = jitter_clone_string (c_type);
}
void
jitterc_vm_stack_set_element_no (struct jitterc_stack *s,
                                 jitter_int element_no)
{
  if (s->element_no != -1)
    jitter_fatal ("stack: element-no set twice");
  if (element_no <= 0)
    jitter_fatal ("stack: element-no must be positive");
  s->element_no = element_no;
}
void
jitterc_vm_stack_set_c_initial_value (struct jitterc_stack *s,
                                      const char *c_initial_value)
{
  if (s->c_initial_value != NULL)
    jitter_fatal ("stack: c-initial_value set twice");
  s->c_initial_value = jitter_clone_string (c_initial_value);
}
void
jitterc_vm_stack_set_implementation (struct jitterc_stack * s,
                                     enum jitterc_stack_implementation i)
{
  if (s->implementation != jitterc_stack_implementation_uninitialized)
    jitter_fatal ("stack: implementation set twice");
  s->implementation = i;
}
void
jitterc_vm_stack_set_guard_underflow (struct jitterc_stack * s,
                                      int value)
{
  if (s->guard_underflow != -1)
    jitter_fatal ("stack: guard-underflow set twice");
  s->guard_underflow = value;
}
void
jitterc_vm_stack_set_guard_overflow (struct jitterc_stack * s,
                                      int value)
{
  if (s->guard_overflow != -1)
    jitter_fatal ("stack: guard-overflow set twice");
  s->guard_overflow = value;
}

void
jitterc_vm_add_stack (struct jitterc_vm *vm,
                      struct jitterc_stack *stack)
{
  /* Fail if any fields which requires initialisation was not initialised. */
  if (stack->letter == '\0')
    jitter_fatal ("stack without a letter");
  if (! jitterc_valid_register_or_class_character (stack->letter))
    jitter_fatal ("invalid stack letter '%c'", stack->letter);
  if (stack->long_name == NULL)
    jitter_fatal ("stack '%c' without a long name", stack->letter);

  /* Set any remaining uninitialised value to a reasonable default. */
  if (stack->element_no == -1)
    stack->element_no = 8192;
  if (stack->c_type == NULL)
    stack->c_type = "union jitter_word";
  if (stack->implementation == jitterc_stack_implementation_uninitialized)
    stack->implementation = jitterc_stack_implementation_no_tos;
  if (stack->guard_underflow == -1)
    stack->guard_underflow = 1;
  if (stack->guard_overflow == -1)
    stack->guard_overflow = 1;

  /* Set fields which are always uninitalised at this point to a reasonable
     default. */
  stack->lower_case_long_name = jitter_clone_string (stack->long_name);
  stack->upper_case_long_name = jitter_clone_string (stack->long_name);
  jitterc_change_case (stack->lower_case_long_name, true,
                       "stack long name");
  jitterc_change_case (stack->upper_case_long_name, false,
                       "stack long name");

  /* Make sure that there is no other stack or register in the VM with the
     same name or letter. */
  jitterc_ensure_register_class_or_stack_consistency
     (vm, stack->letter, stack->lower_case_long_name);

  /* Add the descriptor to the VM. */
  gl_list_add_last (vm->stacks, stack);
}


/* Lookup a stack descriptor given its character; return a pointer to it, or
   NULL if no stack uses the same letter. */
__attribute__ ((unused))
static struct jitterc_stack*
jitterc_lookup_stack_or_NULL (const struct jitterc_vm *vm, char c)
{
  /* I expect stacks to be very few in number; it's useless to use
     elaborate data structure for this. */
  int i;
  for (i = 0; i < gl_list_size (vm->stacks); i ++)
    {
      struct jitterc_stack *a_class
        = ((struct jitterc_stack *) gl_list_get_at (vm->stacks, i));
      if (a_class->letter == c)
        return a_class;
    }

  /* We didn't find the stack. */
  return NULL;
}




struct jitterc_instruction_argument*
jitterc_make_instruction_argument (void)
{
  struct jitterc_instruction_argument *res
    = xmalloc (sizeof (struct jitterc_instruction_argument));
  res->mode = jitterc_instruction_argument_mode_unspecified;
  res->kind = jitterc_instruction_argument_kind_unspecified;

  /* Make the initial register class and literal kind invalid, so that they
     aren't used uninitialized by mistake. */
  res->register_class_character = '\0';
  res->literal_type = jitterc_literal_type_unspecified;

  /* Make the default literal printer name NULL. */
  res->c_literal_printer_name = NULL;

  res->literals
    = jitterc_make_empty_list ();
  return res;
}



/* Return a name for a specialized instruction or a prefix of it (meaning that
   not all specialized arguments need to be supplied), with the given suffix
   appended.  The result is a freshly-allocated string. */
static char*
jitterc_specialized_instruction_name (const char *base,
                                      gl_list_t specialized_arguments,
                                      const char *suffix)
{
  size_t base_length = strlen (base);
  size_t specialized_argument_no = gl_list_size (specialized_arguments);
  /* Each argument look liks "/%r4", "/n10" or "/%rR", where '/' is a fixed
     separator, then you have either '%' and a register class letter or one
     letter indicates the literal kind, and the rest is the printed
     representation of a literal or a register index or "R" for residuals.  In
     order to allow for future extensions we make space for
     JITTERC_MAX_ARGUMENT_PRINTED_SIZE_PREFIX_SIZE plus
     JITTERC_MAXIMUM_ARGUMENT_PRINTED_SIZE characters per specialized argument.
     There is a final '\0' character. */
  size_t allocated_size
    = base_length
      + specialized_argument_no * (JITTERC_MAX_ARGUMENT_PRINTED_SIZE_PREFIX_SIZE
                                   + JITTERC_MAXIMUM_ARGUMENT_PRINTED_SIZE)
      + strlen (suffix) + 1;
  char *temporary = xmalloc (allocated_size);
  size_t used_size = 0;

#define ADD_STRING(STRING)                                        \
  do                                                              \
    {                                                             \
      used_size += sprintf (temporary + used_size, "%s", STRING); \
    }                                                             \
  while (false)
#define ADD(PATTERN, THING)                                         \
  do                                                                \
    {                                                               \
      used_size += sprintf (temporary + used_size, PATTERN, THING); \
    }                                                               \
  while (false)

  ADD_STRING(base);
  int i;
  for (i = 0; i < specialized_argument_no; i ++)
    {
      struct jitterc_specialized_argument *sarg
        = (struct jitterc_specialized_argument*)
        gl_list_get_at (specialized_arguments, i);
      switch (sarg->kind)
        {
        case jitterc_instruction_argument_kind_register:
          ADD("/%%%c", sarg->unspecialized->register_class_character);
          if (sarg->residual)
            ADD_STRING("R");
          else
            ADD("%u", (unsigned)sarg->nonresidual_register->index);
          break;
        case jitterc_instruction_argument_kind_literal:
          ADD_STRING("/n");
          if (sarg->residual)
            ADD_STRING("R");
          else
            {
              long value = sarg->nonresidual_literal->value.fixnum;
              ADD("%li", (unsigned long) value);
            }
          break;
        case jitterc_instruction_argument_kind_label:
          assert (sarg->residual);
          ADD_STRING("/lR");
          break;
        case jitterc_instruction_argument_kind_fast_label:
          assert (sarg->residual);
          ADD_STRING("/fR");
          break;
        case jitterc_instruction_argument_kind_return_address:
          assert (sarg->residual);
          ADD_STRING("/QQretR");
          break;
        default:
          jitter_fatal ("jitterc_specialized_instruction_name: unhandled kind");
        }
    }
  ADD_STRING(suffix);

#undef ADD_STRING
#undef ADD

  /* Don't return the temporary string, which is too large; make a copy which is
     just big enough and free the one we allocated first. */
  char *res = jitter_clone_string (temporary);
  free (temporary);
  return res;
}




struct jitterc_specialized_instruction_tree*
jitterc_make_specialized_instruction_tree_root
   (struct jitterc_vm *vm,
    const struct jitterc_instruction *i)
{
  struct jitterc_specialized_instruction_tree *res
    = xmalloc (sizeof (struct jitterc_specialized_instruction_tree));

  res->unspecialized_instruction_name = i->name;
  res->prefix_name = i->name;
  res->prefix_mangled_name = jitterc_mangle (res->prefix_name);
  res->prefix = jitterc_make_empty_list ();
  res->parent = NULL;
  res->children = jitterc_make_empty_list ();
  res->specialized_instruction = NULL;

  gl_list_add_last (vm->specialized_instruction_forest, res);

  return res;
}

struct jitterc_specialized_instruction_tree*
jitterc_make_specialized_instruction_tree_child
   (struct jitterc_specialized_instruction_tree *parent,
    struct jitterc_specialized_argument *specialized_argument)
{
  struct jitterc_specialized_instruction_tree *res
    = xmalloc (sizeof (struct jitterc_specialized_instruction_tree));

  /* Set the child fields. */
  res->unspecialized_instruction_name = parent->unspecialized_instruction_name;
  res->prefix = jitterc_clone_list (parent->prefix);
  gl_list_add_last (res->prefix, specialized_argument);
  res->prefix_name
    = jitterc_specialized_instruction_name
        (parent->unspecialized_instruction_name, res->prefix, "");
  res->prefix_mangled_name = jitterc_mangle (res->prefix_name);
  res->parent = parent;
  res->children = jitterc_make_empty_list ();
  res->specialized_instruction = NULL;

  /* Now the child points to the parent, but we have to make the parent point to
     the child as well. */
  struct jitterc_specialized_instruction_tree_child *pair
    = xmalloc (sizeof (struct jitterc_specialized_instruction_tree_child));
  pair->specialized_argument = specialized_argument;
  pair->child =res;
  gl_list_add_last (parent->children, pair);

  return res;
}

void
jitterc_specialized_instruction_tree_set_specialized_instruction
   (struct jitterc_specialized_instruction_tree *tree,
    struct jitterc_specialized_instruction *specialized_instruction)
{
  assert (tree->specialized_instruction == NULL);
  assert (gl_list_size (tree->children) == 0);
  tree->specialized_instruction = specialized_instruction;
}

static void
jitterc_print_specialized_instruction_tree_recursive
   (const struct jitterc_specialized_instruction_tree *parent,
    const struct jitterc_specialized_argument *specialized_argument,
    const struct jitterc_specialized_instruction_tree *child,
    unsigned depth)
{
  assert (child->parent == parent); /* The converse is expensive to check. */
  if (parent != NULL)
    assert (gl_list_size (parent->children) > 0);
  size_t prefix_length = gl_list_size (child->prefix);
  if (prefix_length > 0)
    assert (gl_list_get_at (child->prefix, prefix_length - 1) == specialized_argument);
  else
    assert (specialized_argument == NULL);
  int i;
  for (i = 0; i < depth; i ++)
    printf ("  ");
  printf ("%s\n", child->prefix_name);
  size_t grandchildren_no = gl_list_size (child->children);
  for (i = 0; i < grandchildren_no; i ++)
    {
      const struct jitterc_specialized_instruction_tree_child *grandchild_pair
        = (const struct jitterc_specialized_instruction_tree_child *)
          gl_list_get_at (child->children, i);
      const struct jitterc_specialized_argument *grandchild_specialized_argument
        = grandchild_pair->specialized_argument;
      const struct jitterc_specialized_instruction_tree *grandchild
        = grandchild_pair->child;
      jitterc_print_specialized_instruction_tree_recursive
         (child,
          grandchild_specialized_argument,
          grandchild,
          depth + 1);
    }
}

void
jitterc_print_specialized_instruction_forest (const gl_list_t forest)
{
  size_t tree_no = gl_list_size (forest);
  int i;
  for (i = 0; i < tree_no; i ++)
    {
      struct jitterc_specialized_instruction_tree *tree
        = (struct jitterc_specialized_instruction_tree *)
          gl_list_get_at (forest, i);
      jitterc_print_specialized_instruction_tree_recursive (NULL,
                                                              NULL,
                                                              tree,
                                                              0);
    }
}



struct jitterc_instruction*
jitterc_make_instruction (void)
{
  struct jitterc_instruction *res
    = xmalloc (sizeof (struct jitterc_instruction));
  res->name = NULL;
  res->mangled_name = NULL;
  res->hotness = jitterc_hotness_unspecified;
  res->relocatability = jitterc_relocatability_unspecified;
  res->has_fast_labels = false;
  res->branchingness = jitterc_branchingness_unspecified;
  res->callerness = jitterc_callerness_unspecified;
  res->calleeness = jitterc_calleeness_unspecified;
  res->returningness = jitterc_returningness_unspecified;
  res->arguments
    = jitterc_make_empty_list ();
  res->code = NULL;

  return res;
}



static struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_common
   (const struct jitterc_instruction_argument *unspecialized,
    enum jitterc_instruction_argument_kind needed_kind)
{
  struct jitterc_specialized_argument *res
    = xmalloc (sizeof (struct jitterc_specialized_argument));
  res->unspecialized = (struct jitterc_instruction_argument *) unspecialized;

  /* The kind of a specialized instruction argument must be a single value, not
     unspecified and not the or of two simpler kind.  Check that the original
     unspecialized argument contains the right kind (possibly along with
     others), and then set the right kind *only*, in the specialized version. */
  assert (unspecialized->kind & needed_kind);
  res->kind = needed_kind;

  /* This will be a non-replacement by default.  Replacement specialized
     argments are initialized by first cloning an argument and then updating
     some fields, including this, in the clone. */
  res->replacement = false;

  return res;
}

/* Return a pointer to a freshly allocated clone of the pointed specialized
   argument.  The new struct itself will be new, but may point to data shared
   with the old specialized argument. */
static struct jitterc_specialized_argument*
jitterc_clone_specialized_instruction_argument_common
   (const struct jitterc_specialized_argument *sarg)
{
  struct jitterc_specialized_argument *res
    = xmalloc (sizeof (struct jitterc_specialized_argument));
  memcpy (res, sarg, sizeof (struct jitterc_specialized_argument));
  return res;
}

static struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_common
   (const struct jitterc_instruction_argument *unspecialized,
    enum jitterc_instruction_argument_kind needed_kind)
{
  struct jitterc_specialized_argument *res
    = jitterc_make_specialized_instruction_argument_common (unspecialized,
                                                            needed_kind);
  res->residual = true;
  return res;
}

struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_register
   (const struct jitterc_instruction_argument *unspecialized)
{
  return jitterc_make_specialized_instruction_argument_residual_common
            (unspecialized,
             jitterc_instruction_argument_kind_register);
}
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_literal
   (const struct jitterc_instruction_argument *unspecialized)
{
  return jitterc_make_specialized_instruction_argument_residual_common
            (unspecialized,
             jitterc_instruction_argument_kind_literal);
}
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_label
   (const struct jitterc_instruction_argument *unspecialized)
{
  return jitterc_make_specialized_instruction_argument_residual_common
            (unspecialized,
             jitterc_instruction_argument_kind_label);
}
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_fast_label
   (const struct jitterc_instruction_argument *unspecialized)
{
  return jitterc_make_specialized_instruction_argument_residual_common
            (unspecialized,
             jitterc_instruction_argument_kind_fast_label);
}

struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_return_address
   (void)
{
  struct jitterc_instruction_argument *non_specialized_argument
    = jitterc_make_instruction_argument ();
  non_specialized_argument->mode = jitterc_instruction_argument_mode_in;
  non_specialized_argument->kind
    = jitterc_instruction_argument_kind_return_address;
  return jitterc_make_specialized_instruction_argument_residual_common
            (non_specialized_argument,
             jitterc_instruction_argument_kind_return_address);
}

struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_specialized_register
   (const struct jitterc_vm *vm,
    const struct jitterc_instruction_argument *unspecialized,
    const struct jitterc_register *register_)
{
  struct jitterc_specialized_argument *res
    = jitterc_make_specialized_instruction_argument_common
         (unspecialized,
          jitterc_instruction_argument_kind_register);

  /* Lookup the register letter, to make sure it's valid.  Now, after parsing,
     we can be sure that register classes are all defined. */
  jitterc_lookup_register_class (vm,
                                 unspecialized->register_class_character);
  /* Make sure that the register we are specializing on is actually allowed for
     the original argument. */
  assert (unspecialized->register_class_character == register_->class->letter);

  res->residual = false;
  res->nonresidual_register = (struct jitterc_register *) register_;
  return res;
}

struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_specialized_literal
   (struct jitterc_instruction_argument *unspecialized,
    struct jitterc_literal *literal)
{
  struct jitterc_specialized_argument *res
    = jitterc_make_specialized_instruction_argument_common
        (unspecialized,
         jitterc_instruction_argument_kind_literal);

  /* Make sure that the literal we are specializing on is actually allowed for
     the original argument. */
  assert (unspecialized->literal_type == literal->type);

  res->residual = false;
  res->nonresidual_literal = literal;
  return res;
}




/* Return a new specialized instruction.
   - If ui is non-NULL then the specialized instruction is based on it
     and non-special, and the name is ignored;
   - if ui is NULL then the specialized instruction is special; use the
     given name for it.
   Use the given relocatability for the new specialized instruction,
   unless the relocatability is jitterc_relocatability_unspecified (in
   which case ui's relocatability is used).
   If replacement_of is non-NULL then the new specialised instruction
   will be a replacement for the given specialised instruction; in that
   case adapt the replacement name and make the two instructions point to
   each other. */
struct jitterc_specialized_instruction*
jitterc_make_specialized_instruction_internal (struct jitterc_vm *vm,
                                               const struct jitterc_instruction *ui,
                                               const gl_list_t specialized_args_copy,
                                               const char *name,
                                               enum jitterc_relocatability given_r,
                                               struct jitterc_specialized_instruction
                                               *replacement_of)
{
  struct jitterc_specialized_instruction *res
    = xmalloc (sizeof (struct jitterc_specialized_instruction));

  /* The new specialized instruction opcode is not assigned yet.  It will be
     decided late in the generation process. */
  res->opcode = -1;

  /* Decide on relocatability for the new specialized instruction.  First use
     the given value; if that is unspecified, inherit from the unspecialized
     version. */
  enum jitterc_relocatability r = given_r;
  if (r == jitterc_relocatability_unspecified)
    {
      if (ui != NULL)
        /* Inherit relocatability from unspecialized version. */
        r = ui->relocatability;

      /* Sanity check: relocatability must be specified at this point. */
      if (r == jitterc_relocatability_unspecified)
        jitter_fatal ("jitterc_make_specialized_instruction_internal: "
                      "unspecified relocatability");
    }
  res->relocatability = r;

  /* The specialised branchingness is the same as the unspecialised
     branchingness, if an unspecialised version exists.  Otherwise, let us make
     it branching [FIXME: remove special specialised instructions, so that every
     specialised instruction has an unspecialised counterpart] */
  res->branchingness
    = ui == NULL ? jitterc_branchingness_branching : ui->branchingness;

  /* Compute the instruction name, adding a /retR suffix if needed.  Do not
     add a second /retR suffix if one is already present. */
  bool new_retR;
  int arity;
  if ((arity = gl_list_size (specialized_args_copy)) > 0
      && (((const struct jitterc_specialized_argument *)
           gl_list_get_at (specialized_args_copy, arity - 1))->kind
          == jitterc_instruction_argument_kind_return_address))
    new_retR = false;
  else
    new_retR = (res->relocatability == jitterc_relocatability_non_relocatable
                || (ui != NULL
                    && ui->callerness == jitterc_callerness_caller));

  /* Clone arguments into the appropriate result field, and add a retR argument
     to the clone.  It is harmless to share structure.  If the specialized
     instruction has a return-address hidden argument, append it. */
  res->specialized_arguments = jitterc_clone_list (specialized_args_copy);
  if (new_retR)
    {
      struct jitterc_specialized_argument *retR_specialized_argument
        = jitterc_make_specialized_instruction_argument_return_address ();
      gl_list_add_last (res->specialized_arguments,
                        retR_specialized_argument);
    }

  if (replacement_of != NULL)
    {
      /* Set the new specialized instruction name to make it easy to recognize
         visually as a replacement. */
      char *original_name = replacement_of->name;
      size_t original_name_length = strlen (original_name);
      const char *prefix = "!REPLACEMENT-";
      const char *suffix = "!";
      suffix = new_retR ? "/retR": "";
      size_t prefix_length = strlen (prefix);
      size_t suffix_length = strlen (suffix);
      size_t new_name_length = prefix_length + original_name_length + suffix_length + 1;
      char *new_name = xmalloc (new_name_length);
      sprintf (new_name, "%s%s%s", prefix, original_name, suffix);
      res->name = new_name;
    }
  else if (ui != NULL)
    res->name = jitterc_specialized_instruction_name (ui->name,
                                                      specialized_args_copy,
                                                      new_retR ? "/retR" : "");
  else
    res->name = jitter_clone_string (name);

  /* Set some simple fields in the result. */
  res->mangled_name = jitterc_mangle (res->name);
  res->instruction = (struct jitterc_instruction*) ui;

  /* Make this specialised instruction and the specialised instruction it
     replaces point to each other correctly, or set the pointers to NULL if this
     is not a replacement. */
  res->has_as_replacement = NULL;
  if (replacement_of == NULL)
    res->is_replacement_of = NULL;
  else
    {
      res->is_replacement_of = replacement_of;
      replacement_of->has_as_replacement = res;
    }

  /* By default the specialized instruction hotness is the same as the
     unspecialized hotness.  However if a meta-instruction is declared hot its
     specializations involving residual non-label arguments are low-priority,
     and we demote them to cold so that GCC can do a better job at allocating
     registers for instructions involing fast registers only; if VM fast
     registers are well allocated then the other specialized instructions will
     benefit as well.

     I've seen better results with this when the number of
     fast registers is high. */
  int i;
  if (ui == NULL)
    res->hotness = jitterc_hotness_cold;
  else
    if ((res->hotness = ui->hotness) == jitterc_hotness_hot)
      for (i = 0; i < gl_list_size (res->specialized_arguments); i ++)
        {
          const struct jitterc_specialized_argument *sarg
            = gl_list_get_at (res->specialized_arguments, i);
          if (   sarg->residual
              && sarg->kind != jitterc_instruction_argument_kind_label
              && sarg->kind != jitterc_instruction_argument_kind_fast_label)
            {
              res->hotness = jitterc_hotness_cold;
              break;
            }
        }

  /* Add the new specialized instruction to the VM. */
  gl_list_add_last (vm->specialized_instructions, res);

  /* Compute the residual arity of this instruction. */
  size_t specialized_argument_no = gl_list_size (res->specialized_arguments);
  size_t residual_arity = 0;
  for (i = 0; i < specialized_argument_no; i ++)
    {
      struct jitterc_specialized_argument *sarg
        = (struct jitterc_specialized_argument *)
          gl_list_get_at (res->specialized_arguments, i);
      if (sarg->residual)
        residual_arity ++;
    }

  /* Non-relocatable specialized instructions are always cold.*/
  if (res->relocatability == jitterc_relocatability_non_relocatable)
    res->hotness = jitterc_hotness_cold;

  //printf ("RESIDUAL ARITY for %s: %i\n", res->name, (int) residual_arity);

  /* [FIXME: test a non-relocatable non-replacement caller. */
  //if (! (! (res->relocatability == jitterc_relocatability_non_relocatable
  //          && ui != NULL
  //          && ui->callerness == jitterc_callerness_caller)))
  //  printf ("WARNING: non-relocatable caller.  This is probably not supported yet.\n");

  /* Update the maximum residual arity count. */
  if (residual_arity > vm->max_residual_arity)
    {
      vm->max_residual_arity = residual_arity;
      //printf ("max residual arity: %i, because of %s\n", (int) residual_arity, res->name);
    }

  return res;
}

struct jitterc_specialized_instruction*
jitterc_make_ordinary_specialized_instruction (struct jitterc_vm *vm,
                                               const struct jitterc_instruction *ui,
                                               const gl_list_t specialized_args_copy)
{
  return jitterc_make_specialized_instruction_internal (vm, ui,
                                                        specialized_args_copy,
                                                        NULL,
                                                        ui->relocatability,
                                                        NULL);
}

static struct jitterc_specialized_instruction*
jitterc_make_special_specialized_instruction (struct jitterc_vm *vm,
                                              const char *name,
                                              const gl_list_t specialized_args_copy)
{
  return jitterc_make_specialized_instruction_internal (vm, NULL,
                                                        specialized_args_copy,
                                                        name,
                                                        jitterc_relocatability_relocatable,
                                                        NULL);
}

struct jitterc_instruction *
jitterc_vm_last_instruction (struct jitterc_vm *vm)
{
  return (struct jitterc_instruction *)
         gl_list_get_at (vm->instructions, gl_list_size (vm->instructions) - 1);
}

void
jitterc_vm_append_instruction (struct jitterc_vm *vm,
                               struct jitterc_instruction *ins)
{
  gl_list_add_last (vm->instructions, ins);
}

struct jitterc_instruction_argument *
jitterc_vm_last_argument (struct jitterc_vm *vm)
{
  struct jitterc_instruction *ins = jitterc_vm_last_instruction (vm);
  assert (ins != NULL);
  gl_list_t arguments = ins->arguments;
  size_t argument_no = gl_list_size (arguments);
  assert (argument_no != 0);
  return (struct jitterc_instruction_argument *)
         gl_list_get_at (arguments, argument_no - 1);
}

void
jitterc_vm_append_argument (struct jitterc_vm *vm,
                            struct jitterc_instruction_argument *arg)
{
  struct jitterc_instruction *ins = jitterc_vm_last_instruction (vm);
  assert (ins != NULL);
  gl_list_add_last (ins->arguments, arg);
}



static int
jitterc_compare_vm_instructions (const void *ppa, const void *ppb)
{
  const struct jitterc_instruction *pa
    = * (const struct jitterc_instruction **) ppa;
  const struct jitterc_instruction *pb
    = * (const struct jitterc_instruction **) ppb;
  return strcmp (pa->name, pb->name);
}

/* Alphabetically sort the instructions within the given VM, in place.  Fail
   fatally if there are duplicates. */
static void
jitterc_sort_vm (struct jitterc_vm *vm)
{
  /* Make sure that there are enough instructions for sorting to have an
     observable effect before continuing.  Notice that the initialization of
     previous below would fail without this check. */
  size_t instruction_no = gl_list_size (vm->instructions);
  if (instruction_no < 2)
    return;

  /* Alphabetically sort the instruction list into a temporary array . */
  struct jitterc_instruction **array
    = xmalloc (sizeof (struct jitterc_instruction *) * instruction_no);
  int i;
  for (i = 0; i < instruction_no; i ++)
    array [i]
      = (struct jitterc_instruction *)
        gl_list_get_at (vm->instructions, i);
  qsort (array, instruction_no, sizeof (struct jitterc_instruction *),
         jitterc_compare_vm_instructions);

  /* Scan the sorted version: if there were duplicates in the input we will find
     two *consecutive* instructions with the same name here.  Fail fatally if
     that's the case. */
  struct jitterc_instruction *previous = array [0];
  for (i = 1; i < instruction_no; i ++)
    if (! jitterc_compare_vm_instructions (& previous, array + i))
      jitter_fatal ("duplicate instruction %s", previous->name);
    else
      previous = array [i];

  /* Everything's good if we arrived here.  Update the VM and free the temporary
     array. */
  for (i = 0; i < instruction_no; i ++)
    gl_list_set_at (vm->instructions, i, array [i]);
  free (array);
}




/* Specialized instruction properties.
 * ************************************************************************** */

bool
jitterc_specialized_instruction_is_call_related
   (const struct jitterc_specialized_instruction *sins)
{
  struct jitterc_instruction *ins = sins->instruction;
  return (ins != NULL
          && (ins->calleeness == jitterc_calleeness_callee
              || ins->callerness == jitterc_callerness_caller
              || ins->returningness == jitterc_returningness_returning));
}




/* Building the name_to_instruction hash table.
 * ************************************************************************** */

static void
jitterc_fill_name_to_instruction (struct jitterc_vm * vm)
{
  gl_list_t instructions = vm->instructions;
  size_t instruction_no = gl_list_size (instructions);
  int i;
  for (i = 0; i < instruction_no; i ++)
    {
      const struct jitterc_instruction *ins
        = gl_list_get_at (instructions, i);
      union jitter_word w = { .pointer = (void *) ins };
      jitter_string_hash_table_add (& vm->name_to_instruction, ins->name, w);
    }
}




/* Replacement generation.
 * ************************************************************************** */

/* Generate a replacement specialized instruction in the pointed VM for the
   pointed potentially defective specialized instruction.  Update the pointed
   specialized instruction, linking the replacement from it. */
static void
jitterc_generate_replacement_for (struct jitterc_vm *vm,
                                  struct jitterc_specialized_instruction *sins)
{
  /* Make an updated specialized argument list, where each fast label is
     replaced by an ordinary label.  The updated list may share elements with
     the original list. */
  gl_list_t new_specialized_arguments
    = jitterc_clone_list (sins->specialized_arguments);
  int i;
  //bool at_least_one_fast_label = false;
  for (i = 0; i < gl_list_size (new_specialized_arguments); i ++)
    {
      const struct jitterc_specialized_argument *old_sarg
        = ((const struct jitterc_specialized_argument*)
           gl_list_get_at (new_specialized_arguments, i));
      if (old_sarg->kind == jitterc_instruction_argument_kind_fast_label)
        {
          //at_least_one_fast_label = true;
          struct jitterc_specialized_argument *new_sarg
            = jitterc_clone_specialized_instruction_argument_common (old_sarg);
          new_sarg->replacement = true;
          new_sarg->residual = true;
          new_sarg->kind = jitterc_instruction_argument_kind_label;
          gl_list_set_at (new_specialized_arguments, i, new_sarg);
        }
    }

  /* Make a new specialized instruction. */
  jitterc_make_specialized_instruction_internal (vm,
                                                 sins->instruction,
                                                 new_specialized_arguments,
                                                 NULL,
                                                 jitterc_relocatability_non_relocatable,
                                                 sins);
}

/* Generate replacement specialized instructions where needed, updating the
   pointed VM.  This needs non-replacements specialized instructions to already
   be available, so should be called late in the specialization process. */
static void
jitterc_generate_replacements (struct jitterc_vm *vm)
{
  /* Count how many specialized instructions there are now, before starting.  We
     will scan this number of specialized instructions starting from the
     beginning; the replacement specialized instructions to be added will be at
     the end, and will not be scanned by the same loop. */
  const size_t non_defective_sins_no = gl_list_size (vm->specialized_instructions);
  fprintf (stderr, "Specialized instructions, ignoring replacements: %i\n",
           (int) non_defective_sins_no);

  /* For every specialized instruction... */
  int i;
  int potentially_defective_no = 0;
  for (i = 0; i < non_defective_sins_no; i ++)
    {
      /* Get a pointer to the specialized instruction. */
      struct jitterc_specialized_instruction *sins
        = ((struct jitterc_specialized_instruction *)
           gl_list_get_at (vm->specialized_instructions, i));

      /* Special specialised instructions are never replaced.  If sins is
         special, which is to say has no unspecialised counterpart, ignore
         it. */
      if (sins->instruction == NULL)
        continue;

      /* Check the specialized instruction arguments.  A specialized instruction
         is potentially defective if it has at least one fast-label residual
         argument. */
      bool potentially_defective = false;
      int j;
      for (j = 0; j < gl_list_size (sins->specialized_arguments); j ++)
        {
          const struct jitterc_specialized_argument *sarg
            = ((const struct jitterc_specialized_argument*)
               gl_list_get_at (sins->specialized_arguments, j));
          if (sarg->kind == jitterc_instruction_argument_kind_fast_label)
            {
              potentially_defective = true;
              break;
            }
        }

      /* A specialized instruction is also potentially defective if it is
         branching. */
      if (sins->branchingness == jitterc_branchingness_branching)
        potentially_defective = true;

      /* A specialized instruction is also potentially defective if it is
         call-related. */
      if (jitterc_specialized_instruction_is_call_related (sins))
        potentially_defective = true;

      /* If the specialized instruction is potentially defective, for any
         reason, generate a replacement for it.  Also update the potential
         defect counter. */
      if (potentially_defective)
        {
          potentially_defective_no ++;
          jitterc_generate_replacement_for (vm, sins);
        }
    }

  int total_sins_no = gl_list_size (vm->specialized_instructions);
  double replacement_ratio = 0;
  double replacement_overhead = 0;
  if (non_defective_sins_no > 0)
    {
      replacement_ratio = (double) potentially_defective_no / total_sins_no;
      replacement_overhead
        = (double) potentially_defective_no / non_defective_sins_no;
    }
  fprintf (stderr, "Potentially defective specialized instructions: %i\n",
           (int) potentially_defective_no);
  fprintf (stderr, "Replacements: %i (%.1f%% of total, %.1f%% overhead)\n",
           (int) (total_sins_no - non_defective_sins_no),
           replacement_ratio * 100,
           replacement_overhead * 100);
  fprintf (stderr, "Total instructions, including replacements: %i\n",
           (int) total_sins_no);
}




/* Data structure analysis.
 * ************************************************************************** */

void
jitterc_analyze_vm (struct jitterc_vm *vm)
{
  /* First sort instructions by name.  This also fails in case of duplicate
     instruction names, which we want to check for here. */
  jitterc_sort_vm (vm);

  /* Fill the name_to_instruction hash. */
  jitterc_fill_name_to_instruction (vm);

  /* The initial maximum name length is zero. */
  vm->max_instruction_name_length = 0;

  /* For every instruction... */
  size_t instruction_no = gl_list_size (vm->instructions);
  int i;
  for (i = 0; i < instruction_no; i ++)
    {
      /* If the instruction name is longer than the current maximum, update the
         maximum. */
      struct jitterc_instruction *ins
        = ((struct jitterc_instruction *)
           gl_list_get_at (vm->instructions, i));
      size_t name_length = strlen (ins->name);
      if (vm->max_instruction_name_length < name_length)
        vm->max_instruction_name_length = name_length;
    }

  /* Check rewrite rules for semantic problems.  This is only possible after
     every instruction and every rule is in place. */
  jitterc_check_rules (vm);
}




static void
jitterc_make_special_specialized_instructions (struct jitterc_vm *vm)
{
  const gl_list_t no_arguments = jitterc_make_empty_list ();

  /* It's important that !INVALID be the very first instruction, so that its
     opcode will be zero.  The specializer uses specialized instructions opcodes
     as booleans, with !INVALID signifying failure to find a match. */
  assert (gl_list_size (vm->specialized_instructions) == 0);
  jitterc_make_special_specialized_instruction (vm, "!INVALID", no_arguments);

  /* The order is slightly less critical for the other specialized instructions,
     but they still need to come first; they need to respect the encoding of
     enum jitter_specialized_instruction_opcode from jitter/jitter-specialize.h
     . */

  gl_list_t nR_arguments = jitterc_make_empty_list ();
  struct jitterc_instruction_argument *unspecialized_n
    = jitterc_make_instruction_argument ();
  unspecialized_n->mode = jitterc_instruction_argument_mode_in;
  unspecialized_n->kind = jitterc_instruction_argument_kind_literal;
  unspecialized_n->literal_type = jitterc_literal_type_fixnum;
  unspecialized_n->literals = jitterc_make_empty_list ();
  struct jitterc_specialized_argument *nR
    = jitterc_make_specialized_instruction_argument_residual_literal (unspecialized_n);
  gl_list_add_last (nR_arguments, nR);

  jitterc_make_special_specialized_instruction (vm, "!BEGINBASICBLOCK", nR_arguments);
  jitterc_make_special_specialized_instruction (vm, "!EXITVM", no_arguments);
  jitterc_make_special_specialized_instruction (vm, "!DATALOCATIONS", no_arguments);
  jitterc_make_special_specialized_instruction (vm, "!NOP", no_arguments);
  jitterc_make_special_specialized_instruction (vm, "!UNREACHABLE0", no_arguments);
  jitterc_make_special_specialized_instruction (vm, "!UNREACHABLE1", no_arguments);
  jitterc_make_special_specialized_instruction (vm, "!UNREACHABLE2", no_arguments);
}

static void
jitterc_specialize_recursive (struct jitterc_vm *vm,
                              const struct jitterc_instruction *ui,
                              int next_argument_index,
                              gl_list_t specialized_arguments,
                              struct jitterc_specialized_instruction_tree *parent)
{
  if (next_argument_index == gl_list_size (ui->arguments))
    {
      /* We have described a complete specialized instruction.  Make it. */
      struct jitterc_specialized_instruction *si
        = jitterc_make_ordinary_specialized_instruction
             (vm, ui, specialized_arguments);

      // FIXME: no: this is already handled within jitterc_make_specialized_instruction_internal, called by jitterc_make_ordinary_specialized_instruction
      /* /\* If this instruction is non-relocatable then add another residual */
      /*    argument, not taken into account in the forest: it's the non-relocated */
      /*    return address, automatically handled at specialization time and not */
      /*    visible to the user. */
      /*    Also do the same when the instruction is a caller, even if the argument */
      /*    has a different meaning and is not used unconditionally. *\/ */
      /* if (   ui->relocatability == jitterc_relocatability_non_relocatable */
      /*     || ui->callerness == jitterc_callerness_caller) */
      /*   { */
      /*     struct jitterc_instruction_argument *arg_relocated_return */
      /*       = jitterc_make_instruction_argument (); */
      /*     arg_relocated_return->mode = jitterc_instruction_argument_mode_in; */
      /*     arg_relocated_return->kind = jitterc_instruction_argument_kind_literal; */
      /*     arg_relocated_return->literal_type = jitterc_literal_type_fixnum; */
      /*     arg_relocated_return->literals = jitterc_make_empty_list (); */
      /*     // FIXME: shall I bother freeing arg_relocated_return ? */
      /*     struct jitterc_specialized_argument *sarg_relocated_return */
      /*       = jitterc_make_specialized_instruction_argument_residual_literal */
      /*            (arg_relocated_return); */
      /*     gl_list_add_last (si->specialized_arguments, sarg_relocated_return); */
      /*   } */

      jitterc_specialized_instruction_tree_set_specialized_instruction
         (parent, si);
      return;
    }

  /* We are about to recursively generate specialized instructions with one more
     specialized argument, for every choice of a specialized argument which fits
     here.  Add an element to the specialized_arguments list: we are going to
     destructively set it, once per choice. */
  gl_list_add_last (specialized_arguments, NULL);

  struct jitterc_instruction_argument *arg
    = (struct jitterc_instruction_argument*)
      gl_list_get_at (ui->arguments, next_argument_index);
  enum jitterc_instruction_argument_kind kind = arg->kind;

#define HANDLE(SPECIALIZED_ARGUMENT)                                    \
  do                                                                    \
    {                                                                   \
      gl_list_set_at (specialized_arguments,                            \
                      next_argument_index,                              \
                      SPECIALIZED_ARGUMENT);                            \
      struct jitterc_specialized_instruction_tree *tree               \
        = jitterc_make_specialized_instruction_tree_child (parent,    \
                                                             SPECIALIZED_ARGUMENT); \
      jitterc_specialize_recursive (vm, ui, next_argument_index + 1,  \
                                      specialized_arguments,            \
                                      tree);                            \
    }                                                                   \
  while (false)

  /* The order is important here, because it will be followed in the recognizer.
     Specific cases must always come before general case so that a non-residual
     literal is matched before a residual literal. */
  if (kind & jitterc_instruction_argument_kind_register)
    {
      struct jitterc_register_class *class
        = jitterc_lookup_register_class (vm, arg->register_class_character);

      /* Specialize for every fast register.  FIXME: this should be done for
         every register class, when there are more than one. */
      int i;
      for (i = 0; i < class->fast_register_no; i ++)
        {
          struct jitterc_register *reg
            = jitterc_make_register (class, i);
          struct jitterc_specialized_argument *sa
            = jitterc_make_specialized_instruction_argument_specialized_register
                 (vm, arg, reg);
          HANDLE(sa);
        }

      /* Also residualize the register to support the slow-register case, when
         slow registers are enabled in the class. */
      if (class->use_slow_registers)
        {
          struct jitterc_specialized_argument *rsa
            = jitterc_make_specialized_instruction_argument_residual_register
                 (arg);
          HANDLE(rsa);
        }
    }
  if (kind & jitterc_instruction_argument_kind_literal)
    {
      /* Specialize for every literal noted in the original argument, up to the
         maximum number allowed if any. */
      size_t literal_no = gl_list_size (arg->literals);
      if (   vm->max_nonresidual_literal_no != -1
          && literal_no > vm->max_nonresidual_literal_no)
        literal_no = vm->max_nonresidual_literal_no;
      int i;
      for (i = 0; i < literal_no; i ++)
        {
          struct jitterc_literal *lit
            = (struct jitterc_literal *) gl_list_get_at (arg->literals, i);
          struct jitterc_specialized_argument *sa
            = jitterc_make_specialized_instruction_argument_specialized_literal
                 (arg, lit);
          HANDLE(sa);
        }

      /* Also residualize the literal, to support the literals not indicated in
         the specification -- which hopefully will occur less frequently. */
      struct jitterc_specialized_argument *rsa
        = jitterc_make_specialized_instruction_argument_residual_literal
            (arg);
      HANDLE(rsa);
    }
  if (kind & jitterc_instruction_argument_kind_label)
    {
      /* Labels are always residualized. */
      struct jitterc_specialized_argument *sa
        = jitterc_make_specialized_instruction_argument_residual_label (arg);
      HANDLE(sa);
    }
  if (kind & jitterc_instruction_argument_kind_fast_label)
    {
      /* Fast labels are always "residualized", so to speak. */
      struct jitterc_specialized_argument *sa
        = jitterc_make_specialized_instruction_argument_residual_fast_label
             (arg);
      HANDLE(sa);
    }

#undef HANDLE

  /* We are done recursively generating specialized instructions with one more
     specialized argument; remove the element we added to the list, so that our
     caller will find the list as it was before the call to us. */
  gl_list_remove_at (specialized_arguments, next_argument_index);
}

void
jitterc_specialize (struct jitterc_vm *vm,
                    int max_fast_register_no_per_class,
                    int max_nonresidual_literal_no)
{
  /* Remember the limits on how many fast registers we want per class and on the
     number of nonresidual literals (or -1 if there is no limit, in either case),
     and whether we should use slow registers as well. */
  vm->max_fast_register_no_per_class = max_fast_register_no_per_class;
  vm->max_nonresidual_literal_no = max_nonresidual_literal_no;

  /* First generate the special specialized instructions.  Those have to be the
     first ones, and in particular the !INVALID specialized instruction must
     have opcode zero: this is exploited in the specializer, where recognizers
     return zero in case of failure, and use the opcode as a boolean
     condition. */
  jitterc_make_special_specialized_instructions (vm);

  /* Make a list to contain the specialized arguments.  The same list will be
     used for every specialized instruction while visiting the specialized
     instruction tree.  The list is essentially used as a stack tracking the
     specialized arguments of the current specialized instruction; all the
     pushing and popping is done within jitterc_specialize_recursive , with
     the list always empty at the time this function has control. */
  gl_list_t specialized_arguments
    = jitterc_make_empty_list ();

  int i;
  size_t instruction_no = gl_list_size (vm->instructions);
  for (i = 0; i < instruction_no; i ++)
    {
      //assert (gl_list_size (specialized_arguments) == 0);
      //jitterc_empty_list (specialized_arguments);

      struct jitterc_instruction *ins
        = (struct jitterc_instruction *) gl_list_get_at (vm->instructions, i);

      struct jitterc_specialized_instruction_tree *root
        = jitterc_make_specialized_instruction_tree_root (vm, ins);
      jitterc_specialize_recursive (vm, ins, 0, specialized_arguments, root);
    }

  /* We are done with the list used for visiting. */
  gl_list_free (specialized_arguments);

  /* Generate replacements for potentially defective instructions.  This will
     use (non-replacement) specialized instructions to generate more specialized
     instructions. */
  jitterc_generate_replacements (vm);

  /* Now we have generated every specialised instruction we will need, including
     special specialised instructions and replacement specialised instructions.
     Number them by assigning opcodes.  Specialised instruction opcodes have
     been undefined up to this point. */
  for (i = 0; i < gl_list_size (vm->specialized_instructions); i ++)
    {
      /* Get a pointer to the specialized instruction. */
      struct jitterc_specialized_instruction *sins
        = ((struct jitterc_specialized_instruction *)
           gl_list_get_at (vm->specialized_instructions, i));
      sins->opcode = i;
      //printf ("sins %i: %s\n", i, sins->name);
    }
}
