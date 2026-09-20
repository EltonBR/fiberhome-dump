# Fluxo de boot, rede e acesso serial

## Sequência confirmada

1. O BusyBox `init` lê `/etc/inittab`.
2. A entrada `console::sysinit:-/etc/rc.d/rcS` executa `rcS` como root.
3. `rcS` monta `proc`, `sysfs`, `tmpfs` e `devpts`, executa `mdev -s` e chama `/bin/fhdrv_kdrv_mount`.
4. O helper monta, segundo os flags A/B do environment U-Boot:
   - `mtd11` em `/fhcfg`;
   - `mtd6` ou `mtd9` em `/fh/bin`;
   - `mtd7` ou `mtd10` em `/fh/extend`.
   Com os flags atuais iguais a 1, B é selecionado: `mtd9` e `mtd10`.
5. `rcS` carrega módulos base, GPON/EPON, óptica e VoIP; depois chama `/fh/extend/initialize.sh`.
6. No fim, se `initialize.sh` retornar, `rcS` executa `/bin/sh`.
7. Porém, em boot normal `initialize.sh` termina com `/fh/extend/load_cli`, sem `&`. Esse binário permanece em primeiro plano e é o que ocupa o console com a CLI FiberHome.
8. Além disso, `ttyAMA1::respawn:-/bin/sh` em `/etc/inittab` configura um shell serial root para quando esse processo for iniciado pelo init.

## Por que Ctrl+C entrega shell root sem rede

`initialize.sh` imprime `Press Ctrl + C to stop auto setup` e não instala um tratador de sinal. Se `Ctrl+C` interrompe esse script antes de `load_cli`, o `rcS` continua na linha final `/bin/sh`. O shell herda UID 0 do `init`; portanto não passa por `/bin/login` e não consulta `/etc/passwd`.

A rede Ethernet ainda não existe porque a criação ocorre depois, dentro de `initialize.sh`:

1. `hi_xpon_app gpon 2 4095`;
2. `/fh/extend/net_dev_created`;
3. `ifconfig br0 192.168.1.1`.

`net_dev_created` é o binário que cria `wan`, `eth0`–`eth3`, interfaces VoIP e `br0`, adiciona as Ethernet à bridge e atribui MACs. Ao interromper antes dele, não há bridge ou interface configurada para acessar pela rede.

## Autenticação

- O único registro local em `/etc/passwd` é `root`, com hash MD5-crypt; ele é usado por `/bin/login`, Telnet configurado para login, `su` ou `passwd`.
- Ele não controla o shell iniciado diretamente por `init`/`rcS`.
- O prompt `Login:`/`Password:` visto após o boot completo vem de `load_cli`, não de `/bin/sh`: `load_cli` usa `libcli_cli.so` e `libcli_adaptation_layer.so`, que têm uma base própria de usuários/senhas (`um_validate_login`, `um_check_user_db`, `um_set_login_pass`). Há referências binárias a `/fhcfg/confile.ini` e `/fh/bin/confile.ini`, mas esses arquivos não estavam presentes nas árvores extraídas nem foram localizados no runtime testado; a origem concreta da base de credenciais continua **desconhecida**.
- Portanto, `passwd` altera a conta Unix `root`, mas não altera a credencial da CLI FiberHome. A autenticação do WebUI também é implementada por `webs` e pelas funções ASP/configurações chamadas por ele, não apenas por `/etc/passwd`.
- Na captura de runtime, havia `busybox telnetd -p 2323 -l /bin/sh` como root. A opção `-l /bin/sh` executa shell diretamente, portanto também não usa o hash de `/etc/passwd`. A origem dessa ativação ainda não foi determinada.

## Arquivos persistentes que o boot pode alterar

- `mk_cfg_dir.sh` cria diretórios ausentes em `/fhcfg`.
- `cp_cfg.sh` copia padrões para `/fhcfg`; `omci.conf` é copiado incondicionalmente quando existe em `/fh/extend`, enquanto vários PPP/VoIP/DHCP são copiados somente se ausentes.
- `initialize.sh` remove arquivos de atualização pendente, pode limpar `/fhcfg/extend/dhcpforwan/*` e atualiza `/fhcfg/cpepatch/boot_version_control`.

Assim, `/fhcfg` não deve ser usado para um Linux mínimo: ele é persistente e contém estado da ONU. O patch inicial deve operar somente em B e fornecer a configuração Ethernet no próprio filesystem B.
