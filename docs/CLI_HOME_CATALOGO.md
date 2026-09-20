# `/home/cli`: catálogo da CLI proprietária

`/home/cli` não contém binários executáveis. A árvore extraída tem **422
descritores de comando**: arquivos texto INI que o executável
`/usr/bin/cli` interpreta. Cada arquivo declara `functionname`, parâmetros,
tipos, faixas e valores padrão; a função é então executada pelas bibliotecas e
módulos FiberHome/Hisan.

Exemplo:

```text
/home/cli/hal/chip/i2c_attr_set
  functionname=hi_kernel_hal_i2c_attr_set_cli
```

Assim, isto não inicia um programa independente:

```sh
cli /home/cli/hal/chip/i2c_attr_set -v ...
```

É uma chamada ao interpretador `cli` com o descritor como argumento. Os 422
arquivos são interface de gestão e diagnóstico da ONU; não são necessários
para `br0`, GPIO direto, SPI por bit-banging ou a HAL I²C usada pelas novas
ferramentas.

## `port_transform_set` no `initialize.sh`

O bloco original é:

```sh
if [ $DEV == "0x30" ] || [ $DEV == "0x31" ]
then
    cli /home/cli/hal/port/port_transform_set -v tran_port1 3 tran_port3 1
fi
```

Ele não roda na placa analisada, cujo `DEV` é `0x24`.

O descritor chama `hi_kernel_hal_eth_port_transform_set_cli` e define uma
tabela de transformação de portas Ethernet. Os valores declarados são a
identidade (`tran_portN=N`). A chamada fornece explicitamente:

```text
tran_port1 = 3
tran_port3 = 1
```

Logo, a intenção é trocar os índices lógicos 1 e 3 nas variantes `0x30` e
`0x31`; as demais portas permanecem nos valores padrão declarados pelo
descritor. A relação final com conectores físicos depende do `hwcfg` e não foi
medida nesta PCB. Não há motivo para executar esse comando em `0x24`.

Para consulta sem escrever, o par é:

```sh
cli /home/cli/hal/port/port_transform_get
```

## Inventário completo por subsistema

| Raiz | Quantidade | O que controla |
|---|---:|---|
| `hal/` | 268 | HAL do chip: registradores, GPIO, I²C, Ethernet, L2/L3, PON, QoS, segurança e contadores |
| `cfe/` | 73 | CFE: aceleração/encaminhamento, interfaces, aprendizado MAC, NAT e tabelas de rota |
| `sample/` | 35 | adaptadores de multicast e rede de exemplo/integração |
| `voip/` | 19 | canais, codec, tons e diagnóstico VoIP |
| `slic/` | 8 | SLIC: inicialização, registradores e SPI da telefonia |
| `ploam/` | 11 | PLOAM GPON, LOS, laser e SerDes |
| `log_cmd/` | 4 | configuração de logs de kernel e userspace |
| `optical/` | 3 | atributos, potência e estado óptico |
| `ethport/` | 1 | debug do PHY Ethernet |

O índice completo é a própria árvore abaixo de
`extracted/kernel_rootfsB/home/cli/`; cada nome já descreve sua operação.
Para listar todos sem modificar nada:

```sh
find extracted/kernel_rootfsB/home/cli -type f -printf '%P\n' | sort
```

## Grupos `hal/`

| Grupo | Comandos e efeito |
|---|---|
| `chip/` (35) | versão, GPIO, I²C, memória física, SerDes, LED LAN, óptica, watchdog e PDU. `*_get`, `*_read` e `*_dump` são consulta; `*_set`, `*_write`, `init_serdes`, `optical_power_set` e `phymem_write` alteram hardware. |
| `cnt/` (37) | leitura e limpeza de contadores CPU, Ethernet, GPON/EPON, GEM, filas, MAC e razões de descarte. |
| `flow/` (16) | regras de fluxo de entrada/saída: criar, consultar, despejar, remover ou limpar. |
| `l2/` (15) | VLAN, isolamento, aprendizado e tabela MAC. |
| `l3/` (24) | rotas, sessões IP, túneis, ações e contadores WAN. |
| `mc/` (9) | multicast: atributos, usuários e grupos. |
| `nni/` (27) | GPON/EPON: GEM, T-CONT, DBA/DBRu, chaves EPON, mapeamento PON e modo upstream. |
| `oam/` (30) | Ethernet OAM 802.1ag, 802.3ah e Y.1731: CFM, loopback, medição, VLAN e contadores. |
| `port/` (32) | portas Ethernet: 802.1X, status, modo, MTU, espelho, loopback, STP, VLAN/tag, pausa, CAR e transformação de portas. |
| `prbs/` (9) | gerador/verificador PRBS e contadores, usado em teste físico. |
| `qos/` (15) | filas, escalonamento, shaping, CAR e cor de tráfego. |
| `sec/` (19) | filtros ARP/IP/MAC/VLAN e modo de segurança. |

## Grupos fora de `hal/`

| Grupo | Comandos e efeito |
|---|---|
| `cfe/intf/` | cria, relaciona, remove e associa interfaces CFE a GEM, T-CONT, VLAN, PPP, túnel, bridge e VPN. |
| `cfe/hw/`, `cfe/l3_dbg/`, `cfe/virt/`, `cfe/lrn/` | programação/depuração de IPv4/IPv6 e multicast em hardware, tabelas L3, MAC virtual e aprendizado. |
| `cfe/ant/`, `cfe/dia/` | limites e hooks de diagnóstico. |
| `sample/mc_adapter/`, `sample/net_adapter/` | ponte entre multicast/rede Linux e a HAL; não são programas de aplicação. |
| `ploam/`, `optical/` | controle do caminho GPON/laser/SerDes. Não usar como interface genérica de hardware. |
| `slic/`, `voip/` | controle de telefonia. Não executar sem inicializar a pilha FXS. |
| `ethport/phy_debug` | depuração de PHY; potencialmente altera registradores PHY conforme os argumentos. |

## Regra prática de segurança

Pelos nomes, apenas `get`, `read`, `dump`, `status`, `version` e `*_cnt_get`
podem ser considerados candidatos a consulta; ainda assim podem exigir driver
ou módulo carregado. `set`, `write`, `add`, `del`, `clear`, `start`, `stop`,
`init`, `flush`, `reset`, `power` e `transform` mudam estado e não devem ser
executados por tentativa.

Em particular, não usar sem motivo específico:

```text
hal/chip/phymem_write       hal/chip/mem_write
hal/chip/optical_power_set  hal/chip/wdt_mode_set
hal/chip/*serdes*           ploam/setlasermode
slic/*spi*                  voip/*
hal/nni/*                   hal/oam/*
```

## Relação com o Linux mínimo

No `new-overlay`, a única chamada a essa árvore está dentro da condição de
`DEV=0x30`/`0x31`; ela não executa em `0x24`. Portanto `/home/cli` e o binário
`cli` são candidatos a exclusão de uma imagem futura voltada apenas a Linux,
depois de confirmar que nenhum script/manual de manutenção restante os chama.
Não remover agora para preservar a possibilidade de diagnóstico e porque ainda
há módulos do fornecedor no boot.
