# FiberHome SD5116 — contexto operacional

## Escopo e limites

Esta unidade é uma FiberHome baseada em HiSilicon SD5116, `hwcfg=0x24`. Não
generalizar conclusões para outra AN5506-02-B ou outra PCB.

- `firmware-original/` e `extracted/` são evidência imutável.
- Slot A é recuperação: nunca alterar `mtd5`, `mtd6` ou `mtd7`.
- Nunca gravar `mtd0`–`mtd4` nem `mtd11`; nunca usar `dd` em NAND/MTD.
- Não existe mais diretório `modified/`. Experimentos persistentes, se forem
  necessários, devem ser explicitamente autorizados e separados da extração.
- Scripts para a ONU precisam ser POSIX `/bin/sh` compatível com BusyBox 1.18.
- Testes de hardware devem usar `/tmp` e não mudar boot, MTD ou configuração.

## Hardware e boot confirmados

| Item | Valor observado |
|---|---|
| SoC | SD5116; Cortex-A9 (`part 0xc09`) |
| Kernel | Linux 2.6.34.10 ARMv7 |
| RAM Linux | `mem=59M`; mapa `0x80500000–0x83ffffff` |
| NAND | 128 MiB; página 2 KiB; eraseblock 128 KiB; OOB 64 B |
| Console | `ttyAMA1`, 115200 |
| UART0/UART1 | `0x1010e000` IRQ 77 / `0x1010f000` IRQ 78 |
| MTD A | `mtd5` rootfs, `mtd6` app_bin, `mtd7` app_ex |
| MTD B | `mtd8` rootfs, `mtd9` app_bin, `mtd10` app_ex |

64 MiB de RAM física é inferência; 5 MiB abaixo de `PHYS_OFFSET` não são
visíveis ao Linux. Não mudar `mem=59M`.

`rcS` chama `initialize.sh`; `net_dev_created` cria Ethernet e `br0`.
`load_cli` é autenticação FiberHome própria, não o login Unix de
`/etc/passwd`. O `initialize.sh` de teste atual preserva rede estática em
`br0` e não inicia WebUI, CLI, `fh_bsp_led_act` ou `detectHwEvent`.

Não remover `hi_gpon.ko` ou `hi_epon.ko`: a remoção anterior quebrou a cadeia
Ethernet. Para o Linux mínimo, desativar serviços PON de userspace.

## GPIO e SPI por bit-banging

O dispositivo GPIO é `/dev/fhdrv_kdrv_board`. A ABI foi recuperada de
`kdrv_debug` e `libfhdrv_kdrv_board.so`; as bibliotecas locais são a interface
atual, não `kdrv_debug`.

```text
GPIO  6 PHONE LED     GPIO 12 LAN1 LED
GPIO 13 LAN2 LED      GPIO 14 botão LED
GPIO 30 LOS LED       GPIO 31 PON LED
GPIO 32 RESET         GPIO 33 chave geral dos LEDs
```

LEDs são ativos em nível baixo. GPIO 31 e GPIO 12 foram testados fisicamente;
LAN1 manteve controle direto durante o boot com os serviços de LED desativados.

SPI software validado apenas como transmissão dummy visível pelos LEDs:

```text
CS=GPIO 6, SCLK=GPIO 31, MOSI=GPIO 30, MISO=GPIO 12
```

Não há periférico SPI externo ou pinout dedicado confirmado. Para outra PCB,
refazer o mapeamento.

## I²C

Não há nós `/dev/i2c-*`. O acesso atual usa a HAL proprietária direta:

```text
libhi_ubasic.so → libhi_ioreactor.so → libhi_ipc.so → libhi_hal.so
```

`libfh_i2c` expõe I²C0 a 100 ou 400 kHz e mensagens de até 128 bytes. O OLED
SSD1306 externo em I²C0, endereço `0x3c`, foi validado a 100 e 400 kHz; o
framebuffer completo atingiu 10 FPS e 30 FPS, respectivamente.

Os endereços `0x50` e `0x51` também respondem nos canais investigados e fazem
parte do caminho óptico. A associação da HE24C08 física a esses endereços não
foi provada. Não escrever em `0x50`/`0x51`.

## Projetos e bibliotecas atuais

```text
libs/libfh_gpio       GPIO por ioctl direto
libs/libfh_gpio_spi   SPI por bit-banging sobre GPIO
libs/libfh_i2c        I²C HAL direta
libs/libfh_i2c_lcd    framebuffer SSD1306 sobre I²C

projects/sd5116-cli        testes atuais de transmissão GPIO/SPI/I²C
projects/sd5116-i2c-tests  testes de OLED, scan e LCD PCF8574
projects/sd5116-oled-frame renderização de texto/BMP no SSD1306
projects/onu-transfer      push/pull/exec via rede
```

`sd5116-cli` não usa `kdrv_debug`:

```sh
/tmp/onu-gpio-test tx --force 31 100 10000
/tmp/onu-spi-test tx --force --delay-us 10 0x55
/tmp/onu-i2c-test tx --force --speed 400 --repeat 10 0x3c 0x40 0x55
```

## Interfaces ainda sem uso prático

- USB: EHCI/OHCI e `usb-storage` existem no software, mas não foram achados
  D+/D−, VBUS ou conector externo na PCB. Não há porta USB utilizável provada.
- FXS: há SLIC LE9641 e pilha VoIP no firmware, mas reset, SPI, PCM e a
  configuração não foram mapeados. VoIP não inicia no boot mínimo atual.
- GPON/EPON: não usar o transmissor óptico como laser genérico.
- `JB1`: conector I²C identificado: GND, SCL e SDA. Foi usado para os testes
  com OLED SSD1306 e LCD PCF8574.

Documentos de referência: `docs/ANALYSIS.md`,
`docs/estado atual da eng reversa de hw.md` e
`docs/CADERNO_RE_KDRV_DEBUG_GPIO_I2C.md`.
