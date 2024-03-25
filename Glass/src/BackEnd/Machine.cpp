#include "pch.h"

#include "BackEnd/Machine.h"
#include "BackEnd/COFF.h"
#include "Base/Allocator.h"

namespace Glass
{
	LinearAllocator ir_allocator = LinearAllocator(1024 * 1024 * 10);

	struct IlIR_Converter
	{
		std::unordered_map<int, IR_Proc*> proc_lookup;
		std::unordered_map<int, IR_Inst*> inst_lookup;
		std::unordered_map<int, IR_Block*> block_lookup;
	};

	IR_Proc* ir_make_proc(String name, bool external)
	{
		IR_Proc proc = {};
		proc.name = String_Copy(name);
		proc.external = external;

		return ir_allocator.Allocate(proc);
	}

	IR_Block* ir_insert_block(IR_Proc* proc)
	{
		auto new_block = ir_allocator.Allocate(IR_Block{ });
		Array_Add(proc->blocks, new_block);
		proc->insertion_point = new_block;
		return new_block;
	}

	template<typename T>
	T* get_ext(IR_Inst* inst)
	{
		return (T*)(&inst->ops);
	}

	template<typename T>
	IR_Inst* ir_insert_inst_ext(IR_Proc* proc, IR_Inst_Type inst_type, Data_Type dt, T extra)
	{
		IR_Inst* inst = (IR_Inst*)ir_allocator.Allocate_Bytes(sizeof(IR_Inst) + sizeof(T));
		inst->type = inst_type;
		inst->dt = dt;

		T* ext = get_ext<T>(inst);
		*ext = extra;

		Array_Add(proc->insertion_point->instructions, inst);

		return inst;
	}

	IR_Inst* ir_insert_inst(IR_Proc* proc, IR_Inst_Type inst_type, Data_Type dt, Array<IR_Inst*> ops)
	{
		IR_Inst* inst = (IR_Inst*)ir_allocator.Allocate_Bytes(sizeof(IR_Inst) + (ops.count * sizeof(IR_Inst*)));
		inst->type = inst_type;
		inst->dt = dt;
		inst->op_count = ops.count;

		memcpy(&inst->ops, ops.data, ops.count * sizeof(IR_Inst*));

		Array_Add(proc->insertion_point->instructions, inst);

		return inst;
	}

	Data_Type to_ir_type(GS_Type* type)
	{
		Data_Type dt = {};
		dt.size = type->size();
		dt.align = get_type_alignment(type);

		switch (type->kind)
		{
		case Type_Basic:
		{
			u64 flags = get_type_flags(type);

			if (flags & TN_Float_Type)
			{
				dt.kind = DT_KIND_FLOAT;
			}
			else if (flags & TN_Struct_Type)
			{
				dt.kind = DT_KIND_STRUCT;
			}
			else
			{
				dt.kind = DT_KIND_INTEGER;
			}
		}
		break;
		case Type_Pointer:
		{
			dt.kind = DT_KIND_POINTER;
		}
		break;
		default:
			ASSERT_UNIMPL();
			break;
		}

		if (type == get_ts().void_Ty)
		{
			dt.kind = DT_KIND_VOID;
		}

		return dt;
	}

	IR_Program il_to_ir_program(Il_Program& il_program)
	{
		IlIR_Converter ct;
		IR_Program program = {};

		for (int i = 0; i < il_program.procedures.count; i++)
		{
			Il_Proc& il_proc = il_program.procedures[i];
			IR_Proc* ir_proc = ir_make_proc(il_proc.proc_name, il_proc.external);

			Array_Add(program.procs, ir_proc);
			ct.proc_lookup[i] = ir_proc;
		}

		for (int i = 0; i < il_program.procedures.count; i++)
		{
			Il_Proc& il_proc = il_program.procedures[i];
			IR_Proc* ir_proc = ct.proc_lookup[i];

			ct.block_lookup.clear();
			ct.inst_lookup.clear();

			for (int block_id = 0; block_id < il_proc.blocks.count; block_id++)
			{
				Il_Block& block = il_proc.blocks[block_id];

				IR_Block* ir_block = ir_insert_block(ir_proc);
				ct.block_lookup[block_id] = ir_block;

				for (size_t i = 0; i < block.instructions.count; i++)
				{
					int id = block.instructions[i];
					Il_Node& node = il_proc.instruction_storage[id];

					GS_Type* type = get_type_at(node.type_idx);

					IR_Inst* ir_inst = nullptr;

					switch (node.node_type)
					{
					case Il_Store:
					{
						IR_Inst_Alloca aloca;
						aloca.type = to_ir_type(get_type_at(node.aloca.type_idx));
						ir_inst = ir_insert_inst(ir_proc, IR_STORE, to_ir_type(type), Make_Array({ ct.inst_lookup[node.store.value_node_idx],ct.inst_lookup[node.store.ptr_node_idx] }));
					}
					break;
					case Il_Alloca:
					{
						IR_Inst_Alloca aloca;
						aloca.type = to_ir_type(get_type_at(node.aloca.type_idx));
						ir_inst = ir_insert_inst_ext(ir_proc, IR_ALLOCA, to_ir_type(type), aloca);
					}
					break;
					case Il_Param:
					{
						IR_Inst_Param param;
						param.index = node.param.index;
						ir_inst = ir_insert_inst_ext(ir_proc, IR_PARAM, to_ir_type(type), param);
					}
					break;
					case Il_Ret:
					{
						if (get_ts().void_Ty == type)
						{
							ir_inst = ir_insert_inst(ir_proc, IR_RET, to_ir_type(type), {});
						}
						else
						{
							ASSERT_UNIMPL();
						}
					}
					break;
					default:
						ASSERT_UNIMPL();
						break;
					}

					ASSERT(ir_inst);
					ct.inst_lookup[id] = ir_inst;
				}
			}
		}

		return program;
	}

	void code_generator_init(Machine_Gen& g, Il_Program& p, String output)
	{
		g.output_path = output;
		g.p = il_to_ir_program(p);
	}

	void pre_decl_proc_symobls(Machine_Gen& g)
	{
		for (size_t i = 0; i < g.p.procs.count; i++)
		{
			IR_Proc* proc = g.p.procs[i];
			g.proc_to_symbol[proc] = (u32)g.symbols.count;

			MC_Symbol symbol = { 0 };
			symbol.symbol_name = String_Copy(proc->name);
			symbol.value = 0;

			if (proc->external) {
				symbol.section_value = 0;
			}
			else {
				symbol.section_value = g.text_section_index;
			}

			Array_Add(g.symbols, symbol);
		}
	}

	void proc_code_gen(Machine_Gen& g, Gen_Context& ctx, IR_Proc* proc)
	{
		for (int i = 0; i < proc->blocks.count; i++)
		{
			IR_Block* block = proc->blocks[i];

			for (int i = 0; i < block->instructions.count; i++)
			{
				IR_Inst* node = block->instructions[i];

				switch (node->type)
				{
				case IR_RET:
				{
					if (node->dt.kind == DT_KIND_VOID)
					{
					}
					else {
						ASSERT_UNIMPL();
					}
				}
				break;
				default:
					ASSERT_UNIMPL();
					break;
				}
			}
		}
	}

	void code_generator_run(Machine_Gen& g)
	{
		pre_decl_proc_symobls(g);

		for (size_t i = 0; i < g.p.procs.count; i++)
		{
			IR_Proc* proc = g.p.procs[i];

			if (!proc->external) {
				Gen_Context ctx = {};
				proc_code_gen(g, ctx, proc);
			}
		}

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