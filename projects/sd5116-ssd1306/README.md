# SSD1306 em userspace — SD5116

Driver de userspace para OLED I²C SSD1306 **128×64**, no barramento I²C já
confirmado nos pads da ONU. Não altera kernel, módulos, MTD ou firmware.

Ele usa `/fh/extend/kdrv_debug i2c write` para cada byte. Isso é adequado para
texto e telas estáticas, porém a atualização de 1.024 bytes é lenta para
animações.

## Ligações

| OLED | ONU |
|---|---|
| GND | pad GND confirmado |
| VCC | somente 3,3 V regulados; **nunca 5 V** |
| SCL | pad SCL confirmado |
| SDA | pad SDA confirmado |

O OLED padrão costuma usar endereço `0x3c`; alguns módulos usam `0x3d`. O
barramento pertence também ao caminho óptico da ONU. Não conecte outro mestre,
não injete tensão nos pads e não assuma que o pad fornece VCC. Meça SDA/SCL em
repouso antes: o nível alto precisa ser compatível com 3,3 V.

## Compilação e uso

```sh
make

# Inicializa e mostra a tela de teste no endereço padrão 0x3c, canal 0.
/tmp/onu-oled --force demo

# Inicializa, limpa e mostra texto ASCII simples.
/tmp/onu-oled --force init
/tmp/onu-oled --force clear
/tmp/onu-oled --force text 0 0 "OLA SD5116"

# Para OLED configurado em 0x3d ou para testar o canal lógico 1.
/tmp/onu-oled --force --channel 1 --address 0x3d demo
```

`text` usa fonte 5×7 para A–Z, 0–9, espaço, `.`, `:`, `-` e `/`; minúsculas
são convertidas para maiúsculas. `coluna` vai de 0 a 20 e `pagina` de 0 a 7.
Cada chamada `text` limpa a tela antes de desenhar o texto.

## Segurança

`--force` é obrigatório porque cada comando escreve no dispositivo I²C. A
primeira operação deve ser `demo`, com UART aberta. Se houver instabilidade do
link óptico, pare o teste e desconecte o OLED; o barramento é compartilhado.
Esta versão não detecta automaticamente o OLED nem interpreta ACKs falsos do
helper proprietário; falha de comunicação pode aparecer apenas como tela sem
imagem.
