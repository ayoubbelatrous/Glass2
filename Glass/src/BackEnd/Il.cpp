#include "pch.h"

#include "BackEnd/Il.h"

namespace Glass
{
	std::string Il_Print_Proc(Il_Proc& proc)
	{
		std::stringstream stream;

		stream << "proc " << proc.proc_name.data;

		stream << "(";
		for (Il_IDX i = 0; i < proc.parameters.count; i++)
		{
			if (i != 0)
				stream << ", ";
			stream << TypeSystem_Print_Type(*proc.program->type_system, proc.signature->proc.params[i]).data << " ";
			stream << "$" << proc.parameters[i];
		}

		stream << ")";

		stream << ": "
			<< TypeSystem_Print_Type(*proc.program->type_system, proc.signature->proc.return_type).data
			<< "\n";

		for (size_t i = 0; i < proc.blocks.count; i++)
		{
			Il_Block& block = proc.blocks[i];

			if (block.name.data)
			{
				stream << i;
				stream << block.name.data;
				stream << ":\n";
			}
			else {
				stream << i;
				stream << "unnamed:\n";
			}

			for (size_t j = 0; j < (Il_IDX)block.instructions.count; j++)
			{
				Il_IDX i = (Il_IDX)block.instructions[j];
				Il_Node node = proc.instruction_storage[i];

				auto type_flags = TypeSystem_Get_Type_Flags(*proc.program->type_system, &proc.program->type_system->type_storage[node.type_idx]);

				if (node.node_type != Il_Param) {
					stream << "$" << i << " = ";
				}

				switch (node.node_type)
				{
				case Il_Param:
					break;
				case Il_Alloca:
					stream << "alloca " <<
						TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data
						<< " \n";
					break;
				case Il_Store:
					stream << "store " <<
						TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data
						<< " ptr [" << node.store.ptr_node_idx << "] " << "$" << node.store.value_node_idx << "\n";
					break;

				case Il_Load:
					stream << "load " <<
						TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data
						<< " ptr [" << node.load.ptr_node_idx << "]" << "\n";
					break;

				case Il_Const:
					if (type_flags & TN_Float_Type) {
						stream << "constant " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " " << node.constant.as.f64 << "\n";
					}
					else {
						stream << "constant " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " " << node.constant.as.us8 << "\n";
					}
					break;
				case Il_Add:
					stream << "add " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.math_op.left_node_idx << " $" << node.math_op.right_node_idx << "\n";
					break;
				case Il_Sub:
					stream << "sub " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.math_op.left_node_idx << " $" << node.math_op.right_node_idx << "\n";
					break;
				case Il_Mul:
					stream << "mul " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.math_op.left_node_idx << " $" << node.math_op.right_node_idx << "\n";
					break;
				case Il_Div:
					stream << "div " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.math_op.left_node_idx << " $" << node.math_op.right_node_idx << "\n";
					break;
				case Il_Bit_And:
					stream << "and " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.math_op.left_node_idx << " $" << node.math_op.right_node_idx << "\n";
					break;
				case Il_Bit_Or:
					stream << "or " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.math_op.left_node_idx << " $" << node.math_op.right_node_idx << "\n";
					break;
				case Il_Value_Cmp:
				{
					stream << "cmp ";
					if (node.cmp_op.compare_type == Il_Cmp_Equal) stream << "equal ";
					else if (node.cmp_op.compare_type == Il_Cmp_NotEqual) stream << "not_equal ";
					else if (node.cmp_op.compare_type == Il_Cmp_GreaterEqual) stream << "greater_equal ";
					else if (node.cmp_op.compare_type == Il_Cmp_LesserEqual) stream << "lesser_equal ";
					else if (node.cmp_op.compare_type == Il_Cmp_Greater) stream << "greater ";
					else if (node.cmp_op.compare_type == Il_Cmp_Lesser) stream << "lesser ";
					else if (node.cmp_op.compare_type == Il_Cmp_And) stream << "and ";
					else if (node.cmp_op.compare_type == Il_Cmp_Or) stream << "or ";
					else GS_ASSERT_UNIMPL();
					stream << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.cmp_op.left_node_idx << " $" << node.cmp_op.right_node_idx << "\n";
				}
				break;
				case Il_Ret:
				{
					stream << "ret " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data;

					if (node.ret.value_node_idx != -1) {
						stream << " $" << node.ret.value_node_idx;
					}

					stream << "\n";
				}
				break;
				case Il_Call:
				case Il_Call_Ptr:
				{
					if (node.node_type == Il_Call) {
						stream << "call " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " @" << node.call.proc_idx << "(";
					}
					else {
						stream << "call_ptr " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " ptr [" << node.call.proc_idx << "] (";
					}

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						stream << " $" << node.call.arguments[i];
					}

					stream << ")\n";
				}
				break;
				case Il_ZI:
				{
					stream << "zero " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << "\n";
				}
				break;
				case Il_String:
				{
					stream << "string " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " \"" << node.string.str.data << "\"\n";
				}
				break;
				case Il_Cond_Branch:
				{
					std::string true_case_block;
					std::string false_case_block;

					if (proc.blocks[node.c_branch.true_case_block_idx].name.data) {
						true_case_block = proc.blocks[node.c_branch.true_case_block_idx].name.data;
					}
					else {
						true_case_block = "unnamed";
					}

					if (proc.blocks[node.c_branch.false_case_block_idx].name.data) {
						false_case_block = proc.blocks[node.c_branch.false_case_block_idx].name.data;
					}
					else {
						false_case_block = "unnamed";
					}

					stream << "cbr " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " $" << node.c_branch.condition_node_idx << " @" << node.c_branch.true_case_block_idx << true_case_block << " @" << node.c_branch.false_case_block_idx << false_case_block << "\n";
				}
				break;
				case Il_Branch:
				{
					std::string block;

					if (proc.blocks[node.br.block_idx].name.data) {
						block = proc.blocks[node.br.block_idx].name.data;
					}
					else {
						block = "unnamed";
					}

					stream << "br @" << node.br.block_idx << block << "\n";
				}
				break;
				case Il_Cast:
				{
					stream << "cast " <<
						TypeSystem_Print_Type_Index(*proc.program->type_system, node.cast.from_type_idx).data
						<< " -> " <<
						TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data
						<< " $" << node.cast.castee_node_idx << "\n";
				}
				break;
				case Il_Struct_Initializer:
				{
					stream << "SI " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " {";

					ASSERT(node.si.member_count < SI_SMALL_COUNT);

					for (size_t i = 0; i < node.si.member_count; i++)
					{
						stream << " $" << node.si.members_value_nodes[i];
					}

					stream << "}\n";
				}
				break;
				case Il_StructElementPtr:
				{
					stream << "sep " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << "." << node.element_ptr.element_idx << " $" << node.element_ptr.ptr_node_idx << "\n";
				}
				break;
				case Il_ArrayElementPtr:
				{
					stream << "aep " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << "[$" << node.aep.index_node_idx << "] $" << node.aep.ptr_node_idx << "\n";
				}
				break;
				case Il_Global_Address:
				{
					stream << "global_addr " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " @" << node.global_address.global_idx << "\n";
				}
				break;
				case Il_Proc_Address:
				{
					stream << "proc_addr " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " @" << node.proc_address.proc_idx << "\n";
				}
				break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}
		}

		return stream.str();
	}

	Const_Union EE_Constant_Evaluate(Execution_Engine& ee, Array<Il_Node>& nodes, Il_IDX node_idx)
	{
		Il_Node node = nodes[node_idx];

		GS_Type* type = &ee.type_system->type_storage[node.type_idx];
		auto type_size = TypeSystem_Get_Type_Size(*ee.type_system, type);

		switch (node.node_type)
		{
		case Il_Const:
		{
			return node.constant.as;
		}
		default:
			GS_ASSERT_UNIMPL();
			break;
		}
	}

	Const_Union EE_Exec_Program(Execution_Engine& ee, Il_Program* prog, Il_IDX entry)
	{
		u64 globals_storage_size = 0;

		for (size_t i = 0; i < prog->globals.count; i++)
		{
			globals_storage_size += TypeSystem_Get_Type_Size(*prog->type_system, prog->globals[i].type);
		}

		if (globals_storage_size != 0)
			ee.globals_storage = malloc(globals_storage_size);

		ee.globals_address_table = Array_Reserved<void*>(prog->globals.count);

		memset(ee.globals_storage, 0xcc, globals_storage_size);

		u8* global_storage_pointer = (u8*)ee.globals_storage;

		for (size_t i = 0; i < prog->globals.count; i++)
		{
			Il_Global& global = prog->globals[i];

			auto type_size = TypeSystem_Get_Type_Size(*prog->type_system, global.type);
			Array_Add(ee.globals_address_table, (void*)global_storage_pointer);

			if (global.initializer != (Il_IDX)-1)
			{
				auto const_eval_res = EE_Constant_Evaluate(ee, global.initializer_storage, global.initializer);

				void* init_data_ptr = &const_eval_res;

				if (type_size > sizeof(Const_Union)) {
					init_data_ptr = const_eval_res.ptr;
				}

				memcpy(global_storage_pointer, init_data_ptr, type_size);
			}
			else
			{
				memset(global_storage_pointer, 0, type_size);
			}

			global_storage_pointer += type_size;
		}

		for (size_t i = 0; i < prog->libraries.count; i++)
		{
			HMODULE loaded_dll = LoadLibraryA(ee.program->libraries[i].path.data);
			ASSERT(loaded_dll);

			Array_Add(ee.library_handles, (void*)loaded_dll);

			void* dll_main_proc_pointer = GetProcAddress(loaded_dll, "glfw3");
			int x = 30;
		}

		for (size_t i = 0; i < prog->procedures.count; i++)
		{
			Il_Proc& proc = prog->procedures[i];

			if (proc.external) {

				HMODULE loaded_dll = (HMODULE)ee.library_handles[proc.library];

				void* external_proc_pointer = GetProcAddress(loaded_dll, proc.proc_name.data);
				ASSERT(external_proc_pointer);

				Array_Add(ee.proc_pointers, external_proc_pointer);
			}
			else {
				Array_Add(ee.proc_pointers, (void*)nullptr);
			}
		}

		return EE_Exec_Proc(ee, prog->procedures[entry], {});
	}

	Const_Union EE_Exec_Proc(Execution_Engine& ee, Il_Proc& proc, Array<Const_Union> arguments, Array<GS_Type*> argument_types)
	{
		ASSERT(ee.type_system);

		if (proc.external) {
			return EE_Exec_External_Proc(ee, proc, arguments, argument_types);
		}

		Const_Union register_buffer[REG_BUF_SZ] = { 0 };
		GS_Type* register_type_buffer[REG_BUF_SZ];
		u64 stack[STACK_SZ] = { 0 };
		u64 stack_pointer = 0;

		Const_Union returned_value = { 0 };

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
		branch:
			Il_Block* block = &proc.blocks[block_idx];

			for (size_t j = 0; j < block->instructions.count; j++)
			{
				Il_IDX i = (Il_IDX)block->instructions[j];

				Il_Node node = proc.instruction_storage[block->instructions[j]];

				GS_Type* type = nullptr;

				if (node.type_idx != -1) {
					type = &ee.type_system->type_storage[node.type_idx];
				}

				Type_Name_Flags type_flags;
				u64 type_size;

				if (type) {
					type_flags = TypeSystem_Get_Type_Flags(*ee.type_system, type);
					type_size = TypeSystem_Get_Type_Size(*ee.type_system, type);
				}

				register_type_buffer[i] = type;

				switch (node.node_type)
				{
				case Il_ZI:
					break;
				case Il_Param: {
					register_buffer[i] = arguments[node.param.index];
				}
							 break;
				case Il_Global_Address: {
					register_buffer[i].ptr = ee.globals_address_table[node.global_address.global_idx];
				}
									  break;
				case Il_Proc_Address: {
					register_buffer[i].ptr = ee.proc_pointers[node.proc_address.proc_idx];
				}
									break;
				case Il_Alloca: {
					register_buffer[i].ptr = &stack[stack_pointer];
					stack_pointer += std::max(type_size, sizeof(Const_Union));
					register_type_buffer[i] = TypeSystem_Get_Pointer_Type(*proc.program->type_system, register_type_buffer[i], 1);
					if (stack_pointer > STACK_SZ * sizeof(u64)) {
						ASSERT(nullptr, "Stack Overflow!");
					}
				}
							  break;
				case Il_Store: {
					if (proc.instruction_storage[node.store.value_node_idx].node_type == Il_ZI) {
						memset(register_buffer[node.store.ptr_node_idx].ptr, 0, type_size);
					}
					else {
						if (type_size > sizeof(Const_Union)) {
							memcpy(register_buffer[node.store.ptr_node_idx].ptr, register_buffer[node.store.value_node_idx].ptr, type_size);
						}
						else {
							memcpy(register_buffer[node.store.ptr_node_idx].ptr, &register_buffer[node.store.value_node_idx], type_size);
						}
					}
				}
							 break;
				case Il_Load: {
					if (type_size > sizeof(Const_Union)) {
						register_buffer[i].ptr = register_buffer[node.load.ptr_node_idx].ptr;
					}
					else {
						memcpy(&register_buffer[i], register_buffer[node.load.ptr_node_idx].ptr, type_size);
					}
				}
							break;
				case Il_Const: {

					if (!(type_flags & TN_Float_Type)) {
						register_buffer[i] = node.constant.as;
					}
					else {
						if (type_size == 8) {
							register_buffer[i].f64 = node.constant.as.f64;
						}
						else if (type_size == 4) {
							register_buffer[i].f32 = (float)node.constant.as.f64;
						}
						else {
							ASSERT(nullptr);
						}
					}
				}
							 break;
				case Il_StructElementPtr: {
					GS_Struct& strct = ee.type_system->struct_storage[ee.type_system->type_name_storage[type->basic.type_name_id].struct_id];
					register_buffer[i].ptr = ((u8*)register_buffer[node.element_ptr.ptr_node_idx].ptr) + strct.offsets[node.element_ptr.element_idx];
				}
										break;
				case Il_ArrayElementPtr: {
					register_buffer[i].ptr = ((u8*)register_buffer[node.aep.ptr_node_idx].ptr) + register_buffer[node.aep.index_node_idx].us8 * type_size;
				}
									   break;
#define DO_OP(OP, SZ) register_buffer[i].SZ = register_buffer[node.math_op.left_node_idx].SZ OP register_buffer[node.math_op.right_node_idx].SZ

				case Il_Add: {
					if (type_flags & TN_Float_Type) {
						if (type_size == 4) {
							register_buffer[i].f32 = register_buffer[node.math_op.left_node_idx].f32 + register_buffer[node.math_op.right_node_idx].f32;
						}
						else if (type_size == 8) {
							register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 + register_buffer[node.math_op.right_node_idx].f64;
						}
						else {
							ASSERT(nullptr);
						}
					}
					else {
						switch (type_size)
						{
						case 1:	DO_OP(+, s1); break;
						case 2:	DO_OP(+, s2); break;
						case 4:	DO_OP(+, s4); break;
						case 8: DO_OP(+, s8); break;
						}
					}
				}
						   break;
				case Il_Sub: {
					if (type_flags & TN_Float_Type) {
						if (type_size == 4) {
							register_buffer[i].f32 = register_buffer[node.math_op.left_node_idx].f32 - register_buffer[node.math_op.right_node_idx].f32;
						}
						else if (type_size == 8) {
							register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 - register_buffer[node.math_op.right_node_idx].f64;
						}
						else {
							ASSERT(nullptr);
						}
					}
					else {
						switch (type_size)
						{
						case 1:	DO_OP(-, s1); break;
						case 2:	DO_OP(-, s2); break;
						case 4:	DO_OP(-, s4); break;
						case 8: DO_OP(-, s8); break;
						}
					}
				}
						   break;
				case Il_Mul: {
					if (type_flags & TN_Float_Type) {
						if (type_size == 4) {
							register_buffer[i].f32 = register_buffer[node.math_op.left_node_idx].f32 * register_buffer[node.math_op.right_node_idx].f32;
						}
						else if (type_size == 8) {
							register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 * register_buffer[node.math_op.right_node_idx].f64;
						}
						else {
							ASSERT(nullptr);
						}
					}
					else {
						switch (type_size)
						{
						case 1:	DO_OP(*, s1); break;
						case 2:	DO_OP(*, s2); break;
						case 4:	register_buffer[i].s4 = register_buffer[node.math_op.left_node_idx].s4 * register_buffer[node.math_op.right_node_idx].s4; break;
						case 8: DO_OP(*, s8); break;
						}
					}
				}
						   break;
				case Il_Div: {
					if (type_flags & TN_Float_Type) {
						if (type_size == 4) {
							register_buffer[i].f32 = register_buffer[node.math_op.left_node_idx].f32 / register_buffer[node.math_op.right_node_idx].f32;
						}
						else if (type_size == 8) {
							register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 / register_buffer[node.math_op.right_node_idx].f64;
						}
						else {
							ASSERT(nullptr);
						}
					}
					else {
						switch (type_size)
						{
						case 1:	DO_OP(/ , s1); break;
						case 2:	DO_OP(/ , s2); break;
						case 4:	DO_OP(/ , s4); break;
						case 8: DO_OP(/ , s8); break;
						}
					}
				}
						   break;
				case Il_Bit_And: {
					register_buffer[i].s8 = register_buffer[node.cmp_op.left_node_idx].s8 & register_buffer[node.cmp_op.right_node_idx].s8;
				}
							   break;
				case Il_Bit_Or: {
					register_buffer[i].us8 = register_buffer[node.cmp_op.left_node_idx].s8 | register_buffer[node.cmp_op.right_node_idx].s8;
				}
							  break;
#define DO_CMP_OP(OP, SZ) register_buffer[i].s1 = register_buffer[node.cmp_op.left_node_idx].SZ OP register_buffer[node.cmp_op.right_node_idx].SZ
				case Il_Value_Cmp:
				{
					register_buffer[i] = { 0 };
					if (node.cmp_op.compare_type == Il_Cmp_Equal) {
						register_buffer[i].s1 = register_buffer[node.cmp_op.left_node_idx].s8 == register_buffer[node.cmp_op.right_node_idx].s8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_NotEqual) {
						register_buffer[i].s1 = register_buffer[node.cmp_op.left_node_idx].s8 != register_buffer[node.cmp_op.right_node_idx].s8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_GreaterEqual) {
						register_buffer[i].s1 = register_buffer[node.cmp_op.left_node_idx].s8 >= register_buffer[node.cmp_op.right_node_idx].s8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_LesserEqual) {
						register_buffer[i].s1 = register_buffer[node.cmp_op.left_node_idx].s8 <= register_buffer[node.cmp_op.right_node_idx].s8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_Lesser) {
						switch (type_size)
						{
						case 1:DO_CMP_OP(< , s1); break;
						case 2:DO_CMP_OP(< , s2); break;
						case 4:DO_CMP_OP(< , s4); break;
						case 8:DO_CMP_OP(< , s8); break;
						}
					}
					else if (node.cmp_op.compare_type == Il_Cmp_Greater) {
						switch (type_size)
						{
						case 1:DO_CMP_OP(> , s1); break;
						case 2:DO_CMP_OP(> , s2); break;
						case 4:DO_CMP_OP(> , s4); break;
						case 8:DO_CMP_OP(> , s8); break;
						}
					}
					else if (node.cmp_op.compare_type == Il_Cmp_And) {
						register_buffer[i].s1 = register_buffer[node.cmp_op.left_node_idx].s8 && register_buffer[node.cmp_op.right_node_idx].s8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_Or) {
						register_buffer[i].s1 = register_buffer[node.cmp_op.left_node_idx].s8 || register_buffer[node.cmp_op.right_node_idx].s8;
					}
					else { GS_ASSERT_UNIMPL(); }
				}
				break;
				case Il_Ret: {
					returned_value.string = register_buffer[node.ret.value_node_idx].string;
					goto proc_exit;
				}
						   break;
				case Il_Call: {

					Array<Const_Union> call_arguments;
					Array<GS_Type*> call_argument_types;

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Array_Add(call_arguments, register_buffer[node.call.arguments[i]]);
						Array_Add(call_argument_types, register_type_buffer[node.call.arguments[i]]);
					}

					register_buffer[i] = EE_Exec_Proc(ee, proc.program->procedures[node.call.proc_idx], call_arguments, call_argument_types);
				}
							break;
				case Il_Call_Ptr: {

					Array<Const_Union> call_arguments;
					Array<GS_Type*> call_argument_types;

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Array_Add(call_arguments, register_buffer[node.call.arguments[i]]);
						Array_Add(call_argument_types, register_type_buffer[node.call.arguments[i]]);
					}

					register_buffer[i] = EE_Dynamic_Invoke_Proc(ee, register_buffer[node.call.proc_idx].ptr, &ee.type_system->type_storage[node.call.signature], false, call_arguments, call_argument_types);
				}
								break;
				case Il_String: {
					register_buffer[i].ptr = node.string.str.data;
				}
							  break;
				case Il_Cond_Branch:
				{
					if (register_buffer[node.c_branch.condition_node_idx].us1) {
						block_idx = node.c_branch.true_case_block_idx;
					}
					else {
						block_idx = node.c_branch.false_case_block_idx;
					}

					goto branch;
				}
				break;
				break;
				case Il_Branch:
				{
					block_idx = node.br.block_idx;
					goto branch;
				}
				break;
				case Il_Cast:
				{
					register_buffer[i] = register_buffer[node.cast.castee_node_idx];
				}
				break;
				case Il_Struct_Initializer:
				{
					GS_Struct& strct = ee.type_system->struct_storage[ee.type_system->type_name_storage[type->basic.type_name_id].struct_id];

					ASSERT(node.si.member_count < SI_SMALL_COUNT);

					u8* dest = (u8*)&register_buffer[i];

					if (type_size > sizeof(Const_Union))
					{
						dest = (u8*)&stack[stack_pointer];
						stack_pointer += type_size;
					}

					for (size_t i = 0; i < node.si.member_count; i++)
					{
						GS_Type* member_type = strct.members[i];

						auto member_type_size = TypeSystem_Get_Type_Size(*ee.type_system, member_type);

						void* src;

						if (member_type_size > sizeof(Const_Union)) {
							src = register_buffer[node.si.members_value_nodes[i]].ptr;
						}
						else {
							src = &register_buffer[node.si.members_value_nodes[i]];
						}

						memcpy(dest + strct.offsets[i], src, member_type_size);
					}
				}
				break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}
		}

	proc_exit:
		return returned_value;
	}

	typedef double (*dynamic_invoke_d_t)(void*, size_t, size_t*, size_t*);
	typedef float (*dynamic_invoke_f_t)(void*, size_t, size_t*, size_t*);
	typedef size_t(*dynamic_invoke_i_t)(void*, size_t, size_t*, size_t*);
	typedef void(*sin_t)();
	typedef int(*init_t)(int, int, const char*);

	extern "C" size_t dynamic_invoke(void* func, size_t arg_count, size_t * arguments, size_t * argument_types);

	Const_Union EE_Exec_External_Proc(Execution_Engine& ee, Il_Proc& proc, Array<Const_Union> arguments, Array<GS_Type*> argument_types)
	{
		ASSERT(proc.external);

		HMODULE loaded_dll = (HMODULE)ee.library_handles[proc.library];
		ASSERT(loaded_dll);

		Il_IDX proc_idx = (((u64)&proc - (u64)proc.program->procedures.data) / sizeof(Il_Proc));

		void* external_proc_pointer = ee.proc_pointers[proc_idx];
		ASSERT(external_proc_pointer);

		return EE_Dynamic_Invoke_Proc(ee, external_proc_pointer, proc.signature, proc.variadic, arguments, argument_types);
	}

	Const_Union EE_Dynamic_Invoke_Proc(Execution_Engine& ee, void* proc_pointer, GS_Type* signature, bool variadic, Array<Const_Union> arguments, Array<GS_Type*> argument_types /*= {}*/)
	{
		dynamic_invoke_f_t f = (dynamic_invoke_f_t)dynamic_invoke;
		dynamic_invoke_d_t d = (dynamic_invoke_d_t)dynamic_invoke;
		dynamic_invoke_i_t i = (dynamic_invoke_i_t)dynamic_invoke;

		Const_Union return_value = { 0 };

		auto return_flags = TypeSystem_Get_Type_Flags(*ee.type_system, signature->proc.return_type);
		auto return_size = TypeSystem_Get_Type_Size(*ee.type_system, signature->proc.return_type);

		enum Arg_Type {
			Dyn_ARG_FLOAT = 0,
			Dyn_ARG_DOUBLE = 1,
			Dyn_ARG_SCALAR = 2,
		};

		size_t call_argument_data[128] = { 0 };
		size_t call_argument_types[128] = { 0 };

		for (size_t i = 0; i < arguments.count; i++)
		{
			auto argument_flags = TypeSystem_Get_Type_Flags(*ee.type_system, argument_types[i]);
			auto argument_size = TypeSystem_Get_Type_Size(*ee.type_system, argument_types[i]);

			if (argument_flags & TN_Float_Type) {
				if (argument_size == 4) {
					call_argument_data[i] = *(size_t*)&arguments[i].f32;
					call_argument_types[i] = Dyn_ARG_FLOAT;

					if (variadic) {
						double as_double = arguments[i].f32;
						call_argument_data[i] = *(size_t*)&as_double;
						call_argument_types[i] = Dyn_ARG_DOUBLE;
					}
				}
				else if (argument_size == 8) {
					call_argument_data[i] = arguments[i].us8;
					call_argument_types[i] = Dyn_ARG_DOUBLE;
				}
				else {
					ASSERT(nullptr);
				}
			}
			else {
				call_argument_data[i] = arguments[i].us8;
				call_argument_types[i] = Dyn_ARG_SCALAR;
			}
		}

		if (!(return_flags & TN_Float_Type)) {
			return_value.us8 = i(proc_pointer, arguments.count, call_argument_data, call_argument_types);
		}
		else {
			if (return_size == 4) {
				return_value.f32 = f(proc_pointer, arguments.count, call_argument_data, call_argument_types);
			}
			else if (return_size == 8) {
				return_value.f64 = d(proc_pointer, arguments.count, call_argument_data, call_argument_types);
			}
			else {
				ASSERT(nullptr);
			}
		}

		return return_value;
	}
}