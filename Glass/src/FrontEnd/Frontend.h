#pragma once

#include "Base/String.h"
#include "Base/Array.h"
#include "Base/Hash.h"
#include "Application.h"
#include "FrontEnd/Parser.h"
#include "FrontEnd/Type_System.h"
#include "BackEnd/Il.h"
#include <unordered_set>

namespace Glass
{
	struct Source_Loc
	{
		u32 line = (u32)-1;
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

		Enum_Entity,
		Enum_Member_Entity,

		Function,
		Function_Scope,

		Variable,

		Library
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

	struct Entity_Enum
	{
		Type_Name_ID type_name_id;
		bool flags_enum;
	};

	struct Entity_Enum_Member
	{
		Const_Union value;
	};

	struct Function_Parameter
	{
		String name;
		GS_Type* param_type;
	};

	struct Entity_Function
	{
		Array_UI<Function_Parameter> parameters;
		GS_Type* signature;
		Il_IDX proc_idx;

		bool foreign;
		bool c_variadic;
		Entity_ID library_entity_id;

		Il_IDX return_data_ptr_node_idx;
	};

	struct Entity_Variable
	{
		bool global;
		bool immutable;
		bool parameter;
		Il_IDX location;
		GS_Type* code_type;
	};

	struct Entity_Library
	{
		String path;
		Il_IDX library_node_idx;
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

		std::unordered_map<std::string_view, Entity_ID> children_lookup;

		union
		{
			Entity_File_Scope			file_scope;
			Entity_File_Load			file_load;
			Entity_Named_File_Load		named_file_load;
			Entity_Constant				constant;
			Entity_TypeName				type_name;
			Entity_Struct				struct_entity;
			Entity_Enum					enum_entity;
			Entity_Enum_Member			enum_member;
			Entity_Function				func;
			Entity_Variable				var;
			Entity_Library				library;
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
		std::set<std::string> Added_Libraries_Paths;

		Array<Front_End_File> Files;
		std::unordered_map<std::string, File_ID> Path_To_File;
		std::unordered_map<File_ID, Entity_ID> File_ID_To_Scope;
		std::unordered_map<Type_Name_ID, Entity_ID> typename_to_entity_id;

		std::unordered_map<std::string_view, Array<Entity_ID>> name_to_entities;

		Array<Front_End_Message> Messages;

		Array<Entity> entity_storage;
		Entity_ID global_scope_entity = Entity_Null;

		Type_System type_system;

		Type_Name_ID Type_tn;

		Type_Name_ID void_tn;

		Type_Name_ID bool_tn;

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

		GS_Type* int_Ty = nullptr;
		GS_Type* float_Ty = nullptr;

		GS_Type* i8_Ty = nullptr;
		GS_Type* i16_Ty = nullptr;
		GS_Type* i32_Ty = nullptr;
		GS_Type* i64_Ty = nullptr;

		GS_Type* u8_Ty = nullptr;
		GS_Type* u16_Ty = nullptr;
		GS_Type* u32_Ty = nullptr;
		GS_Type* u64_Ty = nullptr;

		GS_Type* f32_Ty = nullptr;
		GS_Type* f64_Ty = nullptr;

		GS_Type* Type_Ty = nullptr;
		GS_Type* void_Ty = nullptr;
		GS_Type* void_ptr_Ty = nullptr;
		GS_Type* bool_Ty = nullptr;

		GS_Type* string_Ty = nullptr;
		GS_Type* Array_Ty = nullptr;
		GS_Type* Any_Ty = nullptr;

		Type_Name_ID TypeInfo_tn = -1;
		GS_Type* TypeInfo_Ty = nullptr;
		GS_Type* TypeInfo_Ptr_Ty = nullptr;

		Il_Program il_program;

		Execution_Engine exec_engine;

		u64 parse_time_micro_seconds = 0;
		u64 lex_time_micro_seconds = 0;
		u64 entity_search_time = 0;
		u64 entity_search_loop_count = 0;

		static Entity_ID Get_Top_Most_Parent(Front_End_Data& data, Entity_ID entity_id);
		static Entity_ID Get_File_Scope_Parent(Front_End_Data& data, Entity_ID entity_id);
		static Entity_ID Get_Func_Parent(Front_End_Data& data, Entity_ID entity_id);
		static Entity_ID Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID parent, String name, Entity_ID visited_load, Entity_ID visited_parent);
		static Entity_ID Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID parent, String name);
	};

	struct CodeGen_Result {
		GS_Type* expression_type = nullptr;
		u16 code_node_id = (u16)-1;
		Entity_ID entity_reference = Entity_Null;
		bool lvalue = false;
		bool constant = false;
		bool immutable = false;
		bool ok = false;

		operator bool() {
			// 
			// 			if (code_node_id == -1 && ok) {
			// 				ASSERT(nullptr);
			// 			}
			// 
			// 			if (code_node_id != -1 && !ok) {
			// 				ASSERT(nullptr);
			// 			}

			return ok;
		}
	};

	struct Iterator_Result {

		bool ok;

		GS_Type* it_type = nullptr;

		Il_IDX it_index_location_node = -1;
		Il_IDX it_location_node = -1;
		Il_IDX condition_node = -1;

		operator bool() {
			return ok;
		}
	};

	struct Eval_Result {
		bool ok = false;
		GS_Type* expression_type = nullptr;
		Entity_ID entity_reference = Entity_Null;
		Const_Union result;

		operator bool() {
			return ok;
		}
	};

	struct Dep_Result {
		bool ok = false;
		Entity_ID referenced_entity = Entity_Null;

		operator bool() {
			return ok;
		}
	};

	class Front_End
	{
	public:
		Front_End(ApplicationOptions options);

		void Compile();
		void Generate_Output();
		bool Load_Base();
		void Load_First();
		void Do_Load_Pass(Entity_ID entity_id);

		bool Do_Tl_Definition_Passes();
		bool Do_Tl_Dependency_Passes();
		bool Do_Tl_Resolution_Passes();
		bool Do_CodeGen();

		bool Generate_TypeInfoTable();

		bool Foreign_Function_CodeGen(Entity& function_entity, Entity_ID func_entity_id, Entity_ID scope_id);
		CodeGen_Result Function_CodeGen(Entity& function_entity, Entity_ID func_entity_id, Entity_ID scope_id);
		CodeGen_Result Statement_CodeGen(Statement* statement, Entity_ID scope_id, Il_Proc& proc);
		CodeGen_Result Expression_CodeGen(Expression* expression, Entity_ID scope_id, Il_Proc& proc, GS_Type* inferred_type = nullptr, bool by_reference = false, bool is_condition = false);

		Iterator_Result Iterator_CodeGen(Expression* expression, Entity_ID scope_id, Il_Proc& proc, Il_IDX before_condition_block, Il_IDX condition_block, Il_IDX after_body_block);

		Eval_Result Expression_Evaluate(Expression* expression, Entity_ID scope_id, GS_Type* inferred_type, bool const_eval = true);

		GS_Type* Evaluate_Type(Expression* expression, Entity_ID scope_id);

		bool Iterate_Tl_All_Files(std::function<bool(File_ID file_id, Entity_ID file_entity_id, Entity& file_entity, Statement* syntax)> f);
		bool Iterate_All_Files(std::function<bool(File_ID file_id, Entity_ID file_entity_id, Entity& file_entity)> f);

		bool Check_Circular_Dependencies(Entity_ID entity_id, Array<Entity_ID> dependencies, Array<Entity_ID>& chain);
		Dep_Result Do_Expression_Dependecy_Pass(Entity_ID scope_id, Expression* expr, Array<Entity_ID>& dependencies, Entity_ID ignore_indirect);

		Entity Do_Load(Entity_ID entity_id, LoadNode* load_node);

		void Push_Error(const std::string& error);
		void Push_Warn(const std::string& warn);
		void Push_Error_Loc(Entity_ID scope_id, Statement* stmt, const std::string& error);

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

		Entity Create_Enum_Entity(String name, Source_Loc source_location, File_ID file_id);
		Entity Create_Enum_Member_Entity(String name, Source_Loc source_location, File_ID file_id);

		Entity Create_Function_Entity(String name, Source_Loc source_location, File_ID file_id);
		Entity Create_Function_Scope_Entity(String name, Source_Loc source_location, File_ID file_id);
		Entity Create_Variable_Entity(String name, Source_Loc source_location, File_ID file_id);

		Entity Create_Library_Entity(String name, Source_Loc source_location, File_ID file_id);

		Front_End_Data Data;
		ApplicationOptions Options;

		Il_IDX main_proc_idx;

		bool function_returns = false;
		bool terminator_encountered = false;

		Array<Il_IDX> return_branches;
		Il_IDX return_storage_node_id = -1;
		Il_IDX global_type_info_table_idx = -1;
		Il_IDX struct_member_typeinfo_global_idx = -1;
		GS_Type* te_Ty = nullptr;

		bool print_il = true;
		std::stringstream printed_il_stream;
	};
}