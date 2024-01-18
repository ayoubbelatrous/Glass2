#include "pch.h"

#include "BackEnd/Il.h"

namespace Glass
{
	Const_Union EE_Exec_Proc(Execution_Engine& ee, Il_Proc& proc)
	{
		GS_CORE_ASSERT(ee.type_system);

		Const_Union returned_value = { 0 };

		for (size_t bidx = 0; bidx < proc.blocks.count; bidx++)
		{
			Il_Block& block = proc.blocks[bidx];

			for (size_t i = 0; i < block.instructions.count; i++)
			{
				Il_Node node = proc.instruction_storage[block.instructions[i]];

				GS_Type* type = &ee.type_system->type_storage[node.type_idx];
				Type_Name_Flags type_flags = TypeSystem_Get_Type_Flags(*ee.type_system, type);

				switch (node.node_type)
				{
				case Il_Const: {
					ee.register_buffer[i].string = node.constant.as.string;
				}
							 break;
				case Il_StructElementPtr: {
					GS_Struct& strct = ee.type_system->struct_storage[ee.type_system->type_name_storage[type->basic.type_name_id].struct_id];
					ee.register_buffer[i].ptr = ((u8*)ee.register_buffer[node.element_ptr.ptr_node_idx].ptr) + strct.offsets[node.element_ptr.element_idx];
				}
										break;
				case Il_Add: {
					if (type_flags & TN_Float_Type) {
						ee.register_buffer[i].f64 = ee.register_buffer[node.math_op.left_node_idx].f64 + ee.register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						ee.register_buffer[i].us8 = ee.register_buffer[node.math_op.left_node_idx].us8 + ee.register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Sub: {
					if (type_flags & TN_Float_Type) {
						ee.register_buffer[i].f64 = ee.register_buffer[node.math_op.left_node_idx].f64 - ee.register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						ee.register_buffer[i].us8 = ee.register_buffer[node.math_op.left_node_idx].us8 - ee.register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Mul: {
					if (type_flags & TN_Float_Type) {
						ee.register_buffer[i].f64 = ee.register_buffer[node.math_op.left_node_idx].f64 * ee.register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						ee.register_buffer[i].us8 = ee.register_buffer[node.math_op.left_node_idx].us8 * ee.register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Div: {
					if (type_flags & TN_Float_Type) {
						ee.register_buffer[i].f64 = ee.register_buffer[node.math_op.left_node_idx].f64 / ee.register_buffer[node.math_op.right_node_idx].f64;
					}
					else {
						ee.register_buffer[i].us8 = ee.register_buffer[node.math_op.left_node_idx].us8 / ee.register_buffer[node.math_op.right_node_idx].us8;
					}
				}
						   break;
				case Il_Ret: {
					returned_value.string = ee.register_buffer[node.ret.value_node_idx].string;
					goto proc_exit;
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