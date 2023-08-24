	.text
	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"Glass"
	.def	print_n;
	.scl	2;
	.type	32;
	.endef
	.globl	print_n                         # -- Begin function print_n
	.p2align	4, 0x90
print_n:                                # @print_n
.Lfunc_begin0:
	.file	1 "./Examples" "HelloWorld.glass"
	.loc	1 2 0                           # HelloWorld.glass:2:0
.seh_proc print_n
# %bb.0:                                # %entry
	pushq	%rsi
	.seh_pushreg %rsi
	subq	$48, %rsp
	.seh_stackalloc 48
	.seh_endprologue
	movq	%rcx, 40(%rsp)
.Ltmp0:
	.loc	1 5 7 prologue_end              # HelloWorld.glass:5:7
	movl	$10, 36(%rsp)
	.loc	1 6 7                           # HelloWorld.glass:6:7
	movl	$0, 32(%rsp)
	leaq	.L__unnamed_1(%rip), %rsi
	.p2align	4, 0x90
.LBB0_1:                                # %loop.cond
                                        # =>This Inner Loop Header: Depth=1
	movl	32(%rsp), %eax
	.loc	1 8 9                           # HelloWorld.glass:8:9
	cmpl	36(%rsp), %eax
	jae	.LBB0_3
# %bb.2:                                # %loop.body
                                        #   in Loop: Header=BB0_1 Depth=1
	.loc	1 9 4                           # HelloWorld.glass:9:4
	movq	40(%rsp), %rdx
	movq	%rsi, %rcx
	callq	printf
	.loc	1 10 4                          # HelloWorld.glass:10:4
	incl	32(%rsp)
	jmp	.LBB0_1
.LBB0_3:                                # %after.loop
	.loc	1 13 3                          # HelloWorld.glass:13:3
	xorl	%eax, %eax
	.loc	1 13 3 epilogue_begin is_stmt 0 # HelloWorld.glass:13:3
	addq	$48, %rsp
	popq	%rsi
	retq
.Ltmp1:
.Lfunc_end0:
	.seh_endproc
                                        # -- End function
	.def	print_vars;
	.scl	2;
	.type	32;
	.endef
	.globl	print_vars                      # -- Begin function print_vars
	.p2align	4, 0x90
print_vars:                             # @print_vars
.Lfunc_begin1:
	.loc	1 15 0 is_stmt 1                # HelloWorld.glass:15:0
.seh_proc print_vars
# %bb.0:                                # %entry
	subq	$80, %rsp
	.seh_stackalloc 80
	.seh_endprologue
	movq	%rdx, 16(%rsp)
	movq	%rcx, 8(%rsp)
.Ltmp2:
	.loc	1 18 9 prologue_end             # HelloWorld.glass:18:9
	movq	%rdx, 24(%rsp)
	.loc	1 19 7                          # HelloWorld.glass:19:7
	movq	%rcx, 32(%rsp)
	.loc	1 21 8                          # HelloWorld.glass:21:8
	movq	%rdx, 40(%rsp)
	.loc	1 22 8                          # HelloWorld.glass:22:8
	leaq	16(%rdx), %rax
	movq	%rax, 48(%rsp)
	.loc	1 24 7                          # HelloWorld.glass:24:7
	movq	8(%rdx), %rax
	movq	%rax, 56(%rsp)
	.loc	1 25 7                          # HelloWorld.glass:25:7
	movq	24(%rdx), %rax
	movl	(%rax), %eax
	movl	%eax, 4(%rsp)
	.loc	1 27 7                          # HelloWorld.glass:27:7
	movq	(%rdx), %rax
	movq	%rax, 64(%rsp)
	.loc	1 28 7                          # HelloWorld.glass:28:7
	movq	16(%rdx), %rax
	movq	%rax, 72(%rsp)
	.loc	1 30 3                          # HelloWorld.glass:30:3
	xorl	%eax, %eax
	.loc	1 30 3 epilogue_begin is_stmt 0 # HelloWorld.glass:30:3
	addq	$80, %rsp
	retq
.Ltmp3:
.Lfunc_end1:
	.seh_endproc
                                        # -- End function
	.def	main;
	.scl	2;
	.type	32;
	.endef
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
main:                                   # @main
.Lfunc_begin2:
	.loc	1 32 0 is_stmt 1                # HelloWorld.glass:32:0
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
.Ltmp4:
	.loc	1 35 7 prologue_end             # HelloWorld.glass:35:7
	movl	$30, -4(%rbp)
	.loc	1 37 3                          # HelloWorld.glass:37:3
	leaq	.L__unnamed_2(%rip), %rax
	movq	%rax, -48(%rbp)
	movq	$7, -56(%rbp)
	movq	%rax, -32(%rbp)
	movq	$7, -40(%rbp)
	leaq	-4(%rbp), %rax
	movq	%rax, -64(%rbp)
	movq	$5, -72(%rbp)
	movq	%rax, -16(%rbp)
	movq	$5, -24(%rbp)
	leaq	-40(%rbp), %rdx
	movq	%rdx, -80(%rbp)
	movq	$2, -88(%rbp)
	movl	$2, %ecx
	callq	print_vars
	.loc	1 39 3                          # HelloWorld.glass:39:3
	leaq	.L__unnamed_3(%rip), %rcx
	callq	print_n
	.loc	1 41 3                          # HelloWorld.glass:41:3
	xorl	%eax, %eax
	.loc	1 41 3 epilogue_begin is_stmt 0 # HelloWorld.glass:41:3
	addq	$128, %rsp
	popq	%rbp
	retq
.Ltmp5:
.Lfunc_end2:
	.seh_endproc
                                        # -- End function
	.section	.rdata,"dr"
.L__unnamed_1:                          # @0
	.asciz	"Hi: %s\n"

.L__unnamed_2:                          # @1
	.asciz	"Hello"

.L__unnamed_3:                          # @2
	.asciz	"Hello World"

	.section	.debug_abbrev,"dr"
.Lsection_abbrev:
	.byte	1                               # Abbreviation Code
	.byte	17                              # DW_TAG_compile_unit
	.byte	1                               # DW_CHILDREN_yes
	.byte	37                              # DW_AT_producer
	.byte	14                              # DW_FORM_strp
	.byte	19                              # DW_AT_language
	.byte	5                               # DW_FORM_data2
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	16                              # DW_AT_stmt_list
	.byte	23                              # DW_FORM_sec_offset
	.byte	27                              # DW_AT_comp_dir
	.byte	14                              # DW_FORM_strp
	.ascii	"\264B"                         # DW_AT_GNU_pubnames
	.byte	25                              # DW_FORM_flag_present
	.byte	17                              # DW_AT_low_pc
	.byte	1                               # DW_FORM_addr
	.byte	18                              # DW_AT_high_pc
	.byte	6                               # DW_FORM_data4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	2                               # Abbreviation Code
	.byte	46                              # DW_TAG_subprogram
	.byte	1                               # DW_CHILDREN_yes
	.byte	17                              # DW_AT_low_pc
	.byte	1                               # DW_FORM_addr
	.byte	18                              # DW_AT_high_pc
	.byte	6                               # DW_FORM_data4
	.byte	64                              # DW_AT_frame_base
	.byte	24                              # DW_FORM_exprloc
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	11                              # DW_FORM_data1
	.byte	39                              # DW_AT_prototyped
	.byte	25                              # DW_FORM_flag_present
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	63                              # DW_AT_external
	.byte	25                              # DW_FORM_flag_present
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	3                               # Abbreviation Code
	.byte	52                              # DW_TAG_variable
	.byte	0                               # DW_CHILDREN_no
	.byte	2                               # DW_AT_location
	.byte	24                              # DW_FORM_exprloc
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	11                              # DW_FORM_data1
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	4                               # Abbreviation Code
	.byte	36                              # DW_TAG_base_type
	.byte	0                               # DW_CHILDREN_no
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	62                              # DW_AT_encoding
	.byte	11                              # DW_FORM_data1
	.byte	11                              # DW_AT_byte_size
	.byte	11                              # DW_FORM_data1
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	5                               # Abbreviation Code
	.byte	15                              # DW_TAG_pointer_type
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	6                               # Abbreviation Code
	.byte	19                              # DW_TAG_structure_type
	.byte	1                               # DW_CHILDREN_yes
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	11                              # DW_AT_byte_size
	.byte	11                              # DW_FORM_data1
	.ascii	"\210\001"                      # DW_AT_alignment
	.byte	15                              # DW_FORM_udata
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	7                               # Abbreviation Code
	.byte	15                              # DW_TAG_pointer_type
	.byte	0                               # DW_CHILDREN_no
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	56                              # DW_AT_data_member_location
	.byte	11                              # DW_FORM_data1
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	8                               # Abbreviation Code
	.byte	59                              # DW_TAG_unspecified_type
	.byte	0                               # DW_CHILDREN_no
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	0                               # EOM(3)
	.section	.debug_info,"dr"
.Lsection_info:
.Lcu_begin0:
	.long	.Ldebug_info_end0-.Ldebug_info_start0 # Length of Unit
.Ldebug_info_start0:
	.short	4                               # DWARF version number
	.secrel32	.Lsection_abbrev        # Offset Into Abbrev. Section
	.byte	8                               # Address Size (in bytes)
	.byte	1                               # Abbrev [1] 0xb:0x164 DW_TAG_compile_unit
	.secrel32	.Linfo_string0          # DW_AT_producer
	.short	2                               # DW_AT_language
	.secrel32	.Linfo_string1          # DW_AT_name
	.secrel32	.Lline_table_start0     # DW_AT_stmt_list
	.secrel32	.Linfo_string2          # DW_AT_comp_dir
                                        # DW_AT_GNU_pubnames
	.quad	.Lfunc_begin0                   # DW_AT_low_pc
	.long	.Lfunc_end2-.Lfunc_begin0       # DW_AT_high_pc
	.byte	2                               # Abbrev [2] 0x2a:0x44 DW_TAG_subprogram
	.quad	.Lfunc_begin0                   # DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
	.secrel32	.Linfo_string3          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
                                        # DW_AT_prototyped
	.long	304                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x43:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	40
	.secrel32	.Linfo_string7          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	2                               # DW_AT_decl_line
	.long	311                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x51:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	36
	.secrel32	.Linfo_string9          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	4                               # DW_AT_decl_line
	.long	323                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x5f:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	32
	.secrel32	.Linfo_string11         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	5                               # DW_AT_decl_line
	.long	323                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x6e:0x9a DW_TAG_subprogram
	.quad	.Lfunc_begin1                   # DW_AT_low_pc
	.long	.Lfunc_end1-.Lfunc_begin1       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	87
	.secrel32	.Linfo_string5          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	15                              # DW_AT_decl_line
                                        # DW_AT_prototyped
	.long	304                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x87:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	8
	.secrel32	.Linfo_string12         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	15                              # DW_AT_decl_line
	.long	330                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0x95:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	24
	.secrel32	.Linfo_string15         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	17                              # DW_AT_decl_line
	.long	349                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xa3:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	32
	.secrel32	.Linfo_string9          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	18                              # DW_AT_decl_line
	.long	359                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xb1:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	40
	.secrel32	.Linfo_string17         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	20                              # DW_AT_decl_line
	.long	354                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xbf:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	48
	.secrel32	.Linfo_string18         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	21                              # DW_AT_decl_line
	.long	354                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xcd:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	56
	.secrel32	.Linfo_string19         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	23                              # DW_AT_decl_line
	.long	311                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xdb:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	4
	.secrel32	.Linfo_string20         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	24                              # DW_AT_decl_line
	.long	304                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xe9:0xf DW_TAG_variable
	.byte	3                               # DW_AT_location
	.byte	145
	.asciz	"\300"
	.secrel32	.Linfo_string21         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	26                              # DW_AT_decl_line
	.long	359                             # DW_AT_type
	.byte	3                               # Abbrev [3] 0xf8:0xf DW_TAG_variable
	.byte	3                               # DW_AT_location
	.byte	145
	.asciz	"\310"
	.secrel32	.Linfo_string22         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	27                              # DW_AT_decl_line
	.long	359                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	2                               # Abbrev [2] 0x108:0x28 DW_TAG_subprogram
	.quad	.Lfunc_begin2                   # DW_AT_low_pc
	.long	.Lfunc_end2-.Lfunc_begin2       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.secrel32	.Linfo_string6          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	32                              # DW_AT_decl_line
                                        # DW_AT_prototyped
	.long	304                             # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x121:0xe DW_TAG_variable
	.byte	2                               # DW_AT_location
	.byte	145
	.byte	124
	.secrel32	.Linfo_string23         # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	34                              # DW_AT_decl_line
	.long	304                             # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	4                               # Abbrev [4] 0x130:0x7 DW_TAG_base_type
	.secrel32	.Linfo_string4          # DW_AT_name
	.byte	5                               # DW_AT_encoding
	.byte	4                               # DW_AT_byte_size
	.byte	5                               # Abbrev [5] 0x137:0x5 DW_TAG_pointer_type
	.long	316                             # DW_AT_type
	.byte	4                               # Abbrev [4] 0x13c:0x7 DW_TAG_base_type
	.secrel32	.Linfo_string8          # DW_AT_name
	.byte	7                               # DW_AT_encoding
	.byte	1                               # DW_AT_byte_size
	.byte	4                               # Abbrev [4] 0x143:0x7 DW_TAG_base_type
	.secrel32	.Linfo_string10         # DW_AT_name
	.byte	7                               # DW_AT_encoding
	.byte	4                               # DW_AT_byte_size
	.byte	6                               # Abbrev [6] 0x14a:0xe DW_TAG_structure_type
	.secrel32	.Linfo_string14         # DW_AT_name
	.byte	16                              # DW_AT_byte_size
	.byte	4                               # DW_AT_alignment
	.byte	7                               # Abbrev [7] 0x151:0x6 DW_TAG_pointer_type
	.long	344                             # DW_AT_type
	.byte	0                               # DW_AT_data_member_location
	.byte	0                               # End Of Children Mark
	.byte	8                               # Abbrev [8] 0x158:0x5 DW_TAG_unspecified_type
	.secrel32	.Linfo_string13         # DW_AT_name
	.byte	5                               # Abbrev [5] 0x15d:0x5 DW_TAG_pointer_type
	.long	354                             # DW_AT_type
	.byte	5                               # Abbrev [5] 0x162:0x5 DW_TAG_pointer_type
	.long	330                             # DW_AT_type
	.byte	4                               # Abbrev [4] 0x167:0x7 DW_TAG_base_type
	.secrel32	.Linfo_string16         # DW_AT_name
	.byte	7                               # DW_AT_encoding
	.byte	8                               # DW_AT_byte_size
	.byte	0                               # End Of Children Mark
.Ldebug_info_end0:
	.section	.debug_str,"dr"
.Linfo_string:
.Linfo_string0:
	.asciz	"Glass Compiler"                # string offset=0
.Linfo_string1:
	.asciz	"HelloWorld.glass"              # string offset=15
.Linfo_string2:
	.asciz	"./Examples/"                   # string offset=32
.Linfo_string3:
	.asciz	"print_n"                       # string offset=44
.Linfo_string4:
	.asciz	"i32"                           # string offset=52
.Linfo_string5:
	.asciz	"print_vars"                    # string offset=56
.Linfo_string6:
	.asciz	"main"                          # string offset=67
.Linfo_string7:
	.asciz	"string"                        # string offset=72
.Linfo_string8:
	.asciz	"u8"                            # string offset=79
.Linfo_string9:
	.asciz	"count"                         # string offset=82
.Linfo_string10:
	.asciz	"u32"                           # string offset=88
.Linfo_string11:
	.asciz	"i"                             # string offset=92
.Linfo_string12:
	.asciz	"args"                          # string offset=94
.Linfo_string13:
	.asciz	"void"                          # string offset=99
.Linfo_string14:
	.asciz	"Any"                           # string offset=104
.Linfo_string15:
	.asciz	"anies"                         # string offset=108
.Linfo_string16:
	.asciz	"u64"                           # string offset=114
.Linfo_string17:
	.asciz	"a"                             # string offset=118
.Linfo_string18:
	.asciz	"b"                             # string offset=120
.Linfo_string19:
	.asciz	"string_a"                      # string offset=122
.Linfo_string20:
	.asciz	"string_b"                      # string offset=131
.Linfo_string21:
	.asciz	"type_a"                        # string offset=140
.Linfo_string22:
	.asciz	"type_b"                        # string offset=147
.Linfo_string23:
	.asciz	"data"                          # string offset=154
	.section	.debug_pubnames,"dr"
	.long	.LpubNames_end0-.LpubNames_start0 # Length of Public Names Info
.LpubNames_start0:
	.short	2                               # DWARF Version
	.secrel32	.Lcu_begin0             # Offset of Compilation Unit Info
	.long	367                             # Compilation Unit Length
	.long	264                             # DIE offset
	.asciz	"main"                          # External Name
	.long	110                             # DIE offset
	.asciz	"print_vars"                    # External Name
	.long	42                              # DIE offset
	.asciz	"print_n"                       # External Name
	.long	0                               # End Mark
.LpubNames_end0:
	.section	.debug_pubtypes,"dr"
	.long	.LpubTypes_end0-.LpubTypes_start0 # Length of Public Types Info
.LpubTypes_start0:
	.short	2                               # DWARF Version
	.secrel32	.Lcu_begin0             # Offset of Compilation Unit Info
	.long	367                             # Compilation Unit Length
	.long	344                             # DIE offset
	.asciz	"void"                          # External Name
	.long	330                             # DIE offset
	.asciz	"Any"                           # External Name
	.long	323                             # DIE offset
	.asciz	"u32"                           # External Name
	.long	316                             # DIE offset
	.asciz	"u8"                            # External Name
	.long	304                             # DIE offset
	.asciz	"i32"                           # External Name
	.long	359                             # DIE offset
	.asciz	"u64"                           # External Name
	.long	0                               # End Mark
.LpubTypes_end0:
	.section	.debug_line,"dr"
.Lsection_line:
.Lline_table_start0:
