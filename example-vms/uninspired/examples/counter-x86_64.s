# This is an x86_64 -*- asm -*- source file.

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
# along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>.


	.text
	.globl	main
	.type	main, @function
main:
	movq	$2000000000, %rax
loop:
        subq    $1, %rax
        jnz     loop

	movq	$0, %rax
	ret
