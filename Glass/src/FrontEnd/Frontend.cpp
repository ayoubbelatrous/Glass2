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
		Il_Program_Init(Data.il_program);
		TypeSystem_Init(Data.type_system);

		// to avoid pointers to these pools get invalidated by insertions
		Data.Files = Array_Reserved<Front_End_File>(1024);
		Data.entity_storage = Array_Reserved<Entity>(1024 * 10 * 10); // about 10 megs

		Entity global_scope_entity;
		global_scope_entity.semantic_name = String_Make("@global_scope");
		global_scope_entity.entity_type = Entity_Type::Global_Scope;

		Data.global_scope_entity = Insert_Entity(global_scope_entity);

		Data.exec_engine.type_system = &Data.type_system;

		auto insert_base_num_type_name = [this](String name, u64 size, u64 alignment, bool is_signed, bool is_float) -> auto {
			Type_Name tn;
			tn.size = size;
			tn.alignment = alignment;
			tn.name = name;
			tn.flags = TN_Base_Type | TN_Numeric_Type;

			if (!is_signed) {
				tn.flags |= TN_Unsigned_Type;
			}

			if (is_float) {
				tn.flags |= TN_Float_Type;
			}

			Type_Name_ID type_name_id = TypeSystem_Insert_TypeName(Data.type_system, tn);

			Entity type_name_entity = Create_TypeName_Entity(name, { 0,0 }, -1);
			type_name_entity.flags |= EF_Constant;
			type_name_entity.type_name.type_name_id = type_name_id;
			type_name_entity.constant_value.type = TypeSystem_Get_Basic_Type(Data.type_system, type_name_id);
			type_name_entity.semantic_type = Data.Type_Ty;

			Insert_Entity(type_name_entity, Data.global_scope_entity);

			return type_name_id;
		};

		{
			Type_Name tn_Type;
			tn_Type.size = 8;
			tn_Type.alignment = 8;
			tn_Type.name = String_Make("Type");
			tn_Type.flags = TN_Base_Type;

			Type_Name_ID Type_type_name_id = TypeSystem_Insert_TypeName(Data.type_system, tn_Type);

			Data.Type_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Type_type_name_id);

			Entity Type_type_name_entity = Create_TypeName_Entity(tn_Type.name, { 0,0 }, -1);
			Type_type_name_entity.semantic_name = String_Make("Type");
			Type_type_name_entity.flags |= EF_Constant;
			Type_type_name_entity.type_name.type_name_id = Type_type_name_id;
			Type_type_name_entity.semantic_type = Data.Type_Ty;
			Type_type_name_entity.constant_value.type = Data.Type_Ty;

			Insert_Entity(Type_type_name_entity, Data.global_scope_entity);

			Data.Type_tn = Type_type_name_id;
		}

		{
			Type_Name tn_Void;
			tn_Void.size = 0;
			tn_Void.alignment = 0;
			tn_Void.name = String_Make("void");
			tn_Void.flags = TN_Base_Type;

			Type_Name_ID void_type_name_id = TypeSystem_Insert_TypeName(Data.type_system, tn_Void);

			Data.void_tn = void_type_name_id;
			Data.void_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.void_tn);

			Entity void_type_name_entity = Create_TypeName_Entity(tn_Void.name, { 0,0 }, -1);
			void_type_name_entity.semantic_name = String_Make("void");
			void_type_name_entity.flags |= EF_Constant;
			void_type_name_entity.type_name.type_name_id = void_type_name_id;
			void_type_name_entity.semantic_type = Data.Type_Ty;
			void_type_name_entity.constant_value.type = Data.void_Ty;

			Insert_Entity(void_type_name_entity, Data.global_scope_entity);

		}

		Data.int_tn = insert_base_num_type_name(String_Make("int"), 4, 4, true, false);

		Data.i8_tn = insert_base_num_type_name(String_Make("i8"), 1, 1, true, false);
		Data.i16_tn = insert_base_num_type_name(String_Make("i16"), 2, 2, true, false);
		Data.i32_tn = insert_base_num_type_name(String_Make("i32"), 4, 4, true, false);
		Data.i64_tn = insert_base_num_type_name(String_Make("i64"), 8, 8, true, false);

		Data.u8_tn = insert_base_num_type_name(String_Make("u8"), 1, 1, false, false);
		Data.u16_tn = insert_base_num_type_name(String_Make("u16"), 2, 2, false, false);
		Data.u32_tn = insert_base_num_type_name(String_Make("u32"), 4, 4, false, false);
		Data.u64_tn = insert_base_num_type_name(String_Make("u64"), 8, 8, false, false);

		Data.float_tn = insert_base_num_type_name(String_Make("float"), 4, 4, false, true);

		Data.f32_tn = insert_base_num_type_name(String_Make("f32"), 4, 4, false, true);
		Data.f64_tn = insert_base_num_type_name(String_Make("f64"), 8, 8, false, true);

		if (Load_Base()) {
			return;
		}

		Load_First();

		if (Do_Tl_Definition_Passes()) {
			return;
		}
		if (Do_Tl_Dependency_Passes()) {
			return;
		}

		Do_Tl_Resolution_Passes();
	}

	bool Front_End::Load_Base()
	{
		fs_path base_file_path = "Library/Base.glass";

		if (!std::filesystem::exists(base_file_path)) {
			Push_Error(FMT("base file '{}' was not found!", base_file_path.string()));
			return true;
		}

		fs_path base_file_path_abs = std::filesystem::absolute(base_file_path);

		File_ID base_file_id = Generate_File(base_file_path, base_file_path_abs);

		if (base_file_id == File_Null) {
			Push_Error(FMT("failed to load the base file at: '{}'", base_file_path.string()));
			return true;
		}

		Entity_ID base_file_scope_entity_id = Insert_Entity(Create_File_Scope_Entity(base_file_id, Data.Files[base_file_id].Syntax), Data.global_scope_entity);

		Data.File_ID_To_Scope[base_file_id] = base_file_scope_entity_id;

		Do_Load_Pass(base_file_scope_entity_id);

		Insert_Entity(Create_File_Load_Entity(base_file_id, File_Null), Data.global_scope_entity);

		return false;
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

		Entity_ID first_file_scope_entity_id = Insert_Entity(Create_File_Scope_Entity(first_file_id, Data.Files[first_file_id].Syntax), Data.global_scope_entity);

		Data.File_ID_To_Scope[first_file_id] = first_file_scope_entity_id;

		Do_Load_Pass(first_file_scope_entity_id);

		Insert_Entity(Create_File_Load_Entity(first_file_id, File_Null), Data.global_scope_entity);

		int x = 0;
	}

	void Front_End::Do_Load_Pass(Entity_ID file_entity_id)
	{
		ASSERT(file_entity_id != Entity_Null);

		Entity& file_entity = Data.entity_storage[file_entity_id];
		ASSERT(file_entity.entity_type == Entity_Type::File_Scope);

		Front_End_File& file = Data.Files[file_entity.file_scope.file_id];

		for (size_t i = 0; i < file.Syntax->Statements.size(); i++)
		{
			Statement* statement = file.Syntax->Statements[i];

			if (statement->GetType() == NodeType::Load) {

				LoadNode* as_load = (LoadNode*)statement;
				Entity load_entity = Do_Load(file_entity_id, as_load);
				//Insert_Entity(load_entity, file_entity_id);

				Insert_Entity(load_entity, Data.global_scope_entity);
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
				Push_Error_Loc(entity_id, load_node, FMT("1oad directive file not found!: '{}'", path.string()));
			}

			loaded_file_id = Generate_File(path, path_abs);
			Entity_ID loaded_file_scope_entity_id = Insert_Entity(Create_File_Scope_Entity(loaded_file_id, Data.Files[loaded_file_id].Syntax), Data.global_scope_entity);

			Data.File_ID_To_Scope[loaded_file_id] = loaded_file_scope_entity_id;

			Do_Load_Pass(loaded_file_scope_entity_id);
		}
		else {
			loaded_file_id = previous_loaded_file_it->second;
		}

		return Create_File_Load_Entity(loaded_file_id, file_entity.file_scope.file_id);
	}

	bool Front_End::Do_Tl_Definition_Passes()
	{
		return Iterate_Tl_All_Files([this](File_ID file_id, Entity_ID file_entity_id, Entity& file_entity, Statement* statement) -> bool {

			if (statement->GetType() == NodeType::Variable) {

				VariableNode* as_var = (VariableNode*)statement;

				GS_CORE_ASSERT(as_var->Assignment);

				String var_name = String_Make(as_var->Symbol.Symbol);

				Entity_ID previous_definition = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, var_name);

				if (previous_definition != Entity_Null) {
					Push_Error_Loc(file_entity_id, as_var, FMT("global constant '{}' has its name already taken", as_var->Symbol.Symbol));
					return true;
				}

				if (as_var->Constant) {

					if (as_var->Assignment->GetType() == NodeType::Load) {

						Entity entity_file_load = Do_Load(file_entity_id, (LoadNode*)as_var->Assignment);
						GS_CORE_ASSERT(entity_file_load.entity_type == Entity_Type::File_Load);

						Entity_ID entity_file_load_id = Insert_Entity(entity_file_load);

						Entity named_load_entity = Create_File_Named_Load_Entity(
							entity_file_load_id,
							var_name,
							Loc_From_Token(as_var->Symbol), file_entity.file_scope.file_id);

						named_load_entity.flags |= EF_Constant;
						named_load_entity.syntax_node = as_var;

						Insert_Entity(named_load_entity, file_entity_id);
					}
					else {

						Entity constant_entity = Create_Constant_Entity(
							{ 0 },
							var_name,
							Loc_From_Token(as_var->Symbol), file_entity.file_scope.file_id);

						constant_entity.flags |= EF_Constant | EF_InComplete;
						constant_entity.syntax_node = as_var;

						Insert_Entity(constant_entity, file_entity_id);
					}
				}
			}
			else if (statement->GetType() == NodeType::StructNode) {

				StructNode* as_struct_node = (StructNode*)statement;

				String struct_name = String_Make(as_struct_node->Name.Symbol);

				Entity_ID previous_definition = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, struct_name);

				if (previous_definition != Entity_Null) {
					Push_Error_Loc(file_entity_id, as_struct_node, FMT("struct '{}' has its name already taken", as_struct_node->Name.Symbol));
					return true;
				}

				Entity struct_entity = Create_Struct_Entity(
					struct_name,
					Loc_From_Token(as_struct_node->Name),
					file_entity.file_scope.file_id);

				struct_entity.flags |= EF_Constant | EF_InComplete;
				struct_entity.syntax_node = as_struct_node;

				Entity_ID inserted_struct_entity_id = Insert_Entity(struct_entity, file_entity_id);

				Entity& inserted_struct_entity = Data.entity_storage[inserted_struct_entity_id];

				for (size_t i = 0; i < as_struct_node->m_Members.size(); i++)
				{
					VariableNode* struct_member_node = as_struct_node->m_Members[i];

					String member_name = String_Make(struct_member_node->Symbol.Symbol);

					for (size_t i = 0; i < inserted_struct_entity.children.count; i++)
					{
						Entity& previous_member = Data.entity_storage[inserted_struct_entity.children[i]];

						if (String_Equal(previous_member.semantic_name, member_name)) {
							Push_Error_Loc(file_entity_id, struct_member_node, FMT("struct '{}' member '{}' already defined", as_struct_node->Name.Symbol, struct_member_node->Symbol.Symbol));
							return true;
						}
					}

					Entity struct_member_entity = Create_Struct_Member_Entity(member_name, Loc_From_Token(struct_member_node->Symbol), file_entity.file_scope.file_id);
					struct_member_entity.syntax_node = struct_member_node;
					//struct_member_entity.flags |= EF_Constant;

					Insert_Entity(struct_member_entity, inserted_struct_entity_id);
				}
			}
			else if (statement->GetType() == NodeType::Enum) {

				StructNode* as_enum_node = (StructNode*)statement;

				String enum_name = String_Make(as_enum_node->Name.Symbol);

				Entity_ID previous_definition = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, enum_name);

				if (previous_definition != Entity_Null) {
					Push_Error_Loc(file_entity_id, as_enum_node, FMT("enum '{}' has its name already taken", as_enum_node->Name.Symbol));
					return true;
				}

				Entity enum_entity = Create_Enum_Entity(enum_name, Loc_From_Token(as_enum_node->Name), file_entity.file_scope.file_id);

				enum_entity.flags |= EF_Constant | EF_InComplete;
				enum_entity.syntax_node = as_enum_node;

				Entity_ID inserted_enum_entity_id = Insert_Entity(enum_entity, file_entity_id);

				Entity& inserted_enum_entity = Data.entity_storage[inserted_enum_entity_id];

				for (size_t i = 0; i < as_enum_node->m_Members.size(); i++)
				{
					Statement* enum_member_node = as_enum_node->m_Members[i];

					Token name_token;
					Expression* assignment = nullptr;

					if (enum_member_node->GetType() == NodeType::Identifier) {
						Identifier* as_ident = (Identifier*)enum_member_node;
						name_token = as_ident->Symbol;
					}
					else if (enum_member_node->GetType() == NodeType::Variable) {

						VariableNode* as_var = (VariableNode*)enum_member_node;

						if (!as_var->Constant) {
							Push_Error_Loc(file_entity_id, enum_member_node, "expected enum member to be a name or an assignment to a name: name :: value;");
							return true;
						}

						name_token = as_var->Symbol;
						assignment = as_var->Assignment;
					}
					else {
						Push_Error_Loc(file_entity_id, enum_member_node, "expected enum member to be a name or an assignment to a name: name :: value");
						return true;
					}

					String member_name = String_Make(name_token.Symbol);

					for (size_t i = 0; i < inserted_enum_entity.children.count; i++)
					{
						Entity& previous_member = Data.entity_storage[inserted_enum_entity.children[i]];

						if (String_Equal(previous_member.semantic_name, member_name)) {
							Push_Error_Loc(file_entity_id, enum_member_node, FMT("enum '{}' member '{}' already defined", as_enum_node->Name.Symbol, name_token.Symbol));
							return true;
						}
					}

					Entity enum_member_entity = Create_Struct_Member_Entity(member_name, Loc_From_Token(name_token), file_entity.file_scope.file_id);
					enum_member_entity.syntax_node = enum_member_node;
					enum_member_entity.flags |= EF_Constant;

					Insert_Entity(enum_member_entity, inserted_enum_entity_id);
				}
			}
			else if (statement->GetType() == NodeType::Function) {

			}
			return false;
			});
	}

	bool Front_End::Do_Tl_Dependency_Passes()
	{

		return Iterate_All_Files([this](File_ID file_id, Entity_ID file_entity_id, Entity& file_entity) -> bool {

			for (size_t i = 0; i < file_entity.children.count; i++)
			{
				Entity_ID tl_file_scope_entity_id = file_entity.children[i];
				Entity& tl_file_scope_entity = Data.entity_storage[tl_file_scope_entity_id];

				if (tl_file_scope_entity.flags & EF_InComplete) {
					if (tl_file_scope_entity.entity_type == Entity_Type::Constant) {

						GS_CORE_ASSERT(tl_file_scope_entity.syntax_node);
						GS_CORE_ASSERT(tl_file_scope_entity.syntax_node->GetType() == NodeType::Variable);

						VariableNode* as_var = (VariableNode*)tl_file_scope_entity.syntax_node;

						if (!as_var->Assignment) {
							Push_Error_Loc(file_entity_id, as_var, FMT("global constant '{}' is not assigned a value", as_var->Symbol.Symbol));
							return true;
						}

						if (!Do_Expression_Dependecy_Pass(file_entity_id, as_var->Assignment, tl_file_scope_entity.dependencies, Entity_Null)) {
							return true;
						}

						Array<Entity_ID> chain;

						if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_file_scope_entity.dependencies, chain)) {

							std::string printed_chain;

							printed_chain = tl_file_scope_entity.semantic_name.data;

							for (size_t i = 0; i < chain.count; i++)
							{
								printed_chain += " -> ";
								printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
							}

							printed_chain += " -> ";

							printed_chain += tl_file_scope_entity.semantic_name.data;

							Entity& tail_entity = Data.entity_storage[chain[0]];

							Push_Error_Loc(file_entity_id, as_var, FMT("global constant '{}' has circular dependency: {}", as_var->Symbol.Symbol, printed_chain));
							Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

							return true;
						}

						if (as_var->Type) {

							if (!Do_Expression_Dependecy_Pass(file_entity_id, as_var->Type, tl_file_scope_entity.dependencies, Entity_Null)) {
								return true;
							}

							Array<Entity_ID> chain;

							if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_file_scope_entity.dependencies, chain)) {

								std::string printed_chain;

								printed_chain = tl_file_scope_entity.semantic_name.data;

								for (size_t i = 0; i < chain.count; i++)
								{
									printed_chain += " -> ";
									printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
								}

								printed_chain += " -> ";

								printed_chain += tl_file_scope_entity.semantic_name.data;

								Entity& tail_entity = Data.entity_storage[chain[0]];

								Push_Error_Loc(file_entity_id, as_var->Type, FMT("the type of global constant '{}' has circular dependency: {}", as_var->Symbol.Symbol, printed_chain));
								Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

								return true;
							}
						}
					}
					else if (tl_file_scope_entity.entity_type == Entity_Type::Struct_Entity) {

						for (size_t i = 0; i < tl_file_scope_entity.children.count; i++)
						{
							Entity& struct_member_entity = Data.entity_storage[tl_file_scope_entity.children[i]];
							VariableNode* as_variable_node = (VariableNode*)struct_member_entity.syntax_node;

							ASSERT(as_variable_node->Type);

							if (!Do_Expression_Dependecy_Pass(file_entity_id, as_variable_node->Type, tl_file_scope_entity.dependencies, tl_file_scope_entity_id))
								return true;

							Array<Entity_ID> chain;
							if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_file_scope_entity.dependencies, chain)) {

								std::string printed_chain;

								printed_chain = tl_file_scope_entity.semantic_name.data;

								for (size_t i = 0; i < chain.count; i++)
								{
									printed_chain += " -> ";
									printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
								}

								printed_chain += " -> ";

								printed_chain += tl_file_scope_entity.semantic_name.data;

								Entity& tail_entity = Data.entity_storage[chain[0]];

								Push_Error_Loc(file_entity_id, as_variable_node->Type, FMT("struct member '{}' type has circular dependency: {}", as_variable_node->Symbol.Symbol, printed_chain));
								Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

								return true;
							}
						}
					}
					else if (tl_file_scope_entity.entity_type == Entity_Type::Enum_Entity) {

						for (size_t i = 0; i < tl_file_scope_entity.children.count; i++)
						{
							Entity& enum_member_entity = Data.entity_storage[tl_file_scope_entity.children[i]];

							if (enum_member_entity.syntax_node->GetType() == NodeType::Variable) {

								VariableNode* as_variable_node = (VariableNode*)enum_member_entity.syntax_node;

								ASSERT(as_variable_node->Assignment);

								if (!Do_Expression_Dependecy_Pass(file_entity_id, as_variable_node->Assignment, tl_file_scope_entity.dependencies, tl_file_scope_entity_id))
									return true;

								Array<Entity_ID> chain;
								if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_file_scope_entity.dependencies, chain)) {

									std::string printed_chain;

									printed_chain = tl_file_scope_entity.semantic_name.data;

									for (size_t i = 0; i < chain.count; i++)
									{
										printed_chain += " -> ";
										printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
									}

									printed_chain += " -> ";

									printed_chain += tl_file_scope_entity.semantic_name.data;

									Entity& tail_entity = Data.entity_storage[chain[0]];

									Push_Error_Loc(file_entity_id, as_variable_node, FMT("enum member '{}' type has circular dependency: {}", as_variable_node->Symbol.Symbol, printed_chain));
									Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

									return true;
								}
							}
						}
					}
				}
			}

			return false;

			});
	}

	bool Front_End::Do_Tl_Resolution_Passes()
	{
		bool all_evaluated = true;

		for (size_t i = 0; i < Data.entity_storage.count; i++)
		{
			Entity& entity = Data.entity_storage[i];

			if ((entity.flags & EF_InComplete)) {

				bool has_dependencies = false;

				for (size_t i = 0; i < entity.dependencies.count; i++)
				{
					if (Data.entity_storage[entity.dependencies[i]].flags & EF_InComplete) {
						has_dependencies = true;
						break;
					}
				}

				if (has_dependencies) {
					all_evaluated = false;
					continue;
				}

				if (entity.entity_type == Entity_Type::Constant) {

					VariableNode* as_var = (VariableNode*)entity.syntax_node;

					if (as_var->Type) {
						entity.semantic_type = Evaluate_Type(as_var->Type, entity.parent);
					}

					Eval_Result assignment_code_gen_result = Expression_Evaluate(as_var->Assignment, entity.parent, nullptr);
					if (!assignment_code_gen_result) {
						break;
					}

					entity.flags &= ~EF_InComplete;

					if (entity.semantic_type) {
						if (assignment_code_gen_result.expression_type != entity.semantic_type) {
							Push_Error_Loc(entity.parent, as_var, FMT("global constant '{}' type mismatch in assignment", as_var->Symbol.Symbol));
							break;
						}
					}
					else {
						entity.semantic_type = assignment_code_gen_result.expression_type;
					}

					entity.constant_value = assignment_code_gen_result.result;
				}
				else if (entity.entity_type == Entity_Type::Struct_Entity)
				{
					VariableNode* as_struct_node = (VariableNode*)entity.syntax_node;

					GS_Struct struct_type;

					Type_Name struct_type_name;
					struct_type_name.name = entity.semantic_name;
					struct_type_name.flags = TN_Struct_Type;
					struct_type_name.size = (u64)-1;
					struct_type_name.alignment = 0;

					entity.struct_entity.type_name_id = TypeSystem_Insert_TypeName_Struct(Data.type_system, struct_type_name, struct_type);
					entity.constant_value.type = TypeSystem_Get_Basic_Type(Data.type_system, entity.struct_entity.type_name_id);

					entity.flags &= ~EF_InComplete;

					for (size_t i = 0; i < entity.children.count; i++)
					{
						Entity& struct_member_entity = Data.entity_storage[entity.children[i]];

						VariableNode* as_variable_node = (VariableNode*)struct_member_entity.syntax_node;

						struct_member_entity.semantic_type = Evaluate_Type(as_variable_node->Type, entity.parent);
						ASSERT(struct_member_entity.semantic_type);

						Array_Add(struct_type.members, struct_member_entity.semantic_type);
						Array_Add(struct_type.offsets, (u64)-1);
					}

					GS_Struct_Data_Layout struct_data_layout = TypeSystem_Struct_Compute_Align_Size_Offsets(Data.type_system, struct_type.members);

					for (size_t i = 0; i < struct_type.offsets.count; i++)
					{
						struct_type.offsets[i] = struct_data_layout.offsets[i];
					}

					Data.type_system.type_name_storage[entity.struct_entity.type_name_id].size = struct_data_layout.size;
					Data.type_system.type_name_storage[entity.struct_entity.type_name_id].alignment = struct_data_layout.alignment;
					Data.type_system.struct_storage[Data.type_system.type_name_storage[entity.struct_entity.type_name_id].struct_id] = struct_type;

					if (String_Equal(String_Make("string"), entity.semantic_name)) { // TODO: move this from here somehow
						Data.string_Ty = TypeSystem_Get_Basic_Type(Data.type_system, entity.struct_entity.type_name_id);
						ASSERT(Data.string_Ty);
					}
				}
				else if (entity.entity_type == Entity_Type::Enum_Entity) {

					EnumNode* as_enum_node = (EnumNode*)entity.syntax_node;

					Type_Name underlying_type_name = Data.type_system.type_name_storage[Data.u64_tn];
					GS_Type* underlying_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.u64_tn);

					Type_Name enum_type_name = underlying_type_name;
					enum_type_name.name = String_Copy(entity.semantic_name);
					enum_type_name.flags = TN_Enum_Type;
					entity.enum_entity.type_name_id = TypeSystem_Insert_TypeName(Data.type_system, enum_type_name);
					entity.constant_value.type = TypeSystem_Get_Basic_Type(Data.type_system, entity.struct_entity.type_name_id);
					entity.flags &= ~EF_InComplete;

					Const_Union next_enum_value = { 0 };

					for (size_t i = 0; i < entity.children.count; i++)
					{
						Entity& enum_member_entity = Data.entity_storage[entity.children[i]];

						if (enum_member_entity.syntax_node->GetType() == NodeType::Variable) {

							VariableNode* as_variable_node = (VariableNode*)enum_member_entity.syntax_node;

							Eval_Result value_code_gen_result = Expression_Evaluate(as_variable_node->Assignment, entity.parent, underlying_type);
							if (!value_code_gen_result)
								return true;

							if (value_code_gen_result.expression_type != underlying_type) {
								Push_Error_Loc(entity.parent, as_variable_node->Assignment, FMT("enum '{}' member type mismatch in assignment", as_enum_node->Name.Symbol, as_variable_node->Symbol.Symbol));
								break;
							}

							enum_member_entity.enum_member.value = value_code_gen_result.result;
							value_code_gen_result.result.us8++;
							next_enum_value = value_code_gen_result.result;
						}
						else {
							enum_member_entity.enum_member.value = next_enum_value;
							next_enum_value.us8++;
						}
					}
				}
			}

			if (i + 1 == Data.entity_storage.count) {
				if (all_evaluated) {

				}
				else {
					all_evaluated = true;
					i = 0;
				}
			}
		}

		return false;
	}

	Eval_Result Front_End::Expression_Evaluate(Expression* expression, Entity_ID scope_id, GS_Type* inferred_type)
	{
		NodeType expression_type = expression->GetType();

		switch (expression_type)
		{
		case NodeType::StringLiteral:
		{
			StringLiteral* as_lit = (StringLiteral*)expression;

			String literal_value = String_Make(as_lit->Symbol.Symbol);
			Const_Union constant_value = { 0 };
			constant_value.string = *(GS_String*)&literal_value;

			GS_Type* literal_type = Data.string_Ty;

			Eval_Result result;
			result.ok = true;
			result.result = constant_value;
			result.expression_type = literal_type;
			return result;
		}
		break;
		case NodeType::NumericLiteral:
		{
			NumericLiteral* as_lit = (NumericLiteral*)expression;

			GS_Type* literal_type;

			if (inferred_type) {

				auto inferred_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, inferred_type);

				if (inferred_type_flags & TN_Numeric_Type) {
					auto inferred_flags = TypeSystem_Get_Type_Flags(Data.type_system, inferred_type);

					if (!(inferred_flags & TN_Float_Type) && (as_lit->type == NumericLiteral::Type::Float)) {
						literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.float_tn);
					}
					else {
						literal_type = inferred_type;
					}
				}
			}
			else {
				if (as_lit->type == NumericLiteral::Type::Float) {
					literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.float_tn);
				}
				else if (as_lit->type == NumericLiteral::Type::Int) {
					literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.int_tn);
				}
				else {
					GS_ASSERT_UNIMPL();
				}
			}

			Const_Union literal_value = { 0 };
			literal_value.ptr = (void*)as_lit->Val.Int;

			Eval_Result result;
			result.ok = true;
			result.result = literal_value;
			result.expression_type = literal_type;
			return result;
		}
		break;
		case NodeType::BinaryExpression:
		{
			BinaryExpression* as_bin_expr = (BinaryExpression*)expression;

			Eval_Result left_result = Expression_Evaluate(as_bin_expr->Left, scope_id, inferred_type);
			Eval_Result right_result = Expression_Evaluate(as_bin_expr->Right, scope_id, left_result.expression_type);

			if (!left_result || !right_result)
				return {};

			if (left_result.expression_type != right_result.expression_type) {
				Push_Error_Loc(scope_id, as_bin_expr, "type mismatch");
				return {};
			}

			if (!left_result.expression_type) {
				Push_Error_Loc(scope_id, as_bin_expr->Left, "<un typed> value does not support math operations");
				return {};
			}

			if (!right_result.expression_type) {
				Push_Error_Loc(scope_id, as_bin_expr->Right, "<un typed> value does not support math operations");
				return {};
			}

			auto left_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, left_result.expression_type);
			auto right_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, right_result.expression_type);

			GS_Type* bin_expr_result_type = left_result.expression_type;

			auto type_flags = TypeSystem_Get_Type_Flags(Data.type_system, bin_expr_result_type);

			if (!(type_flags & TN_Numeric_Type)) {
				Push_Error_Loc(scope_id, as_bin_expr, "types are not numeric and no overload was found");
				return {};
			}

			Const_Union result_value = { 0 };

			switch (as_bin_expr->OPerator)
			{
			case Operator::Add:
				if (type_flags & TN_Float_Type)
					result_value.f64 = left_result.result.f64 + right_result.result.f64;
				else
					result_value.us8 = left_result.result.us8 + right_result.result.us8;
				break;
			case Operator::Subtract:
				if (type_flags & TN_Float_Type)
					result_value.f64 = left_result.result.f64 - right_result.result.f64;
				else
					result_value.us8 = left_result.result.us8 - right_result.result.us8;
				break;
			case Operator::Multiply:
				if (type_flags & TN_Float_Type)
					result_value.f64 = left_result.result.f64 * right_result.result.f64;
				else
					result_value.us8 = left_result.result.us8 * right_result.result.us8;
				break;
			case Operator::Divide:
				//TODO: error when divide by 0
				if (type_flags & TN_Float_Type)
					result_value.f64 = left_result.result.f64 / right_result.result.f64;
				else
					result_value.us8 = left_result.result.us8 / right_result.result.us8;
				break;
			default:
				ASSERT(nullptr, "unknown operator");
				break;
			}

			Eval_Result result;
			result.expression_type = bin_expr_result_type;
			result.result = result_value;
			result.ok = true;
			return result;
		}
		case NodeType::Identifier:
		case NodeType::TE_TypeName:
		{
			Identifier* as_ident = (Identifier*)expression;
			String name = String_Make(as_ident->Symbol.Symbol);
			Entity_ID identified_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, name);

			if (identified_entity_id == Entity_Null) {
				Push_Error_Loc(scope_id, as_ident, FMT("undefined name!: '{}'", as_ident->Symbol.Symbol));
				return {};
			}

			Entity& identified_entity = Data.entity_storage[identified_entity_id];

			ASSERT(identified_entity_id != Entity_Null);
			//ASSERT(identified_entity.semantic_type);
			ASSERT(identified_entity.flags & EF_Constant);
			ASSERT(!(identified_entity.flags & EF_InComplete));

			Eval_Result result;
			result.ok = true;
			result.expression_type = identified_entity.semantic_type;
			result.result = identified_entity.constant_value;
			result.entity_reference = identified_entity_id;
			return result;
		}
		break;
		case NodeType::TE_Pointer:
		{
			GS_Type* evaluated_type = Evaluate_Type(expression, scope_id);
			if (!evaluated_type)
				return {};

			Eval_Result result;
			result.ok = true;
			result.expression_type = Data.Type_Ty;
			result.result.type = evaluated_type;
			return result;
		}
		break;
		case NodeType::MemberAccess:
		{
			MemberAccess* as_member_access = (MemberAccess*)expression;

			Eval_Result object_result = Expression_Evaluate(as_member_access->Object, scope_id, nullptr);

			if (!object_result) {
				return {};
			}

			Identifier* member_as_ident = (Identifier*)as_member_access->Member;
			String member_name = String_Make(member_as_ident->Symbol.Symbol);

			if (object_result.expression_type == nullptr) {
				if (object_result.entity_reference == Entity_Null)
					return {};

				Entity& referenced_entity = Data.entity_storage[object_result.entity_reference];

				if (referenced_entity.entity_type == Entity_Type::Named_File_Load) {

					Entity& referenced_file_scope = Data.entity_storage[Data.File_ID_To_Scope.at(Data.entity_storage[referenced_entity.named_file_load.file_load_entity_id].file_load.loaded_file_id)];

					Identifier* member_as_ident = (Identifier*)as_member_access->Member;

					String member_name = String_Make(member_as_ident->Symbol.Symbol);

					Entity* member_entity = nullptr;
					Entity_ID member_entity_id = Entity_Null;

					for (size_t i = 0; i < referenced_file_scope.children.count; i++)
					{
						if (String_Equal(member_name, Data.entity_storage[referenced_file_scope.children[i]].semantic_name)) {
							member_entity = &Data.entity_storage[referenced_file_scope.children[i]];
							member_entity_id = referenced_file_scope.children[i];
						}
					}

					ASSERT(member_entity);

					Eval_Result result;
					result.ok = true;
					result.expression_type = member_entity->semantic_type;
					result.entity_reference = member_entity_id;
					return result;
				}
				else {
					ASSERT(nullptr);
				}
			}
			else {

				//TODO: improve performance

				if (object_result.expression_type == Data.Type_Ty) {

					GS_Type* type_value = object_result.result.type;
					ASSERT(type_value);

					auto type_value_flags = TypeSystem_Get_Type_Flags(Data.type_system, type_value);

					if (!(type_value_flags & TN_Enum_Type)) {
						Push_Error_Loc(scope_id, as_member_access->Object, "type does not support members");
						return {};
					}

					Entity_ID enum_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, Data.type_system.type_name_storage[type_value->basic.type_name_id].name);

					ASSERT(enum_entity_id != Entity_Null);
					Entity& enum_entity = Data.entity_storage[enum_entity_id];

					ASSERT(enum_entity.entity_type == Entity_Type::Enum_Entity);

					Entity* member_entity = nullptr;
					u64 member_index = -1;

					for (size_t i = 0; i < enum_entity.children.count; i++)
					{
						if (String_Equal(Data.entity_storage[enum_entity.children[i]].semantic_name, member_name)) {
							member_entity = &Data.entity_storage[enum_entity.children[i]];
							member_index = i;
							break;
						}
					}

					if (!member_entity) {
						Push_Error_Loc(scope_id, as_member_access->Member, FMT("enum does not have member named: '{}'", member_as_ident->Symbol.Symbol));
						return {};
					}

					Eval_Result result;
					result.ok = true;
					result.expression_type = type_value;
					result.result = member_entity->enum_member.value;
					return result;
				}
				else {

					auto object_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, object_result.expression_type);

					if (!(object_type_flags & TN_Struct_Type) && !(object_type_flags & TN_Enum_Type)) {
						Push_Error_Loc(scope_id, as_member_access->Object, "type does not support members");
						return {};
					}

					Entity_ID struct_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, Data.type_system.type_name_storage[object_result.expression_type->basic.type_name_id].name);

					ASSERT(struct_entity_id != Entity_Null);
					Entity& struct_entity = Data.entity_storage[struct_entity_id];

					ASSERT(struct_entity.entity_type == Entity_Type::Struct_Entity);

					Entity* member_entity = nullptr;
					u64 member_index = -1;

					for (size_t i = 0; i < struct_entity.children.count; i++)
					{
						if (String_Equal(Data.entity_storage[struct_entity.children[i]].semantic_name, member_name)) {
							member_entity = &Data.entity_storage[struct_entity.children[i]];
							member_index = i;
							break;
						}
					}

					if (!member_entity) {
						Push_Error_Loc(scope_id, as_member_access->Member, FMT("struct does not have member named: '{}'", member_as_ident->Symbol.Symbol));
						return {};
					}

					//	Il_Node sep_node = Il_Make_Struct_Element_Ptr((u16)TypeSystem_Get_Type_Index(Data.type_system, object_result.expression_type), member_index, object_result.code_node_id);
					//	Il_IDX sep_node_id = Il_Proc_Insert(proc, sep_node);

					u64 offset = Data.type_system.struct_storage[Data.type_system.type_name_storage[object_result.expression_type->basic.type_name_id].struct_id].offsets[member_index];

					Eval_Result result;
					result.ok = true;
					result.expression_type = TypeSystem_Increase_Ind(Data.type_system, member_entity->semantic_type);
					result.result.ptr = &(&object_result.result.us1)[offset];
					return result;
				}
			}
		}
		break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return {};
	}

	CodeGen_Result Front_End::Expression_CodeGen(Expression* expression, Entity_ID scope_id, Il_Proc& proc, GS_Type* inferred_type)
	{
		NodeType expression_type = expression->GetType();

		switch (expression_type)
		{
		case NodeType::StringLiteral:
		{
			StringLiteral* as_lit = (StringLiteral*)expression;

			String literal_value = String_Make(as_lit->Symbol.Symbol);
			Const_Union constant_value = { 0 };
			constant_value.string = *(GS_String*)&literal_value;

			GS_Type* literal_type = Data.string_Ty;

			u16 node_id = Il_Proc_Insert(proc, Il_Make_Constant(constant_value, (u16)TypeSystem_Get_Type_Index(Data.type_system, literal_type)));

			CodeGen_Result result;
			result.ok = true;
			result.code_node_id = node_id;
			result.expression_type = literal_type;
			return result;
		}
		break;
		case NodeType::NumericLiteral:
		{
			NumericLiteral* as_lit = (NumericLiteral*)expression;

			GS_Type* literal_type;

			if (inferred_type) {

				auto inferred_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, inferred_type);

				if (inferred_type_flags & TN_Numeric_Type) {
					auto inferred_flags = TypeSystem_Get_Type_Flags(Data.type_system, inferred_type);

					if (!(inferred_flags & TN_Float_Type) && (as_lit->type == NumericLiteral::Type::Float)) {
						literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.float_tn);
					}
					else {
						literal_type = inferred_type;
					}
				}
			}
			else {
				if (as_lit->type == NumericLiteral::Type::Float) {
					literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.float_tn);
				}
				else if (as_lit->type == NumericLiteral::Type::Int) {
					literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.int_tn);
				}
				else {
					GS_ASSERT_UNIMPL();
				}
			}

			u64 literal_value = 0;
			literal_value = as_lit->Val.Int;

			u16 node_id = Il_Proc_Insert(proc, Il_Make_Constant((void*)literal_value, (u16)TypeSystem_Get_Type_Index(Data.type_system, literal_type)));

			CodeGen_Result result;
			result.ok = true;
			result.code_node_id = node_id;
			result.expression_type = literal_type;
			return result;
		}
		break;
		case NodeType::BinaryExpression:
		{
			BinaryExpression* as_bin_expr = (BinaryExpression*)expression;

			CodeGen_Result left_result = Expression_CodeGen(as_bin_expr->Left, scope_id, proc, inferred_type);
			CodeGen_Result right_result = Expression_CodeGen(as_bin_expr->Right, scope_id, proc, left_result.expression_type);

			if (!left_result || !right_result)
				return {};

			if (left_result.expression_type != right_result.expression_type) {
				Push_Error_Loc(scope_id, as_bin_expr, "type mismatch");
				return {};
			}

			if (!left_result.expression_type) {
				Push_Error_Loc(scope_id, as_bin_expr->Left, "<un typed> value does not support math operations");
				return {};
			}

			if (!right_result.expression_type) {
				Push_Error_Loc(scope_id, as_bin_expr->Right, "<un typed> value does not support math operations");
				return {};
			}

			auto left_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, left_result.expression_type);
			auto right_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, right_result.expression_type);

			GS_Type* bin_expr_result_type = left_result.expression_type;

			if (!(TypeSystem_Get_Type_Flags(Data.type_system, bin_expr_result_type) & TN_Numeric_Type)) {
				Push_Error_Loc(scope_id, as_bin_expr, "types are not numeric and no overload was found");
				return {};
			}

			Il_Node_Type il_math_op_type;

			switch (as_bin_expr->OPerator)
			{
			case Operator::Add:
				il_math_op_type = Il_Add;
				break;
			case Operator::Subtract:
				il_math_op_type = Il_Sub;
				break;
			case Operator::Multiply:
				il_math_op_type = Il_Mul;
				break;
			case Operator::Divide:
				il_math_op_type = Il_Div;
				break;
			default:
				ASSERT(nullptr, "unknown operator");
				break;
			}

			Il_Node math_op_node = Il_Make_Math_Op(il_math_op_type, (u16)TypeSystem_Get_Type_Index(Data.type_system, bin_expr_result_type), left_result.code_node_id, right_result.code_node_id);
			Il_IDX result_node_idx = Il_Proc_Insert(proc, math_op_node);

			CodeGen_Result result;
			result.ok = true;
			result.expression_type = bin_expr_result_type;
			result.code_node_id = result_node_idx;
			return result;
		}
		case NodeType::Identifier:
		case NodeType::TE_TypeName:
		{
			Identifier* as_ident = (Identifier*)expression;
			String name = String_Make(as_ident->Symbol.Symbol);
			Entity_ID identified_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, name);

			if (identified_entity_id == Entity_Null) {
				Push_Error_Loc(scope_id, as_ident, FMT("undefined name!: '{}'", as_ident->Symbol.Symbol));
				return {};
			}

			Entity& identified_entity = Data.entity_storage[identified_entity_id];

			ASSERT(identified_entity_id != Entity_Null);
			//ASSERT(identified_entity.semantic_type);
			ASSERT(identified_entity.flags & EF_Constant);
			ASSERT(!(identified_entity.flags & EF_InComplete));

			Il_Node constant_value = Il_Make_Constant(identified_entity.constant_value.ptr, (u16)TypeSystem_Get_Type_Index(Data.type_system, identified_entity.semantic_type));
			Il_IDX value_node_id = Il_Proc_Insert(proc, constant_value);

			CodeGen_Result result;
			result.ok = true;
			result.expression_type = identified_entity.semantic_type;
			result.code_node_id = value_node_id;
			result.entity_reference = identified_entity_id;
			return result;
		}
		break;
		case NodeType::TE_Pointer:
		{
			TypeExpressionPointer* as_pointer = (TypeExpressionPointer*)expression;

			CodeGen_Result pointee_result = Expression_CodeGen(as_pointer->Pointee, scope_id, proc, nullptr);

			if (!pointee_result)
				return {};

			if (pointee_result.expression_type != Data.Type_Ty) {
				Push_Error_Loc(scope_id, as_pointer->Pointee, "Expected expression to be a type!");
				return {};
			}

			ASSERT(pointee_result.expression_type);

			Il_Node& pointee_result_node = proc.instruction_storage[pointee_result.code_node_id];
			ASSERT(pointee_result_node.node_type == Il_Const);

			GS_Type* generated_type = TypeSystem_Get_Pointer_Type(Data.type_system, pointee_result_node.constant.as.type, as_pointer->Indirection);

			Il_Node constant_value = Il_Make_Constant(generated_type, (u16)TypeSystem_Get_Type_Index(Data.type_system, Data.Type_Ty));
			Il_IDX value_node_id = Il_Proc_Insert(proc, constant_value);

			CodeGen_Result result;
			result.ok = true;
			result.expression_type = Data.Type_Ty;
			result.code_node_id = value_node_id;
			return result;
		}
		break;
		case NodeType::MemberAccess:
		{
			MemberAccess* as_member_access = (MemberAccess*)expression;

			CodeGen_Result object_result = Expression_CodeGen(as_member_access->Object, scope_id, proc, nullptr);

			if (!object_result) {
				return {};
			}

			Identifier* member_as_ident = (Identifier*)as_member_access->Member;
			String member_name = String_Make(member_as_ident->Symbol.Symbol);

			if (object_result.expression_type == nullptr) {
				if (object_result.entity_reference == Entity_Null)
					return {};

				Entity& referenced_entity = Data.entity_storage[object_result.entity_reference];

				if (referenced_entity.entity_type == Entity_Type::Named_File_Load) {

					Entity& referenced_file_scope = Data.entity_storage[Data.File_ID_To_Scope.at(Data.entity_storage[referenced_entity.named_file_load.file_load_entity_id].file_load.loaded_file_id)];

					Identifier* member_as_ident = (Identifier*)as_member_access->Member;

					String member_name = String_Make(member_as_ident->Symbol.Symbol);

					Entity* member_entity = nullptr;
					Entity_ID member_entity_id = Entity_Null;

					for (size_t i = 0; i < referenced_file_scope.children.count; i++)
					{
						if (String_Equal(member_name, Data.entity_storage[referenced_file_scope.children[i]].semantic_name)) {
							member_entity = &Data.entity_storage[referenced_file_scope.children[i]];
							member_entity_id = referenced_file_scope.children[i];
						}
					}

					ASSERT(member_entity);

					CodeGen_Result result;
					result.ok = true;
					result.expression_type = member_entity->semantic_type;
					result.entity_reference = member_entity_id;
					return result;
				}
				else {
					ASSERT(nullptr);
				}
			}
			else {

				//TODO: improve performance

				if (object_result.expression_type == Data.Type_Ty) {

					Const_Union type_value_constant = proc.instruction_storage[object_result.code_node_id].constant.as;
					GS_Type* type_value = type_value_constant.type;
					ASSERT(type_value);
					auto type_value_flags = TypeSystem_Get_Type_Flags(Data.type_system, type_value);

					if (!(type_value_flags & TN_Enum_Type)) {
						Push_Error_Loc(scope_id, as_member_access->Object, "type does not support members");
						return {};
					}

					Entity_ID enum_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, Data.type_system.type_name_storage[type_value->basic.type_name_id].name);

					ASSERT(enum_entity_id != Entity_Null);
					Entity& enum_entity = Data.entity_storage[enum_entity_id];

					ASSERT(enum_entity.entity_type == Entity_Type::Enum_Entity);

					Entity* member_entity = nullptr;
					u64 member_index = -1;

					for (size_t i = 0; i < enum_entity.children.count; i++)
					{
						if (String_Equal(Data.entity_storage[enum_entity.children[i]].semantic_name, member_name)) {
							member_entity = &Data.entity_storage[enum_entity.children[i]];
							member_index = i;
							break;
						}
					}

					if (!member_entity) {
						Push_Error_Loc(scope_id, as_member_access->Member, FMT("enum does not have member named: '{}'", member_as_ident->Symbol.Symbol));
						return {};
					}

					Il_Node constant_value = Il_Make_Constant(member_entity->enum_member.value, (u16)TypeSystem_Get_Type_Index(Data.type_system, type_value));
					Il_IDX value_node_id = Il_Proc_Insert(proc, constant_value);

					CodeGen_Result result;
					result.ok = true;
					result.expression_type = type_value;
					result.code_node_id = value_node_id;
					return result;
				}
				else {

					auto object_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, object_result.expression_type);

					if (!(object_type_flags & TN_Struct_Type) && !(object_type_flags & TN_Enum_Type)) {
						Push_Error_Loc(scope_id, as_member_access->Object, "type does not support members");
						return {};
					}

					Entity_ID struct_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, Data.type_system.type_name_storage[object_result.expression_type->basic.type_name_id].name);

					ASSERT(struct_entity_id != Entity_Null);
					Entity& struct_entity = Data.entity_storage[struct_entity_id];

					ASSERT(struct_entity.entity_type == Entity_Type::Struct_Entity);

					Entity* member_entity = nullptr;
					u64 member_index = -1;

					for (size_t i = 0; i < struct_entity.children.count; i++)
					{
						if (String_Equal(Data.entity_storage[struct_entity.children[i]].semantic_name, member_name)) {
							member_entity = &Data.entity_storage[struct_entity.children[i]];
							member_index = i;
							break;
						}
					}

					if (!member_entity) {
						Push_Error_Loc(scope_id, as_member_access->Member, FMT("struct does not have member named: '{}'", member_as_ident->Symbol.Symbol));
						return {};
					}

					//	Il_Node sep_node = Il_Make_Struct_Element_Ptr((u16)TypeSystem_Get_Type_Index(Data.type_system, object_result.expression_type), member_index, object_result.code_node_id);
					//	Il_IDX sep_node_id = Il_Proc_Insert(proc, sep_node);

					Il_Node object_node = proc.instruction_storage[object_result.code_node_id];
					ASSERT(object_node.node_type == Il_Const);

					u64 offset = Data.type_system.struct_storage[Data.type_system.type_name_storage[object_result.expression_type->basic.type_name_id].struct_id].offsets[member_index];

					Il_Node constant_value = Il_Make_Constant(&(&object_node.constant.as.us1)[offset], (u16)TypeSystem_Get_Type_Index(Data.type_system, member_entity->semantic_type));
					Il_IDX value_node_id = Il_Proc_Insert(proc, constant_value);

					CodeGen_Result result;
					result.ok = true;
					result.expression_type = member_entity->semantic_type;
					result.code_node_id = value_node_id;
					return result;
				}
			}
		}
		break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		CodeGen_Result result;
		return result;
	}

	GS_Type* Front_End::Evaluate_Type(Expression* expression, Entity_ID scope_id)
	{
		switch (expression->GetType())
		{
		case NodeType::Identifier:
		case NodeType::TE_TypeName:
		{
			Identifier* as_ident = (Identifier*)expression;
			String name = String_Make(as_ident->Symbol.Symbol);
			Entity_ID identified_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, name);

			if (identified_entity_id == Entity_Null) {
				Push_Error_Loc(scope_id, as_ident, FMT("undefined name!: '{}'", as_ident->Symbol.Symbol));
				return {};
			}

			Entity& identified_entity = Data.entity_storage[identified_entity_id];

			ASSERT(identified_entity_id != Entity_Null);
			//ASSERT(identified_entity.semantic_type);
			ASSERT(identified_entity.flags & EF_Constant);
			ASSERT(!(identified_entity.flags & EF_InComplete));

			if (identified_entity.semantic_type != Data.Type_Ty) {
				Push_Error_Loc(scope_id, as_ident, FMT("'{}' is not a type", as_ident->Symbol.Symbol));
				return {};
			}

			return identified_entity.constant_value.type;
		}
		break;
		case NodeType::TE_Pointer: {
			TypeExpressionPointer* as_pointer = (TypeExpressionPointer*)expression;

			GS_Type* pointee_result = Evaluate_Type(as_pointer->Pointee, scope_id);

			if (!pointee_result)
				return nullptr;

			GS_Type* generated_type = TypeSystem_Get_Pointer_Type(Data.type_system, pointee_result, as_pointer->Indirection);
			return generated_type;
			break;
		}
		default:
			ASSERT(nullptr);
			break;
		}
	}

	Dep_Result Front_End::Do_Expression_Dependecy_Pass(Entity_ID scope_id, Expression* expr, Array<Entity_ID>& dependencies, Entity_ID ignore_indirect)
	{
		GS_CORE_ASSERT(expr);
		GS_CORE_ASSERT(scope_id != Entity_Null);

		Entity& scope_entity = Data.entity_storage[scope_id];

		NodeType syntax_node_type = expr->GetType();

		switch (syntax_node_type)
		{
		case NodeType::Identifier:
		case NodeType::TE_TypeName:
		{
			Identifier* as_ident = (Identifier*)expr;
			String name = String_Make(as_ident->Symbol.Symbol);
			Entity_ID identified_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, name);

			if (identified_entity_id == Entity_Null) {
				GS_CORE_ASSERT(scope_entity.entity_type == Entity_Type::File_Scope);
				Push_Error_Loc(scope_id, as_ident, FMT("undefined name: '{}'", as_ident->Symbol.Symbol));
				return {};
			}

			Entity& identified_entity = Data.entity_storage[identified_entity_id];

			if (!(identified_entity.flags & EF_Constant)) {
				GS_CORE_ASSERT(scope_entity.entity_type == Entity_Type::File_Scope);
				Push_Error_Loc(scope_id, as_ident, FMT("not a compile time constant: '{}'", as_ident->Symbol.Symbol));
				return {};
			}

			Array_Add(dependencies, identified_entity_id);

			return { true, identified_entity_id };
		}
		break;
		case NodeType::BinaryExpression:
		{
			BinaryExpression* as_binary_expression = (BinaryExpression*)expr;
			GS_CORE_ASSERT(as_binary_expression->Right);
			GS_CORE_ASSERT(as_binary_expression->Left);

			if (!Do_Expression_Dependecy_Pass(scope_id, as_binary_expression->Left, dependencies, ignore_indirect)) {
				return {};
			}

			if (!Do_Expression_Dependecy_Pass(scope_id, as_binary_expression->Right, dependencies, ignore_indirect)) {
				return {};
			}
			return { true };
		}
		break;
		case NodeType::NumericLiteral:
		case NodeType::StringLiteral:
			return { true };
			break;
		case NodeType::MemberAccess: {

			MemberAccess* as_member_access = (MemberAccess*)expr;

			GS_CORE_ASSERT(as_member_access->Member);
			GS_CORE_ASSERT(as_member_access->Object);

			Dep_Result object_dep_result = Do_Expression_Dependecy_Pass(scope_id, as_member_access->Object, dependencies, ignore_indirect);

			if (!object_dep_result) {
				return {};
			}

			if (object_dep_result.referenced_entity != Entity_Null) {

				Entity& referenced_entity = Data.entity_storage[object_dep_result.referenced_entity];

				if (referenced_entity.entity_type == Entity_Type::Named_File_Load) {

					Entity& referenced_file_scope = Data.entity_storage[Data.File_ID_To_Scope.at(Data.entity_storage[referenced_entity.named_file_load.file_load_entity_id].file_load.loaded_file_id)];

					Identifier* member_as_ident = (Identifier*)as_member_access->Member;

					String member_name = String_Make(member_as_ident->Symbol.Symbol);

					Entity* member_entity = nullptr;
					Entity_ID member_entity_id = Entity_Null;

					for (size_t i = 0; i < referenced_file_scope.children.count; i++)
					{
						if (String_Equal(member_name, Data.entity_storage[referenced_file_scope.children[i]].semantic_name)) {
							member_entity = &Data.entity_storage[referenced_file_scope.children[i]];
							member_entity_id = referenced_file_scope.children[i];
						}
					}

					if (!member_entity)
					{
						Push_Error_Loc(scope_id, as_member_access->Object, FMT("name '{}' is not found in '{}'", member_as_ident->Symbol.Symbol, std::string(referenced_entity.semantic_name.data)));
						return {};
					}
					else {
						Array_Add(dependencies, member_entity_id);
						return { true };
					}
				}
				else if (referenced_entity.entity_type == Entity_Type::Constant) {

				}
				else if (referenced_entity.entity_type == Entity_Type::Enum_Entity) {

				}
				else {
					Push_Error_Loc(scope_id, as_member_access->Object, "expression does not support members");
					return {};
				}
			}

			return { true };
		}
								   break;
		case NodeType::TE_Pointer:
		{
			TypeExpressionPointer* as_pointer = (TypeExpressionPointer*)expr;
			ASSERT(as_pointer->Pointee);

			Array<Entity_ID> indirect_dependencies;

			if (!Do_Expression_Dependecy_Pass(scope_id, as_pointer->Pointee, indirect_dependencies, ignore_indirect)) {
				return {};
			}

			for (size_t i = 0; i < indirect_dependencies.count; i++)
			{
				if (indirect_dependencies[i] != ignore_indirect) {
					Array_Add(dependencies, indirect_dependencies[i]);
				}
			}

			return { true };
		}
		break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return {};
	}

	bool Front_End::Iterate_Tl_All_Files(std::function<bool(File_ID file_id, Entity_ID file_entity_id, Entity& file_entity, Statement* syntax)> f)
	{
		for (size_t i = 0; i < Data.entity_storage.count; i++)
		{
			if (Data.entity_storage[i].entity_type == Entity_Type::File_Scope) {
				Entity_ID file_entity_id = (Entity_ID)i;
				Entity& file_entity = Data.entity_storage[file_entity_id];

				ASSERT(file_entity.syntax_node);
				ASSERT(file_entity.syntax_node->GetType() == NodeType::ModuleFile);

				ModuleFile* as_mod_file = (ModuleFile*)file_entity.syntax_node;

				for (size_t i = 0; i < as_mod_file->Statements.size(); i++)
				{
					if (f(file_entity.file_scope.file_id, file_entity_id, file_entity, as_mod_file->Statements[i])) {
						return true;
					}
				}
			}
		}
		return false;
	}

	bool Front_End::Iterate_All_Files(std::function<bool(File_ID file_id, Entity_ID file_entity_id, Entity& file_entity)> f)
	{
		for (size_t i = 0; i < Data.entity_storage.count; i++)
		{
			if (Data.entity_storage[i].entity_type == Entity_Type::File_Scope) {
				Entity_ID file_entity_id = (Entity_ID)i;
				Entity& file_entity = Data.entity_storage[file_entity_id];

				ASSERT(file_entity.syntax_node);
				ASSERT(file_entity.syntax_node->GetType() == NodeType::ModuleFile);

				if (f(file_entity.file_scope.file_id, file_entity_id, file_entity)) {
					return true;
				}
			}
		}

		return false;
	}

	bool Front_End::Check_Circular_Dependencies(Entity_ID entity_id, Array<Entity_ID> dependencies, Array<Entity_ID>& chain)
	{
		for (size_t i = 0; i < dependencies.count; i++)
		{
			Entity_ID dependency = dependencies[i];

			if (dependency == entity_id) {
				Array_Add(chain, dependency);
				return true;
			}
			else {
				auto other_dependencies = Data.entity_storage[dependency].dependencies;

				if (Check_Circular_Dependencies(entity_id, other_dependencies, chain)) {
					Array_Add(chain, dependency);
					return true;
				}
			}
		}

		return false;
	}

	void Front_End::Push_Error(const std::string& error)
	{
		Array_Add(Data.Messages, Front_End_Message{ error, Message_Error });
	}

	void Front_End::Push_Warn(const std::string& warn)
	{
		Array_Add(Data.Messages, Front_End_Message{ warn, Message_Warning });
	}

	void Front_End::Push_Error_Loc(Entity_ID scope_id, Statement* stmt, const std::string& error)
	{
		File_ID file_id = Data.entity_storage[Front_End_Data::Get_File_Scope_Parent(Data, scope_id)].file_scope.file_id;
		ASSERT(file_id != File_Null);

		const auto& token = stmt->GetLocation();

		Push_Warn(error);
		Push_Error(fmt::format("{}:{}:{}", Data.Files[file_id].Path.string(), token.Line + 1, token.Begin));
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
			Array_Add(parent.children, entity_identifier);
			entity.parent = parent_id;
		}

		Array_Add(Data.entity_storage, entity);

		return entity_identifier;
	}

	Source_Loc Front_End::Loc_From_Token(const Token& tk)
	{
		return Source_Loc{ (u32)(tk.Line + 1), (u16)(tk.Begin + 1) };
	}

	Entity Front_End::Create_File_Scope_Entity(File_ID file_id, Statement* syntax)
	{
		ASSERT(file_id != File_Null);

		Front_End_File& file = Data.Files[file_id];

		Entity file_entity;
		file_entity.entity_type = Entity_Type::File_Scope;
		file_entity.semantic_name = String_Make(file.Path.filename().string());
		file_entity.source_location = { 0,0 };
		file_entity.syntax_node = syntax;

		file_entity.file_scope.file_id = file_id;

		return file_entity;
	}

	Entity Front_End::Create_File_Load_Entity(File_ID loaded_file_id, File_ID file_id)
	{
		ASSERT(loaded_file_id != File_Null);
		Front_End_File& file = Data.Files[loaded_file_id];

		Entity file_load_entity;
		file_load_entity.entity_type = Entity_Type::File_Load;
		file_load_entity.semantic_name = String_Make("load " + file.Path.string());
		file_load_entity.source_location = { 0,0 };
		file_load_entity.definition_file = file_id;

		file_load_entity.file_load.loaded_file_id = loaded_file_id;

		return file_load_entity;
	}

	Entity Front_End::Create_File_Named_Load_Entity(Entity_ID file_load_entity_id, String name, Source_Loc source_location, File_ID file_id)
	{
		Entity named_file_load_entity;

		named_file_load_entity.entity_type = Entity_Type::Named_File_Load;
		named_file_load_entity.semantic_name = String_Copy(name);
		named_file_load_entity.source_location = source_location;
		named_file_load_entity.definition_file = file_id;

		named_file_load_entity.named_file_load.file_load_entity_id = file_load_entity_id;

		return named_file_load_entity;
	}

	Entity Front_End::Create_Constant_Entity(Const_Union value, String name, Source_Loc source_location, File_ID file_id)
	{
		Entity constant_entity = { 0 };

		constant_entity.entity_type = Entity_Type::Constant;
		constant_entity.semantic_name = String_Copy(name);
		constant_entity.source_location = { 0,0 };
		constant_entity.definition_file = file_id;

		constant_entity.constant.value_as = value;

		return constant_entity;
	}


	Entity Front_End::Create_TypeName_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity tn_entity = { 0 };

		tn_entity.entity_type = Entity_Type::Type_Name_Entity;
		tn_entity.semantic_name = String_Copy(name);
		tn_entity.source_location = source_location;
		tn_entity.definition_file = file_id;
		tn_entity.semantic_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.Type_tn);

		tn_entity.type_name.type_name_id = (Type_Name_ID)-1;

		return tn_entity;
	}

	Entity Front_End::Create_Struct_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity struct_entity = { 0 };

		struct_entity.entity_type = Entity_Type::Struct_Entity;
		struct_entity.semantic_name = String_Copy(name);
		struct_entity.source_location = source_location;
		struct_entity.definition_file = file_id;
		struct_entity.semantic_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.Type_tn);

		struct_entity.struct_entity.type_name_id = (Type_Name_ID)-1;

		return struct_entity;
	}

	Entity Front_End::Create_Struct_Member_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity struct_member_entity = { 0 };

		struct_member_entity.entity_type = Entity_Type::Struct_Member_Entity;
		struct_member_entity.semantic_name = String_Copy(name);
		struct_member_entity.source_location = source_location;
		struct_member_entity.definition_file = file_id;
		struct_member_entity.semantic_type = nullptr;

		return struct_member_entity;
	}

	Entity Front_End::Create_Enum_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity enum_entity = { 0 };

		enum_entity.entity_type = Entity_Type::Enum_Entity;
		enum_entity.semantic_name = String_Copy(name);
		enum_entity.source_location = source_location;
		enum_entity.definition_file = file_id;
		enum_entity.semantic_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.Type_tn);

		enum_entity.enum_entity.flags_enum = false;
		enum_entity.enum_entity.type_name_id = (Type_Name_ID)-1;

		return enum_entity;
	}

	Entity Front_End::Create_Enum_Member_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity enum_member_entity = { 0 };

		enum_member_entity.entity_type = Entity_Type::Enum_Member_Entity;
		enum_member_entity.semantic_name = String_Copy(name);
		enum_member_entity.source_location = source_location;
		enum_member_entity.definition_file = file_id;
		enum_member_entity.semantic_type = nullptr;

		enum_member_entity.enum_member.value = { 0 };

		return enum_member_entity;
	}

	Entity_ID Front_End_Data::Get_Top_Most_Parent(Front_End_Data& data, Entity_ID entity_id)
	{
		Entity& entity = data.entity_storage[entity_id];

		if (entity.parent == Entity_Null) {
			return entity_id;
		}
		else {
			return Get_Top_Most_Parent(data, entity.parent);
		}
	}

	Entity_ID Front_End_Data::Get_File_Scope_Parent(Front_End_Data& data, Entity_ID entity_id)
	{
		Entity& entity = data.entity_storage[entity_id];

		if (entity.entity_type == Entity_Type::File_Scope) {
			return entity_id;
		}

		if (entity.parent == Entity_Null) {
			return entity_id;
		}
		else {

			Entity& parent = data.entity_storage[entity.parent];

			if (parent.entity_type == Entity_Type::File_Scope) {
				return entity.parent;
			}

			return Get_File_Scope_Parent(data, entity.parent);
		}
	}

	Entity_ID Front_End_Data::Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID scope_id, String name)
	{
		Entity_ID visited_load = Get_File_Scope_Parent(data, scope_id);
		ASSERT(data.entity_storage[visited_load].entity_type == Entity_Type::File_Scope);

		return Get_Entity_ID_By_Name(data, scope_id, name, visited_load, Entity_Null);
	}

	Entity_ID Front_End_Data::Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID scope_id, String name, Entity_ID visited_load, Entity_ID visited_parent)
	{
		ASSERT(scope_id != Entity_Null);

		Entity& scope_entity = data.entity_storage[scope_id];

		for (size_t i = 0; i < scope_entity.children.count; i++)
		{
			Entity_ID scope_child_id = scope_entity.children[i];
			ASSERT(scope_child_id != Entity_Null);

			Entity& child_entity = data.entity_storage[scope_child_id];

			if (strcmp(child_entity.semantic_name.data, name.data) == 0) {
				return scope_child_id;
			}

			if (child_entity.entity_type == Entity_Type::File_Load) {

				Entity_ID file_load_file_scope = data.File_ID_To_Scope.at(child_entity.file_load.loaded_file_id);

				if (visited_load != file_load_file_scope) {

					Entity_ID found_in_load = Get_Entity_ID_By_Name(data, file_load_file_scope, name, visited_load, visited_parent);

					if (found_in_load != Entity_Null)
					{
						return found_in_load;
					}
				}
			}
		}

		if (scope_entity.parent != Entity_Null) {

			if (scope_entity.parent == 0) { // global scope
				if (visited_parent == Entity_Null) {
					return Get_Entity_ID_By_Name(data, scope_entity.parent, name, visited_load, 0);
				}
			}
			else
			{
				return Get_Entity_ID_By_Name(data, scope_entity.parent, name, visited_load, visited_parent);
			}
		}

		return Entity_Null;
	}
}