# Quarentena na ONU

O script `tools/move-remove-candidates.sh` é destinado à ONU. Ele move apenas
arquivos de `/fh/extend` para `/fh/extend/remove-candidates`, na mesma partição
JFFS2, e inicia em modo seco.

```sh
sh /tmp/move-remove-candidates.sh
sh /tmp/move-remove-candidates.sh --apply
```

Não move bibliotecas de `/lib` nesta etapa. Assim, `libhi_voip.so`,
`libhi_slic.so`, `libhi_omci_adapter.so` e `libcrypto.so.1.0.0` permanecem
intactas até uma rodada própria de teste do rootfs.
