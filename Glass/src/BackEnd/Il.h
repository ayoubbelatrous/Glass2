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

		Il_Param,

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

	struct Il_Node_Param
	{
		Il_IDX index;
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
		u16 element_idx;
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

	struct Il_Argument
	{
		Il_IDX value_node_idx = 0;
		Type_IDX type_idx = 0;
	};

#define SMALL_ARG_COUNT 16 / sizeof(Il_IDX)

	struct Il_Node_Call
	{
		Il_IDX proc_idx;
		u16 argument_count;

		union {
			Il_IDX arguments[SMALL_ARG_COUNT];
			Il_IDX* arguments_ptr;
		};
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

		//Il_IDX code_location;
	};

	struct Il_Node_Constant
	{
		Const_Union as;
	};

	struct Il_Node
	{
		Type_IDX type_idx;
		Il_Node_Type node_type;

		union
		{
			Il_Node_Constant constant;
			Il_Node_Alloca aloca;
			Il_Node_Load load;
			Il_Node_Store store;
			Il_Node_Math_Op math_op;
			Il_Node_Ret ret;
			Il_Node_Struct_Element_Ptr element_ptr;
			Il_Node_Param param;
			Il_Node_Call call;
		};
	};

	struct Il_Block
	{
		String name;
		Array<Il_IDX> instructions;
	};

	struct Il_Program;

	struct Il_Proc
	{
		Il_Program* program = nullptr;

		String proc_name;
		GS_Type* signature = nullptr;

		Array<Il_IDX> parameters;

		Array<Il_Node> instruction_storage;
		Array<Il_Block> blocks;
		Il_IDX insertion_point = (Il_IDX)-1;
	};

	struct Il_Program
	{
		Array<Il_Proc> procedures;
		Type_System* type_system = nullptr;
	};

	inline Il_Node Il_Make_Param(Il_IDX type_index, Il_IDX param_index) {

		Il_Node param_node = { 0 };
		param_node.node_type = Il_Param;
		param_node.param.index = param_index;

		return param_node;
	}

	inline void Il_Program_Init(Il_Program& prog, Type_System* type_system) {
		prog.procedures = Array_Reserved<Il_Proc>(1024 * 10);
		prog.type_system = type_system;
	}

	inline Il_IDX Il_Proc_Insert(Il_Proc& proc, Il_Node node) {

		ASSERT(node.node_type != Il_Node_Type::Il_None, node.node_type < Il_Node_Type::Il_Max);

		Il_IDX node_idx = (Il_IDX)proc.instruction_storage.count;
		ASSERT(proc.insertion_point != -1);

		Array_Add(proc.instruction_storage, node);
		Array_Add(proc.blocks[proc.insertion_point].instructions, node_idx);

		return node_idx;
	}

	inline Il_IDX Il_Insert_Proc(Il_Program& prog, String name, GS_Type* signature) {

		Il_Proc proc = {};
		proc.proc_name = name;
		proc.instruction_storage = Array_Reserved<Il_Node>(1024 * 10);
		proc.blocks = Array_Reserved<Il_Block>(128);
		proc.program = &prog;
		proc.signature = signature;

		Il_Block entry_block;
		entry_block.instructions = Array_Reserved<Il_IDX>(128);

		Array_Add(proc.blocks, entry_block);
		proc.insertion_point = 0;

		for (size_t i = 0; i < signature->proc.params.count; i++)
		{
			Il_IDX inserted_param_idx = Il_Proc_Insert(proc, Il_Make_Param((Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, signature->proc.params[i]), (Il_IDX)i));
			Array_Add(proc.parameters, inserted_param_idx);
		}

		Array_Add(prog.procedures, proc);

		return (Il_IDX)(prog.procedures.count - 1);
	}

	inline Il_Node Il_Make_Constant(void* value, Type_IDX type_index) {

		Il_Node const_node = { 0 };
		const_node.node_type = Il_Const;
		const_node.type_idx = type_index;
		const_node.constant.as.ptr = value;

		return const_node;
	}

	inline Il_Node Il_Make_Struct_Element_Ptr(Type_IDX type_idx, u16 element_idx, Il_IDX ptr_node_idx) {

		Il_Node sep_node = { 0 };
		sep_node.node_type = Il_StructElementPtr;
		sep_node.type_idx = type_idx;
		sep_node.element_ptr.ptr_node_idx = ptr_node_idx;
		sep_node.element_ptr.element_idx = element_idx;

		return sep_node;
	}

	inline Il_Node Il_Make_Constant(Const_Union value, Type_IDX type_index) {

		Il_Node const_node = { 0 };
		const_node.node_type = Il_Const;
		const_node.type_idx = type_index;
		const_node.constant.as = value;

		return const_node;
	}

	inline Il_Node Il_Make_Ret(Type_IDX type_index, Il_IDX value_node_idx) {

		Il_Node const_node = { 0 };
		const_node.node_type = Il_Ret;
		const_node.type_idx = type_index;
		const_node.ret.value_node_idx = value_node_idx;

		return const_node;
	}

	inline Il_Node Il_Make_Alloca(Type_IDX type_index) {

		Il_Node const_node = { 0 };
		const_node.node_type = Il_Alloca;
		const_node.type_idx = type_index;

		return const_node;
	}

	inline Il_Node Il_Make_Store(Il_IDX ptr, Il_IDX value, Type_IDX type_index) {

		Il_Node store_node = { 0 };
		store_node.node_type = Il_Store;
		store_node.type_idx = type_index;
		store_node.store.ptr_node_idx = ptr;
		store_node.store.value_node_idx = value;

		return store_node;
	}

	inline Il_Node Il_Make_Load(Il_IDX ptr, Type_IDX type_index) {

		Il_Node store_node = { 0 };
		store_node.node_type = Il_Load;
		store_node.type_idx = type_index;
		store_node.load.ptr_node_idx = ptr;

		return store_node;
	}

	inline Il_Node Il_Make_Math_Op(Il_Node_Type math_op_node_type, Type_IDX type_index, Il_IDX left, Il_IDX right) {

		if (math_op_node_type != Il_Add && math_op_node_type != Il_Sub && math_op_node_type != Il_Mul && math_op_node_type != Il_Div) {
			GS_ASSERT_UNIMPL();
		}

		Il_Node math_op_node = { 0 };
		math_op_node.node_type = math_op_node_type;
		math_op_node.type_idx = type_index;
		math_op_node.math_op.left_node_idx = left;
		math_op_node.math_op.right_node_idx = right;

		return math_op_node;
	}

	inline Il_Node Il_Make_Call(Type_IDX return_type_index, Array<Il_Argument> arguments, Il_IDX proc_idx) {

		Il_Node call_node = { 0 };
		call_node.node_type = Il_Call;
		call_node.type_idx = return_type_index;

		if (arguments.count <= SMALL_ARG_COUNT) {
			for (size_t i = 0; i < arguments.count; i++)
			{
				call_node.call.arguments[i] = arguments[i].value_node_idx;
			}
		}
		else {

			call_node.call.arguments_ptr = (Il_IDX*)malloc(arguments.count * sizeof(Il_IDX));

			for (size_t i = 0; i < arguments.count; i++)
			{
				call_node.call.arguments_ptr[i] = arguments[i].value_node_idx;
			}
		}

		call_node.call.argument_count = (u16)arguments.count;
		call_node.call.proc_idx = proc_idx;

		return call_node;
	}

	inline Il_IDX Il_Insert_Alloca(Il_Proc& proc, GS_Type* type) {
		Il_Node alloca_node = Il_Make_Alloca((Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, type));
		return Il_Proc_Insert(proc, alloca_node);
	}

	inline Il_IDX Il_Insert_Store(Il_Proc& proc, GS_Type* type, Il_IDX ptr, Il_IDX value) {
		Il_Node store_node = Il_Make_Store(ptr, value, (Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, type));
		return Il_Proc_Insert(proc, store_node);
	}

	inline Il_IDX Il_Insert_Load(Il_Proc& proc, GS_Type* type, Il_IDX ptr) {
		Il_Node load_node = Il_Make_Load(ptr, (Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, type));
		return Il_Proc_Insert(proc, load_node);
	}

	inline Il_IDX Il_Insert_Constant(Il_Proc& proc, Const_Union constant, GS_Type* type) {
		Il_Node const_node = Il_Make_Constant(constant, (Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, type));
		return Il_Proc_Insert(proc, const_node);
	}

	inline Il_IDX Il_Insert_Math_Op(Il_Proc& proc, GS_Type* type, Il_Node_Type math_op_type, Il_IDX left, Il_IDX right) {
		Il_Node math_op_node = Il_Make_Math_Op(math_op_type, (Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, type), left, right);
		return Il_Proc_Insert(proc, math_op_node);
	}

	inline Il_IDX Il_Insert_Ret(Il_Proc& proc, GS_Type* type, Il_IDX value) {
		Il_Node ret_node = Il_Make_Ret((Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, type), value);
		return Il_Proc_Insert(proc, ret_node);
	}

	inline Il_IDX Il_Insert_Call(Il_Proc& proc, GS_Type* signature, Array<Il_Argument> arguments, Il_IDX callee_proc) {
		Il_Node call_node = Il_Make_Call((Il_IDX)TypeSystem_Get_Type_Index(*proc.program->type_system, signature->proc.return_type), arguments, callee_proc);
		return Il_Proc_Insert(proc, call_node);
	}

	std::string Il_Print_Proc(Il_Proc& proc);

#define REG_BUF_SZ 2048
#define STACK_SZ 1024*10

	struct Execution_Engine {
		Type_System* type_system = nullptr;
		Il_Program* program = nullptr;
	};

	Const_Union EE_Exec_Proc(Execution_Engine& ee, Il_Proc& proc, Array<Const_Union> arguments);
}