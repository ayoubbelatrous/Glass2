#pragma once

#include "Base/Types.h"

//Dwarf_Tags

const u16 DW_TAG_compile_unit = 0x11;
const u16 DW_TAG_subprogram = 0x2E;

// Dwarf_Attrib
const u16 DW_AT_producer = 0x25;
const u16 DW_AT_language = 0x13;
const u16 DW_AT_comp_dir = 0x1b;
const u16 DW_AT_name = 0x03;
const u16 DW_AT_stmt_list = 0x10;
const u16 DW_AT_low_pc = 0x11;
const u16 DW_AT_high_pc = 0x12;
const u16 DW_AT_frame_base = 0x40;
const u16 DW_AT_decl_file = 58;
const u16 DW_AT_decl_line = 59;
const u16 DW_AT_external = 63;

const u16 DW_OP_reg0 = 0x50;
const u16 DW_OP_reg1 = 0x51;
const u16 DW_OP_reg2 = 0x52;
const u16 DW_OP_reg3 = 0x53;
const u16 DW_OP_reg4 = 0x54;
const u16 DW_OP_reg5 = 0x55;
const u16 DW_OP_reg6 = 0x56; //	rbp
const u16 DW_OP_reg7 = 0x57; //	rsp

//Dwarf_FORM
const u16 DW_FORM_strp = 0xe;
const u16 DW_FORM_sec_offset = 0x17;
const u16 DW_FORM_data1 = 0xb;
const u16 DW_FORM_data2 = 0x5;
const u16 DW_FORM_data4 = 0x6;
const u16 DW_FORM_data8 = 0x7;
const u16 DW_FORM_data16 = 0x1e;
const u16 DW_FORM_addr = 0x1;
const u16 DW_FORM_exprloc = 0x18;
const u16 DW_FORM_flag_present = 25;

const u16 DW_CHILDREN_no = 0x00;
const u16 DW_CHILDREN_yes = 0x01;

const u16 DW_C11 = 29;

namespace Glass {

	struct Debug_Info;

	enum Dwarf_Info_Data_Type : u8 {
		Dwarf_Byte,
		Dwarf_Word,
		Dwarf_DWord,
		Dwarf_QWord,

		Dwarf_Secrel32_Str,

		Dwarf_Const_Code_Begin,
		Dwarf_Const_Code_Size,

		Dwarf_Const_Func_Begin,
		Dwarf_Const_Func_Size,

		Dwarf_Abbrev_Begin,
		Dwarf_Line_Table_Begin,
	};

	struct Dwarf_Info_Data {
		Dwarf_Info_Data_Type type;
		union
		{
			u8	byte;
			u16 word;
			u32 dword;
			u64 qword;

			u16 func_idx;
			u16 str_idx;
		};
	};

	struct Dwarf_Builder {
		Dwarf_Builder(Debug_Info* debug_info);
		Debug_Info& Di;

		void Build();
		void Build_String_Offsets();

		Dwarf_Info_Data Build_Byte(u8 data);
		Dwarf_Info_Data Build_Word(u16 data);
		Dwarf_Info_Data Build_DWord(u32 data);
		Dwarf_Info_Data Build_QWord(u64 data);

		Dwarf_Info_Data Build(Dwarf_Info_Data_Type type);
		Dwarf_Info_Data Build_Func_Begin(u16 func_idx);
		Dwarf_Info_Data Build_Func_Size(u16 func_idx);
		Dwarf_Info_Data Build_Str(u16 str_idx);
		Dwarf_Info_Data Build_Abbrev_Begin();
		Dwarf_Info_Data Build_Line_Table_Begin();

		u32 Info_Section_End = 0;

		std::vector<u8> Abbrev_Section;
		std::vector<Dwarf_Info_Data> Info_Section;

		std::vector<u32> String_Offsets_Table;
	};
}