#pragma once

#include "Base/String.h"
#include "Base/Array.h"
#include "Application.h"
#include "FrontEnd/Parser.h"

namespace Glass
{
	enum Il_Node_Type : u8
	{
		Il_Alloca,
		Il_Load,
		Il_Store,
	};

	struct Il_Node_Alloca
	{
		u16 type_idx;
	};

	struct Il_Node_Load
	{
		u16 type_idx;
		u16 ptr_node_idx;
	};

	struct Il_Node_Store
	{
		u16 type_idx;
		u16 ptr_node_idx;
		u16 value_node_idx;
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

		void* ptr;
	};

	struct Il_Node_Constant
	{
		Const_Union as;
		u16 type_idx;
	};

	struct Il_Node
	{
		union
		{
			Il_Node_Constant constant;
			Il_Node_Alloca alloca;
			Il_Node_Load load;
			Il_Node_Store store;

			Il_Node_Type node_type;
		};
	};

	struct Source_Loc
	{
		u32 line = -1;
		u16 column = -1;
	};

	using Entity_ID = u32;
	constexpr Entity_ID Entity_Null = -1;

	enum class Entity_Type
	{
		File_Scope,
		File_Load,
		Named_File_Load,
		Constant,
	};

	using File_ID = u64;
	constexpr File_ID File_Null = -1;

	struct Entity_File_Scope
	{
		File_ID file_id;
	};

	struct Entity_File_Load
	{
		File_ID loaded_file_id;
	};

	struct Entity_Named_File_Load
	{
		Entity_ID file_load_entity_id;
	};

	struct Entity_Constant
	{
		Const_Union value_as;
	};

	struct Entity
	{
		String semantic_name;

		Array<Entity_ID> children;
		Entity_ID parent = Entity_Null;

		Source_Loc source_location;

		Entity_Type entity_type;

		union
		{
			Entity_File_Scope			file_scope;
			Entity_File_Load			file_load;
			Entity_Named_File_Load		named_file_load;
			Entity_Constant				constant;
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

		Array<Front_End_Message> Messages;

		Array<Entity> entity_storage;
	};

	class Front_End
	{
	public:
		Front_End(ApplicationOptions options);

		void Compile();
		void Load_First();
		void Do_Load_Pass(Entity_ID entity_id);
		void Do_Constants_Passes();
		Entity Do_Load(Entity_ID entity_id, LoadNode* load_node);

		void Push_Error(const std::string& error);
		void Push_Warn(const std::string& warn);
		void Push_Error_Loc(File_ID file_id, Statement* stmt, const std::string& error);

		File_ID Generate_File(const fs_path& path, const fs_path& absolute_path);

		Entity_ID Insert_Entity(Entity entity, Entity_ID parent = Entity_Null);

		Source_Loc Loc_From_Token(const Token& tk);

		Entity Create_File_Scope_Entity(File_ID file_id);
		Entity Create_File_Load_Entity(File_ID loaded_file_id);

		Entity Create_File_Named_Load_Entity(Entity_ID file_load_entity_id, String name, Source_Loc source_location);

		Entity Create_Constant_Entity(Const_Union value, String name, Source_Loc source_location);

		Front_End_Data Data;
		ApplicationOptions Options;
	};
}