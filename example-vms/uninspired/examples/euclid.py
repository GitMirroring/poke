# This is a -*- Python -*- file.

# Euclid's subtraction-based algorithm.

# Copyright (C) 2016, 2017 Luca Saiu
# Written by Luca Saiu

# This file is part of GNU Jitter.

# GNU Jitter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# GNU Jitter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>.


def euclid (a, b):
    while a != b:
        if a < b:
            a, b = b, a
        else:
            a -= b
    return a

print (euclid (3333333333, 1))
