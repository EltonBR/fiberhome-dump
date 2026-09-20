# FiberHome AN5506-02-B — análise estática e runtime

Este é o relatório principal da unidade analisada. Ele reúne a inspeção dos
dumps extraídos e a coleta feita com a ONU em execução em `sd5116_hw/`. A data
`1970-01-01` presente na coleta runtime não é a data do firmware: o relógio da
ONU não estava acertado.

## Confirmado pelos dumps e logs desta unidade

- Plataforma em execução: HiSilicon SD5116; o registrador de identificação
  registra `0x51163000`.
- CPU exposta pelo Linux: ARMv7, implementador `0x41` (ARM), part `0xc09`
  (Cortex-A9), revisão 1; BogoMIPS estimado em 996,14. A coleta mostra uma CPU
  lógica (`CPUs=1`); isso descreve a configuração Linux em execução e não prova
  sozinho a quantidade de núcleos físicos do SoC.
- NAND: 128 MiB; bloco de apagamento de 128 KiB, página de 2 KiB e OOB de 64 bytes. O log de geometria menciona `ECC:4Bytes`, mas o driver de placa registra `NAND_ECC_NONE selected`; a configuração efetiva de ECC não deve ser inferida sem captura de OOB. As exportações MTD brutas deste repositório não contêm OOB.
- Kernel Linux `2.6.34.10`; a raiz em execução é JFFS2 em `mtd8`. Os módulos fornecidos anunciam `ARMv7`, enquanto o ELF do BusyBox declara ARM EABI5 com atributo de CPU `v4T`; não se deve transformar essas duas observações em uma afirmação mais forte sobre a ISA sem inspecionar a imagem do kernel.
- Seleção de boot: `kernel_rootfs_curr_flag=1`, `app_bin_curr_flag=1` e `app_ex_curr_flag=1`; o slot ativo é o B.
- A RAM passada ao Linux é 59 MiB. O kernel informa 59 MiB totais, 4.396 KiB reservados e 56.020 KiB disponíveis (`MemTotal`: 56.136 KiB). **Inferido:** o intervalo físico indicado por `PHYS_OFFSET=0x80500000` até `0x84000000` é compatível com 64 MiB físicos, dos quais 5 MiB iniciais foram excluídos pelo layout de boot; o dono dessa reserva é desconhecido.
- `HWCFG=0x24` aparece tanto na linha de comando quanto em `/proc` do driver
  FiberHome.
- U-Boot: `U-Boot 2010.03-svn445507` (2016-03-17). Os dumps de U-Boot A/B são idênticos byte a byte, assim como os environments A/B.
- `mtd5` a `mtd10` são JFFS2 bruto little-endian. `mtd11` contém uma região JFFS2 no offset `0x740000`; os 7,25 MiB anteriores ainda não foram classificados.
- O kernel tem suporte a armazenamento USB, mas a PCB não expõe conector, pads ou trilhas USB e o EHCI testado não enumerou portas. Isso não constitui uma porta USB utilizável.

O plano de transformação preservando o slot A está em [PLANO_LINUX_MINIMO.md](PLANO_LINUX_MINIMO.md); o fluxo de boot e acesso serial está em [FLUXO_BOOT_E_ACESSO.md](FLUXO_BOOT_E_ACESSO.md).

## Inferido, ainda sem identificação independente da versão

Os caminhos de compilação no filesystem citam `AN5506-02-B7G` e uma compilação brasileira. Isso é compatível com a associação documentada B7G → RP2608, porém a identificação RP2608 não foi encontrada diretamente neste conjunto de evidências.

## Tabela de partições MTD (confirmada)

| MTD | Nome | Offset | Tamanho | Formato/função | Segurança de escrita |
|---|---|---:|---:|---|---|
| 0 | startcode | 0x000000 | 128 KiB | estágio de boot ROM | nunca automaticamente |
| 1/2 | u-bootA/B | 0x020000/0x120000 | 1 MiB cada | bootloader | nunca automaticamente |
| 3/4 | envA/B | 0x220000/0x320000 | 1 MiB cada | environment U-Boot | não alterar |
| 5/8 | kernel_rootfsA/B | 0x420000/0x3c20000 | 18 MiB cada | JFFS2; B é a raiz | somente análise |
| 6/9 | app_binA/B | 0x1620000/0x4e20000 | 18 MiB cada | JFFS2 | somente análise |
| 7/10 | app_exA/B | 0x2820000/0x6020000 | 20 MiB cada | JFFS2 | somente análise |
| 11 | cfg | 0x7420000 | 12.160 KiB | dados opacos + JFFS2 em +0x740000 | nunca automaticamente |

Os offsets absolutos e nomes exatos provêm da linha de comando salva do kernel. Nenhum firmware agregado/container flashável foi identificado: estes são dumps lógicos brutos de partições MTD.

## Restrição do rebuild

`tools/rebuild.sh` produz somente uma imagem lógica de partição JFFS2, usando a geometria de página/bloco confirmada. Ele não pode reproduzir OOB da NAND, posicionamento de bad blocks, empacotamento de atualização do fornecedor, assinaturas ou checksums ainda desconhecidos. Deliberadamente não monta nem grava uma imagem NAND completa. Um método de recuperação, o formato do atualizador e o mecanismo de validação devem ser estabelecidos antes de criar qualquer artefato flashável.

## Mapa do rootfs e dos serviços (confirmado no slot B ativo)

- O filesystem raiz está em `extracted/kernel_rootfsB`; as aplicações estão principalmente em `extracted/app_exB`, montadas por `/bin/fhdrv_kdrv_mount` durante `rcS`.
- Os binários de espaço de usuário são ARM EABI5, little-endian, 32 bits e dinâmicos, usando `/lib/ld-uClibc.so.0` (uClibc 0.9.32.1). O `busybox` está stripped; vários binários e bibliotecas do fornecedor preservam informações de depuração.
- `/etc/rc.d/rcS` carrega módulos de hardware/serviço e chama `/fh/extend/initialize.sh`. Ele não inicia Telnet incondicionalmente: `telnetd -p 26` está comentado, e o comentário em chinês informa que o módulo CLI controla o serviço devido ao risco de deixá-lo habilitado por padrão.
- `app_exB/runWeb.sh` inicia `./webs -L 3 -M 1 -S 100 -m all`; a árvore web está em `app_exB/web`. `web/fiberhome/telnet_enable.asp` e a ação `/goform/setTelnetEnable` mapeiam, sem executar, o controle de depuração.
- `cfg` contém os dados persistentes de `/fhcfg`, incluindo `fh_pon/physicSN`, `logicSN`, `logicPWD`, `password` e `pon_type`. Eles permanecem disponíveis no filesystem extraído e as ferramentas não os alteram. Metadados de ownership/permissão do extrator devem ser tratados como evidência até comparação com uma montagem nativa: a extração sem privilégios não recriou device nodes.
- O armazenamento USB está habilitado no software (`usb-storage` registrado), mas não há interface física USB externa identificada nesta PCB. Não planejar funcionalidades que dependam de pendrive sem nova evidência elétrica.

## Estado observado durante a coleta runtime

### Console e serial

- O console ativo é `ttyAMA1`, a 115200 baud.
- O kernel enumera UARTs DesignWare/AMBA em `0x1010e000` (`ttyAMA0`, IRQ 77)
  e `0x1010f000` (`ttyAMA1`, IRQ 78).

### PON, Ethernet e telefonia

- A configuração persistente informa `pon_type=gpon`; o driver GPON foi
  carregado e o log registra `GPON init success`.
- O módulo EPON também estava carregado. Isto mostra suporte dual-mode do
  firmware, não uma sessão EPON ativa.
- Os módulos GPON, EPON, OAM, óptico, MDIO, I²C, SPI, GPIO, VoIP e SLIC foram
  carregados na coleta. Isso confirma suporte de software às interfaces, não
  sua configuração ou uso futuro.
- `PHY1` tinha link a 100 Mb/s (`phy1,status:f100`) no instante da captura.
- A ponte de gestão era `br0`, com `192.168.0.1/24` durante a coleta.
- As interfaces `tel0`, `voip0`, `voip1` e `voip2` existiam e estavam
  `UP/LOWER_UP`. A captura antiga mostra `hi_kvoip`, `hi_kslic_common` e
  `hi_kslic_lt` carregados. A inspeção física posterior identificou o SLIC
  Microchip/Microsemi LE9641, para o qual há `hi_kslic_zl.ko` no firmware. A
  diferença entre o módulo carregado e o CI físico não foi explicada; telefonia
  permanece fora do objetivo do Linux mínimo.

### USB, I²C e SPI

- `usbcore`, EHCI e `usb-storage` estão presentes no kernel. O root hub EHCI
  testado reportou zero portas. Sem D+/D−, VBUS ou conector rastreados na PCB,
  não há USB externo utilizável comprovado.
- Os módulos I²C e SPI estavam carregados. A coleta original não enumerou
  adaptadores I²C nem dispositivos filhos; portanto ela não demonstra por si
  só a detecção da EEPROM HE24C08. A investigação posterior do barramento e da
  HAL está em [I2C_SD5116.md](I2C_SD5116.md) e no
  [caderno de `kdrv_debug`](CADERNO_RE_KDRV_DEBUG_GPIO_I2C.md).

## Nota sobre a integridade da extração

A partição ativa `mtd8` (`kernel_rootfsB`) tem nós JFFS2 inválidos. A associação foi confirmada para o inode 1223 (`0x4c7`), correspondente a `usr/sbin/zsp_min.out`, binário DSP de 331.724 bytes: o nó de dados comprimido em `0x00f61b24` falha no CRC no log do kernel e outro nó desse mesmo inode, em torno de `0x00f701e8`, falha na descompressão durante a extração. O kernel também registrou uma falha de CRC em `0x00a70d80`; esse nó está corrompido a ponto de não ser associado com segurança a um caminho pelo parser atual. Nenhum reparo, normalização ou rebuild foi realizado.
