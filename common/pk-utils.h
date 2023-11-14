/* pk-utils.h - Common utility functions for poke.  */

/* Copyright (C) 2020, 2021, 2022, 2023 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PK_UTILS_H
#define PK_UTILS_H

#include <config.h>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

/* Macros to avoid using strcmp directly.  */

#define STREQ(a, b) (strcmp (a, b) == 0)
#define STRNEQ(a, b) (strcmp (a, b) != 0)

/* Returns zero iff FILENAME is the name
   of an entry in the file system which :
   * is not a directory;
   * is readable; AND
   * exists.
   If it satisfies the above, the function returns NULL.
   Otherwise, returns a pointer to a statically allocated
   error message describing how the file doesn't satisfy
   the conditions.  */

char *pk_file_readable (const char *filename);

/* Integer exponentiation by squaring, for both signed and unsigned
   integers.  */

int64_t pk_ipow (int64_t base, uint32_t exp);
uint64_t pk_upow (uint64_t base, uint32_t exp);

/* Return one of the following strings based on the SIZE and SIGN_P:
     "N", "UN", "B", "UB", "H", "UH", "L", "UL"
   which are valid suffix for integral values in Poke.

   PK_PRINT_BINARY macro uses this function to avoid using any string
   literals (because of Jitter's limitation).  */

const char *pk_integral_suffix (int size, int sign_p);

/* Print the given unsigned 64-bit integer in binary.

   Please note that this macro is used in PVM_PRINT{I,L} macros in
   pvm.jitter; be careful to not call any non-wrapped functions
   or data (including string literals).  */

#define PK_PRINT_BINARY(val, size, sign_p, use_suffix_p)                      \
  do                                                                          \
    {                                                                         \
      char _pkpb_buf[65];                                                     \
      uint64_t _pkpb_val = (val);                                             \
      int _pkpb_sz = (size);                                                  \
                                                                              \
      for (int _pkpb_z = 0; _pkpb_z < _pkpb_sz; _pkpb_z++)                    \
        _pkpb_buf[_pkpb_sz - 1 - _pkpb_z]                                     \
            = ((_pkpb_val >> _pkpb_z) & 0x1) + '0';                           \
      _pkpb_buf[_pkpb_sz] = '\0';                                             \
                                                                              \
      pk_puts (_pkpb_buf);                                                    \
                                                                              \
      if (use_suffix_p)                                                       \
        pk_puts (pk_integral_suffix (_pkpb_sz, (sign_p)));                    \
    }                                                                         \
  while (0)

/* Format the given unsigned 64-bit integer in binary. */
int pk_format_binary (char* out, size_t outlen, uint64_t val, int size,
                      int sign_p, int use_suffix_p);

/* Concatenate string arguments into an malloc'ed string. */
char *pk_str_concat (const char *s0, ...) __attribute__ ((sentinel));

/* Replace all occurrences of SEARCH within IN by REPLACE. */
char *pk_str_replace (const char *in, const char *search, const char *replace);

/* Left and right trim the given string from whitespaces.  */
void pk_str_trim (char **str);

/* Convert floating-point number in input string STR and save the
   result in *FLT.  On conversion error, the function will return a
   non-zero value.

   Note that the whole STR should be a valid floating-point number and
   leading whitespace(s) will be ignored.  */
int pvm_stof (const char *str, float *flt) __attribute__ ((nonnull));

/* Convert floating-point number in input string STR and save the
   result in *DBL.  On conversion error, the function will return a
   non-zero value.

   Note that the whole STR should be a valid floating-point number and
   leading whitespace(s) will be ignored.  */
int pvm_stod (const char *str, double *dbl) __attribute__ ((nonnull));

/* This function is called when the program reaches a supposedly
   unreachable point; print an error message and abort the execution.

   FUNCNAME is the name of function in which PK_UNREACHABLE is invoked.
   FILENAME and LINE are the location information of invocation
   of this function.  */
void pk_unreachable (const char *funcname, const char *filename, int line)
    __attribute__ ((noreturn));

/* Diagnoses reaching unreachable code, and aborts.  */
#define PK_UNREACHABLE() pk_unreachable (__func__, __FILE__, __LINE__)

#endif /* ! PK_UTILS_H */
