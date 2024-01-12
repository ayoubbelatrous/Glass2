#include "pch.h"

#include "FrontEnd/Frontend.h"
#include "FrontEnd/Lexer.h"

#define FMT(...) fmt::format(__VA_ARGS__)

namespace Glass
{
	std::string normalizePath(const std::string& messyPath) {
		std::filesystem::path path(messyPath);
		std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
		std::string npath = canonicalPath.make_preferred().string();
		return npath;
	}

	fs_path normalizePath(const fs_path& messyPath) {
		fs_path path(messyPath);
		fs_path canonicalPath = std::filesystem::weakly_canonical(path);
		return canonicalPath.make_preferred();
	}

	Front_End::Front_End(ApplicationOptions options)
		: Options(options)
	{
	}

	void Front_End::Compile()
	{
		// to avoid pointers to these pools get invalidated by insertions
		Data.Files = Array_Reserved<Front_End_File>(1024);
		Data.entity_storage = Array_Reserved<Entity>(1024 * 10 * 10); // about 10 megs

		Load_First();
		Do_Constants_Passes();
	}

	void Front_End::Load_First()
	{
		if (Options.Files.size() == 0) {
			Push_Error("you need to provide a start file");
		}

		if (Options.Files.size() > 1) {
			Push_Error("you only need to provide 1 start file");
		}

		fs_path first_file_path_abs = std::filesystem::absolute(Options.Files[0]);
		File_ID first_file_id = Generate_File(Options.Files[0], first_file_path_abs);

		if (first_file_id == File_Null) {
			ASSERT(nullptr, "failed to load the first file");
		}

		Entity_ID first_file_entity_id = Insert_Entity(Create_File_Scope_Entity(first_file_id));

		Do_Load_Pass(first_file_entity_id);

		int x = 0;
	}

	void Front_End::Do_Load_Pass(Entity_ID entity_id)
	{
		ASSERT(entity_id != Entity_Null);

		Entity& file_entity = Data.entity_storage[entity_id];
		ASSERT(file_entity.entity_type == Entity_Type::File_Scope);

		Front_End_File& file = Data.Files[file_entity.file_scope.file_id];

		for (size_t i = 0; i < file.Syntax->Statements.size(); i++)
		{
			Statement* statement = file.Syntax->Statements[i];

			if (statement->GetType() == NodeType::Load) {

				LoadNode* as_load = (LoadNode*)statement;
				Entity load_entity = Do_Load(entity_id, as_load);
				Insert_Entity(load_entity, entity_id);
			}
		}
	}

	Entity Front_End::Do_Load(Entity_ID entity_id, LoadNode* load_node)
	{
		Entity& file_entity = Data.entity_storage[entity_id];
		ASSERT(file_entity.entity_type == Entity_Type::File_Scope);

		fs_path path = load_node->FileName->Symbol.Symbol;
		fs_path path_abs = normalizePath(std::filesystem::absolute(path));

		File_ID loaded_file_id = File_Null;

		auto previous_loaded_file_it = Data.Path_To_File.find(path_abs.string());
		if (previous_loaded_file_it == Data.Path_To_File.end()) {

			if (!std::filesystem::exists(path_abs)) {
				Push_Error_Loc(file_entity.file_scope.file_id, load_node, FMT("1oad directive file not found!: '{}'", path.string()));
			}

			loaded_file_id = Generate_File(path, path_abs);
			Entity_ID loaded_file_entity_id = Insert_Entity(Create_File_Scope_Entity(loaded_file_id));

			Do_Load_Pass(loaded_file_entity_id);
		}
		else {
			loaded_file_id = previous_loaded_file_it->second;
		}

		return Create_File_Load_Entity(loaded_file_id);
	}

	void Front_End::Do_Constants_Passes()
	{
		//@DefPass
		for (size_t i = 0; i < Data.entity_storage.count; i++)
		{
			if (Data.entity_storage[i].entity_type == Entity_Type::File_Scope) {
				Entity_ID file_entity_id = (Entity_ID)i;

				Entity& file_entity = Data.entity_storage[file_entity_id];
				Front_End_File& file = Data.Files[file_entity.file_scope.file_id];

				std::vector<Statement*> statements = file.Syntax->Statements;

				for (size_t i = 0; i < statements.size(); i++)
				{
					Statement* statement = statements[i];

					if (statement->GetType() == NodeType::Variable) {

						VariableNode* as_var = (VariableNode*)statement;

						if (as_var->Constant) {

							if (as_var->Assignment->GetType() == NodeType::Load) {
								Entity entity_file_load = Do_Load(file_entity_id, (LoadNode*)as_var->Assignment);
								GS_CORE_ASSERT(entity_file_load.entity_type == Entity_Type::File_Load);

								Entity_ID entity_file_load_id = Insert_Entity(entity_file_load);
								Insert_Entity(
									Create_File_Named_Load_Entity(
										entity_file_load_id,
										String_Make(as_var->Symbol.Symbol),
										Loc_From_Token(as_var->Symbol)),
									file_entity_id);
								int x = 0;
							}
						}
					}
				}
			}
		}
	}

	void Front_End::Push_Error(const std::string& error)
	{
		Array_Add(Data.Messages, Front_End_Message{ error, Message_Error });
	}

	void Front_End::Push_Warn(const std::string& warn)
	{
		Array_Add(Data.Messages, Front_End_Message{ warn, Message_Warning });
	}

	void Front_End::Push_Error_Loc(File_ID file_id, Statement* stmt, const std::string& error)
	{
		ASSERT(file_id != File_Null);

		const auto& token = stmt->GetLocation();

		Push_Error(fmt::format("{}:{}:{}", Data.Files[file_id].Path.string(), token.Line + 1, token.Begin));
		Push_Warn(error);
	}

	File_ID Front_End::Generate_File(const fs_path& path, const fs_path& absolute_path)
	{
		Front_End_File file;
		file.Path = normalizePath(path);
		file.Absolute_Path = normalizePath(absolute_path);

		std::string source;

		{
			std::ifstream in(file.Path);
			std::stringstream buffer;
			buffer << in.rdbuf();
			source = buffer.str();
		}

		Lexer lexer = Lexer(source, file.Path);
		file.Tokens = lexer.Lex();

		Parser parser = Parser(file.Path, file.Tokens);
		file.Syntax = parser.CreateAST();

		File_ID file_identifier = Data.Files.count;

		Array_Add(Data.Files, std::move(file));
		Data.Path_To_File[normalizePath(absolute_path).string()] = file_identifier;

		return file_identifier;
	}

	Entity_ID Front_End::Insert_Entity(Entity entity, Entity_ID parent_id /*= Entity_Null*/)
	{
		Entity_ID entity_identifier = (Entity_ID)Data.entity_storage.count;

		if (parent_id != Entity_Null) {
			Entity& parent = Data.entity_storage[parent_id];
			parent.children.Add(parent_id);
			entity.parent = parent_id;
		}

		Data.entity_storage.Add(entity);

		return entity_identifier;
	}

	Source_Loc Front_End::Loc_From_Token(const Token& tk)
	{
		return Source_Loc{ (u32)(tk.Line + 1), (u16)(tk.Begin + 1) };
	}

	Entity Front_End::Create_File_Scope_Entity(File_ID file_id)
	{
		ASSERT(file_id != File_Null);

		Front_End_File& file = Data.Files[file_id];

		Entity file_entity;
		file_entity.entity_type = Entity_Type::File_Scope;
		file_entity.semantic_name = String_Make(file.Path.filename().string());
		file_entity.source_location = { 0,0 };

		file_entity.file_scope.file_id = file_id;

		return file_entity;
	}

	Entity Front_End::Create_File_Load_Entity(File_ID loaded_file_id)
	{
		ASSERT(loaded_file_id != File_Null);
		Front_End_File& file = Data.Files[loaded_file_id];

		Entity file_load_entity;
		file_load_entity.entity_type = Entity_Type::File_Load;
		file_load_entity.semantic_name = String_Make("load " + file.Path.string());
		file_load_entity.source_location = { 0,0 };

		file_load_entity.file_load.loaded_file_id = loaded_file_id;

		return file_load_entity;
	}

	Entity Front_End::Create_File_Named_Load_Entity(Entity_ID file_load_entity_id, String name, Source_Loc source_location)
	{
		Entity named_file_load_entity;

		named_file_load_entity.entity_type = Entity_Type::File_Load;
		named_file_load_entity.semantic_name = String_Copy(name);
		named_file_load_entity.source_location = source_location;

		named_file_load_entity.named_file_load.file_load_entity_id = file_load_entity_id;

		return named_file_load_entity;
	}

	Entity Front_End::Create_Constant_Entity(Const_Union value, String name, Source_Loc source_location)
	{
		Entity constant_entity = { 0 };

		constant_entity.entity_type = Entity_Type::Constant;
		constant_entity.semantic_name = String_Copy(name);
		constant_entity.source_location = { 0,0 };

		constant_entity.constant.value_as = value;

		return constant_entity;
	}
}