;;; -*- mode: poke-ras -*-
;;; pkl-asm.pks - Assembly routines for the Pkl macro-assembler
;;;

;;; Copyright (C) 2019, 2020, 2021, 2022, 2023 Jose E. Marchesi

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

;;; RAS_MACRO_OGETMN
;;; ( OFF -- OFF ULONG )
;;;
;;; Auxiliary macro to get the normalized magnitude (i.e. in bits) of a
;;; given offset<uint<64>,*>.

        .macro ogetmn
        ogetm                   ; OFF OGETM
        swap                    ; OGETM OFF
        ogetu                   ; OGETM OFF OGETU
        rot                     ; OFF OGETU OGETM
        mullu
        nip2                    ; OFF (OGETU*OGETM
        .end

;;; RAS_MACRO_AREMAP
;;; ( VAL -- VAL )
;;;
;;; Given a map-able PVM value on the TOS, remap it if auto-remap
;;; is enabled in the PVM.  This is the implementation of the
;;; PKL_INSN_AREMAP macro.

        .macro aremap
        pusharem                ; VAL AREM_P
        bzi .label
        drop
        remap
        push null               ; NVAL null
.label:
        drop                    ; NVAL
        .end

;;; RAS_MACRO_REMAP
;;; ( VAL -- VAL )
;;;
;;; Given a map-able PVM value on the TOS, remap it.  This is the
;;; implementation of the PKL_INSN_REMAP macro-instruction.

        .macro remap
        ;; The re-map should be done only if the value is mapped.
        mm                      ; VAL MAPPED_P
        bzi .label              ; VAL MAPPED_P
        drop                    ; VAL
        ;; XXX do not re-map if the object is up to date (cached
        ;; value.)
        mgetw                   ; VAL WCLS
        swap                    ; WCLS VAL
        mgetm                   ; WCLS VAL MCLS
        swap                    ; WCLS MCLS VAL

        mgets                   ; WCLS MCLS VAL STRICT
        swap                    ; WCLS MCLS STRICT VAL
        mgetios                 ; WCLS MCLS STRICT VAL IOS
        swap                    ; WLCS MCLS STRICT IOS VAL
        mgeto                   ; WCLS MCLS STRICT IOS VAL OFF
        swap                    ; WCLS MCLS STRICT IOS OFF VAL
        mgetsel                 ; WCLS MCLS STRICT IOS OFF VAL EBOUND
        swap                    ; WCLS MCLS STRICT IOS OFF EBOUND VAL
        mgetsiz                 ; WCLS MCLS STRICT IOS OFF EBOUND VAL SBOUND
        swap                    ; WCLS MCLS STRICT IOS OFF EBOUND SBOUND VAL
        mgetm                   ; WCLS MCLS STRICT IOS OFF EBOUND SBOUND VAL MCLS
        nip                     ; WCLS MCLS STRICT IOS OFF EBOUND SBOUND MCLS
        call                    ; WCLS MCLS NVAL
        swap                    ; WCLS NVAL MCLS
        msetm                   ; WCLS NVAL
        swap                    ; NVAL WCLS
        msetw                   ; NVAL
        push null               ; NVAL null
.label:
        drop                    ; NVAL
        .end

;;; RAS_MACRO_WRITE
;;; ( VAL -- VAL )
;;;
;;; Given a map-able PVM value on the TOS, invoke its writer.  This is
;;; the implementation of the PKL_INSN_WRITE macro-instruction.

        .macro write
        dup                     ; VAL VAL
        ;; The write should be done only if the value is mapped.
        mm                      ; VAL VAL MAPPED_P
        bzi .label
        drop                    ; VAL VAL
        mgetw                   ; VAL VAL WCLS
        bn .label
        call                    ; VAL null
        dup                     ; VAL null null
.label:
        drop                    ; VAL (VAL|null)
        drop                    ; VAL
        .end

;;; GCD type
;;; ( VAL VAL -- VAL VAL VAL )
;;;
;;; Calculate the greatest common divisor of the integral value
;;; at the TOS, which should be of type TYPE.
;;;
;;; Macro arguments:
;;; @type
;;;   type of the value at the TOS.  It should be an integral type.

        .macro gcd @type
        ;; Iterative Euclid's Algorithm.
        over                     ; A B A
        over                     ; A B A B
.loop:
        bz @type, .endloop      ; ... A B
        mod @type               ; ... A B A%B
        rot                     ; ... B A%B A
        drop                    ; ... B A%B
        ba .loop
.endloop:
        drop                    ; A B GCD
        .end

;;; ADDO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Add the two given offsets in the stack, which must be of the
;;; same base type, and same units.
;;;
;;; Macro arguments:
;;;
;;; #unit
;;;   an ulong<64> with the unit of the result.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro addo @base_type #unit
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        rot                     ; OFF1 OFF2 OFF2M OFF1M
        add @base_type
        nip2                    ; OFF1 OFF2 (OFF2M+OFF1M)
        push #unit              ; OFF1 OFF2 (OFF2M+OFF1M) UNIT
        mkoq                    ; OFF1 OFF2 OFFR
        .end

;;; SUBO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Subtract the two given offsets in the stack, which must be of the
;;; same base type and same units.
;;;
;;; Macro arguments:
;;; #unit
;;;   an ulong<64> with the unit of the result.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro subo @base_type #unit
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        quake                   ; OFF1 OFF2 OFF1M OFF2M
        sub @base_type
        nip2                    ; OFF1 OFF2 (OFF1M+OFF2M)
        push #unit              ; OFF1 OFF2 (OFF1M+OFF2M) UNIT
        mkoq                    ; OFF1 OFF2 OFFR
        .end

;;; MULO base_type
;;; ( OFF VAL -- OFF VAL OFF )
;;;
;;; Multiply an offset with a magnitude.  The result of the operation
;;; is an offset with base type BASE_TYPE.
;;;
;;; Macro arguments:
;;; @base_type
;;;   a pkl_ast_node with the base type of the offset.

        .macro mulo @base_type
        dup                     ; VAL VAL
        tor                     ; VAL
        swap                    ; VAL OFF
        ogetm                   ; VAL OFF OFFM
        rot                     ; OFF OFFM VAL
        mul @base_type
        nip2                    ; OFF (OFFM*VAL)
        swap                    ; (OFFM*VAL) OFF
        ogetu                   ; (OFFM*VAL) OFF UNIT
        quake                   ; OFF (OFFM*VAL) UNIT
        mkoq                    ; OFF OFFR
        fromr                   ; OFF OFFR VAL
        swap                    ; OFF VAL OFFR
        .end

;;; DIVO unit_type base_type
;;; ( OFF OFF -- OFF OFF VAL )
;;;
;;; Divide an offset by another offset.  The result of the operation is
;;; a magnitude.  The types of both the offsets type and the
;;; magnitude type is BASE_TYPE.
;;;
;;; Macro arguments:
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro divo @base_type
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        quake                   ; OFF1 OFF2 OFF1M OFF2M
        div @base_type
        nip2                    ; OFF1 OFF2 (OFF1M/OFF2M)
        .end

;;; MODO unit_type base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; Calculate the modulus of two given offsets. The result of the
;;; operation is an offset having unit UNIT.  The types of both the
;;; offsets type and the magnitude type is BASE_TYPE.
;;;
;;; Macro arguments:
;;; #unit
;;;   an ulong<64> with the unit of the result.
;;; @base_type
;;;   a pkl_ast_node with the base type of the offsets.

        .macro modo @base_type #unit
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        quake                   ; OFF1 OFF2 OFF1M OFF2M
        mod @base_type
        nip2                    ; OFF1 OFF2 (OFF1M%OFF2M)
        push #unit              ; OFF1 OFF2 (OFF1M%OFF2M) UNIT
        mkoq                    ; OFF1 OFF2 OFFR
        .end

;;; ACAT
;;; ( ARR1 ARR2 -- ARR1 ARR2 )
;;;
;;; Given two arrays in the stack, append the elements of the second
;;; array to the first array.

        .macro acat
        pushf 4
        swap                    ; ARR2 ARR1
        sel                     ; ARR2 ARR1 SEL1
        regvar $sel1            ; ARR2 ARR1
        regvar $arr1            ; ARR2 SEL2
        sel                     ; ARR2 SEL2
        regvar $nelem           ; ARR2
        push ulong<64>0         ; ARR2 0UL
        regvar $idx             ; ARR2
     .while
        pushvar $idx            ; ARR2 IDX
        pushvar $nelem          ; ARR2 IDX NELEM
        ltlu
        nip2                    ; ARR2 (IDX<NELEM)
     .loop
        pushvar $idx            ; ARR2 IDX
        aref                    ; ARR2 IDX EVAL
        swap                    ; ARR2 EVAL IDX
        pushvar $sel1           ; ARR2 EVAL IDX SEL1
        addlu
        nip2                    ; ARR2 EVAL (IDX+SEL1)
        pushvar $arr1           ; ARR2 EVAL (IDX+SEL1) ARR1
        nrot                    ; ARR2 ARR1 EVAL (IDX+SEL1)
        swap                    ; ARR2 ARR1 (IDX+SEL1) EVAL
        ains                    ; ARR2 ARR1
        drop                    ; ARR2
        ;; Update index
        pushvar $idx
        push ulong<64>1
        addlu
        nip2
        popvar $idx
     .endloop
        pushvar $arr1           ; ARR2 ARR1
        swap                    ; ARR1 ARR2
        popf 1
        .end

;;; ASETC @array_type
;;; ( ARR ULONG VAL -- ARR )
;;;
;;; Checked ASET with data integrity.
;;;
;;; Given an array, an index and a value, set the element at the
;;; specified position to VAL.
;;;
;;; If the specified index is out of range, then PVM_E_OUT_OF_BOUNDS
;;; is raised.
;;;
;;; If the array type is bounded by size and the new value makes
;;; the total size of the array to change, then PVM_E_CONV is raised.

        .macro asetc @array_type
        tor                     ; ARR IDX [VAL]
        swap                    ; IDX ARR [VAL]
        sel                     ; IDX ARR NELEM [VAL]
        rot                     ; ARR NELEM IDX [VAL]
        lelu                    ; ARR NELEM IDX (NELEM<=IDX) [VAL]
        bzi .bounds_ok
        push PVM_E_OUT_OF_BOUNDS
        raise
.bounds_ok:
        drop                    ; ARR NELEM IDX [VAL]
        nip                     ; ARR IDX [VAL]
        ;; Get a copy of the current element at IDX, since
        ;; we may need it later.
        aref                    ; ARR IDX OVAL [VAL]
        rot                     ; IDX OVAL ARR [VAL]
        quake                   ; OVAL IDX ARR [VAL]
        over                    ; OVAL IDX ARR IDX [VAL]
        fromr                   ; ... ARR IDX VAL
        aset                    ; ... ARR
        .let @array_bound =  PKL_AST_TYPE_A_BOUND (@array_type)
  .c if (@array_bound
  .c     && PKL_AST_TYPE_CODE (PKL_AST_TYPE (@array_bound)) == PKL_TYPE_OFFSET)
  .c {
        .let #array_bounder = PKL_AST_TYPE_A_BOUNDER (@array_type)
        push #array_bounder
        call                    ; ... ARR OFF
        .e ogetmn
        nip                     ; ... ARR ASIZ
        swap                    ; ... ASIZ ARR
        siz                     ; ... ASIZ ARR NSIZ
        rot                     ; ... ARR NSIZ ASIZ
        eqlu
        nip2                    ; ... ARR (NSIZ==ASIZ)
        bnzi .size_ok
        drop                    ; OVAL IDX ARR
        ;; Restore the old value and raise E_conv
        nrot                    ; ARR OVAL IDX
        swap                    ; ARR IDX OVAL
        aset                    ; ARR
        push PVM_E_CONV
        raise
.size_ok:
        drop
        nip2
  .c }
  .c else
  .c {
        ;; Get rid of the OVAL and IDX
        nip2                   ; ARR
  .c }
        .end

;;; SSETC @struct_type
;;; ( SCT STR VAL -- SCT )
;;;
;;; Checked SSET with data integrity.
;;;
;;; Given a struct, a string containing the name of a struct element,
;;; and a value, set the value to the referred element.
;;;
;;; If setting the element causes a problem with the integrity of the
;;; data stored in the struct (for example, a constraint expression
;;; fails) then the operation is aborted and PVM_E_CONSTRAINT is raised.

        .macro ssetc @struct_type
        ;; First, save the previous value of the referred field
        ;; and also the field name.
        nrot                    ; VAL SCT STR
        dup                     ; VAL SCT STR STR
        tor                     ; VAL SCT STR [STR]
        sref                    ; VAL SCT STR OVAL
        tor                     ; VAL SCT STR [STR OVAL]
        rot                     ; SCT STR VAL [STR OVAL]
        ;; Now set the new value.
        sset                    ; SCT [STR OVAL]
        fromr                   ; SCT OVAL [STR]
        fromr                   ; SCT OVAL STR
        rot                     ; OVAL STR SCT
        ;; Invoke the constructor of the struct in itself.  If it
        ;; raises E_constraint, then restore the original value
        ;; and re-raise the exception.
        .let #constructor = PKL_AST_TYPE_S_CONSTRUCTOR (@struct_type)
        push PVM_E_CONSTRAINT
        pushe .integrity_fucked
        dup                     ; OVAL STR SCT SCT CLS
        push #constructor       ; OVAL STR SCT SCT CLS
        call                    ; OVAL STR SCT SCT
        pope
        drop                    ; OVAL STR SCT
        nip2                    ; SCT
        ba .integrity_ok
.integrity_fucked:
        ;; The constructor says this modification violates the
        ;; integrity of the data as defined by the struct type.
        ;; Restore the old value in the struct and re-raise the
        ;; exception.
        tor                     ; OVAL STR SCT [EXCEPTION]
        quake                   ; STR OVAL SCT [EXCEPTION]
        nrot                    ; SCT STR OVAL [EXCEPTION]
        sset                    ; SCT [EXCEPTION]
        fromr                   ; SCT EXCEPTION
        raise
.integrity_ok:
        ;; Everything went ok.  The struct with the new value
        ;; is on the stack.
        .end

;;; AFILL
;;; ( ARR VAL -- ARR VAL )
;;;
;;; Given an array and a a value of the right type, set all the
;;; elements of the array to the given value.
;;;
;;; This is the implementation of the `afill' macro instruction.

        .macro afill
        swap                    ; VAL ARR
        sel                     ; VAL ARR SEL
     .while
        push ulong<64>0         ; VAL ARR IDX 0UL
        eqlu
        nip                     ; VAL ARR IDX (IDX==0UL)
        not
        nip                     ; VAL ARR IDX !(IDX==0UL)
     .loop
        push ulong<64>1         ; VAL ARR IDX 1UL
        sublu
        nip2                    ; VAL ARR (IDX-1UL)
        tor
        atr                     ; VAL ARR (IDX-1UL) [(IDX-1UL)]
        rot                     ; ARR (IDX-1UL) VAL [(IDX-1UL)]
        tor
        atr                     ; ARR (IDX-1UL) VAL [(IDX-1UL) VAL]
        aset                    ; ARR [(IDX-1UL) VAL]
        fromr
        fromr                   ; ARR VAL (IDX-1UL)
        quake                   ; VAL ARR (IDX-1UL)
     .endloop
        drop                    ; VAL ARR
        swap                    ; ARR VAL
        .end

;;; ACONC array_type
;;; ( ARR ARR -- ARR ARR ARR )
;;;
;;;  Push a new array resulting from concatenating the elements of the
;;;  two given arrays.  Both operands have the same type.
;;;
;;;  The resulting array is always unbounded, regardless of the bounds
;;;  the operands.

        .macro aconc
        ;; Create an empty array for the result.
        over                    ; ARR1 ARR2 ARR1
        sel
        nip                     ; ARR1 ARR2 SEL1
        over                    ; ARR1 ARR2 SEL1 ARR2
        sel
        nip                     ; ARR1 ARR2 SEL1 SEL2
        addlu
        nip2                    ; ARR1 ARR2 (SEL1+SEL2)
        over                    ; ARR1 ARR2 (SEL1+SEL2) ARR2
        typof
        nip                     ; ARR1 ARR2 (SEL1+SEL2) ATYPE

        swap                    ; ARR1 ARR2 ATYPE (SEL1+SEL2)
        mka                     ; ARR1 ARR2 ARR
        ;; Append the elements of the first array.
        rot                     ; ARR2 ARR ARR1
        .e acat                 ; ARR2 ARR ARR1
        ;; Append the elements of the second array.
        nrot                    ; ARR1 ARR2 ARR
        swap                    ; ARR1 ARR ARR2
        .e acat
        ;; And we are done.
        swap                    ; ARR1 ARR2 ARR
        .end

;;; ATRIM array_type
;;; ( ARR ULONG ULONG -- ARR )
;;;
;;; Push a new array resulting from the trimming of ARR to indexes
;;; [ULONG..ULONG].
;;;
;;; Macro arguments:
;;; @array_type
;;;    a pkl_ast_node with the type of ARR.

        .macro atrim @array_type
        pushf 4
        regvar $to
        regvar $from
        regvar $array
        ;; Check boundaries
        pushvar $array          ; ARR
        sel                     ; ARR NELEM
        pushvar $to             ; ARR NELEM TO
        ltlu                    ; ARR NELEM TO (NELEM<TO)
        bnzi .ebounds
        drop                    ; ARR NELEM TO
        drop                    ; ARR NELEM
        bnzlu .check_from
        ;; The array has zero elements.  In this case we have to
        ;; check FROM differently.
        pushvar $from           ; ARR NELEM FROM
        ltlu                    ; ARR NELEM FROM (NELEM<FROM)
        bnzi .ebounds
        drop3                   ; ARR
        ba .bounds_ok
.check_from:
        pushvar $from           ; ARR NELEM FROM
        lelu                    ; ARR NELEM FROM (NELEM<=FROM)
        bnzi .ebounds
        drop                    ; ARR NELEM FROM
        drop                    ; ARR NELEM
        drop                    ; ARR
        pushvar $from           ; ARR FROM
        pushvar $to             ; ARR TO
        gtlu
        nip2                    ; ARR (FROM>TO)
        bnzi .ebounds
        drop                    ; ARR
        ba .bounds_ok
.ebounds:
        push PVM_E_OUT_OF_BOUNDS
        raise
.bounds_ok:
        ;; Boundaries are ok.  Build the trimmed array with a
        ;; subset of the elements of the array.
        typof                   ; ARR ATYP
        nip                     ; ATYP
        ;; Calculate the length of the new array.
        pushvar $to             ; ATYP TO
        pushvar $from           ; ATYP TO FROM
        sublu
        nip2
        push ulong<64>1
        addlu
        nip2                    ; ATYP (TO-FROM+1)
        mka                     ; TARR
        ;; Now add the elements to the new array.
        pushvar $from
        regvar $idx
      .while
        pushvar $idx            ; TARR IDX
        pushvar $to             ; TARR IDX TO
        ltlu                    ; TARR IDX TO (IDX<TO)
        nip2                    ; TARR (IDX<=TO)
      .loop
        ;; Add the IDX-FROMth element of the new array.
        pushvar $idx            ; TARR IDX
        pushvar $array          ; TARR IDX ARR
        over                    ; TARR IDX ARR IDX
        aref                    ; TARR IDX ARR IDX EVAL
        nip2                    ; TARR IDX EVAL
        swap                    ; TARR EVAL IDX
        pushvar $from           ; TARR EVAL IDX FROM
        sublu
        nip2                    ; TARR EVAL (IDX-FROM)
        swap                    ; TARR (IDX-FROM) EVAL
        ains                    ; TARR
        ;; Increase index and loop.
        pushvar $idx            ; TARR IDX
        push ulong<64>1         ; TARR IDX 1UL
        addlu
        nip2                    ; TARR (IDX+1UL)
        popvar $idx             ; TARR
      .endloop
        ;; If the trimmed array is mapped then the resulting array
        ;; is mapped as well, with the following attributes:
        ;;
        ;;   OFFSET = original OFFSET + (OFF(FROM) - original OFFSET)
        ;;   EBOUND = TO - FROM + 1
        ;;
        ;; The mapping of the resulting array is always
        ;; bounded by number of elements, regardless of the
        ;; characteristics of the mapping of the trimmed array.
        pushvar $array          ; TARR ARR
        mm                      ; TARR ARR MAPPED_P
        bzi .notmapped
        drop                    ; TARR ARR
        ;; Calculate the new offset.
        mgeto                   ; TARR ARR BOFFSET
        swap                    ; TARR BOFFSET ARR
        pushvar $from           ; TARR BOFFSET ARR FROM
        arefo                   ; TARR BOFFSET ARR FROM BOFF(FROM)
        nip                     ; TARR BOFFSET ARR BOFF(FROM)
        rot                     ; TARR ARR BOFF(FROM) BOFFSET
        dup                     ; TARR ARR BOFF(FROM) BOFFSET BOFFSET
        quake                   ; TARR ARR BOFFSET BOFF(FROM) BOFFSET
        sublu
        nip2                    ; TARR ARR BOFFSET (BOFF(FROM)-BOFFSET)
        addlu
        nip2                    ; TARR ARR BOFFSET
        rot                     ; ARR BOFFSET TARR
        regvar $tarr
        ;; Calculate the new EBOUND.
        swap                    ; BOFFSET ARR
        mgetm                   ; BOFFSET ARR MAPPER
        swap                    ; BOFFSET MAPPER ARR
        mgetw                   ; BOFFSET MAPPER ARR WRITER
        nip                     ; BOFFSET MAPPER WRITER
        pushvar $to
        pushvar $from           ; BOFFSET MAPPER WRITER TO FROM
        sublu
        nip2                    ; BOFFSET MAPPER WRITER (TO-FROM)
;        push ulong<64>1
;        addlu
;        nip2                    ; BOFFSET MAPPER WRITER (TO-FROM+1UL)
        ;; Install mapper, writer, offset and ebound.
        pushvar $tarr           ; BOFFSET MAPPER WRITER (TO-FROM) TARR
        swap                    ; BOFFSET MAPPER WRITER TARR (TO-FROM)
        msetsel                 ; BOFFSET MAPPER WRITER TARR
        swap                    ; BOFFSET MAPPER TARR WRITER
        msetw                   ; BOFFSET MAPPER TARR
        swap                    ; BOFFSET TARR MAPPER
        msetm                   ; BOFFSET TARR
        swap                    ; TARR BOFFSET
        mseto                   ; TARR
        ;; Mark the new array as mapped.
        map                     ; TARR
        ;; Remap!!
        aremap
        push null
        push null
.notmapped:
        drop
        drop
        popf 1
        .end

;;; RAS_MACRO_ARRAY_CONV_SEL
;;; ( ARR -- ARR )
;;;
;;; This macro generates code that checks that ARR has the right number
;;; of elements as specified by an array type bounder.  If the check fails
;;; then PVM_E_CONV is raised.  If the check is ok, then it updates ARR's
;;; type boundary.
;;;
;;; Macro arguments:
;;; @bounder
;;;    a bounder closure.

        .macro array_conv_sel #bounder
        sel                     ; ARR SEL
        push #bounder           ; ARR SEL CLS
        call                    ; ARR SEL BOUND
        eqlu                    ; ARR SEL BOUND (SEL==BOUND)
        bnzi .bound_ok
        push PVM_E_CONV
        raise
.bound_ok:
        drop3                   ; ARR
        typof                   ; ARR TYP
        push #bounder           ; ARR TYP BOUNDER
        tyasetb                 ; ARR TYP
        drop                    ; ARR
        .end

;;; RAS_MACRO_ARRAY_CONV_SIZ
;;; ( ARR -- ARR )
;;;
;;; This macro generates code that checks that ARR has the right size
;;; as specified by an array type bounder.  If the check fails then
;;; PVM_E_CONV is raised.  If the check is ok, then it updates ARR's
;;; type boundary.
;;;
;;; Macro arguments:
;;; @bounder
;;;    a bounder closure.

        .macro array_conv_siz #bounder
        siz                     ; ARR SIZ
        push #bounder           ; ARR SIZ CLS
        call                    ; ARR SIZ BOUND
        .e ogetmn               ; ARR SIZ BOUND BOUNDM
        rot                     ; ARR BOUND BOUNDM SIZ
        eqlu                    ; ARR BOUND BOUNDM SIZ (BOUNDM==SIZ)
        nip2                    ; ARR BOUND (BOUNDM==SIZ)
        bnzi .bound_ok
        push PVM_E_CONV
        raise
.bound_ok:
        drop2                   ; ARR
        typof                   ; ARR TYP
        push #bounder           ; ARR TYP BOUNDER
        tyasetb                 ; ARR TYP
        drop                    ; ARR
        .end

;;; RAS_MACRO_CDIV
;;; ( VAL VAL -- VAL VAL VAL )
;;;
;;; This macro generates code that performs ceil-division on integral
;;; values.
;;;
;;; Macro arguments:
;;; #one
;;;    the integer value one (1) in the same type than the operands.
;;; @type
;;;    pkl_ast_node reflecting the type of the operands.

        .macro cdiv #one @type
        dup
        nrot
        push #one
        sub @type
        nip2
        add @type
        nip2
        swap
        div @type
        .end

;;; RAS_MACRO_CDIVO one base_type
;;; ( OFF OFF -- OFF OFF OFF )
;;;
;;; This macro generates code that performs ceil-division on integral
;;; values.
;;;
;;; Macro arguments:
;;; @type
;;;    pkl_ast_node reflecting the type of the operands.

        .macro cdivo @type
        swap                    ; OFF2 OFF1
        ogetm                   ; OFF2 OFF1 OFF1M
        rot                     ; OFF1 OFF1M OFF2
        ogetm                   ; OFF1 OFF1M OFF2 OFF2M
        quake                   ; OFF1 OFF2 OFF1M OFF2M
        cdiv @type
        nip2                    ; OFF1 OFF2 (OFF1M/^OFF2M)
        .end

;;; RAS_MACRO_AIS
;;; ( VAL ARR -- VAL ARR BOOL )
;;;
;;; This macro generates code that, given an array ARR and a value VAL,
;;; determines whether VAL exists in ARR.  If it does, it pushes int<32>1
;;; to the stack.  Otherwise it pushes int<32>0.
;;;
;;; Macro arguments:
;;; @etype
;;;   AST node containing the type of the elements of ARR.

        .macro ais @etype
        pushf 1
        push ulong<64>0
        regvar $idx
.loop:
        sel                     ; VAL ARR SEL
        pushvar $idx            ; VAL ARR SEL IDX
        gtlu                    ; VAL ARR SEL IDX (SEL>IDX)
        bzi .endloop
        drop                    ; VAL ARR SEL IDX
        nip                     ; VAL ARR IDX
        ;; Get element
        aref                    ; VAL ARR IDX ELEM
        ;; Update index
        swap                    ; VAL ARR ELEM IDX
        push ulong<64>1         ; VAL ARR ELEM IDX 1UL
        addlu
        nip2                    ; VAL ARR ELEM (IDX+1UL)
        popvar $idx             ; VAL ARR ELEM
        ;; Compare element
        rot                     ; ARR ELEM VAL
        eq @etype               ; ARR ELEM VAL (ELEM==VAL)
        bnzi .foundit
        drop                    ; ARR ELEM VAL
        nip                     ; ARR VAL
        swap                    ; VAL ARR
        ba .loop
.endloop:
        drop3
        push int<32>0           ; VAL ARR 0
        ba .done
.foundit:
        drop                    ; ARR ELEM VAL
        nip                     ; ARR VAL
        swap                    ; VAL ARR
        push int<32>1           ; VAL ARR 1
.done:
        popf 1
        .end

;;; RAS_MACRO_BCONC
;;; ( VAL VAL -- VAL VAL VAL )
;;;
;;; This macro generates code that bit-concatenates the two values in
;;; the stack and pushes the result.
;;;
;;; Macro arguments:
;;; #op2_type_size
;;;   uint<32> with the size of op2_type in bits.
;;; @op1_type
;;;   AST node with the type of the first argument.
;;; @op2_type
;;;   AST node with the type of the second argument.
;;; @res_type
;;;   AST node with the type of the result.

        .macro bconc #op2_type_size @op1_type @op2_type @res_type
        tuck                      ; OP2 OP1 OP2
        over                      ; OP2 OP1 OP2 OP1
        swap                      ; OP2 OP1 OP1 OP2
        ;; Convert the second operand to the result type.
        nton @op2_type, @res_type ; ... OP1 OP2 OP2C
        nip                       ; ... OP1 OP2C
        ;; Convert the first operand to the result type.
        swap                      ; ... OP2C OP1
        nton @op1_type, @res_type ; ... OP2C OP1 OP1C
        nip                       ; ... OP2C OP1C
        push #op2_type_size       ; ... OP2C OP1C OP2_SIZE
        sl @res_type              ; ... OP2C OP1C OP2_SIZE (OP1C<<.OP2_SIZE)
        nip2                      ; ... OP2C (OP1C<<.OP2_SIZE)
        bor @res_type             ; ... OP2C (OP1C<<.OP2_SIZE) ((OP1C<<.OP2_SIZE)|OP2C)
        nip2                      ; OP2 OP1 ((OP1C<<.OP2_SIZE)|OP2C)
        .end

;;; RAS_MACRO_EQA
;;; ( ARR ARR -- ARR ARR INT )
;;;
;;; This macro generates code that compares the two arrays in
;;; the stack for equality.
;;;
;;; Macro arguments:
;;; @type_elem
;;;   type of the elements of the arrays.

        .macro eqa @type_elem
        ;; If the two arrays do not have the same number of
        ;; elements, then they are not equal.
        sel                     ; ARR1 ARR2 SEL2
        rot                     ; ARR2 SEL2 ARR1
        sel                     ; ARR2 SEL2 ARR1 SEL1
        quake                   ; ARR2 ARR1 SEL2 SEL1
        eqlu
        nip2                    ; ARR2 ARR1 (SEL2==SEL1)
        quake                   ; ARR1 ARR2 (SEL2==SEL1)
        bzi .done2
        drop                    ; ARR1 ARR2
        ;; At this point both arrays are guaranteed to have the same
        ;; number of elements.  Check equality of the elements
        ;; themselves.
        pushf 2
        sel                     ; ARR1 ARR2 SEL
        regvar $len
        push ulong<64>0         ; ARR1 ARR2 0UL
        regvar $idx
      .while
        pushvar $idx            ; ARR1 ARR2 IDX
        pushvar $len            ; ARR1 ARR2 IDX LEN
        ltlu                    ; ARR1 ARR2 IDX 0UL (IDX<LEN)
        nip2                    ; ARR1 ARR2 (IDX<LEN)
      .loop
        pushvar $idx            ; ARR1 ARR2 IDX
        rot                     ; ARR2 IDX ARR1
        tor                     ; ARR2 IDX [ARR1]
        aref                    ; ARR2 IDX VAL2 [ARR1]
        swap                    ; ARR2 VAL2 IDX [ARR1]
        fromr                   ; ARR2 VAL2 IDX ARR1
        swap                    ; ARR2 VAL2 ARR1 IDX
        aref                    ; ARR2 VAL2 ARR1 IDX VAL1
        nip                     ; ARR2 VAL2 ARR1 VAL1
        quake                   ; ARR2 ARR1 VAL2 VAL1
        eq @type_elem
        nip2                    ; ARR2 ARR1 (VAL2==VAL1)
        quake                   ; ARR1 ARR2 (VAL2==VAL1)
        bzi .done
        drop                    ; ARR1 ARR2
        ;; Update the index.
        pushvar $idx            ; ARR1 ARR2 IDX
        push ulong<64>1         ; ARR1 ARR2 IDX 1UL
        addlu
        nip2                    ; ARR1 ARR2 (IDX+1UL)
        popvar $idx
      .endloop
        ;; The arrays are equal
        push int<32>1           ; ARR1 ARR2 1
.done:
        popf 1
.done2:
        .end
