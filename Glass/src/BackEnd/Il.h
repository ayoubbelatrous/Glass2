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
		Il_String,
		Il_Store,

		Il_Ret,
		Il_Call,
		Il_Call_Ptr,

		Il_Add,
		Il_Sub,
		Il_Mul,
		Il_Div,
		Il_Bit_And,
		Il_Bit_Or,

		Il_StructElementPtr,
		Il_ArrayElementPtr,

		Il_ZI,

		Il_Struct_Initializer,
		Il_Array_Initializer,

		Il_Value_Cmp,
		Il_Value_FCmp,

		Il_Cond_Branch,
		Il_Branch,
		Il_Cast,

		Il_Global_Address,
		Il_Proc_Address,

		Il_Max,
	};

	enum Il_Cmp_Type : u8
	{
		Il_Cmp_Equal,
		Il_Cmp_NotEqual,
		Il_Cmp_Greater,
		Il_Cmp_Lesser,
		Il_Cmp_LesserEqual,
		Il_Cmp_GreaterEqual,
		Il_Cmp_And,
		Il_Cmp_Or,
	};

	enum Il_Cast_Type : u8
	{
		Il_Cast_Ptr,
		Il_Cast_Int2Ptr,
		Il_Cast_Ptr2Int,
		Il_Cast_Int2Float,
		Il_Cast_Float2Int,
		Il_Cast_FloatExt,
		Il_Cast_FloatTrunc,
		Il_Cast_IntTrunc,
		Il_Cast_IntSExt,
		Il_Cast_IntZExt,
	};

	struct Il_Node_Param
	{
		Il_IDX index;
	};

	struct Il_Node_Alloca
	{
		Type_IDX type_idx;
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
		i64 offset;
	};

	struct Il_Node_Bin_Op
	{
		Il_IDX left_node_idx;
		Il_IDX right_node_idx;
	};

	struct Il_Node_Compare
	{
		Il_Cmp_Type compare_type;
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

	struct Il_Node_AEP
	{
		Il_IDX ptr_node_idx;
		Il_IDX index_node_idx;
	};

	struct Il_Node_Proc_Address
	{
		Il_IDX proc_idx;
	};

#define SMALL_ARG_COUNT 16 / sizeof(Il_IDX)

	struct Il_Node_Call
	{
		Il_IDX proc_idx;
		u16 argument_count;
		Type_IDX signature;

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

	struct Il_Node_String
	{
		String str;
	};

	struct Il_Node_Cond_Branch
	{
		Il_IDX condition_node_idx;
		Il_IDX true_case_block_idx;
		Il_IDX false_case_block_idx;
	};

	struct Il_Node_Branch
	{
		Il_IDX block_idx;
	};

	struct Il_Node_Cast
	{
		Il_IDX castee_node_idx;
		Type_IDX from_type_idx;
		Il_Cast_Type cast_type;
	};

#define SI_SMALL_COUNT 16 / sizeof(Il_IDX)

	struct Il_Node_Struct_Initializer
	{
		u16 member_count;

		union
		{
			Il_IDX members_value_nodes[SI_SMALL_COUNT];
			Il_IDX* members_value_nodes_ptr;
		};
	};

	struct Il_Node_Global_Address
	{
		Il_IDX global_idx;
	};

	struct Il_Node_Array_Init
	{
		u32 element_count;
		Il_IDX* element_values;
	};

	struct Il_Node
	{
		Type_IDX type_idx;
		Il_Node_Type node_type;

		union
		{
			Il_Node_Constant			constant;
			Il_Node_Alloca				aloca;
			Il_Node_Load				load;
			Il_Node_Store				store;
			Il_Node_Bin_Op				math_op;
			Il_Node_Compare				cmp_op;
			Il_Node_Ret					ret;
			Il_Node_Struct_Element_Ptr  element_ptr;
			Il_Node_AEP					aep;
			Il_Node_Param				param;
			Il_Node_Call				call;
			Il_Node_String				string;
			Il_Node_Cond_Branch			c_branch;
			Il_Node_Branch				br;
			Il_Node_Cast				cast;
			Il_Node_Struct_Initializer  si;
			Il_Node_Array_Init			ai;
			Il_Node_Global_Address		global_address;
			Il_Node_Proc_Address		proc_address;
		};
	};

	struct Il_Block
	{
		String name;
		Array<Il_IDX> instructions;
	};

	struct Il_Global
	{
		String name;
		GS_Type* type = nullptr;
		Array<Il_Node> initializer_storage;
		Il_IDX initializer = (Il_IDX)-1;
		bool read_only = false;
		bool external = false;
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

		bool variadic = false;
		bool external = false;
		Il_IDX library;
	};

	struct Il_Library
	{
		String path;
	};

	struct Il_Program
	{
		Array<Il_Proc> procedures;
		Array<Il_Library> libraries;
		Array<Il_Global> globals;
	};

	inline Il_IDX Il_Insert_Global(Il_Program& prog, Il_Global global) {
		Array_Add(prog.globals, global);
		return (Il_IDX)(prog.globals.count - 1);
	}

	inline Il_Node Il_Make_Param(Il_IDX type_index, Il_IDX param_index) {

		Il_Node param_node = { 0 };
		param_node.type_idx = type_index;
		param_node.node_type = Il_Param;
		param_node.param.index = param_index;

		return param_node;
	}

	inline void Il_Program_Init(Il_Program& prog) {
		prog.procedures = Array_Reserved<Il_Proc>(1024);
	}

	inline Il_IDX Il_Proc_Insert(Il_Proc& proc, Il_Node node, Il_IDX block_idx = -1) {

		ASSERT(node.node_type != Il_Node_Type::Il_None, node.node_type < Il_Node_Type::Il_Max);

		Il_IDX node_idx = (Il_IDX)proc.instruction_storage.count;
		ASSERT(proc.insertion_point != -1);

		Il_IDX insertion_point = proc.insertion_point;

		bool at_entry = false;

		if (block_idx != -1) {
			insertion_point = block_idx;
			at_entry = true;
		}

		Array_Add(proc.instruction_storage, node);

		if (at_entry) {
			Array<Il_IDX> new_array;
			Array_Add(new_array, node_idx);

			//TODO: cleanup
			for (size_t i = 0; i < proc.blocks[insertion_point].instructions.count; i++)
			{
				Array_Add(new_array, proc.blocks[insertion_point].instructions[i]);
			}

			proc.blocks[insertion_point].instructions = new_array;
		}
		else {
			Array_Add(proc.blocks[insertion_point].instructions, node_idx);
		}

		return node_idx;
	}

	inline Il_IDX Il_Insert_Proc(Il_Program& prog, String name, GS_Type* signature, bool variadic = false) {

		Il_Proc proc = {};
		proc.proc_name = name;
		proc.instruction_storage = Array_Reserved<Il_Node>(1024 * 10);
		proc.blocks = Array_Reserved<Il_Block>(128);
		proc.program = &prog;
		proc.signature = signature;
		proc.variadic = variadic;
		proc.external = false;

		Il_Block entry_block = {};
		entry_block.instructions = Array_Reserved<Il_IDX>(128);
		entry_block.name = "entry";

		Array_Add(proc.blocks, entry_block);
		proc.insertion_point = (Il_IDX)(proc.blocks.count - 1);

		for (size_t i = 0; i < signature->proc.params.count; i++)
		{
			Il_IDX inserted_param_idx = Il_Proc_Insert(proc, Il_Make_Param((Il_IDX)get_type_index(signature->proc.params[i]), (Il_IDX)i));
			Array_Add(proc.parameters, inserted_param_idx);
		}

		Array_Add(prog.procedures, proc);

		return (Il_IDX)(prog.procedures.count - 1);
	}

	inline Il_IDX Il_Insert_Library(Il_Program& prog, String path) {

		Il_Library library = {};
		library.path = path;

		Array_Add(prog.libraries, library);

		return (Il_IDX)(prog.libraries.count - 1);
	}

	inline Il_IDX Il_Insert_External_Proc(Il_Program& prog, String name, GS_Type* signature, Il_IDX library_node_idx, bool variadic = false) {

		Il_Proc proc = {};
		proc.proc_name = name;
		proc.program = &prog;
		proc.signature = signature;
		proc.external = true;
		proc.library = library_node_idx;
		proc.variadic = variadic;

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

	inline Il_Node Il_Make_AEP(Type_IDX type_index, Il_IDX ptr_node_idx, Il_IDX index_node_idx) {

		Il_Node aep_node = { 0 };
		aep_node.node_type = Il_ArrayElementPtr;
		aep_node.type_idx = type_index;

		aep_node.aep.ptr_node_idx = ptr_node_idx;
		aep_node.aep.index_node_idx = index_node_idx;

		return aep_node;
	}

	inline Il_Node Il_Make_Zero_Init(Type_IDX type_index) {

		Il_Node zi_node = { 0 };
		zi_node.node_type = Il_ZI;
		zi_node.type_idx = type_index;

		return zi_node;
	}

	inline Il_Node Il_Make_Struct_Init(Type_IDX struct_type_index, Array<Il_IDX> member_values) {

		Il_Node si_node = { 0 };
		si_node.node_type = Il_Struct_Initializer;
		si_node.type_idx = struct_type_index;

		if (member_values.count <= SI_SMALL_COUNT) {
			for (size_t i = 0; i < member_values.count; i++)
			{
				si_node.si.members_value_nodes[i] = member_values[i];
			}
		}
		else {

			si_node.si.members_value_nodes_ptr = (Il_IDX*)malloc(member_values.count * sizeof(Il_IDX));

			for (size_t i = 0; i < member_values.count; i++)
			{
				si_node.si.members_value_nodes_ptr[i] = member_values[i];
			}
		}

		si_node.si.member_count = (u16)member_values.count;

		return si_node;
	}

	inline Il_Node Il_Make_Array_Init(Type_IDX struct_type_index, Array<Il_IDX> element_values) {

		Il_Node ai_node = { 0 };
		ai_node.node_type = Il_Array_Initializer;
		ai_node.type_idx = struct_type_index;
		ai_node.ai.element_count = element_values.count;

		ai_node.ai.element_values = (Il_IDX*)malloc(element_values.count * sizeof(Il_IDX));

		for (size_t i = 0; i < element_values.count; i++)
		{
			ai_node.ai.element_values[i] = element_values[i];
		}

		return ai_node;
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

	inline Il_Node Il_Make_Alloca(Type_IDX type_index, Type_IDX pointer_type_index) {

		Il_Node const_node = { 0 };
		const_node.node_type = Il_Alloca;
		const_node.aloca.type_idx = type_index;
		const_node.type_idx = pointer_type_index;

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

	inline Il_Node Il_Make_String(String string, Type_IDX type_idx) {

		Il_Node store_node = { 0 };
		store_node.node_type = Il_String;
		store_node.type_idx = type_idx;
		store_node.string.str = string;

		return store_node;
	}

	inline Il_Node Il_Make_Compare(Il_Node_Type cmp_node_type, Il_Cmp_Type compare_type, Type_IDX type_idx, Il_IDX left, Il_IDX right) {

		Il_Node store_node = { 0 };
		store_node.node_type = cmp_node_type;
		store_node.type_idx = type_idx;
		store_node.cmp_op.compare_type = compare_type;
		store_node.cmp_op.left_node_idx = left;
		store_node.cmp_op.right_node_idx = right;

		return store_node;
	}

	inline Il_Node Il_Make_Cast(Il_Cast_Type cast_type, Type_IDX type_idx, Type_IDX from_type_idx, Il_IDX castee_node_idx) {

		Il_Node cast_node = { 0 };
		cast_node.node_type = Il_Cast;
		cast_node.type_idx = type_idx;
		cast_node.cast.cast_type = cast_type;
		cast_node.cast.from_type_idx = from_type_idx;
		cast_node.cast.castee_node_idx = castee_node_idx;

		return cast_node;
	}

	inline Il_Node Il_Make_CBR(Type_IDX type_idx, Il_IDX condition_node_idx, Il_IDX true_case_block, Il_IDX false_case_block) {

		Il_Node cbr_node = { 0 };

		cbr_node.node_type = Il_Cond_Branch;
		cbr_node.type_idx = type_idx;
		cbr_node.c_branch.condition_node_idx = condition_node_idx;
		cbr_node.c_branch.true_case_block_idx = true_case_block;
		cbr_node.c_branch.false_case_block_idx = false_case_block;

		return cbr_node;
	}

	inline Il_Node Il_Make_Br(Il_IDX block_idx) {

		Il_Node br_node = { 0 };

		br_node.node_type = Il_Branch;
		br_node.type_idx = -1;
		br_node.br.block_idx = block_idx;

		return br_node;
	}

	inline Il_Node Il_Make_Math_Op(Il_Node_Type math_op_node_type, Type_IDX type_index, Il_IDX left, Il_IDX right) {

		if (math_op_node_type != Il_Add && math_op_node_type != Il_Sub && math_op_node_type != Il_Mul && math_op_node_type != Il_Div && math_op_node_type != Il_Bit_And && math_op_node_type != Il_Bit_Or) {
			GS_ASSERT_UNIMPL();
		}

		Il_Node math_op_node = { 0 };
		math_op_node.node_type = math_op_node_type;
		math_op_node.type_idx = type_index;
		math_op_node.math_op.left_node_idx = left;
		math_op_node.math_op.right_node_idx = right;

		return math_op_node;
	}

	inline Il_Node Il_Make_Call(Type_IDX return_type_index, Array<Il_Argument> arguments, Il_IDX proc_idx, Il_IDX signature_type_idx) {

		Il_Node call_node = { 0 };
		call_node.node_type = Il_Call;
		call_node.type_idx = return_type_index;
		call_node.call.signature = signature_type_idx;

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

	inline Il_Node Il_Make_Call_Ptr(Type_IDX return_type_index, Array<Il_Argument> arguments, Il_IDX ptr_node_idx, Type_IDX signature) {

		Il_Node call_node = { 0 };
		call_node.node_type = Il_Call_Ptr;
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
		call_node.call.proc_idx = ptr_node_idx;
		call_node.call.signature = signature;

		return call_node;
	}


	inline Il_Node Il_Make_Global_Address(Il_IDX global_idx, Type_IDX type_idx) {

		Il_Node ga_node = { 0 };

		ga_node.node_type = Il_Global_Address;
		ga_node.type_idx = type_idx;
		ga_node.global_address.global_idx = global_idx;

		return ga_node;
	}

	inline Il_Node Il_Make_Proc_Address(Il_IDX proc_idx, Type_IDX type_idx) {

		Il_Node proc_addr_node = { 0 };

		proc_addr_node.node_type = Il_Proc_Address;
		proc_addr_node.type_idx = type_idx;
		proc_addr_node.proc_address.proc_idx = proc_idx;

		return proc_addr_node;
	}

	inline Il_IDX Il_Insert_Alloca(Il_Proc& proc, GS_Type* type) {
		Il_Node alloca_node = Il_Make_Alloca((Il_IDX)get_type_index(type), (Il_IDX)get_type_index(get_pointer_type(type, 1)));
		return Il_Proc_Insert(proc, alloca_node, 0);
	}

	inline Il_IDX Il_Insert_Store(Il_Proc& proc, GS_Type* type, Il_IDX ptr, Il_IDX value) {
		Il_Node store_node = Il_Make_Store(ptr, value, (Il_IDX)get_type_index(type));
		return Il_Proc_Insert(proc, store_node);
	}

	inline Il_IDX Il_Insert_Load(Il_Proc& proc, GS_Type* type, Il_IDX ptr) {
		Il_Node load_node = Il_Make_Load(ptr, (Il_IDX)get_type_index(type));
		return Il_Proc_Insert(proc, load_node);
	}

	inline Il_IDX Il_Insert_Constant(Il_Proc& proc, Const_Union constant, GS_Type* type) {
		Il_Node const_node = Il_Make_Constant(constant, (Il_IDX)get_type_index(type));
		return Il_Proc_Insert(proc, const_node);
	}

	inline Il_IDX Il_Insert_Constant(Il_Proc& proc, void* constant, GS_Type* type) {

		Const_Union constant_value = {};
		constant_value.ptr = constant;

		Il_Node const_node = Il_Make_Constant(constant_value, (Il_IDX)get_type_index(type));
		return Il_Proc_Insert(proc, const_node);
	}

	inline Il_IDX Il_Insert_Math_Op(Il_Proc& proc, GS_Type* type, Il_Node_Type math_op_type, Il_IDX left, Il_IDX right) {
		Il_Node math_op_node = Il_Make_Math_Op(math_op_type, (Il_IDX)get_type_index(type), left, right);
		return Il_Proc_Insert(proc, math_op_node);
	}

	inline Il_IDX Il_Insert_Ret(Il_Proc& proc, GS_Type* type, Il_IDX value) {
		Il_Node ret_node = Il_Make_Ret((Il_IDX)get_type_index(type), value);
		return Il_Proc_Insert(proc, ret_node);
	}

	inline Il_IDX Il_Insert_Call(Il_Proc& proc, GS_Type* signature, Array<Il_Argument> arguments, Il_IDX callee_proc) {
		Il_Node call_node = Il_Make_Call((Il_IDX)get_type_index(signature->proc.return_type), arguments, callee_proc, (Il_IDX)get_type_index(signature));
		return Il_Proc_Insert(proc, call_node);
	}

	inline Il_IDX Il_Insert_Call_Ptr(Il_Proc& proc, GS_Type* signature, Array<Il_Argument> arguments, Il_IDX ptr_node_idx) {
		Il_Node call_ptr_node = Il_Make_Call_Ptr((Il_IDX)get_type_index(signature->proc.return_type), arguments, ptr_node_idx, (Il_IDX)get_type_index(signature));
		return Il_Proc_Insert(proc, call_ptr_node);
	}

	inline Il_IDX Il_Insert_Zero_Init(Il_Proc& proc, GS_Type* type) {
		Il_Node zi_node = Il_Make_Zero_Init((Il_IDX)get_type_index(type));
		return Il_Proc_Insert(proc, zi_node);
	}

	inline Il_IDX Il_Insert_Struct_Init(Il_Proc& proc, GS_Type* struct_type, Array<Il_IDX> members_values) {
		Il_Node si_node = Il_Make_Struct_Init((Il_IDX)get_type_index(struct_type), members_values);
		return Il_Proc_Insert(proc, si_node);
	}

	inline Il_IDX Il_Insert_String(Il_Proc& proc, GS_Type* a_pointer_type, String string) {
		ASSERT(a_pointer_type->kind == Type_Pointer);
		Il_Node string_node = Il_Make_String(string, (Il_IDX)get_type_index(a_pointer_type));
		return Il_Proc_Insert(proc, string_node);
	}

	inline Il_IDX Il_Insert_Compare(Il_Proc& proc, Il_Node_Type cmp_node_type, Il_Cmp_Type compare_type, GS_Type* type, Il_IDX left, Il_IDX right) {
		Il_Node cmp_node = Il_Make_Compare(cmp_node_type, compare_type, (Il_IDX)get_type_index(type), left, right);
		return Il_Proc_Insert(proc, cmp_node);
	}

	inline Il_IDX Il_Insert_Cast(Il_Proc& proc, Il_Cast_Type cast_type, GS_Type* to_type, GS_Type* from_type, Il_IDX castee_node_idx) {
		Il_Node cast_node = Il_Make_Cast(cast_type, (Type_IDX)get_type_index(to_type), (Type_IDX)get_type_index(from_type), castee_node_idx);
		return Il_Proc_Insert(proc, cast_node);
	}

	inline Il_IDX Il_Insert_CBR(Il_Proc& proc, GS_Type* type, Il_IDX condition_node_idx, Il_IDX true_case_block, Il_IDX false_case_block) {
		Il_Node cbr_node = Il_Make_CBR((Type_IDX)get_type_index(type), condition_node_idx, true_case_block, false_case_block);
		return Il_Proc_Insert(proc, cbr_node);
	}

	inline Il_IDX Il_Insert_Br(Il_Proc& proc, Il_IDX block_idx) {
		Il_Node br_node = Il_Make_Br(block_idx);
		br_node.type_idx = 0;
		return Il_Proc_Insert(proc, br_node);
	}

	inline Il_IDX Il_Insert_SEP(Il_Proc& proc, GS_Type* struct_type, u64 member_index, Il_IDX struct_pointer_node_idx) {
		Il_Node sep_node = Il_Make_Struct_Element_Ptr((Type_IDX)get_type_index(struct_type), member_index, struct_pointer_node_idx);
		GS_Struct& _struct = get_struct(struct_type);
		sep_node.element_ptr.offset = _struct.offsets[member_index];
		return Il_Proc_Insert(proc, sep_node);
	}

	inline Il_IDX Il_Insert_Global_Address(Il_Proc& proc, Il_IDX global_idx) {
		Il_Node ga_node = Il_Make_Global_Address(global_idx, (Type_IDX)get_type_index(proc.program->globals[global_idx].type));
		return Il_Proc_Insert(proc, ga_node);
	}

	inline Il_IDX Il_Insert_Proc_Address(Il_Proc& proc, Il_IDX proc_idx) {
		Il_Node proc_addr_node = Il_Make_Proc_Address(proc_idx, (Type_IDX)get_type_index(proc.program->procedures[proc_idx].signature));
		return Il_Proc_Insert(proc, proc_addr_node);
	}

	inline Il_IDX Il_Insert_AEP(Il_Proc& proc, GS_Type* type, Il_IDX ptr_node_idx, Il_IDX index_node_idx) {
		Il_Node aep_node = Il_Make_AEP((Type_IDX)get_type_index(type), ptr_node_idx, index_node_idx);
		return Il_Proc_Insert(proc, aep_node);
	}

	inline Il_IDX Il_Insert_Block(Il_Proc& proc, String name = {}) {

		Il_Block block = {};
		block.instructions = Array_Reserved<Il_IDX>(128);
		block.name = name;

		Array_Add(proc.blocks, block);
		proc.insertion_point = (Il_IDX)(proc.blocks.count - 1);

		return (Il_IDX)proc.insertion_point;
	}

	inline void Il_Set_Insert_Point(Il_Proc& proc, Il_IDX insert_point) {
		ASSERT((Il_IDX)proc.blocks.count > insert_point, "insert point out of range");
		proc.insertion_point = insert_point;
	}

	std::string Il_Print_Proc(Il_Proc& proc);

#define REG_BUF_SZ 4091
#define STACK_SZ 512

	struct Execution_Engine {
		Type_System* type_system = nullptr;
		Il_Program* program = nullptr;
		Array<void*> library_handles;
		Array<void*> proc_pointers;

		void* globals_storage = nullptr;
		Array<void*> globals_address_table;
	};

	Const_Union EE_Exec_Program(Execution_Engine& ee, Il_Program* prog, Il_IDX entry);
	Const_Union EE_Exec_Proc(Execution_Engine& ee, Il_Proc& proc, Array<Const_Union> arguments, Array<GS_Type*> argument_types = {});
	Const_Union EE_Exec_External_Proc(Execution_Engine& ee, Il_Proc& proc, Array<Const_Union> arguments, Array<GS_Type*> argument_types = {});
	Const_Union EE_Dynamic_Invoke_Proc(Execution_Engine& ee, void* proc_pointer, GS_Type* signature, bool variadic, Array<Const_Union> arguments, Array<GS_Type*> argument_types = {});
}