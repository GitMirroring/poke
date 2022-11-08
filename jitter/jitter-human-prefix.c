/* Jitter utility: human prefixes.

   Copyright (C) 2020 Luca Saiu
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


#include <jitter/jitter-human-prefix.h>

#include <stdbool.h>
#include <stdlib.h>   /* for size_t */


/* The description of a metric or IEC prefix including its value and name: for
   example, {1000, "k"}. */
struct jitter_human_descriptor
{
  /* The value. */
  double value;

  /* The prefix abbreviated name. */
  const char *name;
};

/* An array of decimal prefixes in increasing order. */
static const struct jitter_human_descriptor
jitter_human_descriptor_decimal [] =
  {{1e-24, "y"},
   {1e-21, "z"},
   {1e-18, "a"},
   {1e-15, "f"},
   {1e-12, "p"},
   {1e-9,  "n"},
   {1e-6,  "u"},
   {1e-3,  "m"},
   {1,     ""},
   {1e3,   "k"},
   {1e6,   "M"},
   {1e9,   "G"},
   {1e12,  "T"},
   {1e15,  "P"},
   {1e18,  "E"},
   {1e21,  "Z"},
   {1e24,  "Y"}};
/* How many elements there are in jitter_human_descriptor. */
static const size_t jitter_human_descriptor_decimal_no
= (sizeof (jitter_human_descriptor_decimal)
   / sizeof (struct jitter_human_descriptor));

/* Like jitter_human_descriptor and jitter_human_descriptor_decimal_small_no ,
   but for binary prefixes. */
static const struct jitter_human_descriptor
jitter_human_descriptor_binary [] =
  {{1,                                                                 ""},
   {1024.,                                                             "ki"},
   {1024. * 1024.,                                                     "Mi"},
   {1024. * 1024. * 1024.,                                             "Gi"},
   {1024. * 1024. * 1024. * 1024.,                                     "Ti"},
   {1024. * 1024. * 1024. * 1024. * 1024.,                             "Pi"},
   {1024. * 1024. * 1024. * 1024. * 1024. * 1024.,                     "Ei"},
   {1024. * 1024. * 1024. * 1024. * 1024. * 1024. * 1024.,             "Zi"},
   {1024. * 1024. * 1024. * 1024. * 1024. * 1024. * 1024. * 1024.,     "Yi"}};
static const size_t jitter_human_descriptor_binary_no
= (sizeof (jitter_human_descriptor_binary)
   / sizeof (struct jitter_human_descriptor));

/* A helper function for jitter_human_readable.  This assumes that in is
   strictly positive, and searches for the appropriate unit by scanning the
   pointed array of descriptors in order. */
static void
jitter_human_readable_with (const struct jitter_human_descriptor *descriptors,
                            size_t descriptor_no,
                            double *out, const char **prefix, double in)
{
  /* Pendantry: handle empty descriptor tables. */
  if (descriptor_no == 0)
    {
      * out = in;
      * prefix = "";
    }
  /* From now on we can be sure that there is at least one descriptor. */

  /* Search for the most appropriate unit. */
  const struct jitter_human_descriptor *best;
  const struct jitter_human_descriptor *limit = descriptors + descriptor_no;
  const struct jitter_human_descriptor *last = limit - 1;
  if (in < descriptors->value)
    best = descriptors;
  else if (in > last->value)
    best = last;
  else
    for (best = descriptors; best != limit; best ++)
      if (in >= best->value && in < best [1].value)
        break;
  
  /* Use the descriptor we chose. */
  * out = in / best->value;
  * prefix = best->name;  
}

void
jitter_human_readable (double *out, const char **prefix,
                       double in,
                       bool binary)
{
  /* Zero does not require any prefix. */
  if (in == 0)
    {
      * out = 0;
      * prefix = "";
    }
  /* I do not want to depend on the math library just for the abs function.
     Handle the negative case with a trivial recursive call. */
  else if (in < 0)
    {
      jitter_human_readable (out, prefix, - in, binary);
      * out = - * out;
    }
  /* If we arrived here in is non-negative. */
  else if (binary)
    jitter_human_readable_with (jitter_human_descriptor_binary,
                                jitter_human_descriptor_binary_no,
                                out, prefix, in);
  else /* ! binary */
    jitter_human_readable_with (jitter_human_descriptor_decimal,
                                jitter_human_descriptor_decimal_no,
                                out, prefix, in);
}
