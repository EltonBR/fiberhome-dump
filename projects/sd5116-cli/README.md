# Testes de hardware SD5116

Este projeto consome as bibliotecas locais e não chama `kdrv_debug`.

| Binário | Biblioteca | Função |
|---|---|---|
| `onu-gpio-test` | `libfh_gpio` | lê, escreve e gera pulsos em GPIO |
| `onu-spi-test` | `libfh_gpio_spi` → `libfh_gpio` | transmite SPI por bit-banging |
| `onu-i2c-test` | `libfh_i2c` | transmite I²C0 pela HAL direta |

Compile com `make`. GPIO e SPI são estáticos. I²C é dinâmico, pois
`libfh_i2c` carrega a HAL proprietária presente na ONU.

## Testes de transmissão

Os comandos que alteram pinos ou enviam bytes exigem `--force`. Execute-os em
`/tmp` e apenas com pino ou periférico previamente identificado.

```sh
# Pulso baixo/alto no pino escolhido; termina em nível alto.
/tmp/onu-gpio-test tx --force 31 100 10000

# SPI modo 0 pelos LEDs: CS=PHONE(6), SCLK=PON(31), MOSI=LOS(30), MISO=LAN1(12).
/tmp/onu-spi-test tx --force --delay-us 10 0x55
/tmp/onu-spi-test tx --force --repeat 100 0x55 0xaa

# OLED SSD1306 externo: controle de dados 0x40, padrão alternado 0x55.
/tmp/onu-i2c-test tx --force --speed 400 --repeat 10 0x3c 0x40 0x55
```

`onu-spi-test` imprime os bytes recebidos, mas sem MISO conectado isso não
valida um periférico. Para outro mapeamento, use `--pins CS CLK MOSI MISO`.
Os pinos padrão foram medidos nesta PCB e não devem ser aplicados a outra
variante sem repetir o mapeamento.

O modo I²C transmite bytes de verdade. Não use endereços internos como `0x50`
ou `0x51` até identificar completamente o periférico.
