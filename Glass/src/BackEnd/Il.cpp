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
				{
					stream << "call " << TypeSystem_Print_Type_Index(*proc.program->type_system, node.type_idx).data << " @" << node.call.proc_idx << "(";

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
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}
		}

		return stream.str();
	}

	Const_Union EE_Exec_Program(Execution_Engine& ee, Il_Program* prog, Il_IDX entry)
	{
		//		ee.library_handles = Array_Reserved<void*>(prog->libraries.count);

		for (size_t i = 0; i < prog->libraries.count; i++)
		{
			HMODULE loaded_dll = LoadLibraryA(ee.program->libraries[i].path.data);
			ASSERT(loaded_dll);

			Array_Add(ee.library_handles, (void*)loaded_dll);

			void* dll_main_proc_pointer = GetProcAddress(loaded_dll, "glfw3");
			int x = 30;
		}

		//ee.proc_pointers = Array_Reserved<void*>(prog->procedures.count);

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

		Const_Union register_buffer[REG_BUF_SZ];
		GS_Type* register_type_buffer[REG_BUF_SZ];
		u64* stack[STACK_SZ] = { 0 };
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
				case Il_Alloca: {
					register_buffer[i].ptr = &stack[stack_pointer];
					stack_pointer += std::max(type_size, sizeof(Const_Union));
					register_type_buffer[i] = TypeSystem_Get_Pointer_Type(*proc.program->type_system, register_type_buffer[i], 1);
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
						register_buffer[i] = *(Const_Union*)register_buffer[node.load.ptr_node_idx].ptr;
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
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 + register_buffer[node.math_op.right_node_idx].us8;
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
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 - register_buffer[node.math_op.right_node_idx].us8;
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
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 * register_buffer[node.math_op.right_node_idx].us8;
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
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 / register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Bit_And: {
					register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 & register_buffer[node.math_op.right_node_idx].us8;
				}
							   break;
				case Il_Bit_Or: {
					register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 | register_buffer[node.math_op.right_node_idx].us8;
				}
							  break;
				case Il_Value_Cmp:
				{
					register_buffer[i] = { 0 };
					if (node.cmp_op.compare_type == Il_Cmp_Equal) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 == register_buffer[node.cmp_op.right_node_idx].us8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_NotEqual) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 != register_buffer[node.cmp_op.right_node_idx].us8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_GreaterEqual) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 >= register_buffer[node.cmp_op.right_node_idx].us8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_LesserEqual) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 <= register_buffer[node.cmp_op.right_node_idx].us8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_Lesser) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 < register_buffer[node.cmp_op.right_node_idx].us8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_Greater) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 > register_buffer[node.cmp_op.right_node_idx].us8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_And) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 && register_buffer[node.cmp_op.right_node_idx].us8;
					}
					else if (node.cmp_op.compare_type == Il_Cmp_Or) {
						register_buffer[i].us1 = register_buffer[node.cmp_op.left_node_idx].us8 || register_buffer[node.cmp_op.right_node_idx].us8;
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

	extern "C" size_t dynamic_invoke(void* func, size_t arg_count, size_t * arguments, size_t * argument_types);

	Const_Union EE_Exec_External_Proc(Execution_Engine& ee, Il_Proc& proc, Array<Const_Union> arguments, Array<GS_Type*> argument_types)
	{
		ASSERT(proc.external);

		HMODULE loaded_dll = (HMODULE)ee.library_handles[proc.library];
		ASSERT(loaded_dll);

		Il_IDX proc_idx = (((u64)&proc - (u64)proc.program->procedures.data) / sizeof(Il_Proc));

		void* external_proc_pointer = ee.proc_pointers[proc_idx];
		ASSERT(external_proc_pointer);

		dynamic_invoke_f_t f = (dynamic_invoke_f_t)dynamic_invoke;
		dynamic_invoke_d_t d = (dynamic_invoke_d_t)dynamic_invoke;
		dynamic_invoke_i_t i = (dynamic_invoke_i_t)dynamic_invoke;

		Const_Union return_value = { 0 };

		auto return_flags = TypeSystem_Get_Type_Flags(*proc.program->type_system, proc.signature->proc.return_type);
		auto return_size = TypeSystem_Get_Type_Size(*proc.program->type_system, proc.signature->proc.return_type);

		enum Arg_Type {
			Dyn_ARG_FLOAT = 0,
			Dyn_ARG_DOUBLE = 1,
			Dyn_ARG_SCALAR = 2,
		};

		size_t call_argument_data[128] = { 0 };
		size_t call_argument_types[128] = { 0 };

		for (size_t i = 0; i < arguments.count; i++)
		{
			auto argument_flags = TypeSystem_Get_Type_Flags(*proc.program->type_system, argument_types[i]);
			auto argument_size = TypeSystem_Get_Type_Size(*proc.program->type_system, argument_types[i]);

			if (argument_flags & TN_Float_Type) {
				if (argument_size == 4) {
					call_argument_data[i] = *(size_t*)&arguments[i].f32;
					call_argument_types[i] = Dyn_ARG_FLOAT;

					if (proc.variadic) {
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
			return_value.us8 = i(external_proc_pointer, arguments.count, call_argument_data, call_argument_types);
		}
		else {
			if (return_size == 4) {
				return_value.f32 = f(external_proc_pointer, arguments.count, call_argument_data, call_argument_types);
			}
			else if (return_size == 8) {
				return_value.f64 = d(external_proc_pointer, arguments.count, call_argument_data, call_argument_types);
			}
			else {
				ASSERT(nullptr);
			}
		}

		return return_value;
	}
}