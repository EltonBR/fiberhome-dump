# Levantamento para redução da pilha GPON/EPON

## Escopo e regra

Este levantamento cobre somente arquivos associados à pilha GPON/EPON desta
imagem B. Ele **não autoriza remoção automática**: primeiro devem ser aplicados
os patches de desativação, iniciado o equipamento com UART conectado e
confirmado que Ethernet e o shell continuam funcionais.

Classificações:

- **Confirmado**: há uma chamada direta no caminho de boot ou uma dependência
  ELF observada.
- **Candidato condicionado**: pode ser removido apenas depois que todos os
  consumidores também forem removidos ou substituídos.
- **Manter por enquanto**: pode ter relação indireta com hardware, bridge ou
  outros processos ainda ativos.

As configurações persistentes em `/fhcfg` não entram na lista de remoção. Elas
permanecem disponíveis e não são apagadas pelos patches.

## Ordem de redução proposta

1. Desativar somente os inicializadores PON de espaço de usuário em `app_exB`:
   `hi_xpon_app`, `hi_kploam`, `load_omci` e `epon_oam`.
2. Testar UART, `br0`, portas Ethernet e estabilidade de boot.
3. Confirmar em runtime que não existem processos OMCI/GPON/EPON e verificar
   os módulos carregados em `/proc/modules`.
4. Só então criar um patch de remoção por grupos, sempre em `modified/`,
   reconstituir e reextrair para validar.

## Arquivos de inicialização: desativados, mas ainda preservados

| Partição | Arquivo / ponto | Estado | Motivo |
| --- | --- | --- | --- |
| `kernel_rootfsB` | `etc/rc.d/rcS`: `hi_gpon.ko`, `hi_epon.ko` | Manter | O experimento 021 comprovou que a remoção quebra símbolos necessários a HAL/CFE e impede a criação de Ethernet. |
| `app_exB` | `initialize.sh`: `hi_xpon_app` | Confirmado | Executável iniciado diretamente antes de `net_dev_created`; patch 020 o impede. |
| `app_exB` | `initialize.sh`: `hi_kploam.ko` | Confirmado | Módulo de serviço carregado diretamente; patch 020 o impede. |
| `app_exB` | `initialize.sh`: `load_omci` | Confirmado | Iniciado para GPON e para o caso padrão; patch 020 impede as duas chamadas. |
| `app_exB` | `initialize.sh`: `epon_oam` | Confirmado como referência, desconhecido como arquivo | O script chama `/fh/extend/epon_oam`, mas esse arquivo não está presente na extração. Pode ser criado em runtime, fornecido por outra área ou ser uma referência obsoleta. |

Esses arquivos são mantidos inicialmente para que a mudança seja reversível por
uma única edição de script.

## Grupo 1 — inicializadores de espaço de usuário, após teste do boot

| Partição | Arquivo | Tamanho | Classificação | Evidência / condição |
| --- | --- | ---: | --- | --- |
| `kernel_rootfsB` | `lib/hsan/ko/service/hi_kploam.ko` | 67.296 B | Candidato condicionado | Carregamento direto foi desativado; confirmar ausência de `insmod` posterior. |
| `app_exB` | `hi_xpon_app` | 19.296 B | Candidato condicionado | É o inicializador XPON explicitamente desativado. |
| `app_exB` | `load_omci` | 6.119 B | Candidato condicionado | É o carregador OMCI explicitamente desativado. |
| `app_exB` | `omci.conf` | 43 B | Candidato condicionado | `cp_cfg.sh` o copia para `/fhcfg`; remover somente ao também retirar esse trecho de cópia. |
| `app_exB` | `web/state/pon_info.asp` | 2.731 B | Candidato condicionado | Página de estado PON; só faz sentido remover junto com toda a WebUI. |

## Grupo 2 — bibliotecas OMCI/GPON: não remover isoladamente

| Partição | Arquivo | Tamanho | Consumidores observados | Decisão atual |
| --- | --- | ---: | --- | --- |
| `app_exB` | `libnomci.so` | 1.379.221 B | `load_omci` | Candidato após remover `load_omci`; ainda verificar dependências dinâmicas de outros binários. |
| `app_exB` | `libpon_iadcfg.so` | 30.165 B | `load_omci`, `detectHwEvent` | Manter: `detectHwEvent` continua iniciado no boot. |
| `app_exB` | `libgpon_l2.so` | 406.933 B | `load_omci`, `l3mng`, `detectHwEvent` | Manter: os dois últimos continuam ativos. |
| `app_exB` | `libgpon_l3.so` | 229.515 B | `load_omci`, `l3mng`, `detectHwEvent` | Manter pelo mesmo motivo. |
| `app_exB` | `libgpon_l3_api.so` | 4.771 B | `load_omci`, `l3mng`, `detectHwEvent` | Manter pelo mesmo motivo. |
| `app_exB` | `libgpon_rm.so` | 90.459 B | `load_omci`, `l3mng`, `detectHwEvent` | Manter pelo mesmo motivo. |
| `app_exB` | `libdhcpL2OmciCfg.so` | 14.864 B | `load_omci`, `detectHwEvent` | Manter enquanto `detectHwEvent` existir. |
| `app_exB` | `libomci_cli.so` | 44.699 B | CLI/OMCI, consumidor não completamente mapeado | Manter até retirar ou substituir a CLI FiberHome. |
| `app_exB` | `libgpon_l2_cli.so` | 132.539 B | CLI GPON, consumidor não completamente mapeado | Manter até retirar ou substituir a CLI FiberHome. |
| `kernel_rootfsB` | `lib/hsan/so/service/libhi_ploam.so` | 10.120 B | `load_omci`, `l3mng`, `detectHwEvent` | Manter. |
| `kernel_rootfsB` | `lib/hsan/so/sample/libhi_omci_adapter.so` | 123.345 B | Consumidores não confirmados | Manter até inventário ELF completo. |

## Grupo 3 — ferramentas e interface, posterior

`kernel_rootfsB/home/cli/ploam/` e os comandos `home/cli/hal/nni/nni_gpon_*`,
`nni_epon_*`, `nni_pon_*`, `cnt_gpon_get` e `cnt_epon_get` pertencem à CLI de
diagnóstico/configuração. São candidatos somente após a CLI FiberHome ter sido
removida de fato, não apenas deixada de iniciar.

O executável `kernel_rootfsB/usr/bin/hi_xpon` e as páginas Web que mencionam
PON também permanecem. Não há chamada direta a esse executável no `rcS`; é
necessário mapear referências da CLI, da WebUI e de binários antes de removê-lo.

## Itens deliberadamente fora da remoção inicial

- `hi_gpon.ko`, `hi_epon.ko`, `hi_oam.ko`, `hi_l3.ko`, `hi_cnt.ko`,
  `hi_khal.ko` e CFE: manter. Apesar dos nomes PON, formam a base de símbolos
  exigida para a infraestrutura Ethernet desta imagem.
- `hi_bridge.ko`, `net_dev_created`, `hi_l3.ko` e a bridge `br0`: necessários
  ou potencialmente necessários para Ethernet.
- `hi_koptical.ko`: é ligado ao hardware óptico, mas não foi provado que seja
  seguro removê-lo sem afetar outra inicialização. Mantido desligado
  funcionalmente pela ausência da pilha PON.
- `l3mng` e `detectHwEvent`: possuem dependências GPON/OMCI, mas também podem
  participar de rede, LEDs e eventos de hardware. Exigem análise/substituição
  antes de qualquer remoção.
- `/fhcfg/fh_pon/`, OMCI e demais dados persistentes: preservados para evitar
  apagar configuração, identificação ou calibração sem entendimento completo.
