import gdb


class PVMSctPP:
    def __init__(self, val):
        self.val = val

    def children(self):
        sct = self.val

        for f in ("type", "nfields", "nmethods", "fields", "methods"):
            yield "", f
            yield "", sct[f]

    def display_hint(self):
        return "map"


class PVMTypPP:
    def __init__(self, val):
        self.val = val

    def children(self):
        typ = self.val

        typc = typ["code"]
        typv = typ["val"]

        def f(fname):
            yield "", fname
            yield "", typv[fname]

        if typc == 0:  # INTEGRAL
            yield from f("integral")
        if typc == 1:  # STRING
            yield "", "string"
        if typc == 2:  # ARRAY
            yield from f("array")
        if typc == 3:  # STRUCT
            yield from f("sct")
        if typc == 4:  # OFFSET
            yield from f("off")
        if typc == 5:  # CLOSURE
            yield from f("sct")
        if typc == 6:  # VOID
            yield "", "void"
        return typ

    def display_hint(self):
        return "map"


class PVMValPrettyPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        v = int(self.val)
        tag = v & 7
        if tag == 0:
            # INT
            ival = v >> 32
            ibits = ((v >> 3) & 0x1F) + 1
            return f"#<{ival} as int<{ibits}>>"
        elif tag == 1:
            # UINT
            ival = v >> 32
            ibits = ((v >> 3) & 0x1F) + 1
            return f"#<{ival} as uint<{ibits}>>"
        elif tag == 6:
            # BOX
            ptr = v & ~7
            type_code = int(gdb.parse_and_eval(f"*(uintptr_t*){ptr}"))
            if type_code == 0x2:
                # LONG
                long = gdb.parse_and_eval(f"*(pvm_long){ptr}")
                return (
                    f"#<{long['value']} as long<{long['size_minus_one'] + 1}>>"
                )
            if type_code == 0x3:
                # ULONG
                long = gdb.parse_and_eval(f"*(pvm_long){ptr}")
                return f"#<{long['value']} as ulong<{long['size_minus_one'] + 1}>>"
            if type_code == 0x8:
                # STR
                s = gdb.parse_and_eval(f"((pvm_string){ptr})->data")
                return f"#<string {s}>"
            if type_code == 0x9:
                # OFF
                o = gdb.parse_and_eval(f"*(pvm_off){ptr}")
                return str(o)
            if type_code == 0xA:
                # ARR
                arr = gdb.parse_and_eval(f"*(pvm_array){ptr}")
                return str(arr)
            if type_code == 0xB:
                # SCT
                return gdb.parse_and_eval(f"(pvm_struct){ptr}")
            if type_code == 0xC:
                # TYP
                return gdb.parse_and_eval(f"*(pvm_type){ptr}")
            if type_code == 0xD:
                # CLS
                cls = gdb.parse_and_eval(f"*(pvm_cls){ptr}")
                clsn = cls["name"]
                clse = cls["env"]
                clsp = cls["program"]
                return f"#<closure name:{clsn}, env:{clse}, program:{clsp}>"
            if type_code == 0xE:
                # IAR
                iar = gdb.parse_and_eval(f"*(pvm_iarray){ptr}")
                na = iar["nallocated"]
                ne = iar["nelem"]
                tr = ""
                ne_max = 5
                if ne > ne_max:
                    tr = "..."
                elems = iar["elems"]
                elems = [str(elems[i]) for i in range(min(ne, ne_max))]
                return (
                    f"#<iarray nallocated:{na}, nelem:{ne}, elems:{elems}{tr}>"
                )
            if type_code == 0xF:
                # ENV
                env = gdb.parse_and_eval(f"*(pvm_env_){ptr}")
                envv = env["vars"]
                envu = env["env_up"]
                return f"#<env vars:{envv}, env_up:{envu}>"
            if type_code == 0x10:
                # PRG
                prg = gdb.parse_and_eval(
                    f"*(pvm_program_){ptr}"
                )  # FIXME FIXME FIXME
                return prg
            return f"#<type_code:{type_code}>"
        elif tag == 7:
            if tag == 0x7:
                return "PVM_NULL"
            elif tag == 0x17:
                return "GC:INVALID_OBJECT"
            elif tag == 0x27:
                return "GC:UNINITIALIZED_OBJECT"
            elif tag == 0x37:
                return "GC:BROKEN_HEART"
            return f"Garbage(NULL):0x{ptr:0x}"
        return f"Garbage:0x{ptr:0x}"


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


gdb.printing.register_pretty_printer(None, PVMPrettyPrinter(), replace=True)
