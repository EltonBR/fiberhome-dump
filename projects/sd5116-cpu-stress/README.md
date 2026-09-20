# `onu-cpu-stress`

Gerador de carga contínua de CPU para medir consumo da FiberHome SD5116 sob
carga. Ele não acessa NAND, MTD, rede, GPIO ou configurações persistentes.

## Compilação

```sh
make
make host-check
```

O binário resultante é `bin/onu-cpu-stress`, ARM EABI estático com uClibc-ng.

## Uso na ONU

Esta ONU expõe uma CPU lógica ao Linux; portanto, um worker é suficiente para
ocupar a CPU disponível. Mais workers apenas disputam o mesmo núcleo lógico,
mas podem ser úteis para confirmar o agendador.

```sh
# Carga total por dois minutos, com encerramento automático.
/tmp/onu-cpu-stress -d 120

# Carga contínua; interrompa com Ctrl+C no serial ou telnet.
/tmp/onu-cpu-stress

# Quatro processos concorrentes, por 30 segundos.
/tmp/onu-cpu-stress -w 4 -d 30
```

Durante o teste, `top` deve mostrar o worker usando aproximadamente 100% da
CPU. O programa não mede corrente: use um medidor em série com a alimentação
de 12 V, compare repouso e carga mantendo Ethernet e LEDs no mesmo estado, e
anote tensão e corrente.

Comece com 30–120 segundos e acompanhe o equipamento. O slot A continua
intocado; desligar a alimentação encerra a carga, mas não é o método normal de
parada.

## Temperatura

**Não confirmado:** a análise estática do kernel não encontrou strings de
`thermal`, `hwmon`, `temperature`, sensor térmico ou throttling térmico. Os
dados runtime disponíveis também não incluem uma leitura de temperatura. Isso
não prova que o SoC não possua sensor; apenas não há uma interface descoberta.

No shell da ONU, estes comandos são somente leitura e verificam se o kernel
expõe algo em runtime:

```sh
find /sys/class/thermal /sys/class/hwmon -type f 2>/dev/null
find /proc -iname '*temp*' -o -iname '*thermal*' 2>/dev/null
dmesg | grep -i -E 'thermal|temp|overheat|thrott'
```

Se não houver `thermal_zone*/temp` nem `hwmon*/temp*_input`, a alternativa
confiável é medir externamente a temperatura do encapsulamento do SD5116 com
termopar, sonda ou câmera térmica. Não há base para presumir proteção térmica
automática neste firmware.
