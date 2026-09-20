# Serial → OLED

`onu-serial-oled` é um relé de sessão interativa. Ele inicia um novo `/bin/sh`
em um pseudo-terminal, preserva a conversa no terminal em que foi iniciado e
desenha a saída do shell no SSD1306 I²C externo a 400 kHz.

Ele não é leitor passivo de `/dev/ttyAMA1`: a leitura desse dispositivo só
receberia bytes vindos do PC. A saída transmitida pelo console UART não volta
automaticamente ao RX. O relé espelha a saída real em userspace.

```sh
/tmp/onu-serial-oled --force
# use o novo shell normalmente; 'exit' encerra e restaura I²C a 100 kHz
```

Use-o a partir da sessão serial. Ele não instala daemon, não altera boot,
firmware, MTD ou configuração persistente. Caracteres ASCII são mostrados em
maiúsculas no OLED; sequências ANSI são ignoradas.
