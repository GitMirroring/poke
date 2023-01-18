/* pvm-wrappers.h - Wrapped functions to be used in pvm.jitter */

/* Copyright (C) 2023 Jose E. Marchesi */

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

#ifndef PVM_WRAPPERS_H
#define PVM_WRAPPERS_H

void pvm_free (void *p);
int pvm_nanosleep (const struct timespec *rqtp, struct timespec *rmtp);
int pvm_asprintf (char **resultp, const char *format, ...);
int pvm_snprintf (char *str, size_t size, const char *format, ...);
int pvm_random (void);
void pvm_srandom (unsigned int s);
void pvm_gettime (struct timespec *ts);
char *pvm_secure_getenv (char const *name);
void *pvm_memcpy (void *restrict dest, const void *restrict src, size_t n);
int pvm_strcmp (const char *s1, const char *s2);
size_t pvm_strlen (const char *s);
char *pvm_strcpy (char *restrict dest, const char *src);
char *pvm_strncpy (char *restrict dest, const char *restrict src, size_t n);
char *pvm_strcat (char *restrict dest, const char *restrict src);

#endif /* ! PVM_WRAPPERS_H */
