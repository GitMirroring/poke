/* pvm-wrappers.c - Wrapped functions to be used in pvm.jitter */

/* Copyright (C) 2023, 2024, 2025, 2026 Jose E. Marchesi */

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

/* This file contains functions that wrap standard functions like
   printf or free, that can be replaced by gnulib or that can trigger
   the introduction of additional global symbols, like when using
   SOURCE_FORTIFY.  This is a way to simplify the maintenance of
   wrapped symbols in pvm.jitter. */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <timespec.h>
#include <stdio.h>
#include <c-vasprintf.h>
#include <c-vsnprintf.h>
#include <count-one-bits.h>

void
pvm_free (void *p)
{
  free (p);
}

int
pvm_nanosleep (const struct timespec *rqtp, struct timespec *rmtp)
{
  return nanosleep (rqtp, rmtp);
}

int
pvm_asprintf (char **resultp, const char *format, ...)
{
  va_list args;
  int result;

  va_start (args, format);
  result = c_vasprintf (resultp, format, args);
  va_end (args);
  return result;
}

int
pvm_snprintf (char *str, size_t size, const char *format, ...)
{
  va_list args;
  int result;

  va_start (args, format);
  result = c_vsnprintf (str, size, format, args);
  va_end (args);
  return result;
}

int
pvm_random (void)
{
  return random ();
}

void
pvm_srandom (unsigned int s)
{
  srandom (s);
}

void
pvm_gettime (struct timespec *ts)
{
  gettime (ts);
}

char *
pvm_secure_getenv (char const *name)
{
  return secure_getenv (name);
}

void *
pvm_memcpy (void *restrict dest, const void *restrict src, size_t n)
{
  return memcpy (dest, src, n);
}

int
pvm_strcmp (const char *s1, const char *s2)
{
  return strcmp (s1, s2);
}

size_t
pvm_strlen (const char *s)
{
  return strlen (s);
}

char *
pvm_strcpy (char *restrict dest, const char *src)
{
  return strcpy (dest, src);
}

char *
pvm_strncpy (char *restrict dest, const char *restrict src, size_t n)
{
  return strncpy (dest, src, n);
}

char *
pvm_strcat (char *restrict dest, const char *restrict src)
{
  return strcat (dest, src);
}

int
pvm_popcount (uint64_t num)
{
  return count_one_bits_ll (num);
}
