/*
 * Teste temporário de EHCI para o kernel SD5116 em execução.
 *
 * Recursos extraídos da tabela estática do próprio uImage:
 *   MMIO 0x10a40000..0x10a4ffff, IRQ 71.
 * As funções probe/remove não são exportadas; os endereços abaixo foram
 * confirmados em /proc/kallsyms desta imagem específica antes da carga.
 */
	.syntax unified
	.arch armv7-a
	.arm

	.equ HIUSB_EHCI_PROBE,  0xc01dce18
	.equ HIUSB_EHCI_REMOVE, 0xc01dcdc4

	.section .text, "ax", %progbits
	.global init_module
	.type init_module, %function
init_module:
	push	{r4, lr}
	ldr	r0, =device_name
	mov	r1, #0
	ldr	r2, =resources
	mov	r3, #2
	bl	platform_device_register_simple
	mov	r4, r0
	/* IS_ERR(pdev): ponteiros de erro pertencem ao intervalo alto. */
	ldr	r1, =0xfffff001
	cmp	r0, r1
	bhs	init_exit
	/*
	 * struct platform_device começa com name/id e contém struct device em +8.
	 * Nesta ABI, device.dma_mask está em +0x58 e coherent_dma_mask em +0x60.
	 * platform_device_register_simple não os inicializa; o probe EHCI precisa
	 * de DMA de 32 bits para alocar estruturas coerentes.
	 */
	add	r2, r4, #8
	ldr	r1, =dma_mask
	str	r1, [r2, #0x58]
	mvn	r1, #0
	str	r1, [r2, #0x60]
	mov	r1, #0
	str	r1, [r2, #0x64]
	ldr	r3, =HIUSB_EHCI_PROBE
	mov	r0, r4
	blx	r3
	cmp	r0, #0
	bne	init_unregister
	ldr	r1, =pdev_ptr
	str	r4, [r1]
	mov	r0, #0
	b	init_exit
init_unregister:
	mov	r0, r4
	bl	platform_device_unregister
init_exit:
	pop	{r4, pc}

	.global cleanup_module
	.type cleanup_module, %function
cleanup_module:
	push	{r4, lr}
	ldr	r4, =pdev_ptr
	ldr	r0, [r4]
	cmp	r0, #0
	beq	cleanup_exit
	ldr	r3, =HIUSB_EHCI_REMOVE
	blx	r3
	ldr	r0, [r4]
	bl	platform_device_unregister
	mov	r0, #0
	str	r0, [r4]
cleanup_exit:
	pop	{r4, pc}

	.section .rodata, "a", %progbits
device_name:
	.asciz "hiusb-ehci"
	.align 2
resources:
	/* struct resource: start, end, name, flags, parent, sibling, child */
	.word 0x10a40000, 0x10a4ffff, 0, 0x00000200, 0, 0, 0
	.word 71, 71, 0, 0x00000400, 0, 0, 0

	.section .data, "aw", %progbits
	.align 2
pdev_ptr:
	.word 0
dma_mask:
	.word 0xffffffff, 0

	.section .modinfo, "a", %progbits
	.asciz "license=GPL"
	.asciz "description=Teste temporario EHCI HIUSB SD5116"
	.asciz "vermagic=2.6.34.10 mod_unload ARMv7 "

	.section .gnu.linkonce.this_module, "aw", %progbits
	.global __this_module
	.type __this_module, %object
	.size __this_module, 320
__this_module:
	.space 12
	.asciz "hiusb_ehci_test"
	.org 0xbc
	.word init_module
	.org 0x130
	.word cleanup_module
	.org 320

	.section .note.GNU-stack, "", %progbits
