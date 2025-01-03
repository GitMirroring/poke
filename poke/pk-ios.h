/* pk-ios.h - IOS-related functions for poke.  */

/* Copyright (C) 2020, 2021, 2022, 2023, 2024, 2025 Jose E. Marchesi */

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

#ifndef PK_IOS_H
#define PK_IOS_H

#include <config.h>

/* Initialize the IOS stuff.  */

void pk_ios_init (void);

/* Free resources used by the IOS stuff.  */

void pk_ios_shutdown (void);

#endif /* ! PK_IOS_H */
