#pragma once

#include "Base/Array.h"
#include "Base/String.h"
#include "Base/Types.h"
#include "BackEnd/Il.h"
#include "BackEnd/x64_Emit.h"

namespace Glass
{
	enum Data_Type_Kind
	{
		DT_KIND_INTEGER,
		DT_KIND_POINTER,
		DT_KIND_FLOAT,
		DT_KIND_ARRAY,
		DT_KIND_STRUCT,
		DT_KIND_VOID,
	};

	struct Data_Type
	{
		Data_Type_Kind kind;
		u32			   size;
		u32			   align;
	};

	enum IR_Inst_Type
	{
		IR_NONE,
		IR_LOAD,
		IR_STORE,
		IR_RET,
		IR_PARAM,
		IR_ALLOCA,
	};

	struct IR_Inst_Param
	{
		int index;
	};

	struct IR_Inst_Alloca
	{
		Data_Type type;
	};

	struct IR_Inst
	{
		IR_Inst_Type type;
		Data_Type dt;
		int op_count;
		IR_Inst* ops[];
	};

	struct IR_Block
	{
		Array<IR_Inst*> instructions;
	};

	struct IR_Proc
	{
		String			 name;
		bool			 external;
		Array<IR_Block*> blocks;
		IR_Block* insertion_point;
	};

	struct IR_Program
	{
		Array<IR_Proc*> procs;
	};

	struct MC_Symbol
	{
		String symbol_name;
		uint32_t value;
		uint16_t section_value;
	};

	struct MC_Relocation
	{
		uint32_t reloaction_offset;
		uint32_t symbol_idx;
		uint16_t relocation_type;
	};

	enum MC_Inst_Type
	{
		MC_PUSH,
		MC_POP,
		MC_SUB,
		MC_ADD,
	};

	struct MC_Inst
	{
		MC_Inst* next;
		int in_count;
		int out_count;
		int operands[];
	};

	struct Gen_Context
	{
		int stack_usage = 0;
		MC_Inst* first = nullptr;
		MC_Inst* head = nullptr;
	};

	struct Machine_Gen
	{
		String output_path;
		IR_Program p;

		std::unordered_map<IR_Proc*, int> proc_to_symbol;
		Array<MC_Symbol> symbols;

		u16 text_symbol_index = 0;
		u16 rdata_symbol_index = 2;
		u16 data_symbol_index = 4;

		u16 text_section_index = 1;
		u16 rdata_section_index = 2;
		u16 data_section_index = 3;

		Array<u8> coff_obj;
		Array<u8> code;
		Array<u8> rdata;
		Array<u8> data;
		Array<MC_Relocation> code_relocations;
		Array<MC_Relocation> code_relocations_sections;
	};

	void code_generator_init(Machine_Gen& g, Il_Program& p, String output);
	void code_generator_run(Machine_Gen& g);
	void generate_output(Machine_Gen& g);
}