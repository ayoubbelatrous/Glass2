#include "pch.h"

#include "BackEnd/Dwarf.h"
#include "BackEnd/LLR_X86.h"

namespace Glass {

	Dwarf_Builder::Dwarf_Builder(Debug_Info* debug_info)
		: Di(*debug_info)
	{
	}

	struct Dwarf_Compile_Unit_Abbrev {

	};

	void Dwarf_Builder::Build()
	{
		Build_String_Offsets();

		u16 abbrev_counter = 0;
		abbrev_counter++;

		Info_Section.push_back({});						// Section Length - 4
		Info_Section.push_back(Build_Word(4));			// Dwarf Version 4
		Info_Section.push_back(Build_Abbrev_Begin());	//	Abbrev Offset Rel to Abbrev Section
		Info_Section.push_back(Build_Byte(8));			//	Pointer Size

		//	CU
		Abbrev_Section.push_back(1);
		Info_Section.push_back(Build_Byte(1));

		Abbrev_Section.push_back(DW_TAG_compile_unit);
		Abbrev_Section.push_back(DW_CHILDREN_yes);

		Abbrev_Section.push_back(DW_AT_producer);
		Abbrev_Section.push_back(DW_FORM_strp);
		Info_Section.push_back(Build_Str(Di.Compile_Unit.CompilerName_Str_Idx));

		Abbrev_Section.push_back(DW_AT_language);
		Abbrev_Section.push_back(DW_FORM_data2);
		Info_Section.push_back(Build_Word(DW_C11));

		Abbrev_Section.push_back(DW_AT_name);
		Abbrev_Section.push_back(DW_FORM_strp);
		Info_Section.push_back(Build_Str(Di.Compile_Unit.FileName_Str_Idx));

		Abbrev_Section.push_back(DW_AT_comp_dir);
		Abbrev_Section.push_back(DW_FORM_strp);
		Info_Section.push_back(Build_Str(Di.Compile_Unit.Directory_Str_Idx));

		Abbrev_Section.push_back(DW_AT_stmt_list);
		Abbrev_Section.push_back(DW_FORM_sec_offset);
		Info_Section.push_back(Build_Line_Table_Begin());

		Abbrev_Section.push_back(DW_AT_low_pc);
		Abbrev_Section.push_back(DW_FORM_addr);
		Info_Section.push_back(Build(Dwarf_Const_Code_Begin));

		Abbrev_Section.push_back(DW_AT_high_pc);
		Abbrev_Section.push_back(DW_FORM_data4);

		Info_Section.push_back(Build(Dwarf_Const_Code_Size));

		Abbrev_Section.push_back(0);
		Abbrev_Section.push_back(0);

		for (size_t i = 0; i < Di.Procedures.size(); i++)
		{
			abbrev_counter++;

			DIE_Procedure& proc = Di.Procedures[i];

			Abbrev_Section.push_back(abbrev_counter);
			Info_Section.push_back(Build_Byte(abbrev_counter));

			Abbrev_Section.push_back(DW_TAG_subprogram);
			Abbrev_Section.push_back(DW_CHILDREN_no);

			Abbrev_Section.push_back(DW_AT_low_pc);
			Abbrev_Section.push_back(DW_FORM_addr);
			Info_Section.push_back(Build_Func_Begin(proc.Function_Idx));

			Abbrev_Section.push_back(DW_AT_high_pc);
			Abbrev_Section.push_back(DW_FORM_data4);
			Info_Section.push_back(Build_Func_Size(proc.Function_Idx));

			Abbrev_Section.push_back(DW_AT_frame_base);
			Abbrev_Section.push_back(DW_FORM_exprloc);
			Info_Section.push_back(Build_Byte(1)); // 1 byte block
			Info_Section.push_back(Build_Byte(DW_OP_reg6)); // rbp

			Abbrev_Section.push_back(DW_AT_name);
			Abbrev_Section.push_back(DW_FORM_strp);
			Info_Section.push_back(Build_Str(proc.Name_Str_Idx));

			Abbrev_Section.push_back(DW_AT_decl_file);
			Abbrev_Section.push_back(DW_FORM_data1);
			Info_Section.push_back(Build_Byte((u8)(proc.File_Idx + 1)));

			Abbrev_Section.push_back(DW_AT_decl_line);
			Abbrev_Section.push_back(DW_FORM_data2);
			Info_Section.push_back(Build_DWord(proc.Definition_Src_Loc.Line));

			Abbrev_Section.push_back(DW_AT_external);
			Abbrev_Section.push_back(DW_FORM_flag_present);

			Abbrev_Section.push_back(0);
			Abbrev_Section.push_back(0);
		}

		Abbrev_Section.push_back(0);

		Info_Section.push_back(Build_Byte(0));

		Info_Section[0] = Build_DWord(Info_Section_End);
	}

	void Dwarf_Builder::Build_String_Offsets()
	{
		//@Strings

		u32 current_offset = 0;

		for (size_t i = 0; i < Di.String_Table.size(); i++)
		{
			String_Offsets_Table.push_back(current_offset);
			current_offset += Di.String_Table[i].size() + 1;
		}
	}

	Dwarf_Info_Data Dwarf_Builder::Build_Byte(u8 data)
	{
		Info_Section_End += 1;

		Dwarf_Info_Data d;
		d.type = Dwarf_Byte;
		d.byte = data;

		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_Word(u16 data)
	{
		Info_Section_End += 2;

		Dwarf_Info_Data d;
		d.type = Dwarf_Word;
		d.word = data;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_DWord(u32 data)
	{
		Info_Section_End += 4;

		Dwarf_Info_Data d;
		d.type = Dwarf_DWord;
		d.dword = data;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_QWord(u64 data)
	{
		Info_Section_End += 8;

		Dwarf_Info_Data d;
		d.type = Dwarf_QWord;
		d.qword = data;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build(Dwarf_Info_Data_Type type)
	{
		if (type == Dwarf_Const_Code_Begin) {
			Info_Section_End += 8;
		}
		else if (type == Dwarf_Const_Code_Size) {
			Info_Section_End += 4;
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}

		Dwarf_Info_Data d;
		d.type = type;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_Func_Begin(u16 func_idx)
	{
		Info_Section_End += 8;

		Dwarf_Info_Data d;
		d.type = Dwarf_Const_Func_Begin;
		d.func_idx = func_idx;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_Func_Size(u16 func_idx)
	{
		Info_Section_End += 4;

		Dwarf_Info_Data d;
		d.type = Dwarf_Const_Func_Size;
		d.func_idx = func_idx;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_Str(u16 str_idx)
	{
		Info_Section_End += 4;

		Dwarf_Info_Data d;
		d.type = Dwarf_Secrel32_Str;
		d.str_idx = str_idx;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_Abbrev_Begin()
	{
		Info_Section_End += 4;

		Dwarf_Info_Data d;
		d.type = Dwarf_Abbrev_Begin;
		return d;
	}

	Dwarf_Info_Data Dwarf_Builder::Build_Line_Table_Begin()
	{
		Info_Section_End += 4;

		Dwarf_Info_Data d;
		d.type = Dwarf_Line_Table_Begin;
		return d;
	}
}