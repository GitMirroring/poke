/* Jitter: header supplying functions and macros which some systems lack.
   Copyright (C) 2017, 2019 Luca Saiu
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


#ifndef JITTER_MISSING_H_
#define JITTER_MISSING_H_

#include <jitter/jitter-config.h>
#include <stdio.h>


/* GNU C attributes.
 * ************************************************************************** */

/* C compilers not supporting the GNU exensions will not recognize attributes.
   A simple fix is redefining the attribute keyword as a macro. */
#if ! defined (JITTER_HAVE_GNU_C_ATTRIBUTE)
# define attribute(ignored_attributes)      /* Nothing. */
# define __attribute__(ignored_attributes)  /* Nothing. */
#endif /* ! defined (JITTER_HAVE_GNU_C_ATTRIBUTE) */

/* After the previous definition, non-GNU C compilers will not have any problem,
   since every attribute use will be macroexpanded away; therefore it would be
   useless, in the following, to conditionalize over the attribute syntax
   availability.  What remains to be solved is older GNU C compilers not knowing
   about more recently introduced attributes. */
#if ! defined (JITTER_HAVE_ATTRIBUTE_RETURNS_NONNULL)
# define returns_nonnull      /* Nothing. */
# define __returns_nonnull__  /* Nothing. */
#endif /* #if ! defined (JITTER_HAVE_ATTRIBUTE_RETURNS_NONNULL) */




/* I/O functions.
 * ************************************************************************** */

/* The functions declared here are no-ops if we can live with them doing
   nothing; otherwise they fail fatally when called.  Each function
   implementation in jitter-missing.c is disabled by a CPP conditional if
   the system is found to have a real implementation at configure time. */

/* Do nothing. */
void
flockfile (FILE *f)
  __attribute__ ((nonnull (1)));

/* Do nothing. */
void
flockfile (FILE *f)
  __attribute__ ((nonnull (1)));

#endif // #ifndef JITTER_MISSING_H_
