# `onu-transfer`

Cliente estilo ADB para a ONU SD5116. Ele usa o Telnet root da ONU somente para
iniciar uma sessão temporária BusyBox `nc`; os bytes seguem por uma segunda
conexão TCP. Não instala daemon nem deixa porta aberta ao concluir.

Por padrão conecta em `192.168.1.245:23`. Sobrescreve o destino apenas depois
de receber todo o arquivo em um temporário e compara SHA-256 ao final.

## Uso

```sh
./onu-transfer.py push ./programa /tmp/programa
./onu-transfer.py pull /tmp/log.txt ./log.txt
./onu-transfer.py exec 'uname -a'
```

Outro IP/porta:

```sh
./onu-transfer.py --host 192.168.1.245 --port 23 push arquivo /tmp/arquivo
```

No `pull`, o IP do PC é descoberto pela rota até a ONU. Se a ONU não conseguir
alcançá-lo, informe explicitamente o IP anunciado:

```sh
./onu-transfer.py pull --local-host 192.168.1.5 /tmp/log.txt ./log.txt
```

## Limites

- Destinado a uma LAN confiável: Telnet sem login é inerentemente inseguro.
- A ONU precisa ter `nc` BusyBox e rota de retorno para o PC no `pull`.
- `push` usa `nc -l -p PORT` temporariamente na ONU; `pull` abre listener
  temporário no PC.
- Use `--no-verify` somente se o custo do SHA-256 for indesejável.
- A ferramenta não escreve MTD/NAND; caminhos remotos continuam sob
  responsabilidade de quem a invoca.
