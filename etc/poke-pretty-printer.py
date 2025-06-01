import gdb


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
            return f"{ival} as int<{ibits}>"
        elif tag == 1:
            # UINT
            ival = v >> 32
            ibits = ((v >> 3) & 0x1F) + 1
            return f"{ival} as uint<{ibits}>"
        elif tag == 6:
            # BOX
            ptr = v & ~7
            type_code = int(gdb.parse_and_eval(f"*(uintptr_t*){ptr}"))
            if type_code == 0x2:
                # LONG
                long = gdb.parse_and_eval(f"*(pvm_long){ptr}")
                return f"{long['value']} as long<{long['size_minus_one'] + 1}>"
            if type_code == 0x3:
                # ULONG
                long = gdb.parse_and_eval(f"*(pvm_long){ptr}")
                return (
                    f"{long['value']} as ulong<{long['size_minus_one'] + 1}>"
                )
            if type_code == 0x8:
                # STR
                return gdb.parse_and_eval(f"((pvm_string){ptr})->data")
            if type_code == 0x9:
                # OFF
                o = gdb.parse_and_eval(f"*(pvm_off){ptr}")
                return o
            if type_code == 0xA:
                # ARR
                arr = gdb.parse_and_eval(f"*(pvm_array){ptr}")
                return arr
            if type_code == 0xB:
                # SCT
                sct = gdb.parse_and_eval(f"*(pvm_struct){ptr}")
                return str(sct)
            if type_code == 0xC:
                # TYP
                typ = gdb.parse_and_eval(f"*(pvm_type){ptr}")
                typc = typ["code"]
                typv = typ["val"]
                if typc == 0:  # INTEGRAL
                    return f'INTEGRAL:{typv["integral"]}'
                if typc == 1:  # STRING
                    return "STRING"
                if typc == 2:  # ARRAY
                    return f'ARRAY:{typv["array"]}'
                if typc == 3:  # STRUCT
                    return f'STRUCT:{typv["sct"]}'
                if typc == 4:  # OFFSET
                    return f'OFFSET:{typv["off"]}'
                if typc == 5:  # CLOSURE
                    return f'CLOSURE:{typv["cls"]}'
                if typc == 6:  # VOID
                    return "VOID"
                return typ
            if type_code == 0xD:
                # CLS
                cls = gdb.parse_and_eval(f"*(pvm_cls){ptr}")
                return cls
            if type_code == 0xE:
                # IAR
                iar = gdb.parse_and_eval(f"*(pvm_iarray){ptr}")
                return iar
            if type_code == 0xF:
                # ENV
                env = gdb.parse_and_eval(f"*(pvm_env_){ptr}")
                return env
            if type_code == 0x10:
                # PRG
                prg = gdb.parse_and_eval(
                    f"*(pvm_program_){ptr}"
                )  # FIXME FIXME FIXME
                return prg
            return f"type_code:{type_code}"
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


gdb.printing.register_pretty_printer(None, PVMPrettyPrinter(), replace=True)
