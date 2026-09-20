# Árvore de dependências dos módulos — slot B

## Escopo e evidência

Construída a partir de `sd5116_hw/modules_loaded.txt`,
`extracted/kernel_rootfsB/etc/rc.d/rcS` e do campo `depends` dos `.ko` do
rootfs B. Refere-se somente à unidade SD5116 analisada.

**Convenção:** o módulo acima sustenta o ramo abaixo. A árvore é operacional;
não representa uma ordem segura de `rmmod`.

## Árvore observada em runtime

```text
hi_kbasic
├─ hi_kipc
├─ hi_klog_cmd
├─ hi_khrw
├─ hi_kmc_adapter
├─ hi_knet_adapter
├─ hi_kslic_common              [telefonia]
├─ hi_kvoip                     [telefonia]
├─ hi_khal
│  ├─ hi_cnt
│  ├─ hi_oam                    [GPON/EPON]
│  ├─ hi_l3                     [camada 3 FiberHome]
│  ├─ hi_gpon                   [GPON]
│  ├─ hi_epon                   [EPON]
│  ├─ hi_koptical               [óptico]
│  ├─ hi_diag_chip
│  └─ hi_bridge                 [fabric/switch de rede]
└─ cadeia CFE
   └─ hi_kcfe_fh_mark
      └─ hi_kcfe_res
         ├─ hi_kcfe_adapter_l3
         └─ hi_kcfe_adapter

hi_sysctl
├─ hi_mdio                      [PHY Ethernet]
├─ hi_pie                       [motor de encaminhamento]
│  └─ delivery
├─ hi_gpio
├─ hi_i2c
├─ hi_spi
├─ hi_timer
└─ hi_hw
```

## Cadeia a preservar para Ethernet e `br0`

**CONFIRMADO:** `net_dev_created` depende da infraestrutura FiberHome/CFE para
criar `eth*`, `wan`, `voip*` e permitir a montagem de `br0`.

```text
hi_kbasic
hi_kipc
hi_sysctl
hi_mdio
hi_pie
delivery
hi_bridge
hi_kmc_adapter
hi_knet_adapter
hi_khal
hi_kcfe_fh_mark
hi_kcfe_res
hi_kcfe_adapter_l3
hi_kcfe_adapter
```

`hi_bridge` é crítico: apesar do uso por componentes PON, ele integra a
infraestrutura de rede desta plataforma. Sua remoção é compatível com a perda
anterior de Ethernet.

## Módulos que podem deixar de carregar no Linux mínimo

**CONFIRMADO, se telefonia/FXS não for usada:**

```text
hi_kslic_common
hi_kslic_lt ou hi_kslic_zl
hi_kvoip
```

**CONFIRMADO, para cliente IPv4 estático sem NAT, firewall stateful, FTP/TFTP
atravessando NAT, IPv6 ou túneis:** `nf_conntrack_tftp`, `nf_conntrack_ftp`,
`nf_nat`, `nf_nat_ftp`, `nf_nat_tftp`, `xt_connmark`, `xt_state`, `xt_helper`,
`tunnel4`, `xfrm4_tunnel`, `ipip`, `ipv6`, `tunnel6`, `sit`, `ip6_tunnel` e
`nf_conntrack_ipv6`. Manter `nf_conntrack`: `hi_kcfe_res` declara dependência
direta dele.

**Não desativar neste estágio:** `hi_gpon`, `hi_epon`, `hi_oam`, `hi_l3`,
`hi_cnt`, `hi_khal` ou a cadeia CFE. O experimento 021 demonstrou que
`hi_gpon`/`hi_epon` exportam símbolos requisitados por `hi_cnt`, HAL e CFE;
sem eles `net_dev_created` não cria `eth*` e a Ethernet deixa de funcionar.
PON deve ser desativada no espaço de usuário (`hi_xpon_app`, `hi_kploam`,
OMCI/EPON OAM), mantendo essa base de drivers carregada.

`hi_khrw`, `hi_diag_chip`, `hi_koptical` e `hi_koperate` são apenas
**candidatos de teste isolado**, não remoções seguras: ainda podem afetar
diagnóstico, óptica, LEDs ou inicialização indireta.

Não usar `rmmod` em módulos com referências ativas. Alterar somente o
carregamento em um boot de teste e verificar `eth*`/`br0` antes de persistir.

## Ordem original relevante no `rcS`

```text
hi_kbasic → hi_kipc → hi_klog_cmd
hi_sysctl → hi_mdio / hi_pie / GPIO / I²C / SPI / timer / hi_hw
hi_bridge → hi_gpon / hi_epon / hi_l3 / hi_oam / hi_cnt
hi_khal → adaptadores → SLIC/VoIP
netfilter e IPv4/IPv6
adaptadores CFE
```

Essa ordem deve ser preservada enquanto `net_dev_created` continuar em uso.
