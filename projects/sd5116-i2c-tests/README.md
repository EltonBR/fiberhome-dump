# Testes I²C da SD5116

Binários de teste para `/tmp` da ONU. Eles usam a HAL I²C e não alteram NAND,
MTD ou configuração persistente. O estado do hardware e as medições ficam em
[`docs/I2C_SD5116.md`](../../docs/I2C_SD5116.md).

| Binário | Uso |
|---|---|
| `onu-i2c-scan` | varredura por leitura de `0x03..0x77` a 100 kHz |
| `onu-i2c-ssd1306-read` | exercício da operação de recepção no endereço `0x3c` |
| `onu-i2c-oled-recover` | inicialização a 100 kHz e padrão xadrez |
| `onu-i2c-oled-animate` | animação e benchmark do SSD1306 |
| `onu-lcd1602-pcf8574` | demonstração PCF8574T: 16 caracteres em cada linha do LCD 16×2 |
| `onu-lcd1602-map-test` | alterna quatro mapeamentos PCF8574→HD44780, 4 s cada |

## Compilação e execução

```sh
make -C projects/sd5116-i2c-tests
projects/onu-transfer/onu-transfer.py push \
  projects/sd5116-i2c-tests/bin/onu-i2c-oled-animate \
  /tmp/onu-i2c-oled-animate
projects/onu-transfer/onu-transfer.py exec \
  'chmod 755 /tmp/onu-i2c-oled-animate && /tmp/onu-i2c-oled-animate 400 --max'
```
