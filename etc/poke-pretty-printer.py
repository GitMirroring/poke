# GDB pretty-printer for PVM values

import gdb
import gdb.printing


class PVMSctPP:
    def __init__(self, val):
        self.__val = val

    def children(self):
        sct = self.__val

        for f in ("type", "nfields", "nmethods", "fields", "methods"):
            yield "", f
            yield "", sct[f]

    def display_hint(self):
        return "map"


class PVMTypPP:
    def __init__(self, val):
        self.__val = val

    def children(self):
        typ = self.__val

        typc = typ["code"]
        typv = typ["val"]

        def f(fname):
            yield "", fname
            yield "", typv[fname]

        if typc == 0:  # INTEGRAL
            yield from f("integral")
        elif typc == 1:  # STRING
            yield "", "string"
        elif typc == 2:  # ARRAY
            yield from f("array")
        elif typc == 3:  # STRUCT
            yield from f("sct")
        elif typc == 4:  # OFFSET
            yield from f("off")
        elif typc == 5:  # CLOSURE
            yield from f("cls")
        elif typc == 6:  # VOID
            yield "", "void"
        else:
            yield "", "InvalidType"
        return typ

    def display_hint(self):
        return "map"


class PVMOffPP:
    def __init__(self, val):
        self.__val = val

    def children(self):
        off = self.__val

        def f(fname):
            yield "", fname
            yield "", off[fname]

        yield from f("magnitude")
        yield from f("type")

    def display_hint(self):
        return "map"


class PVMStrPP:
    def __init__(self, val):
        self.__val = val

    def children(self):
        s = self.__val

        def f(fname):
            yield "", fname
            yield "", s[fname]

        yield from f("data")

    def display_hint(self):
        return "map"


def cast_ptr_to_array_of_pvm_val(pvm_val_ptr, n):
    assert n > 0
    new_type = pvm_val_ptr.type.target().array(n - 1).pointer()
    return pvm_val_ptr.cast(new_type).dereference()


class PVMIarPP:
    def __init__(self, val):
        self.__val = val

    def to_string(self):
        return "iarray"

    def children(self):
        iar = self.__val

        elems = iar["elems"]
        nelems = int(iar["nelem"])

        for f in ("nallocated", "nelem"):
            yield "", f
            yield "", iar[f]

        if nelems:
            yield "", "elems"
            yield "", cast_ptr_to_array_of_pvm_val(elems, nelems)

    def display_hint(self):
        return "map"


class PVMValPrettyPrinter:
    def __init__(self, val):
        self.__val = val

    def _kind(self):
        v = int(self.__val)
        tag = v & 7
        if tag == 0:
            return "INT"
        elif tag == 1:
            return "UINT"
        elif tag == 6:
            # BOX
            ptr = v & ~7
            type_code = int(gdb.parse_and_eval(f"*(uintptr_t*){ptr}"))
            return {
                0x2: "LONG",
                0x3: "ULONG",
                0x8: "STR",
                0x9: "OFF",
                0xA: "ARR",
                0xB: "SCT",
                0xC: "TYP",
                0xD: "CLS",
                0xE: "IAR",
                0xF: "ENV",
                0x10: "PRG",
            }.get(type_code, f"BOX:{type_code}")
        elif tag == 7:
            return {
                0x7: "PVM_NULL",
                0x17: "GC:INVALID_OBJECT",
                0x27: "GC:UNINITIALIZED_OBJECT",
                0x37: "GC:BROKEN_HEART",
            }.get(v, f"Garbage(NULL):0x{v:0x}")
        return f"Garbage:0x{v:0x}"

    def display_hint(self):
        k = self._kind()
        if k in ("ARR", "SCT", "TYP", "CLS", "IAR", "ENV", "PRG", "STR"):
            return "map"
        return "array"

    def children(self):
        v = int(self.__val)
        tag = v & 7
        if tag == 0:
            # INT
            ival = v >> 32
            ibits = ((v >> 3) & 0x1F) + 1
            yield "", f"{ival} as int<{ibits}>"
            return
        elif tag == 1:
            # UINT
            ival = v >> 32
            ibits = ((v >> 3) & 0x1F) + 1
            yield "", f"{ival} as uint<{ibits}>"
            return
        elif tag == 6:
            # BOX
            ptr = v & ~7
            try:
                type_code = int(gdb.parse_and_eval(f"*(uintptr_t*){ptr}"))
            except gdb.MemoryError as ex:
                type_code = f"BadPtr(0x{ptr:0x})"
            if type_code == 0x2:
                # LONG
                long = gdb.parse_and_eval(f"*(pvm_long){ptr}")
                yield "", f"{long['value']} as long<{long['size_minus_one'] + 1}>"
            elif type_code == 0x3:
                # ULONG
                long = gdb.parse_and_eval(f"*(pvm_long){ptr}")
                yield "", f"{long['value']} as ulong<{long['size_minus_one'] + 1}>"
            elif type_code == 0x8:
                # STR
                yield "", "string"
                yield "", gdb.parse_and_eval(f"*(pvm_string){ptr}")
            elif type_code == 0x9:
                # OFF
                yield "", "off"
                yield "", gdb.parse_and_eval(f"*(pvm_off){ptr}")
            elif type_code == 0xA:
                # ARR
                yield "", "array"
                yield "", gdb.parse_and_eval(f"*(pvm_array){ptr}")
            elif type_code == 0xB:
                # SCT
                yield "", "struct"
                yield "", gdb.parse_and_eval(f"(pvm_struct){ptr}")
            elif type_code == 0xC:
                # TYP
                yield from PVMTypPP(
                    gdb.parse_and_eval(f"*(pvm_type){ptr}")
                ).children()
            elif type_code == 0xD:
                # CLS
                cls = gdb.parse_and_eval(f"*(pvm_cls){ptr}")

                def f(fname):
                    yield "", fname
                    yield "", cls[fname]

                yield from f("name")
                yield from f("env")
                yield from f("program")
            elif type_code == 0xE:
                # IAR
                yield from PVMIarPP(
                    gdb.parse_and_eval(f"*(pvm_iarray){ptr}")
                ).children()
            elif type_code == 0xF:
                # ENV
                env = gdb.parse_and_eval(f"*(pvm_env_){ptr}")

                def f(fname):
                    yield "", fname
                    yield "", env[fname]

                yield from f("vars")
                yield from f("env_up")
            elif type_code == 0x10:
                # PRG
                prg = gdb.parse_and_eval(
                    f"*(pvm_program_){ptr}"
                )  # FIXME FIXME FIXME
                yield "", "program"
                yield "", prg
            else:
                yield "", f"BOXED({type_code})>"
            return
        elif tag == 7:
            if v == 0x7:
                yield "", "PMV_NULL"
                return
            elif v == 0x17:
                yield "", "GC:INVALID_OBJECT"
                return
            elif v == 0x27:
                yield "", "GC:UNINITIALIZED_OBJECT"
                return
            elif v == 0x37:
                yield "", "GC:BROKEN_HEART"
                return
            yield "", f"Garbage(NULL):0x{v:0x}"
            return
        yield "", f"Garbage:0x{v:0x}"


class PVMPrettyPrinter(gdb.printing.PrettyPrinter):
    def __init__(self):
        super(PVMPrettyPrinter, self).__init__(
            "pvm-pretty-printer",
            [],
        )

    def __call__(self, val):
        typename = gdb.types.get_basic_type(val.type).tag
        if typename is None:
            typename = val.type.name

        if typename == "pvm_val":
            return PVMValPrettyPrinter(val)
        if typename == "pvm_struct":
            return PVMSctPP(val)
        if typename == "pvm_type":
            return PVMTypPP(val)
        if typename == "pvm_off":
            return PVMOffPP(val)
        if typename == "pvm_string":
            return PVMStrPP(val)
        if typename == "pvm_iarray":
            return PVMIarPP(val)


gdb.printing.register_pretty_printer(None, PVMPrettyPrinter(), replace=True)
