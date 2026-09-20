# Teste temporário HIUSB

Este diretório contém um módulo ARM experimental para o kernel da ONU:
`2.6.34.10 mod_unload ARMv7`.

Na carga, ele chama somente `hiusb_start_hcd()`, símbolo exportado pelo
kernel. Na remoção, chama `hiusb_stop_hcd()`.

Ele não cria um controlador EHCI no barramento de plataforma, não cria um
root hub, não monta dispositivos e não escreve NAND. O objetivo é confirmar
que a sequência de clock/reset do HIUSB pode ser executada na unidade.

O módulo será enviado para `/tmp` e removido após a coleta de `dmesg`.
Qualquer etapa posterior de registrar EHCI exigirá um módulo separado e nova
validação dos recursos MMIO e IRQ.

O bloco `__this_module` reproduz os campos usados pelo ABI dos módulos nativos
da imagem: nome em `+0x0c`, `init_module` em `+0xbc` e `cleanup_module` em
`+0x130`.

`hiusb-ehci.ko` é a etapa seguinte: cria em RAM um dispositivo de plataforma
com os recursos encontrados no kernel e invoca o probe EHCI interno desta
imagem. Seus endereços internos só são válidos para o kernel B de 2018 que foi
verificado via UART. Ele deve ser removido com `rmmod hiusb_ehci_test` após o
teste.

O dispositivo temporário recebe máscaras DMA coerente e de streaming de
32 bits, nos offsets confirmados por DWARF de módulos nativos. Isso é
necessário antes do probe EHCI alocar memória DMA.
