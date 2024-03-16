#include "pch.h"

#include "BackEnd/Machine.h"
#include "BackEnd/COFF.h"

namespace Glass
{
	Il_Program testprogram;

	void code_generator_init(Machine_Gen& g, Il_Program& p, String output)
	{
		g.output_path = output;

		Il_Program_Init(testprogram);
		int test_proc = il_insert_proc(testprogram, String_Make("main"), get_proc_type(get_ts().void_Ty, {}));

		g.p = &testprogram;
	}

	void pre_decl_proc_symobls(Machine_Gen& g)
	{
		for (size_t i = 0; i < g.p->procedures.count; i++)
		{
			Il_Proc& proc = g.p->procedures[i];
			g.proc_to_symbol[i] = (u32)g.symbols.count;

			MC_Symbol symbol = { 0 };
			symbol.symbol_name = String_Copy(proc.proc_name);
			symbol.value = 0;

			if (proc.external) {
				symbol.section_value = 0;
			}
			else {
				symbol.section_value = g.text_section_index;
			}

			Array_Add(g.symbols, symbol);
		}
	}

	void code_generator_run(Machine_Gen& g)
	{
		pre_decl_proc_symobls(g);
		generate_output(g);
	}

	void generate_output(Machine_Gen& g)
	{

#define COFF_MACHINE (u16)IMAGE_FILE_MACHINE_AMD64

		g.coff_obj = Array_Reserved<u8>(1024 * 1024);

		Array<u8> str_table_data;

		COFF_File_Header header = { 0 };
		header.machine = COFF_MACHINE;
		header.time_date_stamp = (u32)time(nullptr);

		COFF_File_Header* header_ptr = (COFF_File_Header*)Write_Bytes(g.coff_obj, (u8*)&header, IMAGE_SIZEOF_FILE_HEADER);

		u32 num_section_symbols = 0;

		COFF_Section_Header* text_header_ptr = nullptr;

		{
			COFF_Section_Header section_header = { 0 };
			strcpy((char*)section_header.name, ".text");
			section_header.characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_ALIGN_16BYTES;
			text_header_ptr = (COFF_Section_Header*)Write_Bytes(g.coff_obj, (u8*)&section_header, IMAGE_SIZEOF_SECTION_HEADER);
			header_ptr->number_of_sections++;

			num_section_symbols++;
		}

		const int text_section_idx = header_ptr->number_of_sections;

		COFF_Section_Header* rdata_header_ptr = nullptr;

		{
			COFF_Section_Header section_header = { 0 };
			strcpy((char*)section_header.name, ".rdata");
			section_header.characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_ALIGN_16BYTES;

			rdata_header_ptr = (COFF_Section_Header*)Write_Bytes(g.coff_obj, (u8*)&section_header, IMAGE_SIZEOF_SECTION_HEADER);
			header_ptr->number_of_sections++;
			num_section_symbols++;
		}

		const int rdata_section_idx = header_ptr->number_of_sections;

		COFF_Section_Header* data_header_ptr = nullptr;

		{
			COFF_Section_Header section_header = { 0 };
			strcpy((char*)section_header.name, ".data");
			section_header.characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_16BYTES;

			data_header_ptr = (COFF_Section_Header*)Write_Bytes(g.coff_obj, (u8*)&section_header, IMAGE_SIZEOF_SECTION_HEADER);
			header_ptr->number_of_sections++;
			num_section_symbols++;
		}

		const int data_section_idx = header_ptr->number_of_sections;

		text_header_ptr->pointer_to_raw_data = (u32)g.coff_obj.count;

		for (size_t i = 0; i < g.code.count; i++)
		{
			Write_8(g.coff_obj, g.code[i]);
		}

		text_header_ptr->size_of_raw_data = ((u32)g.coff_obj.count - (u32)text_header_ptr->pointer_to_raw_data);
		text_header_ptr->pointer_to_relocs = (u32)g.coff_obj.count;

		u32 symbol_base_count = 2 * num_section_symbols;

		for (size_t i = 0; i < g.code_relocations.count; i++)
		{
			COFF_Relocation relocation;
			relocation.address = g.code_relocations[i].reloaction_offset;
			relocation.symbol_table_idx = g.code_relocations[i].symbol_idx + symbol_base_count;
			relocation.type = g.code_relocations[i].relocation_type;

			Write_Bytes(g.coff_obj, (u8*)&relocation, 10);
		}

		for (size_t i = 0; i < g.code_relocations_sections.count; i++)
		{
			COFF_Relocation relocation;
			relocation.address = g.code_relocations_sections[i].reloaction_offset;
			relocation.symbol_table_idx = g.code_relocations_sections[i].symbol_idx;
			relocation.type = g.code_relocations_sections[i].relocation_type;

			Write_Bytes(g.coff_obj, (u8*)&relocation, 10);
		}

		text_header_ptr->relocation_count = (u32)(g.code_relocations.count + g.code_relocations_sections.count);

		if (g.rdata.count) {

			rdata_header_ptr->pointer_to_raw_data = (u32)g.coff_obj.count;

			for (size_t i = 0; i < g.rdata.count; i++)
			{
				Write_8(g.coff_obj, g.rdata[i]);
			}

			rdata_header_ptr->size_of_raw_data = (u32)((u32)g.coff_obj.count - (u32)rdata_header_ptr->pointer_to_raw_data);
		}

		if (g.data.count) {

			data_header_ptr->pointer_to_raw_data = (u32)g.coff_obj.count;

			for (size_t i = 0; i < g.data.count; i++)
			{
				Write_8(g.coff_obj, g.data[i]);
			}

			data_header_ptr->size_of_raw_data = (u32)((u32)g.coff_obj.count - (u32)data_header_ptr->pointer_to_raw_data);
		}

		header_ptr->pointer_to_symbol_table = (u32)g.coff_obj.count;

		{

			COFF_Symbol symbol_text = { 0 };
			strcpy((char*)symbol_text.short_name, ".text");
			symbol_text.storage_class = IMAGE_SYM_CLASS_STATIC;
			symbol_text.type = IMAGE_SYM_TYPE_NULL;
			symbol_text.value = 0;
			symbol_text.aux_symbols_count = 1;
			symbol_text.section_number = text_section_idx;

			header_ptr->number_of_symbols++;
			Write_Bytes(g.coff_obj, (u8*)&symbol_text, IMAGE_SIZEOF_SYMBOL);

			COFF_AuxSectionSymbol sym_text_aux = { 0 };
			sym_text_aux.length = text_header_ptr->size_of_raw_data;
			sym_text_aux.checksum = 0xffffffff;
			sym_text_aux.number = text_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&sym_text_aux, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;
		}

		{
			COFF_Symbol symbol_rdata = { 0 };
			strcpy((char*)symbol_rdata.short_name, ".rdata");
			symbol_rdata.storage_class = IMAGE_SYM_CLASS_STATIC;
			symbol_rdata.type = IMAGE_SYM_TYPE_NULL;
			symbol_rdata.value = 0;
			symbol_rdata.aux_symbols_count = 1;
			symbol_rdata.section_number = rdata_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&symbol_rdata, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;

			COFF_AuxSectionSymbol sym_rdata_aux = { 0 };
			sym_rdata_aux.length = rdata_header_ptr->size_of_raw_data;
			sym_rdata_aux.checksum = 0xffffffff;
			sym_rdata_aux.number = rdata_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&sym_rdata_aux, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;
		}

		{
			COFF_Symbol symbol_data = { 0 };
			strcpy((char*)symbol_data.short_name, ".data");
			symbol_data.storage_class = IMAGE_SYM_CLASS_STATIC;
			symbol_data.type = IMAGE_SYM_TYPE_NULL;
			symbol_data.value = 0;
			symbol_data.aux_symbols_count = 1;
			symbol_data.section_number = data_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&symbol_data, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;

			COFF_AuxSectionSymbol sym_data_aux = { 0 };
			sym_data_aux.length = data_header_ptr->size_of_raw_data;
			sym_data_aux.checksum = 0xffffffff;
			sym_data_aux.number = data_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&sym_data_aux, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;
		}

		for (size_t i = 0; i < g.symbols.count; i++)
		{
			MC_Symbol mc_symbol = g.symbols[i];

			header_ptr->number_of_symbols++;

			COFF_Symbol symbol = { 0 };

			if (mc_symbol.symbol_name.count > 8) {
				symbol.long_name[0] = 0;
				symbol.long_name[1] = str_table_data.count + 4;

				for (size_t i = 0; i < mc_symbol.symbol_name.count; i++)
				{
					Array_Add(str_table_data, (u8)mc_symbol.symbol_name[i]);
				}

				Array_Add(str_table_data, (u8)0);
			}
			else {
				strcpy((char*)symbol.short_name, mc_symbol.symbol_name.data);
			}

			symbol.storage_class = IMAGE_SYM_CLASS_EXTERNAL;
			symbol.type = IMAGE_SYM_TYPE_NULL + (IMAGE_SYM_DTYPE_FUNCTION << 4);
			symbol.value = mc_symbol.value;
			symbol.aux_symbols_count = 0;
			symbol.section_number = mc_symbol.section_value;

			Write_Bytes(g.coff_obj, (u8*)&symbol, IMAGE_SIZEOF_SYMBOL);
		}

		u32* string_table_length = Write_32_Ptr(g.coff_obj);
		u8* string_table_begin = (u8*)string_table_length + 4;

		for (size_t i = 0; i < str_table_data.count; i++)
		{
			Array_Add(g.coff_obj, str_table_data[i]);
		}

		*string_table_length = (u32)((u64)(g.coff_obj.data + g.coff_obj.count) - (u64)string_table_begin) + 4;

		std::string output_path = std::string(g.output_path.data);

		fs_path obj_path = fs_path(output_path);
		fs_path obj_directory_path = fs_path(output_path);
		obj_directory_path.remove_filename();
		obj_path.replace_extension(".obj");

		std::filesystem::create_directories(obj_directory_path);

		{
			std::ofstream file(obj_path.string(), std::ios::binary);
			file.write((const char*)g.coff_obj.data, g.coff_obj.count);
			file.close();
		}
	}
}