/* Jitter: header for VM generation-time data structures.

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


#ifndef JITTERC_JITTERC_VM_H_
#define JITTERC_JITTERC_VM_H_

/* Include the Gnulib header. */
#include <config.h>

#include <stdlib.h>
#include <stdbool.h>

#include <gl_list.h>

#include <jitter/jitter.h>
#include <jitter/jitter-hash.h>


/* The data structures defined here are used at code generation time, not at VM
   run time.

   some code factoring with the code in the jitter/ directory would be desirable
   in a few cases where duplication is just a consequence of the order the code
   was written in, as the project got more and more ambitious--my original
   experiment had a very crude code generator implemented as a shell script--but
   most of the data structures actually *need* to be distinct: most of the VM
   representation as defined here would remain unused at run time, and most
   run-time structures rely on VM-specific generated code for efficiency. */

/* All the data structures here are malloc-allocated.  We do not bother freeing
   them, since they will all only live as long as the generator, and memory use
   should be small even if the generated C files are large; we're looking at
   something in the order of the tens of thousands of specialized instructions,
   each with a few specialized arguments.  Read-only data is freely shared among
   different structures. */




/* Forward-declarations.
 * ************************************************************************** */

/* This will be defined below in this header. */
struct jitterc_vm;




/* VM registers.
 * ************************************************************************** */

/* A data structure specifying one of the register class for a VM.  For example
   a VM modeled after the m68k could have three: fixnum, address and
   floating-point; but most VMs would use a single class to keep either a fixnum
   or an address, as two fields of a union mentioned in the c_type . */
struct jitterc_register_class
{
  /* The character uniquely identifying the register class within the VM.  For
     example registers of the class with character 'r' will be called "%r0",
     "%r1", and so on.  This must be a lower-case letter. */
  char letter;

  /* The long name for this register class as given by the user. */
  char *long_name;

  /* The long name converted to all lower-case and all upper-case characters. */
  char *lower_case_long_name;
  char *upper_case_long_name;

  /* The C code specifying the type of a variable for this class.  This is a C
     type expression, without delimiters.  A malloc-allocated-string, always
     non-NULL. */
  char *c_type;

  /* An expression in C specifying the initial value for a register of this
     class, or NULL if the registers of this class are not initialised with
     valid elements. */
  char *c_initial_value;

  /* How many fast registers exist for this class.  This is int rather than
     size_t because we want to distinguish a special value of -1 as
     "uninitialised". */
  int fast_register_no;

  /* If 1, use slow registers for this class.  If 0, do not use slow registers
     for this class.  Initialised to -1, to prevent multiple settings from the
     .jitter file. */
  int use_slow_registers;
};

/* Allocate and return a new register class with default fields.  Some field
   values will be invalid and will need to be set later. */
struct jitterc_register_class*
jitterc_make_register_class (void)
  __attribute__ ((returns_nonnull));

/* Set a field of the pointed register class to a new value.  It is permitted to
   change a field from a conventionally "uninitialised" value to an initialised
   value, but not from an initialised value to another; this is to prevent
   contradictions in Jitter specifications. */
void
jitterc_vm_register_class_set_letter (struct jitterc_register_class *rc,
                                      char letter);
void
jitterc_vm_register_class_set_long_name (struct jitterc_register_class *rc,
                                         const char* long_name);
void
jitterc_vm_register_class_set_c_type (struct jitterc_register_class *rc,
                                      const char *c_type);
void
jitterc_vm_register_class_set_c_initial_value (struct jitterc_register_class *rc,
                                               const char *c_initial_value);
void
jitterc_vm_register_class_set_fast_register_no (struct jitterc_register_class
                                                *rc, int fast_register_no);
void
jitterc_vm_register_class_set_use_slow_registers (struct jitterc_register_class
                                                  *rc, int use_slow_registers);

/* Add the pointed register class to the pointed VM, checking that the fields
   are valid. */
void
jitterc_vm_add_register_class (struct jitterc_vm *vm,
                               struct jitterc_register_class* rc)
  __attribute__ ((nonnull (1, 2)));

/* Given a character identifying a register class, return a pointer to its
   register class in the pointed VM or fail fatally. */
struct jitterc_register_class*
jitterc_lookup_register_class (const struct jitterc_vm *vm, char c)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* A specific register in the machine.  This is currently used for fast
   registers only, but it would work for slow registers as well without
   changes. */
struct jitterc_register
{
  /* The class of this register. */
  struct jitterc_register_class *class;

  /* Every register within a given class is identified by a zero-based index. */
  unsigned index;
};

/* Return a pointer to a freshly-allocated register structure, the register
   being of the given class and index. */
struct jitterc_register*
jitterc_make_register (struct jitterc_register_class *class, unsigned index)
  __attribute__ ((returns_nonnull));




/* VM literals.
 * ************************************************************************** */

/* The type of a VM literal.  Right now there is only one valid case, but there
   will be more in the future.  FIXME: support unsigned fixnums. */
enum jitterc_literal_type
  {
    jitterc_literal_type_unspecified = 0,
    jitterc_literal_type_fixnum
  };

/* The value of a VM literal.  There is only one case right now. */
union jitterc_literal_value
{
  jitter_int fixnum;
};

/* A VM literal. */
struct jitterc_literal
{
  /* The literal type. */
  enum jitterc_literal_type type;

  /* The literal value. */
  union jitterc_literal_value value;
};

/* Return a pointer to a fresh literal with the given type and value*/
struct jitterc_literal*
jitterc_make_literal (enum jitterc_literal_type type,
                      union jitterc_literal_value value)
  __attribute__ ((returns_nonnull));




/* VM unspecialized instructions.
 * ************************************************************************** */

/* The mode of an instruction argument.  The mode describes whether an argument
   is used as input, as output, or both.  The possible values can be used as
   bitmasks. */
enum jitterc_instruction_argument_mode
  {
    /* Uninitialized.  This is only used during parsing, and never occurs as
       part of an instruction specification after successful parsing. */
    jitterc_instruction_argument_mode_unspecified = 0,

    /* An input argument. */
    jitterc_instruction_argument_mode_in          = 1,

    /* An output argument. */
    jitterc_instruction_argument_mode_out         = 2,

    /* An input-output argument.  This is a bitwise or of the two previous
       values. */
    jitterc_instruction_argument_mode_in_out      = 3
  };

/* The kind of an instruction argument.  The kind describes the possible
   arguments accepted by an instruction: register, literal fixnum, label, or
   a disjunction of some of them; the values can be used as bitmasks. */
enum jitterc_instruction_argument_kind
  {
    /* Uninitialized.  This is only used during parsing, and never occurs as
       part of an instruction specification after successful parsing. */
    jitterc_instruction_argument_kind_unspecified = 0,

    /* The argument is a register index; either fast or slow registers are
       indifferently accepted. */
    jitterc_instruction_argument_kind_register    = 1,

    /* The argument is a fixnum literal. */
    jitterc_instruction_argument_kind_literal     = 2,

    /* The argument is an instruction pointer literal, expressed as a label in
       human-readable input. */
    jitterc_instruction_argument_kind_label       = 4,

    /* The argument is a fast instruction pointer literal, expressed as a label
       in human-readable input just like in the previous case; the label may
       only be used for jumping to, but yields more efficient code.  This only
       works with no-threading dispatch. */
    jitterc_instruction_argument_kind_fast_label  = 8,

    /* A return address.  This argument kind is special and never occurs in
       non-specialized instructions. */
    jitterc_instruction_argument_kind_return_address = 16
  };

/* A VM unspecialized instruction argument as parsed from the Jitter
   specification.  This is a *formal* argument: there is no assiciated value.
   However the struct contains restrictions on the parameter when appropriate: a
   class for registers or a type for literals. */
struct jitterc_instruction_argument
{
  /* The argument mode: in, out or both. */
  enum jitterc_instruction_argument_mode mode;

  /* The argument kind. */
  enum jitterc_instruction_argument_kind kind;

  /* The register class character, only meaningful if the kind contains
     jitterc_instruction_argument_kind_register .

     This is a register class character and not a class pointer, in order to
     easily allow for instruction arguments to be specified before registers.
     Delaying the association between register arguments and classes is not a
     problem since every mentioned register character will be looked up anyway
     later, at specialization time. */
  char register_class_character;

  /* The literal type, only meaningful if the kind contains
     jitterc_instruction_argument_kind_literal . */
  enum jitterc_literal_type literal_type;

  /* Name of a C function printing the argument as a literal, or NULL if the
     default printer is to be used.  Only meaningful if the kind contains
     jitterc_instruction_argument_kind_literal . */
  char *c_literal_printer_name;

  /* The list of literals to specialize on, non-empty only if the kind contains
     jitterc_instruction_argument_kind_literal . */
  gl_list_t literals;
};

/* Return a freshly-allocated struct jitterc_instruction_argument . */
struct jitterc_instruction_argument*
jitterc_make_instruction_argument (void)
  __attribute__ ((returns_nonnull));

/* The hotness of a VM instruction.  Labels for hot VM instructions are given
   the "hot" GCC attribute, and cold instructions the "cold" attribute.  Since
   we disable reordering the only effect will be that hot instructions will be
   optimized for speed, and cold instructions for size. */
enum jitterc_hotness
  {
    /* The default hotness, for the VM instructions where an explicit value is
       not specified; this is used within the parser, to ensure that no more
       than one value is specified.  By default VM instructions are hot. */
    jitterc_hotness_unspecified,

    /* The VM instruction is hot. */
    jitterc_hotness_hot,

    /* The VM instruction is cold. */
    jitterc_hotness_cold
  };

/* Relocatable instructions are relocated, when using dispatching modes
   supporting it.  Non-relocatable instructions are never relocated, which is
   less efficient but yields smaller generated program code, and also allows for
   assembly code relying on the context, which cannot always be avoided on some
   architectures. */
enum jitterc_relocatability
  {
    /* The default relocatability, for the VM instructions where an explicit
       value is not specified; this is used within the parser, to ensure that no
       more than one value is specified.  [FIXME: what is it?  This will be
       equivalent to one of the two cases, but I've not decided which one
       yet.] */
    jitterc_relocatability_unspecified,

    /* The VM instruction is relocatable. */
    jitterc_relocatability_relocatable,

    /* The VM instruction is non-relocatable. */
    jitterc_relocatability_non_relocatable
  };

/* An instruction has a "caller" callerness if it may (but does not have to)
   execute a branch-and-link operation.  A "non-caller" instruction never
   executes branch-and-link operations.
   Returning instructions are not callers, unless they may also perform
   branch-and-link operations. */
enum jitterc_callerness
  {
    /* The default callerness, for the VM instructions where an explicit value
       is not specified; this is used within the parser, to ensure that no more
       than one value is specified.  If no value is explicitly given after
       parsing the callerness is automatically set to non-caller, and this
       value is never seen by the rest of the code. */
    jitterc_callerness_unspecified,

    /* The instruction may execute branch-and-link operations. */
    jitterc_callerness_caller,

    /* The instruction never executes branch-and-link operations. */
    jitterc_callerness_non_caller
  };

/* An instruction has a "callee" calleeness if it is the target of
   branch-and-link operations---and in this case, branch-and-link operations
   must be the only way to reach it.  A "non-callee" instruction may not be the
   target of branch-and-link operations.
   Return targets are not callees. */
enum jitterc_calleeness
  {
    /* The default calleeness, for the VM instructions where an explicit value
       is not specified; this is used within the parser, to ensure that no more
       than one value is specified.  If no value is explicitly given after
       parsing the calleeness is automatically set to non-callee, and this
       value is never seen by the rest of the code. */
    jitterc_calleeness_unspecified,

    /* The instruction may execute branch-and-link operations. */
    jitterc_calleeness_callee,

    /* The instruction never executes branch-and-link operations. */
    jitterc_calleeness_non_callee
  };

/* An instruction with a "returning" returningness is allowed (even if not
   required) to use the JITTER_RETURN branch. */
enum jitterc_returningness
  {
    /* The default returningness, for the VM instructions where an explicit value
       is not specified; this is used within the parser, to ensure that no more
       than one value is specified.  If no value is explicitly given after
       parsing the returningness is automatically set to non-returning, and this
       value is never seen by the rest of the code. */
    jitterc_returningness_unspecified,

    /* The instruction may execute return operations. */
    jitterc_returningness_returning,

    /* The instruction never executes return operations. */
    jitterc_returningness_non_returning
  };

/* An instruction with "branching" branchingness is allowed (even if not
   required to) to use non-fast branches. */
enum jitterc_branchingness
  {
    /* The default branchingness, for the VM instructions where an explicit value
       is not specified; this is used within the parser, to ensure that no more
       than one value is specified.  If no value is explicitly given after
       parsing the branchingness is automatically set to non-branching, and this
       value is never seen by the rest of the code. */
    jitterc_branchingness_unspecified,

    /* The instruction may execute non-fast branch operations. */
    jitterc_branchingness_branching,

    /* The instruction never executes non-fast branch operations. */
    jitterc_branchingness_non_branching
  };

/* A VM instruction specification as extracted from the text file. */
struct jitterc_instruction
{
  /* The unmangled version of the name.  This is checked for uniqueness during
     the analysis phase. */
  char *name;

  /* The mangled version of the name above, suitable to be used as part of a C
     identifier. */
  char *mangled_name;

  /* A list of struct jitterc_instruction_argument pointers. */
  gl_list_t arguments;

  /* Hot specialized instructions are given the "hot" GCC label attribute; for
     unspecialized instructions this is just a default value, which
     specializations may override. */
  enum jitterc_hotness hotness;

  /* The instruction relocatability. */
  enum jitterc_relocatability relocatability;

  /* True iff the instruction has one or more fast label arguments. */
  bool has_fast_labels;

  /* The instruction branchingness. */
  enum jitterc_branchingness branchingness;

  /* The instruction callerness. */
  enum jitterc_callerness callerness;

  /* The instruction calleeness. */
  enum jitterc_calleeness calleeness;

  /* The instruction returningness. */
  enum jitterc_returningness returningness;

  /* The C code implementing the instruction. */
  char *code;
};




/* VM specialized instructions.
 * ************************************************************************** */

/* An argument for a specialized instruction as extracted from the text file. */
struct jitterc_specialized_argument
{
  /* A pointer to the unspecialized argument of which this argument is a
     specialization. */
  struct jitterc_instruction_argument *unspecialized;

  /* The specialized argument kind.  Specialized instructions with residual
     parameters have each residual arguments of only one specific kind: or-ing
     kinds together is forbidden in this case. */
  enum jitterc_instruction_argument_kind kind;

  /* True if the argument is residual, false if the argument is specialized. */
  bool residual;

  /* Non-false iff this specialized argument is a replacement for a fast
     label, and therefore a non-fast label. */
  bool replacement;

  /* One field of the the following union is used only if the argument is
     specialized; which one depends on the unspecialized argument kind. */
  union
  {
    /* A pointer to the (fast) register we are specializing on.  Only valid if
       the kind is register and the register is nonresidual. */
    struct jitterc_register *nonresidual_register;

    /* A pointer to the literal we are specializing on.  Only valid if the kind
       is literal and the literal is nonresidual. */
    struct jitterc_literal *nonresidual_literal;
  };

  /* There is no need for a register class pointer here.  Such information, when
     relevant, is available from the struct pointed by the unspecialized
     field. */
};

/* Return the specialization of the given instruction argument, residualized for
   a specific kind.  Specialized instructions with residual parameters have each
   residual arguments of only one specific kind: or-in kinds together makes no
   sense in this case. */
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_register
   (const struct jitterc_instruction_argument *unspecialized)
  __attribute__ ((returns_nonnull, nonnull (1)));
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_literal
   (const struct jitterc_instruction_argument *unspecialized)
  __attribute__ ((returns_nonnull, nonnull (1)));
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_residual_label
   (const struct jitterc_instruction_argument *unspecialized)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Return a fresh specialized instruction argument, encoding a return
   address.  There is no explicit non-specialized counterpart. */
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_return_address
   (void)
  __attribute__ ((returns_nonnull));

/* Return the specialization of the given instruction argument, specialized to a
   given register. */
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_specialized_register
   (const struct jitterc_vm *vm,
    const struct jitterc_instruction_argument *unspecialized,
    const struct jitterc_register *register_)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Return the specialization of the given instruction argument, specialized to a
   given literal. */
struct jitterc_specialized_argument*
jitterc_make_specialized_instruction_argument_specialized_literal
   (struct jitterc_instruction_argument *unspecialized,
    struct jitterc_literal *literal)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* We never specialize for specific labels, so there is no need for a function
   jitterc_make_specialized_instruction_argument_specialized_label . */


/* A tree data structure connecting the specialized instruction prefixes of the
   specialization of one instruction.  A VM has a list of such trees, one element
   per unspecialized instruction.
   The tree root, at depth 0, has a 1-depth child *per* specialization of the
   first argument, if there is an argument (if the instruction has no arguments
   then the tree root is also a leaf); each 1-depth child representing an
   instruction with at least two arguments has one 2-depth child per
   specialization of the second argument, and so on.  The leaves of the tree are
   specialized instructions.  Every node of the whole tree, from the root to the
   leaf, is represented by a pointer to a struct
   jitterc_specialized_instruction_tree . */
struct jitterc_specialized_instruction_tree
{
  /* The unspecialized instruction name (for exaple "add"). */
  char *unspecialized_instruction_name;

  /* The prefix name (for example "add~r2~n3"; "add" would be the root). */
  char *prefix_name;

  /* The prefix in mangled form, suitable to be included in a C identifier.  */
  char *prefix_mangled_name;

  /* A list of pointers to struct jitterc_specialized_argument ,
     representing the specialized arguments leading from the root to the
     node, one per level.  For example a 2-depth node "add_r2_n3", would have
     r2 and n3, in this order, as its prefix. */
  gl_list_t prefix;

  /* A pointer to the parent, or NULL for the root. */
  struct jitterc_specialized_instruction_tree *parent;

  /* A list of pointers to struct jitterc_specialized_instruction_tree_child ;
     empty for leaves. */
  gl_list_t children;

  /* The specialized instruction represented by this tree.  NULL for
     non-leaves. */
  struct jitterc_specialized_instruction *specialized_instruction;
};

/* Every element y of the children field in a struct
   jitterc_specialized_instruction_tree x points to a struct
   jitterc_specialized_instruction_tree_child , containing the specialized
   argument to be appended to the prefix of x to obtain y . */
struct jitterc_specialized_instruction_tree_child
{
  /* The specialized argument leading to the child. */
  struct jitterc_specialized_argument *specialized_argument;

  /* The actual child. */
  struct jitterc_specialized_instruction_tree *child;
};

/* Return a pointer to a freshly-allocated tree structure for the given
   instruction.  This does not automatically add further nodes to the tree, and
   since the returned tree has zero children it is a leaf by default; however,
   differently from a leaf in a finished tree, it has the
   specialized_instruction field set to NULL.
   Add the new root to the forest in the pointed vm. */
struct jitterc_specialized_instruction_tree *
jitterc_make_specialized_instruction_tree_root
   (struct jitterc_vm *vm,
    const struct jitterc_instruction *i)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Return a pointer to a freshly-allocated tree structure, child of the given
   tree and reachable from it thru the specialized argument to which a pointer
   is given.  The parent and the newly-created child are linked by updating
   pointers on both sides.  Again the newly-created child is a leaf, but with a
   NULL specialized_instruction field. */
struct jitterc_specialized_instruction_tree *
jitterc_make_specialized_instruction_tree_child
   (struct jitterc_specialized_instruction_tree *parent,
    struct jitterc_specialized_argument *specialized_argument)
  __attribute__ ((returns_nonnull, nonnull (1, 2)));

/* Given a pointer to a tree, which must have no children, set its
   specialized_instruction field to the specialized instruction whose pointer is
   given. */
void
jitterc_specialized_instruction_tree_set_specialized_instruction
   (struct jitterc_specialized_instruction_tree *tree,
    struct jitterc_specialized_instruction *specialized_instruction)
  __attribute__ ((nonnull (1, 2)));

/* Print every tree whose pointer is given in the pointed forest to stdout.
   This is probably only useful for debugging. */
void
jitterc_print_specialized_instruction_forest (const gl_list_t forest)
  __attribute__ ((nonnull (1)));

/* A VM specialized instruction specification. */
struct jitterc_specialized_instruction
{
  /* The unique identifier of the specialized instruction, or -1 if this has not
     been assigned yet.  Specialized instructions are numbered late in the
     generation process. */
  int opcode;

  /* The specialized instruction name. */
  char *name;

  /* The specialized instruction name modified to be usable as a C identifier,
     thru a bijective mapping. */
  char *mangled_name;

  /* Hot specialized instructions are given the "hot" GCC label attribute. */
  enum jitterc_hotness hotness;

  /* The specialized instruction relocatability. */
  enum jitterc_relocatability relocatability;

  /* The specialized instruction branchingness. */
  enum jitterc_branchingness branchingness;

  /* A pointer to the unspecialized instruction of which the present structure
     is one specialization.  This is NULL for special specialized instructions,
     since they have no unspecialized counterpart. */
  struct jitterc_instruction *instruction;

  /* A list of struct jitterc_specialized_argument pointers. */
  gl_list_t specialized_arguments;

  /* A pointer to a replacement specialized instruction, in case this
     specialized instruction is potentially defective.  NULL otherwise. */
  struct jitterc_specialized_instruction *has_as_replacement;

  /* A pointer to the potentially defective specialized instruction this
     specialized instruction replaces.  NULL if this specialized instruction
     is not a replacement. */
  struct jitterc_specialized_instruction *is_replacement_of;
};

/* Return a pointer to a freshly-allocated struct jitterc_instruction . */
struct jitterc_instruction*
jitterc_make_instruction (void)
  __attribute__ ((returns_nonnull));

/* Return a pointer to freshly-allocated struct
   jitterc_specialized_instruction for the given VM, specializing the
   instruction whose pointer is given with the given list of specialized
   arguments.  The list content (made of pointers) is copied, so that the same
   actual argument may be reused.  This updates the VM by adding the specialized
   instruction and updating the maximal residual arity [FIXME: and likely
   updating something else as well in the future: tree data structure?]. */
struct jitterc_specialized_instruction*
jitterc_make_specialized_instruction (struct jitterc_vm *vm,
                                      const struct jitterc_instruction *ui,
                                      const gl_list_t specialized_args_copy)
  __attribute__ ((returns_nonnull, nonnull (1)));




/* Specialized instruction properties.
 * ************************************************************************** */

/* Return true iff the pointed specialized instruction is "call-related".  Here
   call-related means any of:
   - caller;
   - callee;
   - returning. */
bool
jitterc_specialized_instruction_is_call_related
   (const struct jitterc_specialized_instruction *sins)
  __attribute__ ((nonnull (1)));




/* Generation-time desctiptors for run-time VM stacks.
 * ************************************************************************** */

/* The kind of implementation used for a VM stack. */
enum jitterc_stack_implementation
  {
    /* An ordinary stack without TOS optimization, implemented as a pointer. */
    jitterc_stack_implementation_no_tos,

    /* A TOS-optimized stack, implemented as a field holding the top element and
       a pointer to the under-top element -- hopefully both held in physical
       machine registers. */
    jitterc_stack_implementation_tos,

    /* An initial value for a stack, before the user sets a valid one.  If not
       set this turns into the default. */
    jitterc_stack_implementation_uninitialized
  };

/* A declaration for a VM stack. */
struct jitterc_stack
{
  /* The stack letter, lowercase ASCII. */
  char letter;

  /* The stack element type, as expressed in C. */
  char *c_type;

  /* An expression in C specifying the initial value of each element, or NULL
     if the backing is not initialised with valid elements. */
  char *c_initial_value;

  /* The stack name, as given by the user. */
  char *long_name;

  /* The stack name, converted in all lower-case characters and all
     upper-case characters. */
  char *lower_case_long_name;
  char *upper_case_long_name;

  /* The stack size in elements. */
  jitter_uint element_no;

  /* The implementation of this stack. */
  enum jitterc_stack_implementation implementation;

  /* Include underflow and/or overflow guard pages, where available.
     Initialised to the invalid value -1. */
  int guard_underflow;
  int guard_overflow;
};




/* VM data structure.
 * ************************************************************************** */

/* The VM specification as extracted from the text file we parse. */
struct jitterc_vm
{
  /* The source file name. */
  const char *source_file_name;

  /* The directory paths where to write the generated C code. */
  char *directory;
  char *tmp_directory;

  /* The directory containing template files. */
  char *template_directory;

  /* The identifier prefixes to use in generated C code, without final
     underscores or dashes. */
  char *lower_case_prefix;
  char *upper_case_prefix;

  /* The hash identifier prefixes to use in generated C code, without final
     underscores or dashes. */
  char *hash_prefix;

  /* A nice human-readable name for the VM, NULL by default.  When this field is
     NULL a capitalized version of the prefix is used instead. */
  char *name;

  /* A list of struct jitterc_register_class pointers, holding register
     classes. */
  gl_list_t register_classes;

  /* A limit on how many fast registers there can be, per class.  -1 means no
     limit. */
  int max_fast_register_no_per_class;

  /* How many nonresidual literals we support at most for each specialized
     argument, or -1 if there is no limit. */
  int max_nonresidual_literal_no;

  /* A list of struct jitterc_stack pointers, holding stack declarations. */
  gl_list_t stacks;

  /* Generate #line directives iff this is true. */
  bool generate_line;

  /* These are strings, always malloc-allocated and always non-NULL.  They are
     copied verbatim into the generated C code, in the appropriate places. */
  char *legal_notice,
       *initial_header_c_code,
       *initial_vm1_c_code, *initial_vm2_c_code, *initial_vm_main_c_code,
       *early_header_c_code, *late_header_c_code,
       *printer_c_code, *rewriter_c_code,
       *early_c_code, *before_main_c_code,
       *initialization_c_code, *finalization_c_code,
       *state_early_c_code,
       *state_backing_struct_c_code, *state_runtime_struct_c_code,
       *state_initialization_c_code,
       *state_reset_c_code /* NULL if not explicitly specified */,
       *state_finalization_c_code,
       *instruction_beginning_c_code, *instruction_end_c_code;

  /* These are gl_list_t of strings. */
  gl_list_t wrapped_globals, wrapped_functions;

  /* A list of struct jitterc_instruction pointers. */
  gl_list_t instructions;

  /* A string hash table mapping each unspecialized instruction name into an
     instruction pointer.  The hash data point to the same elements contained
     in the instructions field. */
  struct jitter_hash_table name_to_instruction;

  /* A list of struct struct jitterc_rule pointers. */
  gl_list_t rewrite_rules;

  /* A list of struct jitterc_specialized_instruction pointers. */
  gl_list_t specialized_instructions;

  /* The maximum number of residual arguments, considering every specialized
     instruction. */
  size_t max_residual_arity;

  /* The longest unspecialized instruction name length, not mangled, without
     counting the final '\0' character.  Special specialized instruction, having
     no unspecialized counterparts, are ignored here. */
  size_t max_instruction_name_length;

  /* A list of pointers to struct jitterc_specialized_instruction_tree ,
     representing every specialized instruction prefix.  Each element of the
     forest is a (pointer to) a tree, rooted at an unspecialized instruction. */
  gl_list_t specialized_instruction_forest;

  /* A list of file basenames written in the temporary directory, to be moved
     to the actual destination directory in the end if everything succeeds. */
  gl_list_t written_file_names;
};

/* Return a freshly-allocated struct jitterc_vm , still with zero instructions
   and no specialization */
struct jitterc_vm*
jitterc_make_vm (void)
  __attribute__ ((returns_nonnull));

/* Append a setting to a VM. */
void
jitterc_vm_add_setting (struct jitterc_vm *vm,
                        const char *name,
                        const char *value)
  __attribute__ ((nonnull (1, 2, 3)));

/* Make a stack declaration with default values.  Some fields are initially
   invalid and need to be set by the user before the stack can be added to a
   VM. */
struct jitterc_stack *
jitterc_vm_make_stack (void)
  __attribute__ ((returns_nonnull));

/* Set a field of the pointed stack declaration to a new value.  It is permitted
   to change a field from a conventionally "uninitialised" value to an
   initialised value, but not from an initialised value to another; this is to
   prevent contradictions in Jitter specifications. */
void
jitterc_vm_stack_set_letter (struct jitterc_stack *s,
                             char letter);
void
jitterc_vm_stack_set_long_name (struct jitterc_stack *s,
                                const char* long_name);
void
jitterc_vm_stack_set_c_element_type (struct jitterc_stack *s,
                                     const char *c_type);
void
jitterc_vm_stack_set_element_no (struct jitterc_stack *s,
                                 jitter_int element_no);
void
jitterc_vm_stack_set_c_initial_value (struct jitterc_stack *s,
                                      const char *c_initial_value);
void
jitterc_vm_stack_set_fast_register_no (struct jitterc_stack *s,
                                       size_t fast_register_no);
void
jitterc_vm_stack_set_implementation (struct jitterc_stack *s,
                                     enum jitterc_stack_implementation i);
void
jitterc_vm_stack_set_guard_underflow (struct jitterc_stack *s,
                                      int guard);
void
jitterc_vm_stack_set_guard_overflow (struct jitterc_stack *s,
                                     int guard);

/* Add the pointed stack descriptor to the pointed VM, checking that the
   descriptor fields are valid. */
void
jitterc_vm_add_stack (struct jitterc_vm *vm,
                      struct jitterc_stack *s)
  __attribute__ ((nonnull (1, 2)));

/* Return a pointer to the last instruction in the given VM. */
struct jitterc_instruction *
jitterc_vm_last_instruction (struct jitterc_vm *vm)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Append the given instruction to the given VM. */
void
jitterc_vm_append_instruction (struct jitterc_vm *vm,
                               struct jitterc_instruction *ins)
  __attribute__ ((nonnull (1), nonnull (2)));

/* Append the given argument to the last instruction, which is supposed to
   exist, of the given VM. */
void
jitterc_vm_append_argument (struct jitterc_vm *vm,
                            struct jitterc_instruction_argument *arg)
  __attribute__ ((nonnull (1), nonnull (2)));

/* Return the last argument of the last instruction, which are both supposed to
   exist, of the given VM. */
struct jitterc_instruction_argument *
jitterc_vm_last_argument (struct jitterc_vm *vm)
  __attribute__ ((nonnull (1)));




/* Data structure analysis.
 * ************************************************************************** */

/* Compute analyses on a VM having all of its unspecialized instructions
   already.  This currently sorts instructions by name in the list, builds
   the vm->name_to_instruction hash table, computes the maximum instruction
   name length, checks for rewrite-rule semantic violations, assigns
   specialized instruction opcodes.
   Other properties will be easy to add here, if needed. */
void
jitterc_analyze_vm (struct jitterc_vm *vm) __attribute__ ((nonnull (1)));




/* VM specialization.
 * ************************************************************************** */

/* Specialize the given VM using the given number of fast registers and the
   given limit on nonresidual literals (or -1 if there is no limit). */
void
jitterc_specialize (struct jitterc_vm* vm,
                    int max_fast_register_no_per_class,
                    int max_nonresidual_literal_no);

#endif // #ifndef JITTERC_JITTERC_VM_H_
