# Framebuffer SSD1306

Renderiza uma tela completa no OLED externo validado (`I²C0`, `0x3c`) por HAL
direta a 400 kHz. Não grava arquivos persistentes nem muda firmware.

```sh
# Até oito argumentos, cada um uma linha de 21 caracteres 5x7.
/tmp/onu-oled-frame --force text "ONU SD5116" "IP 192.168.1.245"

# BMP BI_RGB exatamente 128x64; aceita 1, 24 ou 32 bpp.
/tmp/onu-oled-frame --force bmp /tmp/tela.bmp
```

BMP 24/32 bpp usa pixels escuros como acesos; BMP 1 bpp usa o bit 1 como
aceso. O programa restaura I²C a 100 kHz ao terminar.
