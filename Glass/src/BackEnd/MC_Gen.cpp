#include "pch.h"

#include "BackEnd/MC_Gen.h"

namespace Glass
{
	inline void MC_Gen_Proc_Codegen(MC_Gen& g, Il_Proc& proc)
	{
		Inst_Op register_values[2048] = {};

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX i = block.instructions[j];
				Il_Node node = proc.instruction_storage[i];

				switch (node.node_type)
				{
				case Il_Const:
				{
					Inst_Op op;
					op.type = Op_Imm32;
					op.imm32 = node.constant.as.s4;

					register_values[i] = op;
				}
				break;
				case Il_Ret:
				{
					if (node.ret.value_node_idx != -1) {
						Write_8(g.code, 0xb8);
						Write_32(g.code, register_values[node.ret.value_node_idx].imm32);
					}

					Emit_Ret(g);
				}
				break;
				default:
					ASSERT(nullptr);
					break;
				}
			}
		}
	}

	inline void MC_Gen_Program_Codegen(MC_Gen& g)
	{
		for (size_t i = 0; i < g.prog->procedures.count; i++)
		{
			Il_Proc& proc = g.prog->procedures[i];
			MC_Gen_Proc_Codegen(g, proc);
		}
	}

	bool MC_Gen_Run(MC_Gen& g)
	{
		MC_Gen_Program_Codegen(g);
		MC_Gen_Output(g);
		return true;
	}

	void MC_Gen_Output(MC_Gen& g)
	{
#define COFF_MACHINE (u16)IMAGE_FILE_MACHINE_AMD64

		g.coff_obj = Array_Reserved<u8>(1024 * 1024);

		COFF_File_Header header = { 0 };
		header.machine = COFF_MACHINE;
		header.time_date_stamp = time(NULL);

		COFF_File_Header* header_ptr = (COFF_File_Header*)Write_Bytes(g.coff_obj, (u8*)&header, IMAGE_SIZEOF_FILE_HEADER);

		COFF_Section_Header section_header = { 0 };
		strcpy((char*)section_header.name, ".text");
		section_header.characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;
		//section_header.characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA;

		COFF_Section_Header* section_header_ptr = (COFF_Section_Header*)Write_Bytes(g.coff_obj, (u8*)&section_header, IMAGE_SIZEOF_SECTION_HEADER);
		header_ptr->number_of_sections++;

		// 		Write_Nop(g);

		section_header_ptr->pointer_to_raw_data = g.coff_obj.count;

		for (size_t i = 0; i < g.code.count; i++)
		{
			Write_8(g.coff_obj, g.code[i]);
		}

		section_header_ptr->size_of_raw_data = (g.coff_obj.count - section_header_ptr->pointer_to_raw_data);

		header_ptr->pointer_to_symbol_table = g.coff_obj.count;

		header_ptr->number_of_symbols++;

		COFF_Symbol symbol_main = { 0 };
		strcpy((char*)symbol_main.short_name, "main");
		//symbol_main.long_name[0] = 0;
		symbol_main.storage_class = IMAGE_SYM_CLASS_EXTERNAL;
		symbol_main.type = IMAGE_SYM_TYPE_NULL + (IMAGE_SYM_DTYPE_FUNCTION << 4);
		symbol_main.value = 0;
		symbol_main.aux_symbols_count = 0;
		symbol_main.section_number = 1;

		COFF_Symbol* symbol_main_ptr = (COFF_Symbol*)Write_Bytes(g.coff_obj, (u8*)&symbol_main, IMAGE_SIZEOF_SYMBOL);
		//symbol_main_ptr->long_name[1] = 4;

		header_ptr->number_of_symbols++;

		COFF_Symbol symbol_text = { 0 };
		strcpy((char*)symbol_text.short_name, ".text");
		symbol_text.storage_class = IMAGE_SYM_CLASS_STATIC;
		symbol_text.type = IMAGE_SYM_TYPE_NULL;
		symbol_text.value = 0;
		symbol_text.aux_symbols_count = 1;
		symbol_text.section_number = 1;

		Write_Bytes(g.coff_obj, (u8*)&symbol_text, IMAGE_SIZEOF_SYMBOL);

		header_ptr->number_of_symbols++;

		COFF_AuxSectionSymbol sym_text_aux = { 0 };
		sym_text_aux.length = section_header_ptr->size_of_raw_data;
		sym_text_aux.checksum = 0xcd35b394;
		sym_text_aux.number = 1;

		Write_Bytes(g.coff_obj, (u8*)&sym_text_aux, IMAGE_SIZEOF_SYMBOL);

		u32* string_table_length = Write_32_Ptr(g.coff_obj);
		u8* string_table_begin = (u8*)string_table_length + 4;

		// 		const char* main_name = "Hello_World!\0";
		// 		Write_Bytes(g.coff_obj, (u8*)main_name, strlen(main_name));

		*string_table_length = (u32)((u64)(g.coff_obj.data + g.coff_obj.count) - (u64)string_table_begin) + 4;

		fs_path obj_path = g.output_path.data;
		fs_path obj_directory_path = g.output_path.data;
		obj_directory_path.remove_filename();
		obj_path.replace_extension(".obj");

		std::filesystem::create_directories(obj_directory_path);

		{
			std::ofstream file(obj_path.string(), std::ios::binary);
			file.write((const char*)g.coff_obj.data, g.coff_obj.count);
		}

		system("link.exe ./.bin/mc.obj /ENTRY:main /DEBUG:FULL /SUBSYSTEM:CONSOLE");
	}

	MC_Gen MC_Gen_Make(MC_Gen_Spec spec, Il_Program* program)
	{
		MC_Gen g = { 0 };
		g.output_path = String_Copy(spec.output_path);
		g.prog = program;
		g.ts = program->type_system;

		ASSERT(g.prog);
		ASSERT(g.ts);

		return g;
	}

	void Emit_Ret(MC_Gen& g)
	{
		Write_8(g.code, 0xc3);
	}
}