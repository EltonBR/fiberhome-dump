# SD5116 — estado da engenharia reversa de hardware

Escopo: esta ONU FiberHome com SD5116 e `hwcfg=0x24`. Não generalizar para
outras AN5506-02-B. Exploração é somente leitura; não usar `phymem_write` nem
escrever em GPIO/MMIO desconhecido.

## Confirmado

| Área | Evidência útil |
|---|---|
| SoC | HiSilicon SD5116; ARM Cortex-A9 (`part 0xc09`), Linux 2.6.34.10. |
| RAM Linux | `mem=59M`; `0x80500000–0x83ffffff` é System RAM. RAM física de 64 MiB é apenas inferência. |
| NAND | 128 MiB; página 2 KiB; eraseblock 128 KiB; OOB 64 B. |
| UART | `ttyAMA0`: `0x1010e000`, IRQ 77. `ttyAMA1`: `0x1010f000`, IRQ 78 e console a 115200. |
| GPIO atribuídos | 6 VOIP LED; 12 LAN1 LED; 13 LAN2 LED; 14 botão LED; 30 LOS LED; 31 PON LED; 32 reset; 33 LED switch. |
| I²C | Canais lógicos 0 e 1 respondem em `0x50` e `0x51`; ambos pertencem ao caminho óptico. |
| USB Host | EHCI `0x10a40000–0x10a4ffff`, IRQ 71; OHCI `0x10a50000–0x10a5ffff`, IRQ 70. |
| GPON/EPON | Drivers GPON, EPON, óptico, I²C, SPI, MDIO e GPIO existem nesta imagem. Isto não autoriza uso alternativo do laser. |

## GPIO e LEDs

O driver de placa publica o mapa em `/proc/driver/fh_bsp_gpio_list`. O acesso
direto por `/dev/fhdrv_kdrv_board` foi revertido e os exemplos do projeto
`projects/sd5116-cli/` podem ler/escrever GPIOs atribuídos.

`fh_bsp_led_act` e `detectHwEvent` estão comentados no
`humen-mod/app_exB/initialize.sh`; portanto não controlam LEDs no boot atual.
`hi_bridge.ko` mantém a thread `phy_manager` e configura LEDs do PHY via MDIO.
Não remover `hi_bridge.ko`: ele é necessário à Ethernet.

**Teste físico confirmado (LAN1):** `onu-led-direct` gravou GPIO 12 em nível
baixo e manteve o LED LAN1 aceso. Desconectar e reconectar o cabo Ethernet não
alterou seu estado. Assim, na configuração de boot atual, o GPIO 12 está sob
controle exclusivo prático do utilitário direto: `fh_bsp_led_act` e
`detectHwEvent` não estavam em execução. Isto não garante exclusividade caso
esses serviços ou outro módulo sejam ativados futuramente.

## I²C e EEPROM

| Endereço | Uso confirmado | Regra |
|---|---|---|
| `0x50` | identificação óptica: fabricante `0x14`, part number `0x28`, serial `0x44` | somente leitura |
| `0x51` | dados/diagnóstico óptico via `hi_koptical.ko` | somente leitura |
| HE24C08 física | CI de 1 KiB observado na PCB; relação com `0x50`/`0x51` desconhecida | não escrever |

O kernel não expõe `/dev/i2c-*`; usar `/tmp/onu-i2c` para leituras. O helper
proprietário retorna sucesso mesmo em falha, por isso o utilitário valida a
presença de `hex data:` na saída.

## USB

O kernel contém EHCI, OHCI e `usb-storage`. As rotinas identificadas pelos
logs `hiusb_start_hcd`/`hiusb_stop_hcd` manipulam os ponteiros virtuais abaixo:

```text
0xf8100134  0xf8100154  0xfc880020  0xfc900200
```

A tradução `virtual - 0xe8000000` é **inferida**, mas associa-os a:

```text
0x10100134  0x10100154  0x14880020  0x14900200
```

`0x14900200` participa diretamente do start/stop USB e recebe, entre outros,
`0x000c6111` e `0x000c6110`. A função exata de cada campo continua
desconhecida; não escrever nesses endereços.

Não há D+/D−, VBUS ou conector USB confirmado na PCB. Logo USB é suportado pelo
software e pelo SoC, mas não é utilizável externamente até rastreamento físico.

Leituras seguras para confirmar acessibilidade:

```sh
cli /home/cli/hal/chip/phymem_read -v addr 0x10a40000
cli /home/cli/hal/chip/phymem_read -v addr 0x10a50000
cli /home/cli/hal/chip/phymem_read -v addr 0x14900200
```

## FXS, óptica e pads desconhecidos

O SLIC físico informado é Microchip/Microsemi LE9641. O firmware tem pilha
VoIP/SLIC, mas reset, SPI/PCM e configuração FXS ainda não foram mapeados; o
`initialize.sh` atual não inicia VoIP. Não ligar telefone/fax até inicializar a
pilha própria de modo controlado.

`JB1` tem dois pinos próximos a 3,3 V e um GND; não é UART conhecida e não há
evidência suficiente para chamá-lo de USB, JTAG ou SPI.

## Próximas ações seguras

1. Ler os MMIO USB acima e preservar a saída.
2. Rastrear D+/D−, VBUS e a HE24C08 com a PCB desligada.
3. Localizar os pads de UART0 por continuidade, sem injetar sinais.
4. Antes de testar FXS ou GPON, mapear os sinais e preservar a configuração
   original.

Detalhes de boot, partições e limites operacionais estão em
`ESTADO_ATUAL_E_LIMITES.md`; detalhes I²C estão em `I2C_SD5116.md`.
