	.text
	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"Glass"
	.def	test;
	.scl	2;
	.type	32;
	.endef
	.globl	test                            # -- Begin function test
	.p2align	4, 0x90
test:                                   # @test
.seh_proc test
# %bb.0:                                # %entry
	subq	$56, %rsp
	.seh_stackalloc 56
	.seh_endprologue
	movq	%rdx, 8(%rsp)
	movq	%rcx, (%rsp)
	movq	$0, 16(%rsp)
	movq	%rcx, 24(%rsp)
	movq	%rdx, 32(%rsp)
	xorl	%eax, %eax
	addq	$56, %rsp
	retq
	.seh_endproc
                                        # -- End function
	.def	main;
	.scl	2;
	.type	32;
	.endef
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
main:                                   # @main
.seh_proc main
# %bb.0:                                # %entry
	pushq	%rbp
	.seh_pushreg %rbp
	subq	$128, %rsp
	.seh_stackalloc 128
	leaq	128(%rsp), %rbp
	.seh_setframe %rbp, 128
	.seh_endprologue
	callq	__main
	movl	$30, -8(%rbp)
	leaq	.L__unnamed_1(%rip), %rax
	movq	%rax, -48(%rbp)
	movq	$7, -56(%rbp)
	movq	%rax, -32(%rbp)
	movq	$7, -40(%rbp)
	leaq	.L__unnamed_2(%rip), %rax
	movq	%rax, -64(%rbp)
	movq	$7, -72(%rbp)
	movq	%rax, (%rbp)
	movq	$7, -8(%rbp)
	leaq	-40(%rbp), %rdx
	movq	%rdx, -80(%rbp)
	movq	$2, -88(%rbp)
	movl	$2, %ecx
	callq	test
	movq	-8(%rbp), %rdx
	leaq	.L__unnamed_3(%rip), %rcx
	callq	printf
	xorl	%eax, %eax
	addq	$128, %rsp
	popq	%rbp
	retq
	.seh_endproc
                                        # -- End function
	.section	.rdata,"dr"
.L__unnamed_4:                          # @0
	.asciz	"t = %s\n"

.L__unnamed_1:                          # @1
	.asciz	"entity"

.L__unnamed_2:                          # @2
	.asciz	"entity"

.L__unnamed_3:                          # @3
	.asciz	"Entity.Age = %i\n"
