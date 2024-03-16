#pragma once

#include "Base/Array.h"
#include "Base/String.h"
#include "Base/Types.h"
#include "BackEnd/Il.h"
#include "BackEnd/x64_Emit.h"

namespace Glass
{
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

	struct Machine_Gen
	{
		String output_path;
		Il_Program* p;

		std::unordered_map<int, int> proc_to_symbol;
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