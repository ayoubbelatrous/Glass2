	.text
	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"output.ll"
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
	pushq	%rsi
	.seh_pushreg %rsi
	pushq	%rax
	.seh_stackalloc 8
	movq	%rsp, %rbp
	.seh_setframe %rbp, 0
	.seh_endprologue
	subq	$32, %rsp
	callq	__main
	addq	$32, %rsp
	movb	$10, 7(%rbp)
	movb	$-1, 6(%rbp)
	leaq	.L__unnamed_1(%rip), %rsi
	cmpb	$0, 7(%rbp)
	je	.LBB0_3
	.p2align	4, 0x90
.LBB0_2:                                # %loop.body
                                        # =>This Inner Loop Header: Depth=1
	movzbl	7(%rbp), %edx
	addb	6(%rbp), %dl
	movb	%dl, 7(%rbp)
	subq	$32, %rsp
	movq	%rsi, %rcx
	callq	printf
	addq	$32, %rsp
	cmpb	$0, 7(%rbp)
	jne	.LBB0_2
.LBB0_3:                                # %after.loop
	movl	$16, %eax
	callq	___chkstk_ms
	subq	%rax, %rsp
	movq	%rsp, %rax
	movb	$10, (%rax)
	xorl	%eax, %eax
	testb	%al, %al
	jne	.LBB0_5
# %bb.4:                                # %then
	subq	$32, %rsp
	leaq	.L__unnamed_2(%rip), %rcx
	callq	printf
	addq	$32, %rsp
.LBB0_5:                                # %cont
	xorl	%eax, %eax
	leaq	8(%rbp), %rsp
	popq	%rsi
	popq	%rbp
	retq
	.seh_endproc
                                        # -- End function
	.section	.rdata,"dr"
.L__unnamed_1:                          # @0
	.asciz	"Hello World, %i\n"

.L__unnamed_2:                          # @1
	.asciz	"Other Condition"

