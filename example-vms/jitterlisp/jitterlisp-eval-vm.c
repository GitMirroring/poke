/* JitterLisp: interpreter: naïve C version.

   Copyright (C) 2018, 2019, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with Jitter under the same license.

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


#include "jitterlisp-eval-vm.h"

#include <jitter/jitter-list.h>

#include "jitterlisp.h"
#include "jitterlispvm-vm.h"




/* Jittery evaluation engine.
 * ************************************************************************** */

// FIXME: remove this section.

/* Error out saying that the VM is not yet implemented. */
__attribute__ ((noreturn))
static void
jitterlisp_vm_unimplemented (void)
{
  jitterlisp_error_cloned ("Jittery VM not implemented yet: plase run "
                           "with --no-vm or explicitly use the AST "
                           "interpreter");
}

jitterlisp_object
jitterlisp_eval_globally_vm (jitterlisp_object form)
{
  jitterlisp_vm_unimplemented ();
}

jitterlisp_object
jitterlisp_eval_vm (jitterlisp_object form, jitterlisp_object env)
{
  jitterlisp_vm_unimplemented ();
}

jitterlisp_object
jitterlisp_apply_vm (jitterlisp_object closure_value,
                     jitterlisp_object operands_as_list)
{
  jitterlisp_vm_unimplemented ();
}




/* State pool.
 * ************************************************************************** */

struct jitter_list_header
jitterlisp_used_states;
struct jitter_list_header
jitterlisp_unused_states;

/* Initialise the links for the pointed state to make it belong to the used
   state list.  This does not touch the unused state list, and is appropriate
   for initialisation of a state. */
static void
jitterlisp_state_initialise_as_used (struct jitterlispvm_state *s)
{
  JITTER_LIST_LINK_FIRST (jitterlispvm_state,
                          jitterlispvm_state_backing.pool_links,
                          & jitterlisp_used_states,
                          s);
}

/* Add the pointed state to the unused pool, and remove it from the used pool.
   The state must not already belong to its new list and must belong to its old
   list. */
static void
jitterlisp_state_used_to_unused (struct jitterlispvm_state *s)
{
  JITTER_LIST_UNLINK (jitterlispvm_state,
                      jitterlispvm_state_backing.pool_links,
                      & jitterlisp_used_states,
                      s);
  JITTER_LIST_LINK_FIRST (jitterlispvm_state,
                          jitterlispvm_state_backing.pool_links,
                          & jitterlisp_unused_states,
                          s);
}

/* Remove the pointed state from the unused pool, and add it to the used pool.
   The state must not already belong to its new list and must belong to its old
   list. */
static void
jitterlisp_state_unused_to_used (struct jitterlispvm_state *s)
{
  JITTER_LIST_UNLINK (jitterlispvm_state,
                      jitterlispvm_state_backing.pool_links,
                      & jitterlisp_unused_states,
                      s);
  JITTER_LIST_LINK_FIRST (jitterlispvm_state,
                          jitterlispvm_state_backing.pool_links,
                          & jitterlisp_used_states,
                          s);
}

/* Procure an unused state.
   If the pool is not empty unlink one state, reset it and return it; if the
   pool is empty make a new state.
   In either case the new state will be in use, and therefore will *not* be
   linked in the unused pool when this function returns. */
static struct jitterlispvm_state *
jitterlisp_get_state (void)
{
  if (jitterlisp_unused_states.first == NULL)
    {
      struct jitterlispvm_state *res
        = jitterlispvm_state_make_with_slow_registers
             (jitterlisp_slow_register_per_class_no);
      jitterlisp_state_initialise_as_used (res);
      return res;
    }
  else
    {
      struct jitterlispvm_state *res = jitterlisp_unused_states.first;
      jitterlispvm_state_reset (res);
      jitterlisp_state_unused_to_used (res);
      return res;
    }
}

/* Initialise the state pool, to contain no state. */
static void
jitterlisp_initialize_state_pool (void)
{
  JITTER_LIST_INITIALIZE_HEADER (& jitterlisp_used_states);
  JITTER_LIST_INITIALIZE_HEADER (& jitterlisp_unused_states);
}

/* Destroy every state still in the pool.  When this function returns the pool
   will be empty. */
static void
jitterlisp_finalize_state_pool (void)
{
  /* Destroy any state in the lists of used and unused VM states. */
  struct jitterlispvm_state *s;
  while ((s = jitterlisp_used_states.first) != NULL)
    {
      JITTER_LIST_UNLINK (jitterlispvm_state,
                          jitterlispvm_state_backing.pool_links,
                          & jitterlisp_used_states,
                          s);
      jitterlispvm_state_destroy (s);
    }
  while ((s = jitterlisp_unused_states.first) != NULL)
    {
      JITTER_LIST_UNLINK (jitterlispvm_state,
                          jitterlispvm_state_backing.pool_links,
                          & jitterlisp_unused_states,
                          s);
      jitterlispvm_state_destroy (s);
    }
}




/* Profiling.
 * ************************************************************************** */

/* A function accepting a pointer to a runtime profile, returning no result. */
typedef
void (*jitterlisp_runtime_profile_function_1)
(struct jitterlispvm_profile_runtime *spr);

/* A function accepting two pointers to runtime profiles of which the second
   pointer to const, returning no result. */
typedef
void (*jitterlisp_runtime_profile_function_2)
(struct jitterlispvm_profile_runtime *spr_accumulator,
 const struct jitterlispvm_profile_runtime *spr);

/* Clear the runtime profile of every state in the pointed list. */
static void
jitterlisp_runtime_profiles_call_1_in (struct jitter_list_header *list,
                                       jitterlisp_runtime_profile_function_1 f1)
{
  struct jitterlispvm_state *s;
  for (s = list->first;
       s != NULL;
       s = s->jitterlispvm_state_backing.pool_links.next)
    {
      struct jitterlispvm_profile_runtime *spr
        = jitterlispvm_state_profile_runtime (s);
      f1 (spr);
    }
}

/* For every element x of the pointed list call the given function on two
   runtime profiles: sp, and the profile from x. */
static void
jitterlisp_runtime_profiles_call_2_in (struct jitter_list_header *list,
                                       struct jitterlispvm_profile_runtime *sp,
                                       jitterlisp_runtime_profile_function_2 f2)
{
  struct jitterlispvm_state *s;
  for (s = list->first;
       s != NULL;
       s = s->jitterlispvm_state_backing.pool_links.next)
    {
      struct jitterlispvm_profile_runtime *spr
        = jitterlispvm_state_profile_runtime (s);
      f2 (sp, spr);
    }
}

/* Merge the runtime profile from every state in the pointed list into the
   pointed runtime profile. */
static void
jitterlisp_profile_runtime_merge_from_every_state_in
   (struct jitterlispvm_profile_runtime *pr,
    struct jitter_list_header *list)
{
  jitterlisp_runtime_profiles_call_2_in (list,
                                         pr,
                                         jitterlispvm_profile_runtime_merge_from);
}

/* Clear the runtime profile of every state in the pointed list. */
static void
jitterlisp_runtime_profiles_clear_in (struct jitter_list_header *list)
{
  jitterlisp_runtime_profiles_call_1_in (list,
                                         jitterlispvm_profile_runtime_clear);
}

struct jitterlispvm_profile_runtime *
jitterlisp_current_profile_runtime (void)
{
  /* Make a new clear state, with every count at zero. */
  struct jitterlispvm_profile_runtime *res
    = jitterlispvm_profile_runtime_make ();

  /* Merge every state from the pool into the result. */
  jitterlisp_profile_runtime_merge_from_every_state_in
     (res, & jitterlisp_used_states);
  jitterlisp_profile_runtime_merge_from_every_state_in
     (res, & jitterlisp_unused_states);

  /* The result has now been composed. */
  return res;
}

void
jitterlisp_reset_profile_runtime (void)
{
  jitterlisp_runtime_profiles_clear_in (& jitterlisp_used_states);
  jitterlisp_runtime_profiles_clear_in (& jitterlisp_unused_states);
}




/* VM initialization and finalization.
 * ************************************************************************** */

jitter_int
jitterlisp_slow_register_per_class_no;

/* The driver routine, used to call compiled closures from the interpreter.
   Jumping from interpreted to compiled code is common and must be fast: we
   keep one driver routine, always the same, to be reused. */
static struct jitterlispvm_mutable_routine *
jitterlisp_driver_vm_routine;
static struct jitterlispvm_executable_routine *
jitterlisp_driver_vm_executable_routine;

void
jitterlisp_vm_initialize (void)
{
  /* Call the Jitter-generated function initializing the VM subsystem. */
  jitterlispvm_initialize ();

  /* Initialise the unused VM state pool. */
  jitterlisp_initialize_state_pool ();

  /* At the beginning, assume zero slow registers per class.  This is useful
     for catching bugs. */
  //jitterlisp_slow_register_per_class_no = 0;
  jitterlisp_slow_register_per_class_no = 1000; // FIXME: obviously wrong.

  /* Make the driver routine and keep it ready to be use, already executable.
     The driver routine consists of exactly two instructions:
        call-from-c
        exitvm
     , of which the second is added implicitly. */
  jitterlisp_driver_vm_routine = jitterlispvm_make_mutable_routine ();
  /* In the particular case of the driver I can keep the default option
     generating a final exitvm instruction at the end.  This will not be
     the same for compiled user code. */
//JITTERLISPVM_MUTABLE_ROUTINE_APPEND_INSTRUCTION(jitterlisp_driver_vm_routine, debug);
  JITTERLISPVM_MUTABLE_ROUTINE_APPEND_INSTRUCTION(jitterlisp_driver_vm_routine,
                                                  call_mfrom_mc);
//JITTERLISPVM_MUTABLE_ROUTINE_APPEND_INSTRUCTION(jitterlisp_driver_vm_routine, debug);
//JITTERLISPVM_MUTABLE_ROUTINE_APPEND_INSTRUCTION(jitterlisp_driver_vm_routine, debug);
  jitterlisp_driver_vm_executable_routine
    = jitterlispvm_make_executable_routine (jitterlisp_driver_vm_routine);
}

void
jitterlisp_vm_finalize (void)
{
  /* Destroy the driver routine and invalidate its pointer to catch mistakes. */
  jitterlispvm_destroy_executable_routine
     (jitterlisp_driver_vm_executable_routine);
  jitterlispvm_destroy_mutable_routine (jitterlisp_driver_vm_routine);
  jitterlisp_driver_vm_routine = NULL;
  jitterlisp_driver_vm_executable_routine = NULL;

  /* Destroy any state existing in the pool (unused or not), and make the lists
     empty. */
  jitterlisp_finalize_state_pool ();

  /* Call the Jitter-generated function finalizing the VM subsystem. */
  jitterlispvm_finalize ();
}




/* Call into VM code.
 * ************************************************************************** */

/* FIXME: is there a sensible way of making the interpreter->vm interaction a
   tail call? */

/* Jump to the first program point of the driver on the current VM state, whose
   main stack must contain a compiled closure, its evaluated actuals and the
   (unencoded) number of actuals.  When the VM is done pop the result off the
   stack and return it.
   Use the pointed state, and return it to the unused list in the pool before
   exit so it can be reused.
   This factors the trailing part of both jitterlisp_call_compiled and
   jitterlisp_apply_compiled. */
static inline jitterlisp_object
jitterlisp_jump_to_driver_and_return_result (struct jitterlispvm_state *s)
{
#if 0
  static bool shown = false;
  if (! shown)
    {
      shown = true;
      jitterlisp_log_char_star ("Driver:\n");
      if (jitterlisp_settings.cross_disassembler)
        jitterlispvm_executable_routine_disassemble (jitterlisp_print_context, jitterlisp_driver_vm_executable_routine, true, JITTER_CROSS_OBJDUMP, NULL);
      else
        jitterlispvm_executable_routine_disassemble (jitterlisp_print_context, jitterlisp_driver_vm_executable_routine, true, JITTER_OBJDUMP, NULL);
      jitterlisp_log_char_star ("\n");
    }
#endif

  /* Run the driver which will immediately call the closure, leaving only
     the result on the stack. */
  jitterlispvm_execute_executable_routine
     (jitterlisp_driver_vm_executable_routine, s);

  /* Pop the result off the stack and return it (it would in fact not be really
     necessary to alter the stack, since the state is going to be reset before
     being used again).  Before doing so, link the state to the unused list in
     the pool so it can be reused. */
  jitterlisp_object res = JITTERLISPVM_TOP_MAINSTACK (s);
  JITTERLISPVM_DROP_MAINSTACK (s);
  jitterlisp_state_used_to_unused (s);
  return res;
}

jitterlisp_object
jitterlisp_call_compiled (jitterlisp_object rator_value,
                          const jitterlisp_object *rand_asts,
                          jitter_uint rand_ast_no,
                          jitterlisp_object env)
{
  /* Get a VM state from the pool. */
  struct jitterlispvm_state *s = jitterlisp_get_state ();

  /* Here I can be sure that operator_value is a compiled closure. */
  const struct jitterlisp_closure *c = JITTERLISP_CLOSURE_DECODE(rator_value);

  // FIXME: shall I check the arity *before* evaluating actuals or after?
  // Here I do it before, but it's easy to change.
  // In either case compiled code must have the same semantics.  Do whatever
  // is faster on compiled code.  There are two other identical cases below.

  /* Check that the arity matches. */
  if (__builtin_expect ((c->in_arity != rand_ast_no), false))
    jitterlisp_error_cloned ("arity mismatch in call to compiled closure");

  /* Push the compiled closure, tagged, on the VM stack. */
  JITTERLISPVM_PUSH_MAINSTACK (s, rator_value);

  // FIXME: handle errors without leaving observable changes on the stack:
  // that doesn't need to be done here: error recovery code in the REPL should
  // simply reset the state.

  /* Evaluate actuals and push them on the VM stack. */
  int i;
  for (i = 0; i < rand_ast_no; i ++)
    {
      jitterlisp_object ith_rand
        = jitterlisp_eval_interpreter_ast (rand_asts [i], env);
      JITTERLISPVM_PUSH_MAINSTACK (s, ith_rand);
    }

  /* Push the number of closure operands plus one, unencoded.  The driver
     programs expects this last operand on the top of the stack, to be able to
     find the closure below. */
  JITTERLISPVM_PUSH_MAINSTACK (s, rand_ast_no + 1);

  /* Pass control to VM code, which will also destroy the state. */
  return jitterlisp_jump_to_driver_and_return_result (s);
}

jitterlisp_object
jitterlisp_apply_compiled (jitterlisp_object rator_value,
                           jitterlisp_object rand_value_list)
{
  /* Get a VM state from the pool. */
  struct jitterlispvm_state *s = jitterlisp_get_state ();

  // FIXME: handle errors without leaving observable changes on the stack:
  // that doesn't need to be done here: error recovery code in the REPL should
  // simply reset the state.

  /* Here I can be sure that operator_value is a compiled closure. */
  const struct jitterlisp_closure *c = JITTERLISP_CLOSURE_DECODE(rator_value);

  /* Push the compiled closure on the VM stack. */
  JITTERLISPVM_PUSH_MAINSTACK (s, rator_value);

  /* Push the actuals on the VM stack.  No need to check that the arguments
     actually come in a list. */
  jitterlisp_object rest = rand_value_list;
  jitter_uint provided_in_arity = 0;
  while (! JITTERLISP_IS_EMPTY_LIST(rest))
    {
      JITTERLISPVM_PUSH_MAINSTACK (s, JITTERLISP_EXP_C_A_CAR(rest));
      rest = JITTERLISP_EXP_C_A_CDR(rest);
      provided_in_arity ++;
    }

  /* Check that the arity matches. */
  if (__builtin_expect ((c->in_arity != provided_in_arity), false))
    jitterlisp_error_cloned ("arity mismatch in apply to compiled closure");

  /* Push the number of closure operands plus one, unencoded.  The driver
     routine expects this last operand on the top of the stack, to be able to
     find the closure below. */
  JITTERLISPVM_PUSH_MAINSTACK (s, provided_in_arity + 1);

  /* Pass control to VM code, destroy the state and return the result. */
  return jitterlisp_jump_to_driver_and_return_result (s);
}
