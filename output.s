	.text
	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"output.ll"
	.def	print;
	.scl	2;
	.type	32;
	.endef
	.globl	print                           # -- Begin function print
	.p2align	4, 0x90
print:                                  # @print
.seh_proc print
# %bb.0:                                # %entry
	pushq	%rax
	.seh_stackalloc 8
	.seh_endprologue
	movq	%rcx, (%rsp)
	xorl	%eax, %eax
	popq	%rcx
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
	subq	$32, %rsp
	.seh_stackalloc 32
	leaq	32(%rsp), %rbp
	.seh_setframe %rbp, 32
	.seh_endprologue
	callq	__main
	leaq	.L__unnamed_1(%rip), %rcx
	callq	*.refptr.printf(%rip)
	xorl	%eax, %eax
	addq	$32, %rsp
	popq	%rbp
	retq
	.seh_endproc
                                        # -- End function
	.section	.rdata,"dr"
.L__unnamed_1:                          # @0
	.asciz	"Helllo Me"

	.section	.rdata$.refptr.printf,"dr",discard,.refptr.printf
	.p2align	3, 0x0
	.globl	.refptr.printf
.refptr.printf:
	.quad	printf
	.weak	printf
