# I²C rápido para o OLED externo

## Estado

**CONFIRMADO por desmontagem do firmware:** o atributo I²C tem quatro campos
de 32 bits: índice, habilitação, modo de endereço e baud rate. Os arquivos de
CLI da imagem definem `baud_rate=0` como 100 kHz e `baud_rate=1` como 400 kHz.

**CONFIRMADO:** `kdrv_debug i2c write` sempre chama a HAL com `baud_rate=0`
antes de enviar cada byte. Por isso, alterar só o atributo não acelera
`onu-i2c` nem `onu-oled` atuais.

Este experimento carrega as bibliotecas HAL já existentes na ONU e chama
`hi_hal_i2c_attr_set()` e `hi_hal_i2c_data_send()` diretamente. Não instala
bibliotecas e não escreve em firmware, MTD ou configuração.

## Limite de segurança

O binário aceita somente I²C0 e o endereço `0x3c` do OLED SSD1306 externo já
validado. Ele configura 400 kHz ao iniciar e tenta restaurar 100 kHz em
`Ctrl+C` ou término normal. Não interrompa com `kill -9` e não o use enquanto
o caminho óptico depender ativamente de I²C0.

## Compilar e testar

```sh
make host-check
make

# na ONU, após copiar apenas para /tmp:
/tmp/onu-i2c-fast demo --force

# ou mantenha tráfego repetitivo para medir SCL:
/tmp/onu-i2c-fast wave-write --force 0 0x3c 0x40 0x55
```

No osciloscópio, o período de SCL deve cair de aproximadamente 10 µs para
aproximadamente 2,5 µs. Pare com `Ctrl+C`; a mensagem final deve confirmar a
restauração para 100 kHz.

`demo` inicializa o SSD1306 e transmite uma moldura com padrão alternado em
dezoito transações: inicialização, janela e cada página de 128 pixels é
dividida em 127+1 porque o driver HAL aceita no máximo 128 bytes incluindo o
byte de controle. Ele é uma
validação prática do envio em blocos; não usa `kdrv_debug` por byte.
