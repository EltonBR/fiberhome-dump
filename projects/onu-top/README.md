# onu-top

Monitor interativo de processos para a ONU SD5116. Lê somente `/proc`, não usa
`ncurses` e não atualiza sozinho: `r` solicita uma nova amostra e `q` encerra.

## Compilar

```sh
make
```

O resultado é `bin/onu-top`, ARM e estático com uClibc.

## Uso na ONU

```sh
onu-top
onu-top -d 2000 -n 10
onu-top -1
```

- `r`: atualiza manualmente;
- `q`: encerra;
- `-n`: número de processos exibidos;
- `-1`: uma amostra, útil em scripts ou via telnet;
- `--no-clear`: não limpa o terminal entre amostras.

O percentual de CPU é calculado entre duas amostras de `/proc/stat` e
`/proc/<pid>/stat`. RSS é memória residente real; não confundir com VSZ do
`top` BusyBox.
