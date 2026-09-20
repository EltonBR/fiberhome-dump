# Mapa de consumidores de `extracted/cfg`

## Escopo e método

`extracted/cfg` é a extração da área persistente `mtd11`, montada no sistema
como `/fhcfg`. Este documento cruza cada arquivo presente na extração com
referências em scripts e strings de executáveis das árvores B:

- `kernel_rootfsB` (`mtd8`);
- `app_binB` (`mtd9`, montada em `/fh/bin`);
- `app_exB` (`mtd10`, montada em `/fh/extend`).

Classificação da evidência:

- **Direta**: o caminho `/fhcfg/...` foi encontrado literalmente no arquivo
  consumidor, ou há uma chamada explícita no script.
- **Inferida**: a biblioteca/serviço é comprovadamente responsável pelo
  subsistema, mas o nome é construído em runtime ou não ocorre como string
  completa.
- **Sem referência estática**: não foi localizada referência neste conjunto;
  pode ser log, lock, estado transitório, referência construída em runtime ou
  usada por componente ausente da extração.

Encontrar um nome em um ELF **não prova** que ele é aberto em toda execução.
Da mesma forma, arquivos em `/fhcfg` podem ser alterados pelos consumidores;
este levantamento não autoriza apagá-los.

## Fluxo de criação no boot

**Confirmado:** `rcS` monta `mtd11` em `/fhcfg`; depois `initialize.sh` chama,
nesta ordem, `mk_cfg_dir.sh` e `cp_cfg.sh`.

- `mk_cfg_dir.sh` garante os diretórios `fh_wifi/`, `fh_pon/`, `cpepatch/`,
  `l3_def/`, `ppp/` e `extend/dhcpforwan/`.
- `cp_cfg.sh` somente cria valores padrão ausentes para PPP, PPPoE, VoIP,
  `dhcpc.script` e `l3_def/udhcpd.conf`; ele sempre copia `omci.conf` quando
  existe em `/fh/extend`.
- `initialize.sh` lê `fh_pon/pon_type` e `voip.conf` diretamente e inicia os
  serviços correspondentes.

## PON, OMCI e identificação óptica

| Configuração persistente | Consumidores | Evidência | Observação |
| --- | --- | --- | --- |
| `fh_pon/pon_type` | `initialize.sh` | Direta | Seleciona o ramo EPON/GPON e a chamada de `load_omci`/`epon_oam`. Se ausente, o próprio script grava `gpon`. |
| `fh_pon/logicSN`, `fh_pon/logicPWD`, `fh_pon/physicSN`, `fh_pon/password`, `fh_pon/dba_mode` | `libgpon_l2.so`, `libgpon_l2_cli.so` | Direta | Parâmetros da pilha PON e da CLI GPON. |
| `fh_pon/omci_pid` | Não localizado | Sem referência estática | Arquivo binário de estado; o nome sugere PID/estado de OMCI, mas isso é hipótese. |
| `omci.conf` | `cp_cfg.sh`, `libnomci.so` | Direta | `cp_cfg.sh` copia a versão de `/fh/extend` para `/fhcfg`; `libnomci.so` contém o caminho persistente. |
| `omci_cfg.xml` | `detectHwEvent`, `libcm.so` | Direta | `detectHwEvent` também contém ação de remover esse XML. |
| `wan_service_omci.dat` | `l3mng`, `libgpon_l3.so`, `libgl3_cli.so`, `libgpon_rm.so`, `tr069/bin/libdev.so` | Direta | Estado/serviços WAN derivados de OMCI. `l3mng` pode removê-lo. |
| `pon_port_ratelimit` | `libgpon_l2.so` | Direta | Limitação de porta associada à pilha PON. |

## Camada 3, WAN, firewall e IPv6

| Configuração persistente | Consumidores | Evidência | Observação |
| --- | --- | --- | --- |
| `WanCtlCfg.ini` | `libl3ctl.so`, `tr069/bin/libdev.so` | Direta | Parâmetros WAN, incluindo modos DHCP, IP estático e PPPoE. |
| `wan_service_tr069.dat` | `l3mng`, `libl3ctl.so`, `libgl3_cli.so`, `libgl3_advance.so`, `libgpon_rm.so`, `libgpon_l3.so`, `tr069/bin/libdev.so` | Direta | Estado WAN relacionado ao TR-069; `l3mng` pode removê-lo. |
| `l3_advance.dat` | `libgl3_advance.so`, `libgpon_l3.so` | Direta | Configuração avançada L3. |
| `l3_alg_status_cfg` | `libgl3_advance.so`, `libl3ctl.so` | Direta | Estado/configuração de ALG L3. |
| `local_access_ctrl` | `libl3ctl.so`, `libgl3_advance.so`, `libgpon_l3.so` | Direta | Controle de acesso local; o conteúdo inclui opções de Telnet/Web e porta de serviço. |
| `ipv6.conf` | `libgpon_l3.so`, `libl3ctl.so`, `libipv6.so` | Direta | Configuração IPv6. |
| `MTUCfg.ini` | `libgpon_l3.so`, `libl3ctl.so` | Direta | MTU. |
| `RouteParaCfg.ini` | `libl3ctl.so` | Direta | Rotas persistentes. |
| `NatParaCfg.ini`, `PortForwardParaCfg.ini`, `PortTriggerParaCfg.ini` | `libl3ctl.so`, `libgpon_l3.so` | Direta | NAT, encaminhamento e port triggering. |
| `firewall.conf`, `firewall6.conf` | `webs`, `libl3ctl.so`, `libgpon_l3.so` | Inferida | Não há caminho completo literal, mas `webs` chama APIs de leitura/escrita de firewall e as bibliotecas L3 são os consumidores do subsistema. |
| `port_time_mng.cfg` | `libgl3_advance.so` | Direta | Agenda/controle temporal de portas. |
| `dhcp_dns_set_flag.conf` | `libl3ctl.so` | Direta | Flag de DNS obtido por DHCP. |
| `mas_cfg.xml`, `mas_cfg2.xml` | `libcm.so`, `detectHwEvent` | Direta | Configuração MAS; `detectHwEvent` é consumidor direto. |
| `mas_conflict.xml` | `libcm.so` | Direta | Estado/conflitos MAS. |

## DHCP: servidor, cliente e opções

| Configuração persistente | Consumidores | Evidência | Observação |
| --- | --- | --- | --- |
| `udhcpd.conf` | `udhcpd`, `libl3ctl.so` | Direta | Configuração ativa do servidor DHCP. Aponta leases para `extend/udhcpd.leases`. |
| `l3_def/udhcpd.conf` | `cp_cfg.sh` | Direta | Molde padrão copiado de `/fh/extend` caso esteja ausente. |
| `udhcpd_temp.conf` | `libl3ctl.so` | Direta | Configuração temporária do servidor DHCP. |
| `extend/udhcpd.leases` | `udhcpd.conf` | Direta | Base de leases; é escrita pelo `udhcpd`. |
| `DhcpServerParaCfg.ini` | `libgpon_l3.so`, `libl3ctl.so` | Direta | Parâmetros do servidor DHCP administrado pela pilha L3. |
| `DHCPCondServingCfg.ini`, `DHCPOptionParaCfg.ini`, `DHCPStaticAddressParaCfg.ini` | `libudhcpdctl.so` | Direta | Regras condicionais, opções e reservas estáticas do servidor DHCP. |
| `DHCPCOptionParaCfg0.ini`, `DHCPCOptionParaCfg1.ini`, `DHCPCOptionParaCfg2.ini`, `DHCPCOptionParaCfg3.ini`, `DHCPCOptionParaCfg4.ini` | `libudhcpcctl.so`, `libdhcpcoptionctl.so` | Inferida | Família indexada de opções DHCP cliente; a indexação é construída em runtime. |
| `SentDHCPOptionParaCfg0.ini`, `SentDHCPOptionParaCfg1.ini`, `SentDHCPOptionParaCfg2.ini`, `SentDHCPOptionParaCfg3.ini`, `SentDHCPOptionParaCfg4.ini` | `libdhcpcoptionctl.so` | Inferida; referência literal confirmada para índice 4 em `cfg` | Opções enviadas pelo cliente DHCP, separadas por WAN/índice. |
| `ReqDHCPOptionParaCfg0.ini`, `ReqDHCPOptionParaCfg1.ini`, `ReqDHCPOptionParaCfg2.ini`, `ReqDHCPOptionParaCfg3.ini`, `ReqDHCPOptionParaCfg4.ini` | `libdhcpcoptionctl.so` | Inferida; referência literal confirmada para índice 4 em `cfg` | Opções requisitadas pelo cliente DHCP, separadas por WAN/índice. |
| `dhcpc.script` | `cp_cfg.sh`, `libh248Call.so`, `libsip_oneip.so` | Direta | Script persistente de DHCP cliente; as bibliotecas de voz o referenciam. |
| `extend/dhcpforwan/` | `libdhcpctl.so`, `l3mng` | Direta | Diretório de estado por WAN: PID, IP, máscara, DNS e gateway de clientes DHCP. A extração está vazia. |

## PPP e PPPoE

| Configuração persistente | Consumidores | Evidência | Observação |
| --- | --- | --- | --- |
| `ppp/chap-secrets`, `ppp/chap-secrets-bak`, `ppp/pap-secrets`, `ppp/pap-secrets-bak`, `ppp/options`, `ppp/ip-up`, `ppp/ip-down` | `cp_cfg.sh`; `pppd` por convenção | Direta para a cópia; inferida para `pppd` | O script cria esses arquivos em `/fhcfg/ppp` se faltarem. `ip-up` e `ip-down` são scripts de hook PPP. |
| `pppoe.conf` | `cp_cfg.sh`, `libl3ctl.so`, `libpppoe.so`, `pppoe-status` | Direta | Perfil principal PPPoE. |
| `pppoe1.conf` | `cp_cfg.sh`, `pppoeconfig`, `pppoe1config`, `pppoe-status`, `libl3ctl.so`, `libpppoe.so` | Direta | Perfil PPPoE de índice 1. |
| `pppoe2.conf` | `cp_cfg.sh`, `pppoe2config`, `pppoe-status`, `libl3ctl.so`, `libpppoe.so` | Direta | Perfil PPPoE de índice 2. |
| `pppoe3.conf` | `cp_cfg.sh`, `pppoe3config`, `pppoe-status`, `libl3ctl.so`, `libpppoe.so` | Direta | Perfil PPPoE de índice 3. |
| `pppoe4.conf` | `cp_cfg.sh`, `pppoe4config`, `pppoe-status` | Direta | Perfil PPPoE de índice 4. |
| `pppoe5.conf` | `cp_cfg.sh`, `pppoe5config`, `pppoe-status` | Direta | Perfil PPPoE de índice 5. |
| `pppoe6.conf` | `cp_cfg.sh`, `pppoe6config`, `pppoe-status` | Direta | Perfil PPPoE de índice 6. |
| `pppoesim.conf` | `cp_cfg.sh`, `pppoesimconfig`, `pppoe-status`, `libpppoe.so` | Direta | Perfil PPPoE de simulação. |

`pppoeManage` não carrega esses caminhos literalmente, mas carrega
`libpppoe.so`, `libgpon_l3_api.so`, `libgpon_rm.so` e `libdhcpcoptionctl.so`;
portanto é um consumidor indireto do conjunto WAN/PPPoE.

## VoIP, FXS e telefonia

| Configuração persistente | Consumidores | Evidência | Observação |
| --- | --- | --- | --- |
| `voip.conf` | `initialize.sh`, `cp_cfg.sh`, `voip_preStart`, `libvoice_cli.so`, `libtr104.so`, `libsip_oneip.so`, `libh248Call.so` | Direta | `initialize.sh` usa `voipType` para escolher SIP, H.248 ou `voip_preStart`. |

Não foi encontrada na extração uma configuração persistente adicional de
contas SIP/H.248 com caminho literal. Ela pode ser entregue por OMCI/TR-069,
guardada em arquivos criados dinamicamente ou administrada por `libtr104.so`.

## Web, CLI, logs e controle de acesso

| Configuração persistente | Consumidores | Evidência | Observação |
| --- | --- | --- | --- |
| `ptys_name` | `fh_printf_redirect.sh`, `fh_printf_redirect` | Direta | Guarda nomes de pseudoterminais usados pelo redirecionador de saída. |
| `web_log/` | `runWeb.sh` | Direta | O script cria o diretório se ausente. |
| `web_log/web.log`, `web_log/web.log.bak`, `web_log/cu_log.txt` | `webs` / WebUI | Inferida | Logs da WebUI; não há referência a cada nome como string completa. |
| `webconfig` | Não localizado | Sem referência estática | Arquivo binário; provável estado/configuração Web, mas a função é desconhecida. |
| `web_portcfg` | `libnomci.so`, `libcm.so` | Direta | Configuração de portas exposta/consumida por OMCI e gerenciador de configuração. |
| `web_accout_enable` | `webs` | Inferida | `webs` contém a string `web_accout_enable` e rotinas de consulta; o caminho completo não ocorre literalmente. |
| `webServer.lock` | Não localizado | Sem referência estática | Provável lock de processo Web. |
| `umconfig.txt` | `libcm.so`, `libnomci.so`; CLI por contexto | Direta para bibliotecas | Banco de usuários/grupos em texto. O prompt serial normal usa a CLI FiberHome, não `/etc/passwd`. |
| `local_access_ctrl` | ver camada L3 | Direta | Também controla habilitação de Telnet/Web e porta de serviço. |

Há referências de CLI a `/fhcfg/confile.ini`, `/fhcfg/barcode` e
`/fhcfg/fhbsp_syslog/log`, mas esses caminhos não estão presentes nesta
extração. Isso é uma diferença importante entre o snapshot de `cfg` e todos
os arquivos que o firmware sabe usar.

## TR-069, manutenção e versão

| Configuração persistente | Consumidores | Evidência | Observação |
| --- | --- | --- | --- |
| `tr069/` e `tr069/fhcfg/` | `tr069/bin/runTr069`, `tr069/bin/agent` | Direta | `runTr069` garante esses diretórios e inicia `agent` com base `/fhcfg/`. |
| `tr069.lock` | `tr069/bin/agent` | Direta | Lock do agente. |
| `cpe.log` | `tr069/bin/agent` | Direta | Log persistente do agente. |
| `cpepatch/boot_version_control` | `initialize.sh`, `libgpon_rm.so`, `tr069/bin/agent` | Direta | `initialize.sh` pode copiar o controle de versão de `/fh/extend`. |
| `bootconfig` | `libcliom.so` | Direta | Metadados de versão/slot vistos pela CLI OMCI. |
| `burn_image_flag.conf` | `libcliom.so` | Direta | Flag relacionada a imagem/atualização. |
| `hw_version.ini` | `libcliom.so` | Direta | Identificação de hardware; nesta extração contém `WKE2.134.321B7G`. |
| `tr069.lock` | `tr069/bin/agent` | Direta | Lock de execução. |

## Arquivos de estado, diretórios vazios e itens sem consumidor localizado

| Configuração persistente | Consumidores | Estado da análise |
| --- | --- | --- |
| `fh_wifi/` | Não localizado | Diretório reservado; a unidade é `0x24` sem Wi-Fi confirmado. |
| `temp/` | Não localizado | Diretório transitório/reservado. |
| `extend/` | `udhcpd.conf`, `libdhcpctl.so` | Diretório pai de leases e DHCP WAN. |
| `webServer.lock` | Não localizado | Lock/estado provável. |
| `tr069.lock` | `tr069/bin/agent` | Consumido diretamente; listado também na seção TR-069. |
| `dhcp_dns_set_flag.conf` | `libl3ctl.so` | Consumido diretamente; listado também na seção L3. |
| `log_module_level` | `libl3log.so`, `libl3ctl.so`, `libgpon_rm.so`, `libpppoe.so` | Direta | Nível de log dos módulos. |
| `cpepatch/` (demais arquivos) | `tr069/bin/agent` e scripts de patch | Inferida | Na extração só há `boot_version_control` vazio. |

## Observações para o Linux mínimo

- Desativar serviços não torna automaticamente os arquivos de `/fhcfg`
  dispensáveis. Por exemplo, `l3mng`, `detectHwEvent` e bibliotecas comuns
  ainda usam arquivos PON e L3 mesmo quando OMCI não é iniciado.
- Para uma configuração Ethernet estática sem WebUI, TR-069, PPPoE ou PON, os
  conjuntos mais prováveis de se tornarem inativos são PON/OMCI, PPP/PPPoE,
  TR-069, WebUI e parte de WAN/L3. Isso é **inferência**, não autorização para
  apagar o conteúdo persistente.
- `bootconfig`, `burn_image_flag.conf`, `hw_version.ini`, identificação PON e
  quaisquer dados de `/fhcfg` devem ser preservados até que o processo de boot
  e de atualização esteja completamente substituído.
