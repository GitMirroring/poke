/* VM library: native code patching for PowerPC .

   Copyright (C) 2017, 2019, 2020 Luca Saiu
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


#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <jitter/jitter-fatal.h>

#include <jitter/jitter-patch.h>
#include <jitter/jitter-machine-common.h>

#include "jitter-machine.h"


void
jitter_invalidate_icache (char *from, size_t byte_no)
{
  /* We have to invalidate every icache "block", which appears to mean L1I cache
     line.  On some models the line width is 32 bytes, on others 64.  I have to
     play it safe here, unless there is some good way of finding what the CPU
     is, possibly at initialization.  The clcs instruction returns the cache
     line size, but it only works on POWER -- not on PowerPC.

     This method seems to work reliably on my machine, but it might be overkill.

     For each block of written instructions:
     - flush the block from dcache to "memory" (I hope this actually means
       from L1D to L2);
     - invalidate the same block in L1I.
     After we have done this for every block, do a sync to wait until everything
     which was supposed to go to "memory" is actually there.
     Finally, do an isync to drop any previosuly prefetched instruction having
     already found its way into the pipeline. */
  char * const limit = from + byte_no;
  char *p;
  for (p = from ; p < limit; p += 32)
    /* Both dcbf and icbi take two registers as parameters, whose contents are
       summed to get a logical address, unless ther first register is %r0 --
       in which case only the second register is used.
       See the comment for load_sign_extended_16bit_to_register_0 in
       jitter-machine-assembly.S about the reason why the first operand here
       is "0" rather than "%%r0". */
    asm volatile ("dcbf 0, %[pointer]\n\t"
                  "icbi 0, %[pointer]\n\t"
                  :
                  : [pointer] "r" (p)
                  : "memory");

  /* Ensure the instructions we wrote are now in memory. */
  asm volatile ("sync");

  /* Discard prefetched instructions.  It's unlikely that we have already
     prefetched the new code or jitter was in its place before, but let's play
     it safe. */
  asm volatile ("isync");
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char *immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write)
{
  int32_t immediate = * (int32_t*) immediate_pointer;

  /* The PowerPC, differently from MIPS, has no good way of zero-extending a
     16-bit immediate into a word; however it can sign-extend. */
  if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 16))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_sign_extended_16bit_to_register_0;
      case 1:  return jitter_snippet_load_sign_extended_16bit_to_register_1;
      case 2:  return jitter_snippet_load_sign_extended_16bit_to_register_2;
      case 3:  return jitter_snippet_load_sign_extended_16bit_to_register_3;
      case 4:  return jitter_snippet_load_sign_extended_16bit_to_register_4;
      case 5:  return jitter_snippet_load_sign_extended_16bit_to_register_5;
      case 6:  return jitter_snippet_load_sign_extended_16bit_to_register_6;
      case 7:  return jitter_snippet_load_sign_extended_16bit_to_register_7;
      case 8:  return jitter_snippet_load_sign_extended_16bit_to_register_8;
      case 9:  return jitter_snippet_load_sign_extended_16bit_to_register_9;
      case 10:  return jitter_snippet_load_sign_extended_16bit_to_register_10;
      case 11:  return jitter_snippet_load_sign_extended_16bit_to_register_11;
      case 12:  return jitter_snippet_load_sign_extended_16bit_to_register_12;
      case 13:  return jitter_snippet_load_sign_extended_16bit_to_register_13;
      default: jitter_fatal ("impossible");
      }
  else
    /* The only other case is loading a full 32-bit word. */
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_32bit_to_register_0;
      case 1:  return jitter_snippet_load_32bit_to_register_1;
      case 2:  return jitter_snippet_load_32bit_to_register_2;
      case 3:  return jitter_snippet_load_32bit_to_register_3;
      case 4:  return jitter_snippet_load_32bit_to_register_4;
      case 5:  return jitter_snippet_load_32bit_to_register_5;
      case 6:  return jitter_snippet_load_32bit_to_register_6;
      case 7:  return jitter_snippet_load_32bit_to_register_7;
      case 8:  return jitter_snippet_load_32bit_to_register_8;
      case 9:  return jitter_snippet_load_32bit_to_register_9;
      case 10:  return jitter_snippet_load_32bit_to_register_10;
      case 11:  return jitter_snippet_load_32bit_to_register_11;
      case 12:  return jitter_snippet_load_32bit_to_register_12;
      case 13:  return jitter_snippet_load_32bit_to_register_13;
      default: jitter_fatal ("impossible");
      }
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write)
{
  jitter_fatal ("PowerPC residual memory: not implemented yet");
}

void
jitter_patch_load_immediate_to_register (char *native_code,
                                     size_t native_code_size,
                                     const char *immediate_pointer,
                                     enum jitter_snippet_to_patch snippet)
{
  uint32_t u = * (uint32_t*) immediate_pointer;
  uint16_t low = u & ((1 << 16) - 1);
  uint16_t high = u >> 16;
  /* See the comments in machine-assembly.S about offsets. */
  switch (snippet)
    {
    case jitter_snippet_load_sign_extended_16bit_to_register_0:
    case jitter_snippet_load_sign_extended_16bit_to_register_1:
    case jitter_snippet_load_sign_extended_16bit_to_register_2:
    case jitter_snippet_load_sign_extended_16bit_to_register_3:
    case jitter_snippet_load_sign_extended_16bit_to_register_4:
    case jitter_snippet_load_sign_extended_16bit_to_register_5:

    case jitter_snippet_load_sign_extended_16bit_to_memory:
      /* Each of these snippets is implemented by a single 32-bit instruction
         with a literal in the rightmost 16 bits; they can all be patched in
         the same way. */
#ifdef JITTER_WORDS_BIGENDIAN
      memcpy (native_code + 2, &low, 2);
#else
      memcpy (native_code + 0, &low, 2);
#endif // #ifdef JITTER_WORDS_BIGENDIAN
      break;
    case jitter_snippet_load_32bit_to_register_0:
    case jitter_snippet_load_32bit_to_register_1:
    case jitter_snippet_load_32bit_to_register_2:
    case jitter_snippet_load_32bit_to_register_3:
    case jitter_snippet_load_32bit_to_register_4:
    case jitter_snippet_load_32bit_to_register_5:
    case jitter_snippet_load_32bit_to_memory:
      /* Not much more difficult.  Here we have two 32-bit instructions to
         patch, each with a 16-bit literal in the end.  The high part comes
         first. */
#ifdef JITTER_WORDS_BIGENDIAN
      memcpy (native_code + 2, &high, 2);
      memcpy (native_code + 6, &low, 2);
#else
      memcpy (native_code + 0, &high, 2);
      memcpy (native_code + 4, &low, 2);
#endif // #ifdef JITTER_WORDS_BIGENDIAN
      break;

    default:
      jitter_fatal ("impossible");
    }
}

void
jitter_patch_load_immediate_to_memory (char *native_code,
                                   size_t native_code_size,
                                   unsigned int memory_index,
                                   const char *immediate_pointer,
                                   enum jitter_snippet_to_patch snippet)
{
  jitter_fatal ("PowerPC residual memory: not implemented yet");
}

/* I keep this conditional to be able to test with patch-in disabled, by just
   commenting one line in the header. */
#ifdef JITTER_HAVE_PATCH_IN
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp)
{
  jitter_uint patch_in_case = dp->patch_in_case;
  switch (patch_in_case)
    {
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL:
      return jitter_snippet_jump_unconditional_26bit_offset_no_link;

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK:
      return jitter_snippet_jump_and_link_26bit_offset;

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY:
      return jitter_snippet_jump_conditional_16bit_offset;

    default:
      jitter_fatal ("jitter_snippet_for_patch_in: unsupported patch-in case");
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN

/* I keep this conditional to be able to test with patch-in disabled, by just
   commenting one line in the header. */
#ifdef JITTER_HAVE_PATCH_IN
void
jitter_patch_patch_in (char *native_code,
                       const char *immediate_pointer,
                       const struct jitter_patch_in_descriptor *descriptor,
                       enum jitter_snippet_to_patch snippet)
{
  /* On PowerPC relative branch targets are encoded as signed displacements from
     the *beginning* of the jumping instruction. */
  char *jump_target = * (char**) immediate_pointer;
  int64_t offset = jump_target - native_code;

  if (((uint64_t) offset) & 3)
    jitter_fatal ("unaligned branch: this should never happen");

  switch (snippet)
    {
    case jitter_snippet_jump_unconditional_26bit_offset_no_link:
    case jitter_snippet_jump_and_link_26bit_offset:
      {
        /* Here the displacement must fit in 26 bits.  The two least significant
           bits are flags which I want to keep cleared; the six most significant
           bits are the opcode. */
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (offset, 26))
          jitter_fatal ("branch displacement for b too far");

        /* The instruction might be unaligned with respect to a 64-bit word (for
           the future: PowerPC64 is not supported yet), but since PowerPC allows
           unaligned memory accesses I'll just use a 32-bit pointer, and ignore
           endianness problems. */
        /* FIXME: this trick is very nice, but I'm worried about endianness; I
           can't easily test on little-endian PowerPCs.  It works fine on a
           big-endian machine. */
        /*
        struct b_instruction {
          unsigned long opcode : 6;
          signed long offset_without_00 : 24;
          unsigned long flags : 2;
        } __attribute__ ((packed));
        struct b_instruction instruction
          = {.opcode = 18u,
             .offset_without_00 = offset >> 2,
             .flags = 0};
        * (struct b_instruction *) native_code = instruction;
        */
        /* FIXME: this should work with either endianness.  For the time being
           I'm keeping it. */
        uint32_t shifted_opcode = 18u << 26;
        uint32_t truncated_offset = ((uint32_t) offset) & ((2u << 25) - 1);

        /* Set the LK flag iff we are linking. */
        uint32_t flags = 0;
        if (snippet == jitter_snippet_jump_and_link_26bit_offset)
          flags = 1;
        uint32_t truncated_offset_with_flags = (truncated_offset & ~ 3) | flags;
        uint32_t instruction
          = shifted_opcode | truncated_offset_with_flags;
        * (uint32_t *) native_code = instruction;
        break;
      }

    case jitter_snippet_jump_conditional_16bit_offset:
      {
        /* Here the displacement must fit in 26 bits.  The two least significant
           bits are flags which I want to keep cleared; the six most significant
           bits are the opcode. */
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (offset, 16))
          jitter_fatal ("branch displacement for bc too far");

        // FIXME: factor with the previous case, following the style I adopted
        // in the RISC-V port.

        /* Read the current unpatched instruction from memory. */
        uint32_t * instruction_p = (uint32_t *) native_code;
        uint32_t instruction = * instruction_p;

        /* Clear the low 16 bits of the instruction, replacing them with the
           displacement.  Since the displacement is aligned the two least
           significant bits will be zero, which is what we need here since they
           represent the AA and LK flags. */
        instruction &= ~ ((1LU << 16) - 1);

        /* Insert the offset. */
        instruction |= ((uint32_t) offset & ((1LU << 16) - 1));

        /* Replace the original instruction with its patched copy. */
        * instruction_p = instruction;
        break;
      }

    default:
      jitter_fatal ("jitter_patch_patch_in: unsupported snippet %li",
                    (long) snippet);
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN
