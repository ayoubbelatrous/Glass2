#pragma once

#include "BackEnd/Il.h"
#include "BackEnd/COFF.h"

namespace Glass
{
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