# sd5116-gpio-spi

Projeto experimental para SPI por bit-banging sobre GPIO na ONU FiberHome
SD5116. Ele usa diretamente `/dev/fhdrv_kdrv_board`, sem `kdrv_debug`, shell
ou biblioteca proprietária de userspace.

## Limite elétrico importante

Os GPIOs escolhidos já dirigem LEDs. Não conecte um periférico SPI diretamente
ao encapsulamento do LED, ao seu anodo ou ao lado da alimentação.

### O que foi confirmado

| LED | GPIO do SD5116 | Estado que acende |
|---|---:|---|
| PON | 31 | nível baixo |
| LOS | 30 | nível baixo (padrão do driver; validar visualmente) |
| PHONE/VoIP | 6 | nível baixo (padrão do driver; validar visualmente) |
| LAN1 | 12 | nível baixo (padrão do driver; validar visualmente) |
| LAN2 | 13 | nível baixo (padrão do driver; validar visualmente) |

O PON foi validado fisicamente: escrever zero no GPIO 31 acende o LED.

### Direção elétrica do LED

**CONFIRMADO por medição e teste na placa:** a topologia é
`3,3 V -> resistor -> LED+ -> LED− -> GPIO do SoC`. Com o LED apagado,
`LED+` e `LED−` medem aproximadamente 3,3 V em relação ao GND. Para acender,
o GPIO do SoC puxa diretamente o lado `LED−` para nível baixo; portanto ele
atua como *current sink*.

| Ponto | Papel provável |
|---|---|
| Pino GPIO listado acima | lado do SoC; conexão ao `LED−` após o resistor/LED |
| `LED+` | lado ligado à alimentação através de resistor limitador; 3,3 V quando apagado |
| `LED−` | 3,3 V quando apagado; o GPIO do SoC o puxa para baixo ao acender |

Não há transistor: a ligação GPIO–LED é direta e o resistor/LED permanece uma
carga inadequada para um barramento externo. Confirme o ponto de solda com
multímetro antes de conectar qualquer periférico:

1. Com a ONU desligada, localize o resistor em série de cada LED.
2. Meça continuidade entre o GPIO/SoC e `LED−` apenas se precisar localizar
   o ponto físico de solda.
3. Localize o lado de `LED+` por continuidade para o rail de alimentação,
   nunca injetando tensão no barramento.
4. Só exponha um sinal externo no **lado do SoC**, de preferência depois de
   isolar/remover o resistor ou LED correspondente.

O LED e seu resistor são uma carga inadequada para SPI externo: distorcem
níveis, adicionam capacitância e, no caso de MISO, podem puxar a linha para a
alimentação. Sem isolamento, o projeto serve apenas para demonstração de
software e os LEDs piscarão durante a transferência.

## Mapeamento SPI padrão

| Sinal | GPIO | LED associado |
|---|---:|---|
| CS | 6 | PHONE/VoIP |
| SCLK | 31 | PON |
| MOSI | 30 | LOS |
| MISO | 12 | LAN1 |

Este é o padrão salvo para esta unidade e também é usado pelo comando
`xfer-default`. Não são pinos SPI dedicados nem um barramento externo já
confirmado. O comando `xfer` continua aceitando quatro GPIOs distintos para
experimentos controlados. Evite GPIO 32 (botão Reset) e GPIO 14 (botão LED).

## Compilação

```sh
make
make host-check
```

O resultado é `bin/onu-gpio-spi`, ELF ARM estático. O código usa a mesma ABI
ioctl de GPIO já validada pelo `onu-led-direct`:

```text
config_mux: 0x40085300
set_mode:   0x40085301
write:      0x40085303
read:       0xc0085304
```

## Uso

```sh
/tmp/onu-gpio-spi info

# Usa o mapeamento padrão CS=6, SCLK=31, MOSI=30, MISO=12.
/tmp/onu-gpio-spi xfer-default --force 0 100 0x9f 0x00 0x00

# Três bytes, modo 0, com 100 us entre transições.
/tmp/onu-gpio-spi xfer --force 0 100 6 31 30 12 0x9f 0x00 0x00
```

`--force` é obrigatório porque a transferência muda fisicamente os GPIOs e
pode acender LEDs. Ao terminar — inclusive em erro — a ferramenta recoloca os
quatro GPIOs como saídas em nível alto, que deixa esses LEDs apagados.

Os quatro modos SPI padrão são suportados (`0` a `3`, CPOL/CPHA), em
transferências de 1 a 64 bytes. O sinal CS é ativo-baixo. Comece com atraso de
`100` us e reduza somente após observar os sinais. Não o use para flash SPI,
EEPROM, BOSA, GPON, FXS ou qualquer circuito interno da ONU.
