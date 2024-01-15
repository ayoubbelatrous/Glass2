#pragma once

#include "Base/String.h"
#include "Base/Array.h"
#include "Base/Hash.h"
#include "Application.h"
#include "FrontEnd/Parser.h"

namespace Glass
{
	enum Type_Name_Flag : u64
	{
		TN_Base_Type = BIT(0),
		TN_Numeric_Type = BIT(1),
		TN_Float_Type = BIT(2),
		TN_Unsigned_Type = BIT(3),
		TN_Struct_Type = BIT(4),
	};

	using Type_Name_ID = u32;
	using Struct_ID = u32;
	using Type_Name_Flags = u64;

	struct Type_Name
	{
		String name;
		Type_Name_Flags flags = 0;
		u64 size = -1;
		u64 alignment = -1;
		Struct_ID struct_id = -1;
	};

	struct GS_Type;

	struct GS_Basic_Type
	{
		Type_Name_ID type_name_id;
	};

	struct GS_Pointer_Type
	{
		GS_Type* pointee;
		u32 indirection;
	};

	struct GS_Array_Type
	{
		GS_Type* element_type;
		u64 size;
	};

	struct GS_Dyn_Array_Type
	{
		GS_Type* element_type;
	};

	struct GS_Struct_Type
	{
		Array<GS_Type*> members;
	};

	enum Type_Kind
	{
		Type_Basic,
		Type_Pointer,
		Type_Array,
		Type_Dyn_Array,
	};

	struct GS_Type
	{
		GS_Type() = default;

		Type_Kind kind;
		u64 type_hash;

		union
		{
			GS_Basic_Type		 basic;
			GS_Pointer_Type		 pointer;
			GS_Array_Type		 array;
			GS_Dyn_Array_Type	 dyn_array;
		};
	};

	struct GS_Struct
	{
		Array<GS_Type*> members;
		Array<u64> offsets;
	};

	struct Type_System
	{
		Array<Type_Name> type_name_storage;
		Array<GS_Struct> struct_storage;
		Array<GS_Type> type_storage;
		std::unordered_map<u64, GS_Type*> type_lookup;
	};

	struct GS_Struct_Data_Layout
	{
		u64 size;
		u64 alignment;
		Array<u64> offsets;
	};

	inline u64 TypeSystem_Get_Type_Index(Type_System& ts, GS_Type* type) {
		return ((u64)ts.type_storage.data - (u64)type) / sizeof(GS_Type);
	}

	inline void TypeSystem_Init(Type_System& ts) {
		ts.type_name_storage = Array_Reserved<Type_Name>(1024 * 10);
		ts.type_storage = Array_Reserved<GS_Type>(1024 * 100);
	}

	inline Type_Name_ID TypeSystem_Insert_TypeName(Type_System& ts, Type_Name type_name) {
		Array_Add(ts.type_name_storage, type_name);
		return (Type_Name_ID)(ts.type_name_storage.count - 1);
	}

	inline Type_Name_ID TypeSystem_Insert_TypeName_Struct(Type_System& ts, Type_Name type_name, GS_Struct strct) {
		type_name.struct_id = (Type_Name_ID)ts.struct_storage.count;
		Array_Add(ts.type_name_storage, type_name);
		Array_Add(ts.struct_storage, strct);
		return (Type_Name_ID)(ts.type_name_storage.count - 1);
	}

	inline u64 GS_Basic_Type_Hash(Type_Name_ID type_name_id) {
		return Combine2Hashes(std::hash<u32>{}(type_name_id), std::hash<std::string>{}("basic_type"));
	}

	inline u64 GS_Pointer_Type_Hash(u64 pointee_hash, u32 indirection) {
		return Combine2Hashes(pointee_hash, std::hash<u32>{}(indirection));
	}

	inline GS_Type* TypeSystem_Get_Basic_Type(Type_System& ts, Type_Name_ID type_name_id) {

		Type_Name tn = ts.type_name_storage[type_name_id]; // check it exists

		u64 type_hash = GS_Basic_Type_Hash(type_name_id);

		auto it = ts.type_lookup.find(type_hash);
		if (it != ts.type_lookup.end()) {
			ASSERT(it->second->kind == Type_Basic);
			return it->second;
		}

		GS_Type new_type;
		new_type.kind = Type_Basic;
		new_type.type_hash = type_hash;
		new_type.basic.type_name_id = type_name_id;

		GS_Type* inserted_type = Array_Add(ts.type_storage, new_type);
		ts.type_lookup[type_hash] = inserted_type;

		return inserted_type;
	}

	inline GS_Type* TypeSystem_Get_Pointer_Type(Type_System& ts, GS_Type* pointee, u32 indirection) {

		ASSERT(pointee);
		ASSERT(indirection);
		ASSERT(pointee->kind != Type_Pointer);

		u64 type_hash = GS_Pointer_Type_Hash(pointee->type_hash, indirection);

		auto it = ts.type_lookup.find(type_hash);
		if (it != ts.type_lookup.end()) {
			ASSERT(it->second->kind == Type_Pointer);
			return it->second;
		}

		GS_Type new_type;
		new_type.kind = Type_Pointer;
		new_type.type_hash = type_hash;
		new_type.pointer.pointee = pointee;
		new_type.pointer.indirection = indirection;

		GS_Type* inserted_type = Array_Add(ts.type_storage, new_type);
		ts.type_lookup[type_hash] = inserted_type;

		return inserted_type;
	}

	inline Type_Name_Flags TypeSystem_Get_Type_Flags(Type_System& ts, GS_Type* type) {
		if (type->kind == Type_Basic) {
			return ts.type_name_storage[type->basic.type_name_id].flags;
		}
		else {
			return 0;
		}
	}

	inline Type_Name_Flags TypeSystem_Get_Type_Alignment(Type_System& ts, GS_Type* type) {
		if (type->kind == Type_Basic) {
			return ts.type_name_storage[type->basic.type_name_id].alignment;
		}
		else if (type->kind == Type_Pointer) {
			return 8;
		}
		else {
			ASSERT(nullptr);
		}
	}

	inline Type_Name_Flags TypeSystem_Get_Type_Size(Type_System& ts, GS_Type* type) {
		if (type->kind == Type_Basic) {
			return ts.type_name_storage[type->basic.type_name_id].size;
		}
		else if (type->kind == Type_Pointer) {
			return 8;
		}
		else {
			ASSERT(nullptr);
		}
	}

	inline GS_Struct_Data_Layout TypeSystem_Struct_Compute_Align_Size_Offsets(Type_System& ts, Array<GS_Type*> members) {

		GS_Struct_Data_Layout dl;

		dl.alignment = 1;
		dl.offsets = Array_Reserved<u64>(members.count);
		dl.offsets.count = members.count;

		for (size_t i = 0; i < members.count; i++)
		{
			GS_Type* member = members[i];
			u64 member_size = TypeSystem_Get_Type_Size(ts, member);
			u64 member_alignment = TypeSystem_Get_Type_Alignment(ts, member);

			if (member_alignment > dl.alignment) {
				dl.alignment = member_alignment;
			}
		}

		u64 offset = 0;

		for (size_t i = 0; i < members.count; i++)
		{
			GS_Type* member = members[i];
			u64 member_size = TypeSystem_Get_Type_Size(ts, member);
			u64 member_alignment = TypeSystem_Get_Type_Alignment(ts, member);

			dl.offsets[i] = offset;

			if ((offset + member_size) % member_alignment != 0) {
				i64 padding = (member_alignment - ((offset + member_size) % member_alignment)) % member_alignment;
				offset += padding;
				dl.offsets[i] = offset;
				offset += member_size;
			}
			else {
				offset += member_size;
			}
		}

		dl.size = offset;

		i64 finalPadding = (dl.alignment - ((dl.size) % dl.alignment)) % dl.alignment;
		dl.size += finalPadding;

		return dl;
	}

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

	inline Const_Union EE_Exec_Proc(Execution_Engine& ee, Il_Proc& proc) {

		GS_CORE_ASSERT(ee.type_system);

		Const_Union returned_value = { 0 };

		for (size_t bidx = 0; bidx < proc.blocks.count; bidx++)
		{
			Il_Block& block = proc.blocks[bidx];

			for (size_t i = 0; i < block.instructions.count; i++)
			{
				Il_Node node = proc.instruction_storage[block.instructions[i]];

				Type_Name_Flags type_flags = TypeSystem_Get_Type_Flags(*ee.type_system, &ee.type_system->type_storage[node.type_idx]);

				switch (node.node_type)
				{
				case Il_Const: {
					ee.register_buffer[i].us8 = node.constant.as.us8;
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
					returned_value.us8 = ee.register_buffer[node.ret.value_node_idx].us8;
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

	struct Source_Loc
	{
		u32 line = -1;
		u16 column = -1;
	};

	using Entity_ID = u32;
	constexpr Entity_ID Entity_Null = -1;

	enum class Entity_Type
	{
		Global_Scope,
		File_Scope,
		File_Load,
		Named_File_Load,
		Constant,

		Type_Name_Entity,

		Struct_Entity,
		Struct_Member_Entity,
	};

	enum Entity_Flag : u64
	{
		EF_InComplete = BIT(0),
		EF_Constant = BIT(1),
	};

	using Entity_Flags = u64;

	using File_ID = u64;
	constexpr File_ID File_Null = -1;

	struct Entity_File_Scope
	{
		File_ID file_id;
	};

	struct Entity_File_Load
	{
		File_ID loaded_file_id;
		Entity_ID file_scope_entity_id;
	};

	struct Entity_Named_File_Load
	{
		Entity_ID file_load_entity_id;
	};

	struct Entity_Constant
	{
		Const_Union value_as;
	};

	struct Entity_TypeName
	{
		Type_Name_ID type_name_id;
	};

	struct Entity_Struct
	{
		Type_Name_ID type_name_id;
	};

	struct Entity
	{
		String semantic_name;

		Array<Entity_ID> children;
		Entity_ID parent = Entity_Null;

		Statement* syntax_node = nullptr;
		Source_Loc source_location;
		File_ID definition_file = File_Null;

		Entity_Type entity_type;
		Entity_Flags flags = (Entity_Flags)0;

		Array<Entity_ID> dependencies;

		GS_Type* semantic_type = nullptr;
		Const_Union constant_value;

		union
		{
			Entity_File_Scope			file_scope;
			Entity_File_Load			file_load;
			Entity_Named_File_Load		named_file_load;
			Entity_Constant				constant;
			Entity_TypeName				type_name;
			Entity_Struct				struct_entity;
		};
	};

	enum Front_End_Message_Type
	{
		Message_Info,
		Message_Warning,
		Message_Error,
	};

	struct Front_End_File {
		fs_path Absolute_Path;
		fs_path Path;
		std::vector<Token> Tokens;
		ModuleFile* Syntax;
	};

	struct Front_End_Message
	{
		std::string Message;
		Front_End_Message_Type Message_Type;
	};

	struct Front_End_Data
	{
		Array<Front_End_File> Files;
		std::unordered_map<std::string, File_ID> Path_To_File;
		std::unordered_map<File_ID, Entity_ID> File_ID_To_Scope;

		Array<Front_End_Message> Messages;

		Array<Entity> entity_storage;
		Entity_ID global_scope_entity = Entity_Null;

		Type_System type_system;

		Type_Name_ID Type_tn;

		Type_Name_ID void_tn;

		Type_Name_ID int_tn;

		Type_Name_ID i8_tn;
		Type_Name_ID i16_tn;
		Type_Name_ID i32_tn;
		Type_Name_ID i64_tn;

		Type_Name_ID u8_tn;
		Type_Name_ID u16_tn;
		Type_Name_ID u32_tn;
		Type_Name_ID u64_tn;

		Type_Name_ID float_tn;
		Type_Name_ID f32_tn;
		Type_Name_ID f64_tn;

		GS_Type* Type_Ty = nullptr;
		GS_Type* void_Ty = nullptr;

		Il_Program il_program;

		Execution_Engine exec_engine;

		static Entity_ID Get_Top_Most_Parent(Front_End_Data& data, Entity_ID entity_id);
		static Entity_ID Get_File_Scope_Parent(Front_End_Data& data, Entity_ID entity_id);
		static Entity_ID Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID parent, String name, Entity_ID visited_load);
		static Entity_ID Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID parent, String name);
	};

	struct CodeGen_Result {
		GS_Type* expression_type = nullptr;
		u16 code_node_id = (u16)-1;
		bool ok = false;

		operator bool() {

			if (code_node_id == -1 && ok) {
				ASSERT(nullptr);
			}

			if (code_node_id != -1 && !ok) {
				ASSERT(nullptr);
			}

			return ok;
		}
	};

	class Front_End
	{
	public:
		Front_End(ApplicationOptions options);

		void Compile();
		void Load_First();
		void Do_Load_Pass(Entity_ID entity_id);
		void Do_Constants_Passes();

		CodeGen_Result Expression_CodeGen(Expression* expression, Entity_ID scope_id, Il_Proc& proc, GS_Type* inferred_type);

		bool Iterate_Tl_All_Files(std::function<bool(File_ID file_id, Entity_ID file_entity_id, Entity& file_entity, Statement* syntax)> f);
		bool Iterate_All_Files(std::function<bool(File_ID file_id, Entity_ID file_entity_id, Entity& file_entity)> f);

		bool Check_Circular_Dependencies(Entity_ID entity_id, Array<Entity_ID> dependencies, Array<Entity_ID>& chain);
		bool Do_Expression_Dependecy_Pass(Entity_ID scope_id, Expression* expr, Array<Entity_ID>& dependencies, Entity_ID ignore_indirect);

		Entity Do_Load(Entity_ID entity_id, LoadNode* load_node);

		void Push_Error(const std::string& error);
		void Push_Warn(const std::string& warn);
		void Push_Error_Loc(File_ID file_id, Statement* stmt, const std::string& error);

		File_ID Generate_File(const fs_path& path, const fs_path& absolute_path);

		Entity_ID Insert_Entity(Entity entity, Entity_ID parent = Entity_Null);

		Source_Loc Loc_From_Token(const Token& tk);

		Entity Create_File_Scope_Entity(File_ID file_id, Statement* syntax);
		Entity Create_File_Load_Entity(File_ID loaded_file_id, File_ID file_id);

		Entity Create_File_Named_Load_Entity(Entity_ID file_load_entity_id, String name, Source_Loc source_location, File_ID file_id);

		Entity Create_Constant_Entity(Const_Union value, String name, Source_Loc source_location, File_ID file_id);

		Entity Create_TypeName_Entity(String name, Source_Loc source_location, File_ID file_id);

		Entity Create_Struct_Entity(String name, Source_Loc source_location, File_ID file_id);
		Entity Create_Struct_Member_Entity(String name, Source_Loc source_location, File_ID file_id);

		Front_End_Data Data;
		ApplicationOptions Options;
	};
}