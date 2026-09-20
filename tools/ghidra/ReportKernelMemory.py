# -*- coding: utf-8 -*-
#@category FiberHome

from ghidra.app.decompiler import DecompInterface

base = 0x81008000
targets = [
    ("PHYS_OFFSET", 3246130),
    ("core.c", 3253044),
    ("bootmem", 3271555),
    ("ZSP hook", 3544166),
]

decomp = DecompInterface()
decomp.openProgram(currentProgram)
seen = set()

for label, offset in targets:
    address = toAddr(base + offset)
    print("\n===== {} @ {} =====".format(label, address))
    data = getDataAt(address)
    print("data: {}".format(data))
    for ref in getReferencesTo(address):
        print("ref: {}".format(ref))
        function = getFunctionContaining(ref.getFromAddress())
        if function is None:
            continue
        key = str(function.getEntryPoint())
        if key in seen:
            continue
        seen.add(key)
        print("----- {} -----".format(function))
        result = decomp.decompileFunction(function, 60, monitor)
        if result.decompileCompleted():
            print(result.getDecompiledFunction().getC())
        else:
            print("Falha: {}".format(result.getErrorMessage()))
