/* Instruction rewrite functionality header.

   Copyright (C) 2017, 2018 Luca Saiu
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


#ifndef JITTER_REWRITE_H_
#define JITTER_REWRITE_H_

#include <jitter/jitter.h>
#include <jitter/jitter-instruction.h>
#include <jitter/jitter-mutable-routine.h>


/* Rewriter API.
 * ************************************************************************** */

/* Instruction rewriting entails replacing a sequence of one or more
   unspecialized instructions with another sequence of zero or more equivalent
   unspecified instructions; each instruction in the replacement will be
   specializable, while the original instruction might not be.

   There are two distinct use cases for rewriting:
     (a) peephole optimization;
     (b) reducing the number of specialized instructions in order to make the
         generated C code smaller, possibly enabling the user of more fast
         registers.
   Case (b) is important, but is not exploited yet.

   About case (b):
   The rewriting mechanism can reduce the number of specialized instructions by
   removing redundant ways of describing the same operation.  Let us assume for
   example a meta-instruction add (?r, ?r, !r); then the two unspecialized
   instructions
     add %r0, %r1, %r2
   and
     add %r1, %r0, %r2
   have the same semantics because of commutativity, and we can systematically
   rewrite one of them into the other (for example the one having input register
   indices in non-decreasing order), arbitrarily chosen as the normal form.
   This cuts the specialized instructions in half, allowing the user to compile
   a Jittery system using less resources, or to keep using the same resources
   but affording a higher number of fast registers.

   Other rewritings yield slightly suboptimal code, but reduce the specialized
   instruction set even more dramatically.  The add instruction above has O(n^3)
   specializations, but by requring the second and third arguments to be equal
   we can cut specialized instructions to O(n^2).  Assuming a register copying
   meta-instruction mov (?r, !r) such a rewriting would require replacing, for
   example,
     add %r0, %r1, %r2
   with the two-instruction sequence
     mov %r1, %r2
     add %r0, %r2, %r2
   . Other add instructions already having the same register as their second
   and third arguments would not be rewritten.

   Rewriting happens after each instruction is completed, by a call to
   jitter_mutable_routine_append_instruction_name or one of the jitter_mutable_routine_append_*_parameter
   functions.  Rewriting can never happen "across" a label: an original
   unspecialized instruction before rewriting is replaced with a sequence, and
   both the original and the rewritten sequence lie completely before or
   completely after each label: it is not possible, by construction, to have
   part of the replacement before and part of the replacement after a label.

   As an exception the implicit label added after a callee instruction is not
   restricted by the limitation above, at least for the purposes of case (a): it
   is possible, and in fact useful, to rewrite a sequence containing a call
   instruction into a more efficient sequence.  As always the user has the
   responsibility of ensuring that her rewritten code is semantically equivalent
   to the original, but calls seem to present no particular problem. */




/* This API is not for the user.
 * ************************************************************************** */

/* The functions and macros delcared here are used internally by the rewriter or
   by generated code, and not intended for the user.  The user specifies rewrite
   rules as part of the Jitter VM speciefication, using Jitter syntax. */




/* Rewriter entry point.
 * ************************************************************************** */

/* The rewriter entry point is the machine-generated vmprefix_rewrite , called
   at instruction closing thru the rewrite function pointer within struct
   jitter_vm.

   See the comment for vmprefix_rewrite in the template header. */




/* Rewriter internal functions.
 * ************************************************************************** */

/* These functions are used internally for the implementation of
   vmprefix_rewrite. */

/* Return a pointer to the last instruction in the given program; the returned
   pointer refers the only instance of the instruction, and is not a copy.  Fail
   fatally if no instructions exist or if the last one is not complete, or if the
   program is not unspecialized. */
struct jitter_instruction*
jitter_last_instruction (struct jitter_mutable_routine *p);

/* Return a pointer pointing within an array of pointers to instructions,
   how_many elements from the last one.  The pointed memory may be invalidated
   by any instruction modification, so this is only meant for *reading* the last
   few instructions in order to check whether a rewrite rule applies.
   The intended way of using this is for checking whether a rule applying
   to N instruction can fire: it will be called with how_many = N, and
   the result will be an array of N pointers to the last instructions. */
/* Fail fatally if no instructions exist or if the last one is not complete, or
   if the program is not unspecialized.  [FIXME: possibly change this] */
struct jitter_instruction**
jitter_last_instructions (struct jitter_mutable_routine *p, size_t how_many);

/* Pop the last instruction off the pointed program, which must be complete, and
   return a pointer to it; fail fatally if the instruction is not complete or
   does not exist, or if the program is unspecialized.  This removes the pointer
   to the instruction from the dynamic buffer in the program, but does not
   deallocate the instruction: in fact the result of this function is the
   pointer to the still-valid instruction, allocated according to the
   conventions in jitter-instruction.h .  It is the caller's responsibility to
   release memory after usage, normally by a call to jitter_destroy_instruction
   .  The intended use case for this is fisrt checking whether a replacement
   should occour, and then if so calling jitter_pop_instruction once; the result
   pointer will be used to determine which instructions to append as a
   replacement, by the usual jitter_mutable_routine_append_instruction_name /
   jitter_mutable_routine_append_*_parameter functions, which may in their turn trigger rewrites
   -- or by jitter_mutable_routine_append_instruction, which also triggers rewrites.

   Notice that rewrites are applied eagerly, as soon as each appended
   instruction is complete and until no more rewrites are possible.  For this
   reason simply removing instructions from the end of a program never triggers
   new rewrites, and therefore the caller of this function can assume that after
   the function returns every program instruction before the last one which was
   removed remain unchanged.

   It is the user's responsiblity to ensure that her rewrite rules don't loop
   forever. */
struct jitter_instruction*
jitter_pop_instruction (struct jitter_mutable_routine *p);

/* Pop the last how_many instructions from the given program, and destroy them.
   Undefined behavior if the program has less than how_many rewritable
   instructions.  The last instruction must be complete.

   Notice, again, that destroying the last how_many instructions of a program
   can never trigger a rewrite: see the jitter_pop_instruction comment for a
   justification. */
void
jitter_destroy_last_instructions (struct jitter_mutable_routine *p,
                                  size_t how_many);




/* Fixed code for vmprefix_rewrite.
 * ************************************************************************** */

/* Expand to C declarations and statements to be emitted at the beginning of
   vmprefix_rewrite .  This macro is used in the machine-generated definition of
   vmprefix_rewrite .  It may expand to a sequence of statements and
   declarations of variables to be visible to the entire function, so it is not
   do..while(false)-protected. */
#define JITTTER_REWRITE_FUNCTION_PROLOG_                                      \
  /* How many instructions are rewritable in the current program.  This */    \
  /* value will be used a lot in the function, and it's better to cache */    \
  /* it in an automatic const variable rather than re-loading it from */      \
  /* memory every time, in a context where GCC might not be able to infer */  \
  /* that the value is, in fact, constant. */                                 \
  const int jitter_rewritable_instruction_no =                                \
    jitter_mutable_routine_p->rewritable_instruction_no;                      \
  /* A pointer to the first instruction which is potentially a candidate */   \
  /* for rewriting, with any rule.  Making this a constant pointer to */      \
  /* constant data should help GCC to share condition computations across */  \
  /* rules; this is correct, as rule conditions don't in fact change */       \
  /* instructions -- Only if a rule matches some memory changes are made, */  \
  /* and in that case we exit this function after the rule section ends. */   \
  const struct jitter_instruction * const * const                             \
     jitter_all_rewritable_instructions __attribute__ ((unused))              \
       = ((const struct jitter_instruction * const * const)                   \
          jitter_last_instructions (jitter_mutable_routine_p,                 \
                                    jitter_rewritable_instruction_no))





/* Rewriting macros for rule compilation: introduction.
 * ************************************************************************** */

/* The macros in the following sections provide a facility for executing rewrite
   rules at runtime (not at generation time) from machine-generated C, expressed
   in a structured, abstract way.  The expanded C code is meant to be fast in
   the more common case, which is to say when rules do *not* fire.

   Each rewrite rule is translated into a C conditional with a complex condition
   made of clauses all in logical and, having side effects on local variables,
   which eventually evaluates to true if the rule fires.  In that case some head
   instruction arguments are cloned heap-to-heap into templates, head
   instruction destroyed, and new instructions appended in their place, using
   the cloned placeholder to instantiate the rule template; after the new
   instructions are appended the cloned templates are destroyed, and the
   rewriting function returns.  When a rule does not match, instead, evaluation
   falls thru to the conditional for the next rule.

   The rewriting process is inherently recursive: appending an instruction, be
   it from the user C code or from another rewrite, may in its turn trigger a
   rewrite.  This recursion is indirect and spans compilation units, which can
   make it difficult for the C compiler to optimize away; however recursive
   calls are at least in tail position when there are no templates to destroy.

   Notice that macro-level rule "sections" don't match the statement nesting of
   C-level blocks: in particular the rule conditional body is opened by
   JITTER_RULE_END_CONDITIONS and closed by
   JITTER_RULE_END_PLACEHOLDER_DESTRUCTION .  Macro-level sections are nested in
   a rigid, fixed way, to keep the generated code legible.  Their C
   implementation is focused on exploiting C's statement evaluation in an
   appropriate way (in particular, there are complex side-effects within an if
   conditional) and that C variables have the intended scope.
   Some sections may be empty, but their opening and closing is mandatory for
   every rule, in the required order.

   An example of the rewrite rule changing
       pop $a; push $a
   into
       copy-to-r $a
   translated into C:

   JITTER_RULE_BEGIN(2)
     JITTER_RULE_BEGIN_PLACEHOLDER_DECLARATIONS
       JITTER_RULE_DECLARE_PLACEHOLDER_(a);
     JITTER_RULE_END_PLACEHOLDER_DECLARATIONS
     JITTER_RULE_BEGIN_CONDITIONS
       JITTER_RULE_CONDITION_MATCH_OPCODE(0, pop)
       JITTER_RULE_CONDITION_MATCH_OPCODE(1, push)
       JITTER_RULE_CONDITION_MATCH_PLACEHOLDER(0, 0, a)
       JITTER_RULE_CONDITION_MATCH_PLACEHOLDER(1, 0, a)
     JITTER_RULE_END_CONDITIONS
     JITTER_RULE_BEGIN_PLACEHOLDER_CLONING
       JITTER_RULE_CLONE_PLACEHOLDER_(a);
     JITTER_RULE_END_PLACEHOLDER_CLONING
     JITTER_RULE_BEGIN_BODY
       JITTER_RULE_APPEND_INSTRUCTION_(copy_mto_mr);
       JITTER_RULE_APPEND_PLACEHOLDER_(a);
     JITTER_RULE_END_BODY
     JITTER_RULE_BEGIN_PLACEHOLDER_DESTRUCTION
       JITTER_RULE_DESTROY_PLACEHOLDER_(a);
     JITTER_RULE_END_PLACEHOLDER_DESTRUCTION
   JITTER_RULE_END  */




/* Rewriting macros for rule compilation: sections.
 * ************************************************************************** */

/* Open a rule section, for a rule whose head matches the given number of VM
   instructions.  This must be followed by a call to
   JITTER_RULE_BEGIN_PLACEHOLDER_DECLARATIONS . */
#define JITTER_RULE_BEGIN(_jitter_head_instruction_no)                         \
  { /* Begin the rule block.  */                                               \
    /* Set the head size as a variable, so that we don't have to pass */       \
    /* _jitter_head_instruction_no to multiple macros. */                      \
    const int jitter_head_instruction_no = (_jitter_head_instruction_no);      \
    /* A pointer to the first instruction which is potentially a */            \
    /* candidate for rewriting, with the current rule.  This might not */      \
    /* even point to a valid instruction: we have to check how many */         \
    /* rewritable instructions there are before using it. */                   \
    const struct jitter_instruction * const * const                            \
       jitter_candidate_instructions __attribute__ ((unused))                  \
         = (jitter_all_rewritable_instructions                                 \
            + jitter_rewritable_instruction_no - jitter_head_instruction_no);  \
    /* This will contain placeholder variable declarations, and then */        \
    /* the rule conditional. */

/* Close a rule section. */
#define JITTER_RULE_END          \
  } /* Close the rule block. */

/* Open the placeholder declaration section.  This must come right after the
   JITTER_RULE_BEGIN call. */
#define JITTER_RULE_BEGIN_PLACEHOLDER_DECLARATIONS                            \
  /* Nothing, not even a "{": we need the coming variable declarations to */  \
  /* be visible in the following code, up until placeholder destruction. */

/* Close the placeholder declaration section. */
#define JITTER_RULE_END_PLACEHOLDER_DECLARATIONS  \
  /* Nothing, not even a "}".  See above. */

/* Begin the rule condition section.  This must come right after placeholder
   declarations.  The condition sections may contain any number of conditions,
   which must *all* evaluate to true for the rule to fire. */
#define JITTER_RULE_BEGIN_CONDITIONS                        \
  if (   (jitter_rewritable_instruction_no                  \
            >= (jitter_head_instruction_no))                \
      /* Here will come the other conditions, all in && */
      /* with one another. */

/* Close the rule condition section. */
#define JITTER_RULE_END_CONDITIONS                                          \
     ) /* Close the if condition */                                         \
    { /* Begin the rule conditional body, executed when the rule fires. */  \
      /* The rule conditional body is distinct from the rule body! */

/* Open the block holding placeholder cloning calls.  This block must occur
   after rule conditions and before the rule body. */
#define JITTER_RULE_BEGIN_PLACEHOLDER_CLONING  \
  { /* Open the placeholder cloning block. */

/* Close the block holding placeholder cloning calls. */
#define JITTER_RULE_END_PLACEHOLDER_CLONING     \
  } /* Close the placeholder cloning block. */

/* Open the block holding placeholder destructions calls.  This block must occur
   after the rule body. */
#define JITTER_RULE_BEGIN_PLACEHOLDER_DESTRUCTION      \
    { /* Open the placeholder destruction block... */

/* Close the block holding placeholder destructions calls. */
#define JITTER_RULE_END_PLACEHOLDER_DESTRUCTION                  \
    } /* ...Close the placeholder destruction block. */          \
    /* One rewrite rule fired, and if we have appended new */    \
    /* instructions they have been rewritten as well.  Done. */  \
    return; /*goto jitter_rewrite_again_label;*/                 \
  } /* Close the rule conditional body. */

/* Open the rule body section.  This must come after placeholder cloning. */
#define JITTER_RULE_BEGIN_BODY                                               \
  { /* Open the rule body block, which occurs within the rule */             \
    /* conditional body. */                                                  \
    /* Destroy the instructions matching the rule head.  We have already */  \
    /* copied the arguments we need for rewriting. */                        \
    jitter_destroy_last_instructions (jitter_mutable_routine_p,              \
                                      jitter_head_instruction_no);           \
    /* From here on it's incorrect to use head instructions or non-cloned */ \
    /* placeholders. */

/* Close the rule body section. */
#define JITTER_RULE_END_BODY                                             \
  } /* Close the rule body block, but not the rule conditional body. */  \
    /* That will be closed later, after placeholders are destroyed. */




/* Rewriting macros for rule compilation: placeholders.
 * ************************************************************************** */

/* The name of a placeholder variable as a C identifier. */
#define JITTER_PLACEHOLDER_NAME(_jitter_suffix)                \
  JITTER_CONCATENATE_TWO(jitter_placeholder_, _jitter_suffix)

/* Expand to a pointer variable declaration for the given placeholder, also
   initializing the pointer to NULL so that it matches any instruction argument.
   It is sensible to declare the variable to point to const data: this will
   allow GCC to share checks on arguments across different rule conditions;
   arguments are in fact not modified, until one rule matches -- and then
   the other rules become irrelevent.  Not do..while(false)-protected. */
#define JITTER_RULE_DECLARE_PLACEHOLDER_(_jitter_placeholder_name)  \
  const struct jitter_parameter *                                   \
     JITTER_PLACEHOLDER_NAME(_jitter_placeholder_name)              \
       = NULL

/* Replace the pointer variable for the given placeholder, which must be
   non-NULL, with a pointer to a freshly cloned argument.  This way the
   placeholder will be usable from the rule body to fill the rule template,
   after the matching head instructions are destroyed.  Not
   do..while(false)-protected. */
#define JITTER_RULE_CLONE_PLACEHOLDER_(_jitter_placeholder_name)  \
  JITTER_PLACEHOLDER_NAME(_jitter_placeholder_name)               \
    = jitter_clone_instruction_parameter                          \
         (JITTER_PLACEHOLDER_NAME(_jitter_placeholder_name))

/* Destroy the instruction parameter pointed by the local placeholder variable
   for the named placeholder.  The pointer must be non-NULL.  Not
   do..while(false)-protected. */
#define JITTER_RULE_DESTROY_PLACEHOLDER_(_jitter_placeholder_name)  \
  jitter_destroy_instruction_parameter                              \
     ((struct jitter_parameter *)                                   \
      JITTER_PLACEHOLDER_NAME(_jitter_placeholder_name));




/* Rewriting macros for rule compilation: instruction/argument access.
 * ************************************************************************** */

/* Expand to a struct jitter_instruction * r-value evaluating to the
   _jitter_instruction_idx-th (0-based) candidate instruction.
   This is useful for referring to the first, second, and so on, instrction
   among the ones matching the head of a rule. */
#define JITTER_RULE_INSTRUCTION(_jitter_instruction_idx)     \
  (jitter_candidate_instructions [_jitter_instruction_idx])

/* Expand to a struct jitter_parameter * r-value evaluating to the
   _jitter_argument_idx-th (0-based) argument of the
   _jitter_instruction_idx-th (0-based) candidate instruction.
   This is useful for referring to a given argument of a given instruction
   among the ones matching the head of a rule. */
#define JITTER_RULE_INSTRUCTION_ARGUMENT(_jitter_instruction_idx,  \
                                         _jitter_argument_idx)     \
  (JITTER_RULE_INSTRUCTION(_jitter_instruction_idx)                \
     ->parameters [_jitter_argument_idx])

/* Expand to an r-value of type enum jitter_parameter_type evaluating to the
   actual type of the given argument. */
#define JITTER_RULE_ARGUMENT_TYPE(_jitter_instruction_idx,    \
                                  _jitter_argument_idx)       \
  (JITTER_RULE_INSTRUCTION_ARGUMENT(_jitter_instruction_idx,  \
                                    _jitter_argument_idx)     \
      ->type)




/* Rewriting macros for rule compilation: macros for argument field extraction.
 * ************************************************************************** */

/* Given an r-value evaluating to a pointer to a (possibly const) struct
   jitter_parameter , expand to an r-value which evaluates to its content as
   literal.  The argument type is not checked. */
#define JITTER_RULE_LITERAL_FIELD(_jitter_argument_expression)  \
  ((_jitter_argument_expression)->literal)

/* Given an r-value evaluating to a pointer to a (possibly const) struct
   jitter_parameter , expand to an r-value which evaluates to its register-index
   content.  The argument type is not checked. */
#define JITTER_RULE_REGISTER_INDEX_FIELD(_jitter_argument_expression)  \
  ((_jitter_argument_expression)->register_index)




/* Rewriting macros for rule compilation: macros for condition evaluation.
 * ************************************************************************** */

/* Expand to a boolean r-value, evaluating to true iff the given actual
   argument is a register. */
#define JITTER_RULE_ARGUMENT_IS_A_REGISTER(_jitter_instruction_idx,  \
                                           _jitter_argument_idx)     \
  (JITTER_RULE_ARGUMENT_TYPE(_jitter_instruction_idx,                \
                              _jitter_argument_idx)                  \
   == jitter_parameter_type_register_id)

/* Expand to a boolean r-value, evaluating to true iff the given actual
   argument is a literal. */
#define JITTER_RULE_ARGUMENT_IS_A_LITERAL(_jitter_instruction_idx,  \
                                          _jitter_argument_idx)     \
  (JITTER_RULE_ARGUMENT_TYPE(_jitter_instruction_idx,               \
                             _jitter_argument_idx)                  \
   == jitter_parameter_type_literal)

/* Expand to a boolean r-value, evaluating to true iff the given actual
   argument is a label -- there is no distinction here between fast
   and slow labels. */
#define JITTER_RULE_ARGUMENT_IS_A_LABEL(_jitter_instruction_idx,  \
                                        _jitter_argument_idx)     \
  (JITTER_RULE_ARGUMENT_TYPE(_jitter_instruction_idx,             \
                             _jitter_argument_idx)                \
   == jitter_parameter_type_label)




/* Rewriting macros for rule compilation: conditions.
 * ************************************************************************** */

/* Expand to a condition, to be &&'ed to the previous conditions in the rule
   conditional.  A condition expression may have side effects, and its result
   must be non-zero if evaluation of the following conditions is to continue;
   when a condition expression evaluates to zero the following conditions are
   not evaluated, and the current rule does not fire.
   A condition must not have side effects visible out of the current rule
   block, since any following condition may fail.  Setting placeholders is
   allowed because placeholder variables are just pointers local to the block,
   and don't refer to newly allocated memory during the condition evaluation.
   Only if the condition eventually evaluates to a non-zero value
   placeholders are cloned, and head instructions are destroyed: at that point
   the rule is known to fire, and its effects will be visible. */
#define JITTER_RULE_CONDITION(_jitter_condition_expression)  \
      && (_jitter_condition_expression)

/* Expand to a condition on the opcode of the _jitter_instruction_idx-th
   instruction (0-based, from the first head candidate).  Conditions like these
   should always be the first ones in a rule conditional: after they have all
   matched we can safely access instruction arguments knowing that the arities
   will be what we are expecting from their respective instructions.
   The opcode is given with the same conventions as the second argument of
   VMPREFIX_APPEND_INSTRUCTION . */
#define JITTER_RULE_CONDITION_MATCH_OPCODE(_jitter_instruction_idx,        \
                                           _jitter_mangled_opcode_suffix)  \
  JITTER_RULE_CONDITION(                                                   \
     (JITTER_RULE_INSTRUCTION(_jitter_instruction_idx)                     \
         ->meta_instruction->id)                                           \
      == JITTER_CONCATENATE_THREE(JITTER_VM_PREFIX_LOWER_CASE,             \
                                  _meta_instruction_id_,                   \
                                  _jitter_mangled_opcode_suffix))

/* Match the _jitter_argument_idx-th (0-based) argument of the
   _jitter_instruction_idx-th (0-based) instruction with the given
   placeholder.
   - if the placeholder is NULL, set it to the argument pointer and evaluate to
     true;
   - if the placeholder is already set, check if it's equal to the given
     parameter (this will occur in non-linear patterns): if so evaluate to true,
     otherwise evaluate to false.
     In either case the parameter is not cloned: the placeholder at this
     point may only hold a pointer to an *existing* heap-allocated datum. */
#define JITTER_RULE_CONDITION_MATCH_PLACEHOLDER(_jitter_instruction_idx,   \
                                                _jitter_argument_idx,      \
                                                _jitter_placeholder_name)  \
  JITTER_RULE_CONDITION(                                                   \
     ((JITTER_PLACEHOLDER_NAME(_jitter_placeholder_name) == NULL)          \
      ? ((JITTER_PLACEHOLDER_NAME(_jitter_placeholder_name)                \
            = JITTER_RULE_INSTRUCTION_ARGUMENT(_jitter_instruction_idx,    \
                                               _jitter_argument_idx))      \
         /* Without ", true" GCC would complain about ? : having a */      \
         /* pointer and and integer on different branches. */              \
         , true)                                                           \
      : jitter_instruction_parameters_equal                                \
           (JITTER_PLACEHOLDER_NAME(_jitter_placeholder_name),             \
            JITTER_RULE_INSTRUCTION_ARGUMENT(_jitter_instruction_idx,      \
                                             _jitter_argument_idx))))

/* Expand to a rule condition like above, evaluating to true iff the mentioned
   instruction argument is a literal with the given value.  Do not check the
   register class, as each instruction register argument can have only one, and
   that has been already checked statically.  No side effects. */
#define JITTER_RULE_CONDITION_MATCH_REGISTER_ARGUMENT(_jitter_instruction_idx, \
                                                      _jitter_argument_idx,    \
                                                      _jitter_register_index)  \
  JITTER_RULE_CONDITION(                                                       \
        JITTER_RULE_ARGUMENT_IS_A_REGISTER(_jitter_instruction_idx,            \
                                           _jitter_argument_idx)               \
     && (JITTER_RULE_INSTRUCTION_ARGUMENT(_jitter_instruction_idx,             \
                                          _jitter_argument_idx)                \
           ->register_index                                                    \
         == (_jitter_argument_value)))

/* Expand to a rule condition like above, evaluating to true iff the mentioned
   instruction argument is a register with the given index.  No side effects. */
#define JITTER_RULE_CONDITION_MATCH_LITERAL_ARGUMENT(_jitter_instruction_idx, \
                                                     _jitter_argument_idx,    \
                                                     _jitter_argument_value)  \
  JITTER_RULE_CONDITION(                                                      \
        JITTER_RULE_ARGUMENT_IS_A_LITERAL(_jitter_instruction_idx,            \
                                          _jitter_argument_idx)               \
     && (JITTER_RULE_LITERAL_FIELD(                                           \
            JITTER_RULE_INSTRUCTION_ARGUMENT(_jitter_instruction_idx,         \
                                             _jitter_argument_idx))           \
               .fixnum                                                        \
         == (_jitter_argument_value)))

/* Expand to the predicate JITTER_RULE_ARGUMENT_IS_A_REGISTER within a
   condition.  Like in the case of JITTER_RULE_ARGUMENT_IS_A_REGISTER there is
   no need to supply a register class; see its comment. */
#define JITTER_RULE_CONDITION_ARGUMENT_IS_A_REGISTER(_jitter_instruction_idx,  \
                                                     _jitter_argument_idx)     \
  JITTER_RULE_CONDITION(                                                       \
     JITTER_RULE_ARGUMENT_IS_A_REGISTER(_jitter_instruction_idx,               \
                                        _jitter_argument_idx))

/* Expand to the predicate JITTER_RULE_ARGUMENT_IS_A_LITERAL within a
   condition. */
#define JITTER_RULE_CONDITION_ARGUMENT_IS_A_LITERAL(_jitter_instruction_idx,  \
                                                    _jitter_argument_idx)     \
  JITTER_RULE_CONDITION(                                                      \
     JITTER_RULE_ARGUMENT_IS_A_LITERAL(_jitter_instruction_idx,               \
                                       _jitter_argument_idx))

/* Expand to the predicate JITTER_RULE_ARGUMENT_IS_A_LABEL within a
   condition.  There is no distinction between fast and slow labels. */
#define JITTER_RULE_CONDITION_ARGUMENT_IS_A_LABEL(_jitter_instruction_idx,  \
                                                  _jitter_argument_idx)     \
  JITTER_RULE_CONDITION(                                                    \
     JITTER_RULE_ARGUMENT_IS_A_LABEL(_jitter_instruction_idx,               \
                                     _jitter_argument_idx))




/* Rewriting macros for rule compilation: template expressions.
 * ************************************************************************** */

/* Template expressions expand to C r-values, having access to constants and
   placeholders but not to the original instructions, which except for the case
   of the rule guard have already been destroyed.
   The expansion has no side effects except in the case of errors, which should
   never happen if typechecking has passed.
   The C type of the expansion depends on the context, and is checked
   statically. */

/* Expand to a template expression evaluating to the boolean constants true and
   false. */
#define JITTER_RULE_EXPRESSION_TRUE  \
  true
#define JITTER_RULE_EXPRESSION_FALSE  \
  false

/* Expand to a template expression evaluating to an integer expression
   containing the given constant. */
#define JITTER_RULE_EXPRESSION_INTEGER(_jitter_literal1)  \
  (_jitter_literal1)

/* Expand to a template expression evaluating to a boolean expression combining
   the given operands with the appropriate connective. */
#define JITTER_RULE_EXPRESSION_LOGICAL_AND(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) && (_jitter_e2))
#define JITTER_RULE_EXPRESSION_LOGICAL_OR(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) || (_jitter_e2))
#define JITTER_RULE_EXPRESSION_LOGICAL_NOT(_jitter_e1)  \
  (! (_jitter_e1))

/* Expand to a template expression evaluating to an arithmetic expression
   combining the given operands with the appropriate operation. */
#define JITTER_RULE_EXPRESSION_PLUS(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) + (_jitter_e2))
#define JITTER_RULE_EXPRESSION_MINUS(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) - (_jitter_e2))
#define JITTER_RULE_EXPRESSION_TIMES(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) * (_jitter_e2))
#define JITTER_RULE_EXPRESSION_DIVIDED(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) / (_jitter_e2))
#define JITTER_RULE_EXPRESSION_REMAINDER(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) % (_jitter_e2))

/* Expand to a template boolean expression comparing the given arithmetic
   operands with the appropriate. */
#define JITTER_RULE_EXPRESSION_EQUAL(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) == (_jitter_e2))
#define JITTER_RULE_EXPRESSION_NOTEQUAL(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) != (_jitter_e2))
#define JITTER_RULE_EXPRESSION_LESS(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) < (_jitter_e2))
#define JITTER_RULE_EXPRESSION_LESSEQUAL(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) <= (_jitter_e2))
#define JITTER_RULE_EXPRESSION_GREATER(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) > (_jitter_e2))
#define JITTER_RULE_EXPRESSION_GREATEREQUAL(_jitter_e1, _jitter_e2)  \
  ((_jitter_e1) >= (_jitter_e2))




/* Rewriting macros for rule compilation: body.
 * ************************************************************************** */

/* Append the instruction opcode whose mangled suffix (see the comment for
   VMPREFIX_APPEND_INSTRUCTION ) is given; the arguments, if any, need to be
   added explicitly after calling this macro.  Not
   do..while(false)-protected. */
#define JITTER_RULE_APPEND_INSTRUCTION_(_jitter_mangled_suffix)  \
  JITTER_CONCATENATE_TWO(JITTER_VM_PREFIX_UPPER_CASE,            \
                         _MUTABLE_ROUTINE_APPEND_INSTRUCTION)    \
     (jitter_mutable_routine_p, _jitter_mangled_suffix)

/* Append a copy of the parameter pointed by the named placeholder variable,
   which must be non-NULL, to the current instruction.  Not
   do..while(false)-protected. */
#define JITTER_RULE_APPEND_PLACEHOLDER_(_jitter_placeholder_name)          \
  jitter_mutable_routine_append_parameter_copy (jitter_mutable_routine_p,  \
                                JITTER_PLACEHOLDER_NAME(                   \
                                   _jitter_placeholder_name));


#endif // #ifndef JITTER_REWRITE_H_
