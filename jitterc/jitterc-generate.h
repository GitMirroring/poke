/* Jitter: generator header.

   Copyright (C) 2017 Luca Saiu
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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTERC_JITTERC_GENERATE_H_
#define JITTERC_JITTERC_GENERATE_H_

/* Include the Gnulib header. */
#include <config.h>

#include <stdbool.h>

#include <gl_list.h>

#include "jitterc/jitterc-vm.h"


/* Generate files for the VM whose pointer is given in the given directory,
   using the templates from the given directory, and also generating a frontend
   file if the bool paramter is true.  The VM is updated to include the given
   directories as part of its state. */
void
jitterc_generate (struct jitterc_vm *vm,
                  bool generate_frontend,
                  const char *template_directory,
                  const char *output_directory)
  __attribute__ ((nonnull (1, 3, 4)));


#endif // #ifndef JITTERC_JITTERC_GENERATE_H_
