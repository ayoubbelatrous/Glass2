#pragma once

#include "BackEnd/Il.h"
#include "BackEnd/COFF.h"

namespace Glass
{
	enum MC_Inst_Type : i8
	{
		Inst_IMM,
		Inst_MOV,
		Inst_LEA,
		Inst_ADD,
		Inst_SUB,
		Inst_OR,
		Inst_AND,
		Inst_DIV,
		Inst_IDIV,
		Inst_IMUL,
		Inst_CMP,

		Inst_MOVSXB,
		Inst_MOVSXW,

		Inst_MOVZXB,
		Inst_MOVZXW,

		Inst_MOVABS,

		Inst_CDQ,
		Inst_CQO,

		Inst_CALL,
		Inst_LABEL,

		Inst_JMP,
		Inst_JL,
		Inst_JG,
		Inst_JE,
		Inst_JNE,
		Inst_JMP_MAX,

		Inst_SETO,
		Inst_SETG,
		Inst_SETL,
		Inst_SETGE,
		Inst_SETLE,
		Inst_SETE,
		Inst_SETNE,
		Inst_SET_MAX,

		Inst_MOVF,
		Inst_ADDF,
		Inst_SUBF,
		Inst_MULF,
		Inst_DIVF,
		Inst_FExt,

		Inst_MovF2G,
	};

	enum MC_Inst_Flags : u8
	{
		Inst_Flag_Imm = BIT(0),
		Inst_Flag_Disp = BIT(1),
		Inst_Flag_Proc = BIT(2),
		Inst_Flag_R_Data = BIT(2),
	};

	struct MC_Inst
	{
		Il_IDX next;
		Il_IDX prev;
		int time;
		MC_Inst_Type	op_code;
		u8				flags;
		u8				bit_size;
		i32				disp;

		union
		{
			i32	imm;
			i64	abs;
		};

		bool			direction = 0;
		i8				in_count;
		Il_IDX			operands[10];
		i8				out_count;
		Il_IDX			output;
		i8				tmp_count;
		Il_IDX			tmps[16];
	};

	struct MC_Ctx
	{
		Array<MC_Inst> instructions;
		Il_IDX inst_head = 0xffff;
		Il_IDX inst_first = 0xffff;

		Il_Proc* proc;
		Type_System* ts;
		i32 stack_usage = 0;
		bool rsp_relative = false;
	};

	struct MC_Gen_Spec
	{
		String output_path;
	};

	struct MC_Relocation
	{
		uint32_t reloaction_offset;
		uint32_t symbol_idx;
		uint16_t relocation_type;
	};

	struct MC_Symbol
	{
		String symbol_name;
		uint32_t value;
		uint16_t section_value;
	};

	struct MC_JumpPatch
	{
		u32 offset;
		Il_IDX label_idx;
	};

	struct MC_Label
	{
		u32 offset;
	};

	struct MC_VReg
	{
		i32		stack_slot;
		u8		assigned_register;
		bool	fp;
		i8		reg = -1;
		Il_IDX	hint = -1;
		int		uses[128];
		int		use_count;
		int		range_start = -1;
		int		range_end = -1;
		Il_IDX	split_child = -1;
	};

	struct MC_Gen
	{
		String output_path;
		Il_Program* prog;
		Type_System* ts;

		u16 text_symbol_index = 0;
		u16 rdata_symbol_index = 2;
		u16 data_symbol_index = 4;

		u16 text_section_index = 1;
		u16 rdata_section_index = 2;
		u16 data_section_index = 3;

		Array<u8> code;
		Array<u8> rdata;
		Array<u8> data;
		Array<u8> coff_obj;
		Array<u32> proc_to_symbol;
		Array<u32> global_to_symbol;
		Array<u32> global_section_offsets;
		Array<MC_Symbol> symbols;
		Array<MC_Relocation> code_relocations;
		Array<MC_Relocation> code_relocations_sections;

		u32 program_counter;

		Array<MC_Inst> instruction_buffer;
		Array<MC_Label> label_buffer;
		Array<MC_JumpPatch> jump_patches_buffer;
		Array<MC_VReg> virtual_registers_buffer;
	};

	inline void MC_Gen_Output_Raw(MC_Gen& g) {

		fs_path raw_path = g.output_path.data;
		fs_path raw_directory_path = g.output_path.data;
		raw_directory_path.remove_filename();
		raw_path.replace_extension(".bin");

		std::filesystem::create_directories(raw_directory_path);

		{
			std::ofstream file(raw_path.string(), std::ios::binary);
			file.write((const char*)g.code.data, g.code.count);
		}
	}

	void MC_Gen_Output(MC_Gen& g);
	bool MC_Gen_Run(MC_Gen& g);
	MC_Gen MC_Gen_Make(MC_Gen_Spec spec, Il_Program* program);
}
