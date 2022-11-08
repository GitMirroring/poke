# This is an -*- asm -*- file, so to speak, for the Parrot virtual machine.

# Euclid's subtraction-based algorithm.

# Copyright (C) 2016 Luca Saiu
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


.sub 'fibo' :main
        $I0 = 3333333333
        $I1 = 1
loop:
        eq $I0, $I1, done
        lt $I0, $I1, swap
        sub $I0, $I0, $I1
        branch loop
swap:
        set $I2, $I1
        set $I1, $I0
        set $I0, $I2
        branch loop
done:
        print  $I1
        print  "\n"
.end
