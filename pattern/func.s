	.file	"func.c"
	.comm	k,4,4
	.section	.rodata
.LC0:
	.string	"\n"
	.text
	.globl	try
	.type	try, @function
try:
.LFB0:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	k(%rip), %eax
	cmpl	$9, %eax
	jg	.L1
	movl	k(%rip), %eax
	movl	%eax, %edi
	movl	$0, %eax
	call	write
	movl	$.LC0, %edi
	movl	$0, %eax
	call	write
	movl	k(%rip), %eax
	addl	$1, %eax
	movl	%eax, k(%rip)
	movl	$0, %eax
	call	try
.L1:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	try, .-try
	.globl	main
	.type	main, @function
main:
.LFB1:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	$0, k(%rip)
	movl	$0, %eax
	call	try
	movl	$0, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 4.8.4-2ubuntu1~14.04) 4.8.4"
	.section	.note.GNU-stack,"",@progbits
