# FiberHome SD5116 — contexto operacional

## Escopo e regras

ONU FiberHome baseada em HiSilicon SD5116, `hwcfg=0x24`. Não generalizar para
outras AN5506-02-B. Shell root por serial já existe; não redescobrir acesso.

- `firmware-original/` e `extracted/` são imutáveis.
- Slot A é recuperação: nunca alterar `mtd5–mtd7`.
- Nunca gravar `mtd0–mtd4` ou `mtd11`; nunca usar `dd` em NAND/MTD.
- Experimentos de filesystem ficam em `modified/`; scripts da ONU usam BusyBox
  POSIX `sh`.
- Engenharia de hardware é leitura primeiro: não usar `phymem_write` nem
  escrever em MMIO, I²C, óptica ou GPIO desconhecido.

## Sistema confirmado

| Item | Valor |
|---|---|
| SoC | SD5116, ARM Cortex-A9 (`part 0xc09`) |
| Kernel | Linux 2.6.34.10 ARMv7 |
| RAM Linux | `mem=59M`; System RAM `0x80500000–0x83ffffff` |
| NAND | 128 MiB; página 2 KiB; eraseblock 128 KiB; OOB 64 B |
| Console | `ttyAMA1`, 115200 (`console=ttyAMA1,115200`) |
| UART0 | `0x1010e000`, IRQ 77 |
| UART1 | `0x1010f000`, IRQ 78 |

64 MiB de RAM física é **inferência**. Os 5 MiB anteriores ao mapa Linux são
excluídos pelo layout U-Boot/kernel; U-Boot carrega `uImage` em `0x80008000`.
O ocupante completo do intervalo após o boot é desconhecido. Não mudar
`mem=59M`: isso não recupera a faixa inferior e pode mapear RAM inexistente.

## Boot e rede

- Partições B: `mtd8` rootfs, `mtd9` `/fh/bin`, `mtd10` `/fh/extend`.
- A ROM inicia `mtd0` (`startcode`), que valida U-Boot A/B por instrução,
  magic, tamanho e CRC antes de transferir controle; ele contém fallback para
  uma cópia válida. A prioridade exata entre A/B ainda é desconhecida.
- `fhdrv_kdrv_mount` monta `cfg` em `/fhcfg` e seleciona as aplicações A/B.
- `rcS` chama `initialize.sh`; `net_dev_created` cria `eth*` e `br0`.
- `load_cli` é a autenticação FiberHome; não usa `/etc/passwd`.
- O `humen-mod` atual mantém rede estática em `br0` e não inicia WebUI, CLI,
  `fh_bsp_led_act` ou `detectHwEvent`.
- Não remover `hi_gpon.ko`/`hi_epon.ko`: a tentativa anterior quebrou a cadeia
  Ethernet/CFE. Desativar serviços PON em espaço de usuário, não os drivers.

## GPIO, LEDs e PHY

```text
GPIO  6  VOIP LED       GPIO 12  LAN1 LED
GPIO 13  LAN2 LED       GPIO 14  botão LED
GPIO 30  LOS LED        GPIO 31  PON LED
GPIO 32  reset          GPIO 33  LED switch
```

Mapa publicado em `/proc/driver/fh_bsp_gpio_list`. Há acesso direto por
`/dev/fhdrv_kdrv_board` e exemplos em `projects/sd5116-cli/`.

`hi_bridge.ko` mantém `phy_manager` e configura LEDs do PHY por MDIO. Sem o
daemon `fh_bsp_led_act`, ele é o candidato à atividade automática de LAN1/LAN2.
Teste físico: `onu-led-direct` mantém LAN1 (GPIO 12, ativo-baixo) aceso mesmo
após desconectar/reconectar Ethernet. No boot atual, o utilitário tem controle
exclusivo prático desse LED; não é garantia se serviços de LED forem reativados.

## I²C, SPI, óptica e FXS

- I²C 0 e 1 respondem apenas em `0x50` e `0x51`.
- `0x50`: identificação óptica; `0x51`: dados/diagnóstico óptico (`hi_koptical`).
- HE24C08 física: relação com esses endereços é **desconhecida**. Não escrever.
- Não há `/dev/i2c-*`; usar `/tmp/onu-i2c`/`kdrv_debug i2c` para leitura.
- Há `hi_spi` e SPI de SerDes no firmware, mas não há pinout SPI externo
  confirmado. `OUT_SLIC_SPI_CS` vale `-1` nesta variante.
- Padrão do SPI por bit-banging em `projects/sd5116-gpio-spi/`: CS GPIO 6
  (PHONE), SCLK GPIO 31 (PON), MOSI GPIO 30 (LOS), MISO GPIO 12 (LAN1).
  Foi validado apenas como transferência dummy pelos LEDs; não é SPI dedicado
  nem há cartão/periférico conectado.
- O SLIC físico informado é LE9641. A pilha FXS existe no firmware, mas
  reset/SPI/PCM/configuração ainda não foram mapeados; o init atual não inicia
  VoIP.
- GPON/EPON e óptica são suportados pelo firmware; não usar o laser como fonte
  genérica.

## USB: resultado estático

```text
EHCI  0x10a40000–0x10a4ffff  IRQ 71
OHCI  0x10a50000–0x10a5ffff  IRQ 70
```

O kernel contém EHCI, OHCI e `usb-storage`. `hiusb_start_hcd`/`stop_hcd`
manipulam `0xf8100134`, `0xf8100154`, `0xfc880020` e `0xfc900200`.

`virtual - 0xe8000000` é uma tradução **inferida**, que produz
`0x10100134`, `0x10100154`, `0x14880020` e `0x14900200`. O último participa
diretamente da inicialização USB e recebe `0x000c6111`/`0x000c6110`; a função
dos registradores/bits ainda é desconhecida.

Não há D+/D−, VBUS, conector USB ou relação com `JB1` confirmados. USB é
suportado pelo SoC/software, mas não é ainda uma porta externa utilizável.

## Pads e próximos passos

- `JB1`: dois pinos próximos a 3,3 V e um GND; não é a UART conhecida. Pode
  ser GPIO, fábrica, JTAG, USB ou outro barramento: **desconhecido**.
- Leituras seguras prioritárias:

```sh
cli /home/cli/hal/chip/phymem_read -v addr 0x10a40000
cli /home/cli/hal/chip/phymem_read -v addr 0x10a50000
cli /home/cli/hal/chip/phymem_read -v addr 0x14900200
```

- Depois: rastrear D+/D−/VBUS, HE24C08 e UART0 com PCB desligada; não injetar
  tensão nem alterar registradores.

Detalhes ficam em `docs/`; o resumo de hardware é
`docs/estado atual da eng reversa de hw.md`.
