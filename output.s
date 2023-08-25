	.text
	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
.set @feat.00, 0
	.file	"Glass"
	.def	main;
	.scl	2;
	.type	32;
	.endef
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
main:                                   # @main
.Lfunc_begin0:
	.file	1 ".\\Examples" "HelloWorld.glass"
	.loc	1 35 0                          # HelloWorld.glass:35:0
.seh_proc main
# %bb.0:                                # %entry
	pushq	%rbp
	.seh_pushreg %rbp
	pushq	%rsi
	.seh_pushreg %rsi
	subq	$56, %rsp
	.seh_stackalloc 56
	leaq	48(%rsp), %rbp
	.seh_setframe %rbp, 48
	.seh_endprologue
	movq	TypeInfoArray(%rip), %rsi
	callq	__main
.Ltmp0:
	.loc	1 38 13 prologue_end            # HelloWorld.glass:38:13
	movq	%rsi, (%rbp)
	.loc	1 40 3                          # HelloWorld.glass:40:3
	movq	8(%rbp), %r8
	movq	16(%rbp), %r9
	movq	24(%rbp), %rax
	movq	%rax, 32(%rsp)
	leaq	.L__unnamed_1(%rip), %rcx
	movq	%rsi, %rdx
	callq	printf
	.loc	1 42 3                          # HelloWorld.glass:42:3
	xorl	%eax, %eax
	.loc	1 42 3 epilogue_begin is_stmt 0 # HelloWorld.glass:42:3
	addq	$56, %rsp
	popq	%rsi
	popq	%rbp
	retq
.Ltmp1:
.Lfunc_end0:
	.seh_endproc
                                        # -- End function
	.section	.rdata,"dr"
	.globl	TypeInfoArray                   # @TypeInfoArray
	.p2align	4, 0x0
TypeInfoArray:
	.zero	64

.L__unnamed_1:                          # @0
	.asciz	"i32_ti = %p, %p, %p, %p"

	.file	2 "<stdin>"
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
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	6                               # DW_FORM_data4
	.ascii	"\210\001"                      # DW_AT_alignment
	.byte	15                              # DW_FORM_udata
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	7                               # Abbreviation Code
	.byte	13                              # DW_TAG_member
	.byte	0                               # DW_CHILDREN_no
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	6                               # DW_FORM_data4
	.byte	56                              # DW_AT_data_member_location
	.byte	11                              # DW_FORM_data1
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	8                               # Abbreviation Code
	.byte	13                              # DW_TAG_member
	.byte	0                               # DW_CHILDREN_no
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	73                              # DW_AT_type
	.byte	19                              # DW_FORM_ref4
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	6                               # DW_FORM_data4
	.ascii	"\210\001"                      # DW_AT_alignment
	.byte	15                              # DW_FORM_udata
	.byte	56                              # DW_AT_data_member_location
	.byte	11                              # DW_FORM_data1
	.byte	0                               # EOM(1)
	.byte	0                               # EOM(2)
	.byte	9                               # Abbreviation Code
	.byte	13                              # DW_TAG_member
	.byte	0                               # DW_CHILDREN_no
	.byte	3                               # DW_AT_name
	.byte	14                              # DW_FORM_strp
	.byte	58                              # DW_AT_decl_file
	.byte	11                              # DW_FORM_data1
	.byte	59                              # DW_AT_decl_line
	.byte	6                               # DW_FORM_data4
	.ascii	"\210\001"                      # DW_AT_alignment
	.byte	15                              # DW_FORM_udata
	.byte	56                              # DW_AT_data_member_location
	.byte	11                              # DW_FORM_data1
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
	.byte	1                               # Abbrev [1] 0xb:0xac DW_TAG_compile_unit
	.secrel32	.Linfo_string0          # DW_AT_producer
	.short	2                               # DW_AT_language
	.secrel32	.Linfo_string1          # DW_AT_name
	.secrel32	.Lline_table_start0     # DW_AT_stmt_list
	.secrel32	.Linfo_string2          # DW_AT_comp_dir
                                        # DW_AT_GNU_pubnames
	.quad	.Lfunc_begin0                   # DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0       # DW_AT_high_pc
	.byte	2                               # Abbrev [2] 0x2a:0x25 DW_TAG_subprogram
	.quad	.Lfunc_begin0                   # DW_AT_low_pc
	.long	.Lfunc_end0-.Lfunc_begin0       # DW_AT_high_pc
	.byte	1                               # DW_AT_frame_base
	.byte	86
	.secrel32	.Linfo_string3          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	35                              # DW_AT_decl_line
                                        # DW_AT_prototyped
	.long	79                              # DW_AT_type
                                        # DW_AT_external
	.byte	3                               # Abbrev [3] 0x43:0xb DW_TAG_variable
	.secrel32	.Linfo_string5          # DW_AT_name
	.byte	1                               # DW_AT_decl_file
	.byte	37                              # DW_AT_decl_line
	.long	86                              # DW_AT_type
	.byte	0                               # End Of Children Mark
	.byte	4                               # Abbrev [4] 0x4f:0x7 DW_TAG_base_type
	.secrel32	.Linfo_string4          # DW_AT_name
	.byte	5                               # DW_AT_encoding
	.byte	4                               # DW_AT_byte_size
	.byte	5                               # Abbrev [5] 0x56:0x5 DW_TAG_pointer_type
	.long	91                              # DW_AT_type
	.byte	6                               # Abbrev [6] 0x5b:0x48 DW_TAG_structure_type
	.secrel32	.Linfo_string12         # DW_AT_name
	.byte	32                              # DW_AT_byte_size
	.byte	2                               # DW_AT_decl_file
	.long	3435973836                      # DW_AT_decl_line
	.byte	4                               # DW_AT_alignment
	.byte	7                               # Abbrev [7] 0x67:0xf DW_TAG_member
	.secrel32	.Linfo_string6          # DW_AT_name
	.long	163                             # DW_AT_type
	.byte	2                               # DW_AT_decl_file
	.long	3435973836                      # DW_AT_decl_line
	.byte	0                               # DW_AT_data_member_location
	.byte	8                               # Abbrev [8] 0x76:0x10 DW_TAG_member
	.secrel32	.Linfo_string8          # DW_AT_name
	.long	170                             # DW_AT_type
	.byte	2                               # DW_AT_decl_file
	.long	3435973836                      # DW_AT_decl_line
	.byte	8                               # DW_AT_alignment
	.byte	1                               # DW_AT_data_member_location
	.byte	8                               # Abbrev [8] 0x86:0x10 DW_TAG_member
	.secrel32	.Linfo_string10         # DW_AT_name
	.long	163                             # DW_AT_type
	.byte	2                               # DW_AT_decl_file
	.long	3435973836                      # DW_AT_decl_line
	.byte	16                              # DW_AT_alignment
	.byte	0                               # DW_AT_data_member_location
	.byte	9                               # Abbrev [9] 0x96:0xc DW_TAG_member
	.secrel32	.Linfo_string11         # DW_AT_name
	.byte	2                               # DW_AT_decl_file
	.long	3435973836                      # DW_AT_decl_line
	.byte	24                              # DW_AT_alignment
	.byte	0                               # DW_AT_data_member_location
	.byte	0                               # End Of Children Mark
	.byte	4                               # Abbrev [4] 0xa3:0x7 DW_TAG_base_type
	.secrel32	.Linfo_string7          # DW_AT_name
	.byte	7                               # DW_AT_encoding
	.byte	8                               # DW_AT_byte_size
	.byte	5                               # Abbrev [5] 0xaa:0x5 DW_TAG_pointer_type
	.long	175                             # DW_AT_type
	.byte	4                               # Abbrev [4] 0xaf:0x7 DW_TAG_base_type
	.secrel32	.Linfo_string9          # DW_AT_name
	.byte	7                               # DW_AT_encoding
	.byte	1                               # DW_AT_byte_size
	.byte	0                               # End Of Children Mark
.Ldebug_info_end0:
	.section	.debug_str,"dr"
.Linfo_string:
.Linfo_string0:
	.asciz	"Glass Compiler"                # string offset=0
.Linfo_string1:
	.asciz	"Main.glass"                    # string offset=15
.Linfo_string2:
	.asciz	"."                             # string offset=26
.Linfo_string3:
	.asciz	"main"                          # string offset=28
.Linfo_string4:
	.asciz	"i32"                           # string offset=33
.Linfo_string5:
	.asciz	"i32_ti"                        # string offset=37
.Linfo_string6:
	.asciz	"id"                            # string offset=44
.Linfo_string7:
	.asciz	"u64"                           # string offset=47
.Linfo_string8:
	.asciz	"name"                          # string offset=51
.Linfo_string9:
	.asciz	"u8"                            # string offset=56
.Linfo_string10:
	.asciz	"size"                          # string offset=59
.Linfo_string11:
	.asciz	"flags"                         # string offset=64
.Linfo_string12:
	.asciz	"TypeInfo"                      # string offset=70
	.section	.debug_pubnames,"dr"
	.long	.LpubNames_end0-.LpubNames_start0 # Length of Public Names Info
.LpubNames_start0:
	.short	2                               # DWARF Version
	.secrel32	.Lcu_begin0             # Offset of Compilation Unit Info
	.long	183                             # Compilation Unit Length
	.long	42                              # DIE offset
	.asciz	"main"                          # External Name
	.long	0                               # End Mark
.LpubNames_end0:
	.section	.debug_pubtypes,"dr"
	.long	.LpubTypes_end0-.LpubTypes_start0 # Length of Public Types Info
.LpubTypes_start0:
	.short	2                               # DWARF Version
	.secrel32	.Lcu_begin0             # Offset of Compilation Unit Info
	.long	183                             # Compilation Unit Length
	.long	163                             # DIE offset
	.asciz	"u64"                           # External Name
	.long	175                             # DIE offset
	.asciz	"u8"                            # External Name
	.long	79                              # DIE offset
	.asciz	"i32"                           # External Name
	.long	91                              # DIE offset
	.asciz	"TypeInfo"                      # External Name
	.long	0                               # End Mark
.LpubTypes_end0:
	.section	.debug_line,"dr"
.Lsection_line:
.Lline_table_start0:
