# -*- coding: utf-8 -*-
#@category FiberHome
# Relatório temporário de funções relevantes ao layout de RAM do U-Boot SD5116.

from ghidra.app.decompiler import DecompInterface

targets = [
    "81216a24", # escolhe início/layout de RAM conforme 0x10100800
    "81216ad0", # inicialização que lê o registrador do SoC
    "8120b26c", # caminho bootm que imprime "Memory Start"
    "8120ae4c", # subcomando bootm Linux
]

decomp = DecompInterface()
decomp.openProgram(currentProgram)

for value in targets:
    address = toAddr(value)
    function = getFunctionContaining(address)
    if function is None:
        disassemble(address)
        function = createFunction(address, None)
    print("\n===== {}: {} =====".format(value, function))
    if function is None:
        continue
    result = decomp.decompileFunction(function, 60, monitor)
    if result.decompileCompleted():
        print(result.getDecompiledFunction().getC())
    else:
        print("Falha ao descompilar: {}".format(result.getErrorMessage()))

for value in ["81227ea9", "81227eb7", "81227fb0", "81227fe3"]:
    address = toAddr(value)
    print("\n===== Referências para {} =====".format(address))
    for ref in getReferencesTo(address):
        print(ref)
