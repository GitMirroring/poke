# This is an x86_64 -*- asm -*- source file.
# gcc -static euclid-x86_64.s -o euclid-assembly

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
# along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>.


.text
.align 16
        .globl  main
        .type   main, @function
main:
	movq	$3333333333, %rax
	movq	$1, %rbx
.loop:
        cmpq    %rax, %rbx
        je      .done
        jl      .swap
        subq    %rax, %rbx
        jmp     .loop
.swap:
        movq    %rbx, %rcx
        movq    %rax, %rbx
        movq    %rcx, %rax
        jmp     .loop
.done:
	movq	$string, %rdi
        movq    %rax, %rsi
        movq    $0, %rax
	call	printf
        movq    $0, %rax
	ret

string:
	.string	"%lu\n"
        .byte 0
