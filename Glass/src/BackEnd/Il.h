#pragma once

#include "Base/String.h"
#include "Base/Array.h"
#include "Base/Types.h"
#include "FrontEnd/Type_System.h"

namespace Glass
{
	struct GS_String
	{
		u64 count;
		u8* data;
	};

	using Il_IDX = u16;

	enum Il_Node_Type : u8
	{
		Il_None = 0,

		Il_Const,

		Il_Alloca,
		Il_Load,
		Il_Store,

		Il_Ret,
		Il_Call,

		Il_Add,
		Il_Sub,
		Il_Mul,
		Il_Div,

		Il_StructElementPtr,

		Il_Max,
	};

	struct Il_Node_Alloca
	{
	};

	struct Il_Node_Load
	{
		Il_Node_Type node_type;
		Il_IDX ptr_node_idx;
	};

	struct Il_Node_Store
	{
		Il_IDX ptr_node_idx;
		Il_IDX value_node_idx;
	};

	struct Il_Node_Struct_Element_Ptr
	{
		Il_IDX ptr_node_idx;
		u32 element_idx;
	};

	struct Il_Node_Math_Op
	{
		Il_IDX left_node_idx;
		Il_IDX right_node_idx;
	};

	struct Il_Node_Ret
	{
		Il_IDX value_node_idx;
	};

	struct Il_Node_Call
	{
		Il_IDX* arguments;
		u16 argument_count;
	};

	union Const_Union
	{
		u8	us1;
		u16 us2;
		u32 us4;
		u64 us8;

		i8	s1;
		i16 s2;
		i32 s4;
		i64 s8;

		float f32;
		double f64;

		void* ptr;

		GS_Type* type;

		GS_String string;
	};

	struct Il_Node_Constant
	{
		Const_Union as;
	};

	struct Il_Node
	{
		u16 type_idx;
		Il_Node_Type node_type;

		union
		{
			Il_Node_Constant constant;
			Il_Node_Alloca alloca;
			Il_Node_Load load;
			Il_Node_Store store;
			Il_Node_Math_Op math_op;
			Il_Node_Ret ret;
			Il_Node_Struct_Element_Ptr element_ptr;
		};
	};

	struct Il_Block
	{
		String name;
		Array<Il_IDX> instructions;
	};

	struct Il_Proc
	{
		String proc_name;
		Array<Il_Node> instruction_storage;
		Array<Il_Block> blocks;

		Il_IDX insertion_point = (Il_IDX)-1;
	};

	struct Il_Program
	{
		Array<Il_Proc> procedures;
	};

	inline void Il_Program_Init(Il_Program& prog) {
		prog.procedures = Array_Reserved<Il_Proc>(1024 * 10);
	}

	inline Il_IDX Il_Insert_Proc(Il_Program& prog, String name) {

		Il_Proc proc;
		proc.proc_name = name;
		proc.instruction_storage = Array_Reserved<Il_Node>(1024 * 10);
		proc.blocks = Array_Reserved<Il_Block>(128);

		Il_Block entry_block;
		entry_block.instructions = Array_Reserved<Il_IDX>(128);

		Array_Add(proc.blocks, entry_block);
		proc.insertion_point = 0;

		Array_Add(prog.procedures, proc);

		return (Il_IDX)(prog.procedures.count - 1);
	}

	inline Il_IDX Il_Proc_Insert(Il_Proc& proc, Il_Node node) {

		ASSERT(node.node_type != Il_Node_Type::Il_None, node.node_type < Il_Node_Type::Il_Max);

		Il_IDX node_idx = (Il_IDX)proc.instruction_storage.count;
		ASSERT(proc.insertion_point != -1);

		Array_Add(proc.instruction_storage, node);
		Array_Add(proc.blocks[proc.insertion_point].instructions, node_idx);

		return node_idx;
	}

	inline Il_Node Il_Make_Constant(void* value, u16 type_index) {

		Il_Node const_node;
		const_node.node_type = Il_Const;
		const_node.type_idx = type_index;
		const_node.constant.as.ptr = value;

		return const_node;
	}

	inline Il_Node Il_Make_Struct_Element_Ptr(u16 type_idx, u16 element_idx, u16 ptr_node_idx) {

		Il_Node sep_node;
		sep_node.node_type = Il_StructElementPtr;
		sep_node.type_idx = type_idx;
		sep_node.element_ptr.ptr_node_idx = ptr_node_idx;
		sep_node.element_ptr.element_idx = element_idx;

		return sep_node;
	}

	inline Il_Node Il_Make_Constant(Const_Union value, u16 type_index) {

		Il_Node const_node;
		const_node.node_type = Il_Const;
		const_node.type_idx = type_index;
		const_node.constant.as = value;

		return const_node;
	}

	inline Il_Node Il_Make_Ret(u16 value_node_idx, u16 type_index) {

		Il_Node const_node;
		const_node.node_type = Il_Ret;
		const_node.type_idx = type_index;
		const_node.ret.value_node_idx = value_node_idx;

		return const_node;
	}

	inline Il_Node Il_Make_Math_Op(Il_Node_Type math_op_node_type, u16 type_index, u16 left, u16 right) {

		if (math_op_node_type != Il_Add && math_op_node_type != Il_Sub && math_op_node_type != Il_Mul && math_op_node_type != Il_Div) {
			GS_ASSERT_UNIMPL();
		}

		Il_Node math_op_node;
		math_op_node.node_type = math_op_node_type;
		math_op_node.type_idx = type_index;
		math_op_node.math_op.left_node_idx = left;
		math_op_node.math_op.right_node_idx = right;

		return math_op_node;
	}

#define REG_BUF_SZ 2048

	struct Execution_Engine {
		Const_Union register_buffer[REG_BUF_SZ];
		Type_System* type_system = nullptr;
	};

	Const_Union EE_Exec_Proc(Execution_Engine& ee, Il_Proc& proc);

}