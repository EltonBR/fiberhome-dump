/*
 * Teste temporário para o kernel 2.6.34.10 da ONU SD5116.
 *
 * Não registra um dispositivo USB nem grava em flash. Ao carregar, chama a
 * rotina já exportada pelo kernel hiusb_start_hcd(); ao remover, chama
 * hiusb_stop_hcd(). O tamanho de __this_module (320 bytes) foi confirmado
 * em módulos nativos desta imagem.
 */
	.syntax unified
	.arch armv7-a
	.arm

	.section .text, "ax", %progbits
	.global init_module
	.type init_module, %function
init_module:
	push	{r4, lr}
	bl	hiusb_start_hcd
	mov	r0, #0
	pop	{r4, pc}

	.global cleanup_module
	.type cleanup_module, %function
cleanup_module:
	push	{r4, lr}
	bl	hiusb_stop_hcd
	pop	{r4, pc}

	.section .modinfo, "a", %progbits
	.asciz "license=GPL"
	.asciz "description=Teste temporario de inicializacao HIUSB SD5116"
	.asciz "vermagic=2.6.34.10 mod_unload ARMv7 "

	.section .gnu.linkonce.this_module, "aw", %progbits
	.global __this_module
	.type __this_module, %object
	.size __this_module, 320
__this_module:
	/* struct module do kernel 2.6.34.10: nome em +0x0c. */
	.space 12
	.asciz "hiusb_start"
	.org 0xbc
	.word init_module
	.org 0x130
	.word cleanup_module
	.org 320

	.section .note.GNU-stack, "", %progbits
