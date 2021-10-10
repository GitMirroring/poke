/* Jittery structured language example: code generator common machinery.

   Copyright (C) 2017 Luca Saiu
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


#ifndef STRUCTURED_CODE_GENERATOR_H_
#define STRUCTURED_CODE_GENERATOR_H_

#include "structuredvm-vm.h"
#include "structured-syntax.h"

/* Core type definitions.
 * ************************************************************************** */

/* The type definition for a register index.  It is a very good idea for this to
   be signed, to make it possible to represent special invalid values as
   well. */
typedef int
structured_register_index;

/* A temporary identifier, unique within a structured program. */
typedef int
structured_temporary;




/* Compile-time environment: initialization, finalization.
 * ************************************************************************** */

/* A static environment structure contains a datum-to-register-index
   mapping.  This is used as an abstract type, the actual definition
   being in structured-code-generator.h */
struct structured_static_environment;

/* Return a pointer to a fresh static environment. */
struct structured_static_environment*
structured_static_environment_make (void);

/* Free resources for the pointed static environment. */
void
structured_static_environment_destroy (struct structured_static_environment *e);




/* Compile-time environment: access.
 * ************************************************************************** */

/* Bind a new variable with the given name in the static environment,
   associating it to a currently unused register.  Return the register index for
   the new variable.
   The given name may shadow another existing variable. */
structured_register_index
structured_static_environment_bind_variable
   (struct structured_static_environment *e,
    const structured_variable v);

/* Unbind the given variable, which must be the most recent one to be bound and
   not unbound yet, and after which no temporaries must have been bound without
   being unbound first.
   Unbinding works in a strictly LIFO fashion: only the last recently bound
   datum can be unbound, and the v argument is only used to check that this
   constraint is respected. */
void
structured_static_environment_unbind_variable
   (struct structured_static_environment *e,
    const structured_variable v);

/* Bind a new temporary with the given index in the static environment,
   associating it to a currently unused register.  Return the register index for
   the new temporary. */
structured_register_index
structured_static_environment_bind_temporary
   (struct structured_static_environment *e,
    const structured_temporary t);

/* Unbind the given temporary, which must be the most recent one to be bound and
   not unbound yet, and after which no variables must have been bound without
   being unbound first.
   Unbinding works in a strictly LIFO fashion: only the last recently bound
   datum can be unbound, and the v argument is only used to check that this
   constraint is respected.  */
void
structured_static_environment_unbind_temporary
   (struct structured_static_environment *e,
    structured_temporary t);

/* Return non-false iff the given variable is bound in the pointed
   environment. */
bool
structured_static_environment_has (struct structured_static_environment *e,
                                   const structured_variable v);

/* Return the register index associated to the given variable in the pointed
   environment.  If a variable name has been bound multiple time, return the
   register from the most recent binding.
   Fail fatally if no binding for the variable exists. */
structured_register_index
structured_static_environment_lookup_variable
   (struct structured_static_environment *e,
    const structured_variable v);

/* Return the register index associated to the given temporary in the pointed
   environment.  If a variable name has been bound multiple time, return the
   register from the most recent binding.
   Fail fatally if no binding for the variable exists. */
structured_register_index
structured_static_environment_lookup_temporary
   (struct structured_static_environment *e,
    const structured_temporary t);

/* Return a register index which is unused in the pointed environment. */
structured_register_index
structured_static_environment_fresh_register
   (struct structured_static_environment *e);

/* Return a temporary identifier which has never been used before in this
   environment. */
structured_temporary
structured_static_environment_fresh_temporary
   (struct structured_static_environment *e);

/* Return the number of registers used.  This assumes that registers are
   allocated consecutively, and that if N registers are in use then the
   highest-index register in use will have index N - 1. */
size_t
structured_static_environment_used_register_no
   (struct structured_static_environment *e);

/* Bind a procedure with the given name in the static environment to the given
   label. */
void
structured_static_environment_bind_procedure
   (struct structured_static_environment *e,
    char *procedure_name,
    structuredvm_label label_non_tail_recursive,
    structuredvm_label label_tail_recursive,
    size_t arity);

/* Return non-false iff the pointed static environment contains a binding for
   the pointed procedure name. */
bool
structured_static_environment_has_procedure
   (struct structured_static_environment *e,
    char *procedure_name);

/* Return the label associated to the pointed procedure name in the pointed
   static environment, or fail fatally if no association exists. */
structuredvm_label
structured_static_environment_lookup_procedure
   (struct structured_static_environment *e,
    char *procedure_name);

/* Same as structured_static_environment_lookup_procedure , but return the
   program point meant for tail calls. */
structuredvm_label
structured_static_environment_lookup_procedure_tail
   (struct structured_static_environment *e,
    char *procedure_name);

/* Return the arity of the given procedure.  Fail fatally if the procedure
   is not defined.*/
size_t
structured_static_environment_lookup_procedure_arity
   (struct structured_static_environment *e,
    char *procedure_name);

#endif // #ifndef STRUCTURED_CODE_GENERATOR_H_
