# Toolchain ARM/uClibc-ng da ONU SD5116

Padrão dos projetos C: `armv5-eabi--uclibc--stable-2025.08-1` da Bootlin.

- alvo: ARMv5 EABI little-endian, adequado ao Cortex-A9 desta ONU;
- libc: uClibc-ng 1.0.54;
- compilador: GCC 14.3.0;
- uso: estático (`-static` nos Makefiles);
- prefixo: `arm-buildroot-linux-uclibcgnueabi-`.

A seleção ARMv5 é intencional: privilegia compatibilidade de instruções e ABI
sobre otimização específica do Cortex-A9. Os utilitários usam APIs básicas já
testadas no kernel 2.6.34 da ONU.

O arquivo `armv5-eabi--uclibc--stable-2025.08-1.sha256` foi baixado junto da
toolchain e validado antes da extração. A origem é a página oficial da Bootlin:
https://toolchains.bootlin.com/releases_armv5-eabi.html

Para sobrescrever a toolchain em um projeto:

```sh
make CROSS_COMPILE=/caminho/para/prefixo-
```
