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

			for (Il_IDX i = 0; i < (Il_IDX)block.instructions.count; i++)
			{
				Il_Node node = proc.instruction_storage[block.instructions[i]];

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
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}
		}

		return stream.str();
	}

	Const_Union EE_Exec_Proc(Execution_Engine& ee, Il_Proc& proc, Array<Const_Union> arguments)
	{
		GS_CORE_ASSERT(ee.type_system);

		Const_Union register_buffer[REG_BUF_SZ];
		u64 stack[STACK_SZ] = { (u64)-1 };
		u64 stack_pointer = 0;

		Const_Union returned_value = { 0 };

		for (size_t bidx = 0; bidx < proc.blocks.count; bidx++)
		{
			Il_Block& block = proc.blocks[bidx];

			for (size_t i = 0; i < block.instructions.count; i++)
			{
				Il_Node node = proc.instruction_storage[block.instructions[i]];

				GS_Type* type = &ee.type_system->type_storage[node.type_idx];
				Type_Name_Flags type_flags = TypeSystem_Get_Type_Flags(*ee.type_system, type);
				auto type_size = TypeSystem_Get_Type_Size(*ee.type_system, type);

				switch (node.node_type)
				{
				case Il_Param: {
					register_buffer[i] = arguments[node.param.index];
				}
							 break;
				case Il_Alloca: {
					stack_pointer += std::max(type_size, sizeof(Const_Union));
					register_buffer[i].ptr = &stack[stack_pointer];
				}
							  break;
				case Il_Store: {
					memcpy(register_buffer[node.store.ptr_node_idx].ptr, &register_buffer[node.store.value_node_idx], type_size);
				}
							 break;
				case Il_Load: {
					register_buffer[i] = *(Const_Union*)register_buffer[node.load.ptr_node_idx].ptr;
				}
							break;
				case Il_Const: {
					register_buffer[i].string = node.constant.as.string;
				}
							 break;
				case Il_StructElementPtr: {
					GS_Struct& strct = ee.type_system->struct_storage[ee.type_system->type_name_storage[type->basic.type_name_id].struct_id];
					register_buffer[i].ptr = ((u8*)register_buffer[node.element_ptr.ptr_node_idx].ptr) + strct.offsets[node.element_ptr.element_idx];
				}
										break;
				case Il_Add: {
					if (type_flags & TN_Float_Type) {
						register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 + register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 + register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Sub: {
					if (type_flags & TN_Float_Type) {
						register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 - register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 - register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Mul: {
					if (type_flags & TN_Float_Type) {
						register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 * register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 * register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Div: {
					if (type_flags & TN_Float_Type) {
						register_buffer[i].f64 = register_buffer[node.math_op.left_node_idx].f64 / register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						register_buffer[i].us8 = register_buffer[node.math_op.left_node_idx].us8 / register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Ret: {
					returned_value.string = register_buffer[node.ret.value_node_idx].string;
					goto proc_exit;
				}
						   break;
				case Il_Call: {

					Array<Const_Union> arguments;

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Array_Add(arguments, register_buffer[node.call.arguments[i]]);
					}

					register_buffer[i] = EE_Exec_Proc(ee, proc.program->procedures[node.call.proc_idx], arguments);
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
}