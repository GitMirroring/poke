# This is a MIPS -*- asm -*- source file.

# A simple counter loop decrementing a register from 2000000000 to 0.

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
# along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>.


        .text

        .set    noreorder
        .set    nomacro
        .set    nomips16
        .set    nomicromips

        .align  2
        .globl  main
        .type   main, @function
main:
        lui     $2,     %hi(2000000000)
        addiu   $2, $2, %lo(2000000000)
loop:
        addiu   $2, $2, -1
        bne     $2, $0, loop
        nop     # The loop could certainly be rewritten to use the delay slot,
                # but I want to keep the code similar to the VM code and across
                # architectures.

        # Return 0.
        move    $2, $0
        j       $31
        nop
