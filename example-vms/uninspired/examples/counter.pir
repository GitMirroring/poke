# This is an -*- asm -*- file, so to speak, for the Parrot virtual machine.

# Simple counter.

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


.sub 'counter' :main
        $I0 = 2000000000
loop:
        sub $I0, $I0, 1
        ne  $I0, 1, loop
.end
