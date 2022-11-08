/* Jitter: VM-specific configuration and internal implementation header.

   Copyright (C) 2017, 2018, 2019 Luca Saiu
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


#include <stdio.h>

#include <jitter/jitter.h>
#include <jitter/jitter-vm.h>
#include <jitter/jitter-mutable-routine.h>

#include <jitter/jitter-fatal.h>


const char *
jitter_vm_instrumentation_to_string (enum jitter_vm_instrumentation i)
{
  switch (i)
    {
    case jitter_vm_instrumentation_none:
      return "";
    case jitter_vm_instrumentation_count:
      return "count-profiling";
    case jitter_vm_instrumentation_sample:
      return "sample-profiling";
    case jitter_vm_instrumentation_count_and_sample:
      return "count+sample-profiling";

    default:
      jitter_fatal ("unknown instrumentation %i", (int) i);
    }
}

void
jitter_print_vm_configuration (FILE *f,
                               const struct jitter_vm_configuration *c)
{
#define PRINT(...)                                          \
  do                                                        \
    {                                                       \
      if (fprintf (f, __VA_ARGS__) < 0)                     \
        jitter_fatal ("could not print VM configuration");  \
    }                                                       \
  while (false)

  PRINT("lower_case_prefix:              %s\n", c->lower_case_prefix);
  PRINT("upper_case_prefix:              %s\n", c->upper_case_prefix);
  PRINT("hash_prefix:                    %s\n", c->hash_prefix);
  PRINT("max_fast_register_no_per_class: %i\n",
        (int) c->max_fast_register_no_per_class);
  PRINT("max_nonresidual_literal_no:     %i\n", (int) c->max_nonresidual_literal_no);
  PRINT("dispatch:                       %s\n", c->dispatch_human_readable);
  PRINT("profile instrumentation:        %s\n",
        jitter_vm_instrumentation_to_string (c->instrumentation));

#undef PRINT
}
