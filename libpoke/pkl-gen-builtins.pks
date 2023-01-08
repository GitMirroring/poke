;;; -*- mode: poke-ras -*-
;;; pkl-gen-builtins.pks - Built-in bodies

;;; Copyright (C) 2021, 2022, 2023 Jose E. Marchesi

;;; This program is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY ; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program.  If not, see <http: //www.gnu.org/licenses/>.

;;; This file contains the bodies of the several compiler built-ins.
;;; Note that each macro should expand to the body of a function,
;;; and handle its arguments and return value whenever necessary.

;;; RAS_MACRO_BUILTIN_GET_COLOR_BGCOLOR
;;;
;;; Body of the `term_get_color' and `term_get_bgcolor' compiler
;;; built-in with prototypes
;;; () int<32>[3]
;;;
;;; This macro requires the C variable `comp_stmt_builtin' defined to
;;; either PKL_AST_BUILTIN_TERM_GET_COLOR or
;;; PKL_AST_BUILTIN_TERM_GET_BGCOLOR.

        .macro builtin_get_color_bgcolor
        .let #itype = pvm_make_integral_type (pvm_make_ulong (32, 64), pvm_make_int (1, 32))
        push #itype
        .call _pkl_mkclsn
        mktya
        push ulong<64>3
        mka                     ; ARR
        tor                     ; _
   .c if (comp_stmt_builtin == PKL_AST_BUILTIN_TERM_GET_COLOR)
   .c {
        pushoc                  ; R G B
   .c }
   .c else
   .c {
        pushobc                 ; R G B
   .c }
        swap
        rot                     ; B G R
        fromr                   ; B G R ARR
        push ulong<64>0
        rot
        ains                    ; B G ARR
        push ulong<64>1
        rot
        ains                    ; B ARR
        push ulong<64>2
        rot
        ains                    ; ARR
        return
        .end

;;; RAS_MACRO_BUILTIN_SET_COLOR_BGCOLOR
;;;
;;; Body of the `term_set_color' and `term_set_bgcolor' compiler
;;; built-in with prototypes
;;; (int<32>[3] color) void
;;;
;;; This macro requires the C variable `comp_stmt_builtin' defined to
;;; either PKL_AST_BUILTIN_TERM_SET_COLOR or
;;; PKL_AST_BUILTIN_TERM_SET_BGCOLOR.

        .macro builtin_set_color_bgcolor
        pushvar 0, 0
        push ulong<64>0
        aref
        tor
        drop
        push ulong<64>1
        aref
        tor
        drop
        push ulong<64>2
        aref
        tor
        drop
        drop
        fromr
        fromr
        fromr
        swap
        rot
   .c if (comp_stmt_builtin == PKL_AST_BUILTIN_TERM_SET_COLOR)
   .c {
        popoc
   .c }
   .c else
   .c {
        popobc
   .c }
        .end
