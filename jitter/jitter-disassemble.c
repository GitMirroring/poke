/* VM library: native code disassembler.

   Copyright (C) 2017, 2019, 2020, 2021 Luca Saiu
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


#include <stdio.h>
#include <sys/types.h> /* for getpid */
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <jitter/jitter.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-fatal.h>

#include <jitter/jitter-instruction.h>
#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-specialize.h>
#include <jitter/jitter-disassemble.h>
#include <jitter/jitter-vm.h>
#include <jitter/jitter-print.h>

/* Begin using a class in the given print context, where the class name is
   formed by the concatenation of the lower-case prefix for the VM of the
   pointed executable routine, concatenated to a dash, concatenated
   to the given suffix.
   For example, if the mutable routine r belonged to a VM named "foo",
     jitter_disassemble_begin_class (ctx, r, "label")
   would open a class in the context ctx named "foo-label". */
static void
jitter_disassemble_begin_class (jitter_print_context ctx,
                                const struct jitter_executable_routine *er,
                                const char *suffix)
{
  char *prefix = er->vm->configuration->lower_case_prefix;
  size_t size = strlen (prefix) + 1 + strlen (suffix) + 1;
  char *buffer = jitter_xmalloc (size);
  sprintf (buffer, "%s-%s", prefix, suffix);
  jitter_print_begin_class (ctx, buffer);
  free (buffer);
}

/* Almost nothing of what follows is relevant with switch-dispatching. */
#ifdef JITTER_DISPATCH_SWITCH
__attribute__ ((noinline, noclone))
void
jitter_executable_routine_disassemble (jitter_print_context out,
                                       const struct jitter_executable_routine
                                       *er, bool raw,
                                       const char *objdump_name,
                                       const char *objdump_options_or_NULL)
{
  /* Just refuse to disassemble under switch dispatching. */
  jitter_disassemble_begin_class (out, er, "warning");
  jitter_print_char_star (out,
                          "<switch dispatching: refusing to disassemble>\n");
  jitter_print_end_class (out);
}

#else /* not switch-dispatching */

#ifdef JITTER_HAVE_POPEN

static const char *
endianness_option
#ifdef JITTER_WORDS_BIGENDIAN
  = "--endian=big";
#else
  = "--endian=little";
#endif // #ifdef JITTER_WORDS_BIGENDIAN

/* Return a the full pathname of a fresh temporary file name whose basename
   starts with the given prefix, or NULL on failure.  This might be worth
   factoring into the Jitter library, but the non-fatal failure mode is probably
   only needed here. */
static char *
jitter_temporary_file_pathname (const char *basename_prefix)
{
  /* Find a directory for temporary files. */
  char *tmp = getenv ("TMPDIR");
  if (tmp == NULL)
    tmp = "/tmp";

  /* I would like to just use mktemp here as the security risk is completely
     laughable in this case, but if I do it the linker on GNU prints a scary
     warning which might give users a wrong impression.  I'll just make my own
     crude imitation. */
  size_t pathname_length
    = strlen (tmp) + 1 + strlen (basename_prefix) + 1 + 100 + 1;
  char *pathname = jitter_xmalloc (pathname_length);
  sprintf (pathname, "%s/%s-%lu%lu%lu", tmp, basename_prefix,
           (unsigned long) rand (), (unsigned long) rand (),
           (unsigned long) rand ());

  /* There is no actual way to fail right now; but if later I switch to mktemp
     or mkstemp this will change. */
  return pathname;
}

/* The "state" (in the DFA sense) we are in, while reprinting and decorating
   the output of objdump. */
enum disassemble_objdump_state
  {
    disassemble_objdump_state_address,
    disassemble_objdump_state_whitespace_after_address,
    disassemble_objdump_state_hex,
    disassemble_objdump_state_whitespace_after_hex,
    disassemble_objdump_state_disassembly
  };

/* A helper function for jitter_disassemble_range_objdump.
   Print the given character, coming from objdump, with the appropriate class
   for the given state, to the given print context; keep into account the
   previous character.  Return the next state */
static enum disassemble_objdump_state
jitter_disassemble_print_char (jitter_print_context output,
                               const struct jitter_executable_routine *er,
                               enum disassemble_objdump_state old_state,
                               char previous_c,
                               char c)
{
  enum disassemble_objdump_state new_state = old_state;
  bool whitespace = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
  switch (old_state)
    {
    case disassemble_objdump_state_address:
      if (whitespace)
        new_state = disassemble_objdump_state_whitespace_after_address;
      else
        jitter_disassemble_begin_class (output, er, "memory-address");
      break;
    case disassemble_objdump_state_whitespace_after_address:
      if (! whitespace)
        {
          new_state = disassemble_objdump_state_hex;
          jitter_disassemble_begin_class (output, er, "native-instruction-hex");
        }
      break;
    case disassemble_objdump_state_hex:
      /* Objdump behaves differently on different architectures, in some cases
         visually separating bytes with a space, in other case not.  Anyway
         there is always a wider separator between hexadecimal instructions and
         their disassembly: either multiple spaces or a tab.  This is the most
         reliable general way I can think of for finding where hexadecimal
         instructions end and the spaces after them begins. */
      if (whitespace && (c == '\t' || previous_c == ' '))
        new_state = disassemble_objdump_state_whitespace_after_hex;
      else if (! whitespace)
        jitter_disassemble_begin_class (output, er, "native-instruction-hex");
      break;
    case disassemble_objdump_state_whitespace_after_hex:
      if (! whitespace)
        {
          new_state = disassemble_objdump_state_disassembly;
          jitter_disassemble_begin_class (output, er, "disassembly");
        }
      break;
    case disassemble_objdump_state_disassembly:
      if (c == '\n')
        new_state = disassemble_objdump_state_address;
      else if (! whitespace)
        jitter_disassemble_begin_class (output, er, "disassembly");
      break;
    default:
      jitter_fatal ("impossible");
    };
  jitter_print_char (output, c);
  if (! whitespace)
    jitter_print_end_class (output);

  return new_state;
}

/* Disassemble a range on the given output.  Return 0 on success, nonzero on
   failure. */
static int
jitter_disassemble_range_objdump (jitter_print_context output,
                                  const struct jitter_executable_routine *er,
                                  const void *beginning, size_t size_in_bytes,
                                  const char *prefix,
                                  bool raw,
                                  const char *objdump_name,
                                  const char *objdump_options)
{
  /* Copy the raw native instruction range from memory to a temporary file. */
  char *temporary_file_name
    = jitter_temporary_file_pathname ("jitter-disassembly");
  if (temporary_file_name == NULL)
    return -1;
  FILE *temporary_file = fopen (temporary_file_name, "w");
  if (temporary_file == 0)
    {
      free (temporary_file_name);
      return -1;
    }
  if (fwrite (beginning, 1, size_in_bytes, temporary_file) != size_in_bytes)
    {
      fclose (temporary_file);
      unlink (temporary_file_name);
      free (temporary_file_name);
      return -1;
    }
  if (fclose (temporary_file) != 0)
    {
      unlink (temporary_file_name);
      free (temporary_file_name);
      return -1;
    }
  /* Prepare our complicated command line calling objdump disassemble the
     temporary file. */
  char *command_line =
    jitter_xmalloc (  strlen (objdump_name)
                    + strlen (objdump_options)
                    + 1000);
  sprintf (command_line,
           "LC_ALL=C; LANG=C; LANGUAGE=C; %s --disassemble-all -b binary --%sshow-raw-insn %s %s --adjust-vma=0x%lx --prefix-addresses --wide %s 2> /dev/null",
           objdump_name,
           (raw ? "" : "no-"),
           endianness_option,
           objdump_options,
           (unsigned long) (jitter_uint) beginning,
           temporary_file_name);
//fprintf (stderr, "%s\n", command_line);

  /* Run the complicated command in a pipe, so that we have access to its
     output.  At this point we are also done with the command line,
     irrespectively of any errors. */
  FILE *objdump_output = popen (command_line, "r");
  free (command_line);
  if (objdump_output == NULL)
    {
      unlink (temporary_file_name);
      free (temporary_file_name);
      return -1;
    }

  /* Read the command output character by character, reproducing it into the
     print context.  I can't use a char for c, which could be unsigned on some
     platforms and therefore not able to store EOF.  This bit me on ARM. */
  int c;

  /* We want to skip the initial output from objdump until the first occurrence
     of "\n0x", and insert the given prefix before each newline; therefore we
     need to keep track of the last character we saw before the current one, and
     in the beginning phase the two previous ones as well. */
  char previous_char = '\0', previous_previous_char = '\0',
       previous_previous_previous_char = '\0';
  bool the_useful_part_has_started = false;
  enum disassemble_objdump_state state = disassemble_objdump_state_address;
//fprintf (stderr, "Beginning the loop\n");
  while (   ! feof (objdump_output)
         && (c = fgetc (objdump_output)) != EOF)
    {
//fprintf (stderr, "[%c %i]\n", c, c);
      /* Are we already past the first "\n0x" in the output from objdump? */
      if (the_useful_part_has_started)
        {
          /* Yes.  Print the prefix if we have just started a line, then, in any
             case, the character coming from objdump, with the decorations
             appropriate for the current state.  Update the state. */
          if (previous_char == '\n')
            jitter_print_char_star (output, prefix);
          state = jitter_disassemble_print_char (output, er, state,
                                                 previous_char, c);
        }
      else if (   previous_previous_previous_char == '\n'
               && previous_previous_char == '0'
               && previous_char == 'x')
        {
          /* The part we want to keep has begun, but in order to recognize it we
             have not printed the prefix "\n0x"; print the "0x" part (but not an
             initial newline, which would look ugly) now, after the prefix, and
             then the last seen character, which we had also skipped. */
          jitter_print_char_star (output, prefix);
          jitter_disassemble_begin_class (output, er, "memory-address");
          jitter_print_char_star (output, "0x");
          jitter_print_char (output, c);
          jitter_print_end_class (output);
          the_useful_part_has_started = true;
        }

      /* Keep track of the last three characters we saw.  Doing two of the three
         assignments is actually useless after the_useful_part_has_started , but
         this code is not worth optimizing.  The bottleneck is obviously not
         here. */
      previous_previous_previous_char = previous_previous_char;
      previous_previous_char = previous_char;
      previous_char = c;
    }
//fprintf (stderr, "After the loop: c is [%c %i]\n", c, c);

  /* Close the pipe, and remove the temporary file.  We get the subprocess exit
     status from pclose. */
  int res = pclose (objdump_output);
//fprintf (stderr, "res was %i\n", res);
  unlink (temporary_file_name); // !!! A useful line to disable when debugging.
  free (temporary_file_name);

#if defined (JITTER_HOST_CPU_IS_RISCV) && (JITTER_SIZEOF_VOID_P == 4)
  /* Work around a qemu-user bug I have seen appear in qemu-riscv (32-bit only)
     when updating from a 2019 to a late 2020 snapshot, with qemu downloaded
     from git: closing a pipe stream fails with errno set to ENOSYS, while the
     operation still works reliably.  On every other architecture including
     riscv64 pclose returns 0.  I might want to remove this workaround in the
     future. */
  if (res != 0 && errno == ENOSYS)
    return 0;
#endif // the configuration which is buggy for me
  return res;
}
#endif // #ifdef JITTER_HAVE_POPEN

/* Print a memory dump of the given range.  This is useful as a fallback
   solution when objdump is not usable. */
static void
jitter_dump_range (jitter_print_context output,
                   const struct jitter_executable_routine *er,
                   const void *beginning, size_t size_in_bytes,
                   const char *prefix,
                   size_t bytes_per_row)
{
  /* Prepare a format string such as "0x%08x" or "0x%016x" to print addresses,
     with every hexadecimal digit shown and left-padded with zeroes to fit a
     full word size. */
  char address_format [100];
  sprintf (address_format, "0x%%0%ulx", JITTER_BYTES_PER_WORD * 2);

  /* For every byte in the range... */
  const unsigned char *p = beginning;
  const unsigned char * const limit = p + size_in_bytes;
  size_t index;
  for (index = 0; p < limit; p ++, index ++)
    {
      /* Keep track of the byte index within the row, 0-based. */
      size_t index_in_row = index % bytes_per_row;

      /* It this is the first byte in a row then print the prefix and the
         address. */
      if (index_in_row == 0)
        {
          char buffer [100];
          sprintf (buffer, address_format, (long) (jitter_int) p);
          jitter_print_char_star (output, prefix);
          jitter_disassemble_begin_class (output, er, "memory-address");
          jitter_print_char_star (output, buffer);
          jitter_print_end_class (output);
        }

      /* Print the byte, preceded by a space; this way there will be no trailing
         spaces on the right. */
      jitter_print_char (output, ' ');
      char buffer [10];
      sprintf (buffer, "%02x", * p);
      jitter_disassemble_begin_class (output, er, "native-instruction-hex");
      jitter_print_char_star (output, buffer);
      jitter_print_end_class (output);

      /* If this is the last byte in a full row, or the last byte in the whole
         buffer in case the last row is incomplete, we need to close the line. */
      if (index_in_row == bytes_per_row - 1
          || p + 1 == limit)
        {
          /* If the last line is incomplete add spaces before the disassembly:
             one space plus two (omitted) nybbles per missing byte. */
          int i;
          for (i = 0; i < bytes_per_row - index_in_row - 1; i ++)
            jitter_print_char_star (output, "   ");

          /* Close the line with the "disassembly", so the speak. */
          jitter_print_char (output, '\t');
          jitter_disassemble_begin_class (output, er, "disassembly");
          jitter_print_char (output, '?');
          jitter_print_end_class (output);
          jitter_print_char (output, '\n');
        }
    }
}

static void
jitter_disassemble_range (jitter_print_context output,
                          const struct jitter_executable_routine *er,
                          const void *beginning, size_t size_in_bytes,
                          bool raw,
                          const char *objdump_name,
                          const char *objdump_options)
{
  /* Optimization: do nothing if the range is empty. */
  if (size_in_bytes == 0)
    return;

  /* Avoid invoking objdump every time when it may fail, since that is
     relatively expensive.  We also don't want to check whether objdump works at
     startup, since that is not normally used in production, and again running a
     subprocess is expensive.
     Using a static varaible with no synchronization here in practice is
     harmless; multiple threads may write to it in parallel, but it would be
     always the same value. */
  static bool is_objdump_known_to_fail = false;

  /* If objdump has a possibility of working then try it, and set
     is_objdump_known_to_fail.  If it remains false after calling objdump we are
     done. */
#ifdef JITTER_HAVE_POPEN
  if (! is_objdump_known_to_fail)
      is_objdump_known_to_fail
        = jitter_disassemble_range_objdump (output,
                                            er,
                                            beginning, size_in_bytes,
                                            "    ",
                                            raw,
                                            objdump_name,
                                            objdump_options);
#else // ! defined (JITTER_HAVE_POPEN)
  /* With no pipes there is no way to use the output of objdump. */
  is_objdump_known_to_fail = true;
#endif // #ifdef JITTER_HAVE_POPEN

  /* If objdump fails then we have not printed anything at this point.  Use the
     fallback solution. */
  if (is_objdump_known_to_fail)
    /* The number of bytes per row here is arbitrary, even if 4 is a common
       instruction size; shall I add arguments to control this? */
    jitter_dump_range (output, er,
                       beginning, size_in_bytes,
                       "    ",
                       4);
}

/* The opcode parameter represents a VM-specific specialized opcode of type enum
   vmprefix_specialized_instruction_opcode , but this code is VM-independent.
   Any opcode can be encoded in a sufficiently wide unsigned integer such as
   jitter_uint . */
static void
jitter_disassemble_show_specialized_instruction
   (jitter_print_context f,
    const struct jitter_executable_routine *er,
    const struct jitter_mutable_routine *p,
    jitter_uint opcode,
    const union jitter_word * const first_residual_argument_pointer,
    size_t residual_argument_no,
    const char *native_code,
    size_t native_code_size,
    bool raw,
    const char *objdump_name,
    const char *objdump_options)
{
  jitter_disassemble_begin_class (f, er, "comment");
  jitter_print_char_star (f, p->vm->specialized_instruction_names [opcode]);
  int i;
  const union jitter_word *residual_argument_pointer
    = first_residual_argument_pointer;
  for (i = 0; i < residual_argument_no; i ++)
    {
      char buffer [1000];
      sprintf (buffer, " 0x%lx%s",
               (unsigned long)((residual_argument_pointer ++)->ufixnum),
               i < residual_argument_no - 1 ? "," : "");
      jitter_print_char_star (f, buffer);
    }
  jitter_print_char_star (f, " (");
  jitter_print_long (f, 10, (long) native_code_size);
  jitter_print_char_star (f, " B):");
  jitter_print_end_class (f);
  jitter_print_char (f, '\n');
  jitter_disassemble_range (f, er, native_code, native_code_size, raw,
                            objdump_name, objdump_options);
}

__attribute__ ((noinline, noclone))
void
jitter_executable_routine_disassemble (jitter_print_context f,
                                       const struct jitter_executable_routine
                                       *er,
                                       bool raw,
                                       const char *objdump_name,
                                       const char *objdump_options_or_NULL)
{
  /* Get the non-executable routine for er.  If that is no longer available we
     have to work differently. */
  const struct jitter_mutable_routine *p = er->routine;
  if (p == NULL)
    {
#if defined(JITTER_DISPATCH_SWITCH)
# error "this code should never be compiled."
#elif defined(JITTER_DISPATCH_DIRECT_THREADING)
      jitter_print_char_star (f, "<cannot disassemble direct-threaded code without\n");
      jitter_print_char_star (f, " non-executable routine>\n");
#elif defined(JITTER_DISPATCH_MINIMAL_THREADING)
#elif defined(JITTER_DISPATCH_NO_THREADING)
      jitter_disassemble_range (f, er,
                                er->native_code, er->native_code_size, raw,
                                objdump_name,
                                ((objdump_options_or_NULL != NULL)
                                 ? objdump_options_or_NULL
                                 : JITTER_OBJDUMP_OPTIONS));
#else
# error "unknown dispatch: this should never happen."
#endif // dispatch
      return;
    }

  /* If we arrived here then the non-executable routine is available. */
  
  /* Refuse to disassemble if threads are overlapping or of negative size. */
  if (! p->vm->threads_validated)
    {
      jitter_disassemble_begin_class (f, er, "warning");
      jitter_print_char_star (f, "<threads not validated: refusing to disassemble>\n");
      jitter_print_end_class (f);
      return;
    }

  /* The specialized_instructions field has been extracted to the specialized
     routine, in every dispatching mode except no-threading , which doesn't
     need it at all. */
  const union jitter_word *specialized_instructions
#if   (defined(JITTER_DISPATCH_SWITCH)                 \
       || defined(JITTER_DISPATCH_DIRECT_THREADING)    \
       || defined(JITTER_DISPATCH_MINIMAL_THREADING))
    = (const union jitter_word *) er->specialized_program;
#elif defined(JITTER_DISPATCH_NO_THREADING)
    = jitter_dynamic_buffer_to_const_pointer (& p->specialized_program);
#else
# error "unknown dispatch: this should not happen"
#endif /* dispatch */

  const int specialized_instruction_no
    = jitter_dynamic_buffer_size (& p->replicated_blocks)
      / sizeof (struct jitter_replicated_block);
  const struct jitter_replicated_block * const replicated_blocks
    = jitter_dynamic_buffer_to_const_pointer (& p->replicated_blocks);
  const union jitter_word *next_thread
    = specialized_instructions;
  int i;

  const char *objdump_options = JITTER_OBJDUMP_OPTIONS;
  if (objdump_options_or_NULL != NULL)
    objdump_options = objdump_options_or_NULL;

#ifdef JITTER_REPLICATE
  if (p->stage != jitter_routine_stage_replicated)
    jitter_fatal ("disassembling non-replicated routine");

  for (i = 0; i < specialized_instruction_no; i ++)
    {
      /* Find the native code for the i-th VM specialized instruction. */
      const struct jitter_replicated_block *replicated_block
        = replicated_blocks + i;
      /* See the comment before
         jitter_disassemble_show_specialized_instruction about the opcode
         type. */
      jitter_uint opcode = replicated_block->specialized_opcode;
      char *native_code = replicated_block->native_code;
      size_t native_code_size = replicated_block->native_code_size;
      size_t residual_argument_no
        = p->vm->specialized_instruction_residual_arities [opcode];
      /*
      fprintf (f, "%s(%li)/%li (size %li): The next code block size is %li\n",
              p->vm->specialized_instruction_names [opcode], (long)opcode,
              (long)residual_argument_no,
              (long) (p->vm->thread_sizes [opcode]),
              (long)next_native_code_block_size);
      */

      /* Disassemble this VM instruction, and only this. */
      jitter_disassemble_begin_class (f, er, "comment");
      jitter_print_char_star (f, "# ");
      /* The thread address is irrelevant after specialisation for no-threading,
         but useful with direct-threading and even with minimal threading. */
#if ! defined (JITTER_DISPATCH_NO_THREADING)
      jitter_print_pointer (f, (void *) next_thread);
      jitter_print_char_star (f, ": ");
#endif // #if ! defined (JITTER_DISPATCH_NO_THREADING)
      jitter_print_end_class (f);
      jitter_disassemble_show_specialized_instruction
         (f,
          er,
          p,
          opcode,
          next_thread,
          residual_argument_no,
          native_code,
          native_code_size,
          raw,
          objdump_name,
          objdump_options);

      /* Move the native code pointer to the next VM specialized instruction,
         and the thread pointer past the arguments of the current
         instruction. */
      next_thread += residual_argument_no;
    }
#else
  if (p->stage != jitter_routine_stage_specialized)
    jitter_fatal ("disassembling non-specialized routine");
  for (i = 0; i < specialized_instruction_no; i ++)
    {
      /* Find the native code for the i-th VM specialized instruction.  See the
         comment before jitter_disassemble_show_specialized_instruction about
         the opcode type. */
      jitter_uint opcode = replicated_blocks [i].specialized_opcode;
      const char *native_code_block = p->vm->threads [opcode];
      size_t native_code_block_size = p->vm->thread_sizes [opcode];
      size_t residual_argument_no
        = p->vm->specialized_instruction_residual_arities [opcode];

      /* Disassemble this VM instruction, and only this, to stdout. */
      jitter_disassemble_begin_class (f, er, "comment");
      jitter_print_char_star (f, "# ");
      jitter_print_pointer (f, (void*) next_thread);
      jitter_print_char_star (f, ": ");
      jitter_print_end_class (f);
      jitter_disassemble_show_specialized_instruction
         (f, er, p,
          opcode,
          next_thread,
          residual_argument_no + 1,
          native_code_block,
          native_code_block_size,
          raw,
          objdump_name,
          objdump_options);

      /* Move the thread pointer past the arguments of the current
         instruction. */
      next_thread += residual_argument_no + 1;
    }
  return;
#endif // #ifdef JITTER_REPLICATE
}
#endif // #ifdef JITTER_DISPATCH_SWITCH
