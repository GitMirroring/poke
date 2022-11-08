# This is a SPARC -*- asm -*- source file.

# A simple counter loop decrementing a register from 2000000000 to 0.

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


        .text

        .globl  main
        .type   main, @function
main:
        sethi   %hi(2000000000), %l0
        or      %l0, %lo(2000000000), %l0
loop:
        add     %l0, -1, %l0
        cmp     %l0, 0

!         beq     out
!         nop
!         b       loop
!         nop
! out:

        bne     loop
        nop     ! The loop could certainly be rewritten to use the delay slot,
                ! but I want to keep the code similar to the VM code and across
                ! architectures.

        ! Return 0.
        retl
        mov     %g0, %i0
