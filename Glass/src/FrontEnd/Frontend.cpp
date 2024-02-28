#include "pch.h"

#include "FrontEnd/Frontend.h"
#include "FrontEnd/Lexer.h"
#include "BackEnd/MC_Gen.h"
#include "BackEnd/LLVM_Converter.h"
#include "microsoft_craziness.h"

#define FMT(...) fmt::format(__VA_ARGS__)

namespace Glass
{
	String Operator_To_String(Operator op)
	{
		switch (op)
		{
		case Operator::Add:
			return String_Make("+");
		case Operator::Subtract:
			return String_Make("-");
		case Operator::Multiply:
			return String_Make("*");
		case Operator::Modulo:
			return String_Make("%");
		case Operator::Divide:
			return String_Make("/");
		case Operator::Assign:
			return String_Make("=");
		case Operator::AddAssign:
			return String_Make("+=");
		case Operator::SubAssign:
			return String_Make("-=");
		case Operator::MulAssign:
			return String_Make("*=");
		case Operator::DivAssign:
			return String_Make("/=");
		case Operator::BitOrAssign:
			return String_Make("|=");
		case Operator::BitAndAssign:
			return String_Make("&=");
		case Operator::Not:
			return String_Make("!");
		case Operator::Equal:
			return String_Make("==");
		case Operator::NotEqual:
			return String_Make("!=");
		case Operator::GreaterThan:
			return String_Make(">");
		case Operator::LesserThan:
			return String_Make("<");
		case Operator::GreaterThanEq:
			return String_Make(">=");
		case Operator::LesserThanEq:
			return String_Make("<=");
		case Operator::BitAnd:
			return String_Make("&");
		case Operator::BitOr:
			return String_Make("|");
		case Operator::And:
			return String_Make("and");
		case Operator::Or:
			return String_Make("or");
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return String_Make("unknown operator");
	}

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

		GS_PROFILE_FUNCTION();

		Il_Program_Init(Data.il_program, &Data.type_system);
		TypeSystem_Init(Data.type_system);

		// to avoid pointers to these pools get invalidated by insertions
		Data.Files = Array_Reserved<Front_End_File>(1024);
		Data.entity_storage = Array_Reserved<Entity>(1024); // about 10 megs

		Entity global_scope_entity;
		global_scope_entity.semantic_name = String_Make("@global_scope");
		global_scope_entity.entity_type = Entity_Type::Global_Scope;

		Data.global_scope_entity = Insert_Entity(global_scope_entity);

		Data.exec_engine.type_system = &Data.type_system;
		Data.exec_engine.program = &Data.il_program;

		auto insert_base_type_name = [this](String name, u64 size, u64 alignment, bool is_signed, bool is_float, bool numeric = true, bool bool_type = false) -> auto {
			Type_Name tn;
			tn.size = size;
			tn.alignment = alignment;
			tn.name = name;
			tn.flags = TN_Base_Type;

			if (numeric) {
				tn.flags |= TN_Numeric_Type;
			}

			if (!is_signed) {
				tn.flags |= TN_Unsigned_Type;
			}

			if (is_float) {
				tn.flags |= TN_Float_Type;
			}

			if (bool_type) {
				tn.flags |= TN_Bool_Type;
			}

			Type_Name_ID type_name_id = TypeSystem_Insert_TypeName(Data.type_system, tn);

			Entity type_name_entity = Create_TypeName_Entity(name, { 0,0 }, -1);
			type_name_entity.flags |= EF_Constant;
			type_name_entity.type_name.type_name_id = type_name_id;
			type_name_entity.constant_value.type = TypeSystem_Get_Basic_Type(Data.type_system, type_name_id);
			type_name_entity.semantic_type = Data.Type_Ty;

			Data.typename_to_entity_id[type_name_id] = Insert_Entity(type_name_entity, Data.global_scope_entity);

			return type_name_id;
		};

		{
			Type_Name tn_Type;
			tn_Type.size = 8;
			tn_Type.alignment = 8;
			tn_Type.name = String_Make("Type");
			tn_Type.flags = TN_Base_Type;

			Type_Name_ID Type_type_name_id = TypeSystem_Insert_TypeName(Data.type_system, tn_Type);

			Data.Type_tn = Type_type_name_id;

			Data.Type_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Type_type_name_id);

			Entity Type_type_name_entity = Create_TypeName_Entity(tn_Type.name, { 0,0 }, -1);
			Type_type_name_entity.semantic_name = String_Make("Type");
			Type_type_name_entity.flags |= EF_Constant;
			Type_type_name_entity.type_name.type_name_id = Type_type_name_id;
			Type_type_name_entity.semantic_type = Data.Type_Ty;
			Type_type_name_entity.constant_value.type = Data.Type_Ty;

			Data.typename_to_entity_id[Type_type_name_id] = Insert_Entity(Type_type_name_entity, Data.global_scope_entity);

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
			Data.void_ptr_Ty = TypeSystem_Get_Pointer_Type(Data.type_system, Data.void_Ty, 1);

			Data.type_system.void_Ty = Data.void_Ty;

			Entity void_type_name_entity = Create_TypeName_Entity(tn_Void.name, { 0,0 }, -1);
			void_type_name_entity.semantic_name = String_Make("void");
			void_type_name_entity.flags |= EF_Constant;
			void_type_name_entity.type_name.type_name_id = void_type_name_id;
			void_type_name_entity.semantic_type = Data.Type_Ty;
			void_type_name_entity.constant_value.type = Data.void_Ty;

			Data.typename_to_entity_id[void_type_name_id] = Insert_Entity(void_type_name_entity, Data.global_scope_entity);
		}

		Data.bool_tn = insert_base_type_name(String_Make("bool"), 1, 1, true, false, false, true);
		Data.bool_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.bool_tn);

		Data.int_tn = insert_base_type_name(String_Make("int"), 4, 4, true, false);
		Data.int_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.int_tn);

		Data.i8_tn = insert_base_type_name(String_Make("i8"), 1, 1, true, false);
		Data.i16_tn = insert_base_type_name(String_Make("i16"), 2, 2, true, false);
		Data.i32_tn = insert_base_type_name(String_Make("i32"), 4, 4, true, false);
		Data.i64_tn = insert_base_type_name(String_Make("i64"), 8, 8, true, false);

		Data.i8_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.i8_tn);
		Data.i16_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.i16_tn);
		Data.i32_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.i32_tn);
		Data.i64_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.i64_tn);

		Data.u8_tn = insert_base_type_name(String_Make("u8"), 1, 1, false, false);
		Data.u16_tn = insert_base_type_name(String_Make("u16"), 2, 2, false, false);
		Data.u32_tn = insert_base_type_name(String_Make("u32"), 4, 4, false, false);
		Data.u64_tn = insert_base_type_name(String_Make("u64"), 8, 8, false, false);

		Data.u8_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.u8_tn);
		Data.u16_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.u16_tn);
		Data.u32_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.u32_tn);
		Data.u64_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.u64_tn);

		Data.float_tn = insert_base_type_name(String_Make("float"), 4, 4, false, true);
		Data.float_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.float_tn);

		Data.f32_tn = insert_base_type_name(String_Make("f32"), 4, 4, true, true);
		Data.f64_tn = insert_base_type_name(String_Make("f64"), 8, 8, true, true);

		Data.f32_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.f32_tn);
		Data.f64_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.f64_tn);

		Data.type_system.i8_Ty = Data.i8_Ty;
		Data.type_system.i16_Ty = Data.i16_Ty;
		Data.type_system.i32_Ty = Data.i32_Ty;
		Data.type_system.i64_Ty = Data.i64_Ty;

		Data.type_system.u8_Ty = Data.u8_Ty;
		Data.type_system.u16_Ty = Data.u16_Ty;
		Data.type_system.u32_Ty = Data.u32_Ty;
		Data.type_system.u64_Ty = Data.u64_Ty;

		Data.type_system.f64_Ty = Data.f64_Ty;
		Data.type_system.f32_Ty = Data.f32_Ty;

		Data.type_system.float_Ty = Data.float_Ty;
		Data.type_system.int_Ty = Data.int_Ty;
		Data.type_system.void_ptr_Ty = Data.void_ptr_Ty;
		Data.type_system.Type_Ty = Data.Type_Ty;

		auto begin = std::chrono::high_resolution_clock::now();

		if (Load_Base()) {
			return;
		}

		Load_First();

		{
			Il_Global struct_member_typeinfo_global;
			struct_member_typeinfo_global.type = Data.void_Ty;
			struct_member_typeinfo_global.name = String_Make("__TypeInfo_Members_Array__");
			struct_member_typeinfo_global.initializer = -1;

			if (Options.Backend == Backend_Option::LLVM_Backend) {
				struct_member_typeinfo_global_idx = Il_Insert_Global(Data.il_program, struct_member_typeinfo_global);
			}

			Il_Global type_info_table_global;
			type_info_table_global.type = Data.void_Ty;
			type_info_table_global.name = String_Make("__TypeInfo_Table__");
			type_info_table_global.initializer = -1;

			if (Options.Backend == Backend_Option::LLVM_Backend) {
				global_type_info_table_idx = Il_Insert_Global(Data.il_program, type_info_table_global);
			}

			GS_Type* c_str_type = TypeSystem_Get_Pointer_Type(Data.type_system, TypeSystem_Get_Basic_Type(Data.type_system, Data.u8_tn), 1);
			ASSERT(c_str_type);

			GS_Struct te_struct;

			Type_Name struct_type_name;
			struct_type_name.name = String_Make("TypeInfoEntry");
			struct_type_name.flags = TN_Struct_Type;
			struct_type_name.size = 64;
			struct_type_name.alignment = 0;

			auto te_type_name_id = TypeSystem_Insert_TypeName_Struct(Data.type_system, struct_type_name, te_struct);
			te_Ty = TypeSystem_Get_Basic_Type(Data.type_system, te_type_name_id);

			Data.typename_to_entity_id[te_type_name_id] = Entity_Null;

#define TE_MEM_COUNT 8

			for (size_t i = 0; i < TE_MEM_COUNT; i++)
			{
				Array_Add(te_struct.members, Data.u64_Ty);
				Array_Add(te_struct.offsets, (u64)-1);
			}

			GS_Struct_Data_Layout te_data_layout = TypeSystem_Struct_Compute_Align_Size_Offsets(Data.type_system, te_struct.members);

			for (size_t i = 0; i < te_struct.offsets.count; i++)
			{
				te_struct.offsets[i] = te_data_layout.offsets[i];
			}

			Data.type_system.type_name_storage[te_type_name_id].size = te_data_layout.size;
			Data.type_system.type_name_storage[te_type_name_id].alignment = te_data_layout.alignment;
			Data.type_system.struct_storage[Data.type_system.type_name_storage[te_type_name_id].struct_id] = te_struct;
		}

		Timer timer;
		if (Do_Tl_Dcl_Passes()) {
			return;
		}
		GS_CORE_INFO("declaration pass took: {} s", timer.Elapsed());

		timer.Reset();
		if (Do_Tl_Dependency_Passes()) {
			return;
		}
		GS_CORE_INFO("global dependecy pass took: {} s", timer.Elapsed());

		timer.Reset();

		if (Do_Tl_Resolution_Passes()) {
			return;
		}

		GS_CORE_INFO("dependecy resolve pass took: {} s", timer.Elapsed());

		timer.Reset();
		if (Do_CodeGen())
			return;

		GS_CORE_INFO("emit il time: {} s", timer.Elapsed());

		if (print_il)
		{
			GS_PROFILE_SCOPE("print il");
			std::ofstream il_file(".bin/program.il");
			il_file << printed_il_stream.str();
			il_file.close();
		}

		Data.type_system.Array_Ty = Data.Array_Ty;
		Data.type_system.Any_Ty = Data.Any_Ty;
		Data.type_system.string_Ty = Data.string_Ty;

		auto end = std::chrono::high_resolution_clock::now();

		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
		double front_end_time_f = microseconds;
		front_end_time_f /= 1000000.0;

		double parse_time_f = Data.parse_time_micro_seconds;
		parse_time_f /= 1000000.0;

		double lex_time_f = Data.lex_time_micro_seconds;
		lex_time_f /= 1000000.0;

		double emit_il_time_f = Data.emit_il_time;
		emit_il_time_f /= 1000000.0;

		double search_time_f = Data.entity_search_time;
		search_time_f /= 1000000000.0;

		GS_CORE_INFO("frontend took: {} s", front_end_time_f - (parse_time_f + lex_time_f));
		GS_CORE_INFO("frontend total took: {} s", front_end_time_f);
		GS_CORE_INFO("resolution pass iteration count: {}", Data.resolution_pass_iteration_count);
		GS_CORE_INFO("enitity search took: {} s, total loops: {}", search_time_f, Data.entity_search_loop_count);
		GS_CORE_INFO("lexing took: {} s", parse_time_f);
		GS_CORE_INFO("lines processed: {}", g_LinesProcessed);
		GS_CORE_INFO("parsing took: {} s", lex_time_f);

		Generate_Output();
	}

	void Front_End::Generate_Output()
	{
		//auto result = EE_Exec_Program(Data.exec_engine, &Data.il_program, main_proc_idx);
		//GS_CORE_INFO("main returned: {}", result.us8);

		GS_PROFILE_FUNCTION();

		std::filesystem::create_directories(".bin");

		bool llvm = Options.Backend == Backend_Option::LLVM_Backend;

		std::string linked_objects;

		if (llvm)
		{
			GS_PROFILE_SCOPE("run llvm converter");

			LLVM_Converter_Spec lc_spec;
			lc_spec.output_path = String_Make(".bin/llvm.obj");

			LLVM_Converter llvm_converter = LLVM_Converter_Make(lc_spec, &Data.il_program);
			auto begin = std::chrono::high_resolution_clock::now();
			LLVMC_Run(llvm_converter);
			auto end = std::chrono::high_resolution_clock::now();

			auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
			double llvm_time_f = microseconds;
			llvm_time_f /= 1000000.0;

			GS_CORE_INFO("llvm took: {}", llvm_time_f);

			linked_objects = "./.bin/llvm.obj";
		}
		else
		{
			GS_PROFILE_SCOPE("run il mc generator");

			MC_Gen_Spec mc_gen_spec;
			mc_gen_spec.output_path = String_Make(".bin/il.obj");

			MC_Gen mc_generator = MC_Gen_Make(mc_gen_spec, &Data.il_program);
			MC_Gen_Run(mc_generator);

			linked_objects = "./.bin/il.obj";

		}

		bool disassemble = Options.Dissassemble;

		if (disassemble)
		{
			system("objdump.exe --no-show-raw-insn -D -Mintel ./.bin/il.obj > ./.bin/il.s");
		}

		//auto result = EE_Exec_Proc(Data.exec_engine, Data.il_program.procedures[main_proc_idx], {});

		std::string output_exe_name = "a.exe";
		Find_Result find_result;

		{
			GS_PROFILE_SCOPE("find windows sdk");
			find_result = find_visual_studio_and_windows_sdk();
		}

		fs_path msvc_linker_path = find_result.vs_exe_path;
		msvc_linker_path.append("link.exe");

		std::string linker_options = "/nologo /ignore:4210 /NODEFAULTLIB /IMPLIB:C /DEBUG:FULL /SUBSYSTEM:CONSOLE /INCREMENTAL:NO";

		std::string program_libraries;

		for (auto& added_library_path : Data.Added_Libraries_Paths)
		{
			program_libraries.append(added_library_path);
			program_libraries.append(".lib");
			program_libraries.append(" ");
		}

		std::wstring ucrt_lib_path_w = find_result.windows_sdk_ucrt_library_path;
		std::wstring um_lib_path_w = find_result.windows_sdk_um_library_path;
		std::wstring vs_lib_path_w = find_result.vs_library_path;

		fs_path ucrt_lib_path = ucrt_lib_path_w;
		fs_path um_lib_path = um_lib_path_w;
		fs_path vs_lib_path = vs_lib_path_w;

		std::string linker_lib_paths = fmt::format("/LIBPATH:\"{}\" /LIBPATH:\"{}\" /LIBPATH:\"{}\"", vs_lib_path.string(), um_lib_path.string(), ucrt_lib_path.string());

		std::string linker_command = fmt::format("{} {} {} {} {} /OUT:{}", msvc_linker_path.string(), linker_lib_paths, program_libraries, linker_options, linked_objects, output_exe_name);

		GS_CORE_INFO("running linker: {}", linker_command);

		{
			GS_PROFILE_SCOPE("run msvc-linker");
			system(linker_command.c_str());
		}

		//free_resources(&find_result);
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
		GS_PROFILE_FUNCTION();

		Entity& file_entity = Data.entity_storage[entity_id];
		ASSERT(file_entity.entity_type == Entity_Type::File_Scope);

		auto& current_file = Data.Files[file_entity.file_scope.file_id];

		fs_path path = load_node->FileName->Symbol.Symbol;
		fs_path path_abs = normalizePath(std::filesystem::absolute(path));

		File_ID loaded_file_id = File_Null;

		if (!std::filesystem::exists(path_abs)) {

			fs_path relative_to_file_path = current_file.Path / "../" / path;

			if (!std::filesystem::exists(relative_to_file_path))
			{
				Push_Error_Loc(entity_id, load_node, FMT("load directive file not found!: '{}'", path.string()));
			}
			else
			{
				path = relative_to_file_path;
				path_abs = normalizePath(std::filesystem::absolute(path));
			}
		}

		auto previous_loaded_file_it = Data.Path_To_File.find(path_abs.string());

		if (previous_loaded_file_it == Data.Path_To_File.end()) {

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

	bool Front_End::Do_Tl_Dcl_Passes()
	{
		GS_PROFILE_FUNCTION();

		return Iterate_Tl_All_Files([this](File_ID file_id, Entity_ID file_entity_id, Entity& file_entity, Statement* statement) -> bool {

			if (statement->GetType() == NodeType::Variable) {

				VariableNode* as_var = (VariableNode*)statement;

				String var_name = String_Make(as_var->Symbol.Symbol);

				Entity_ID previous_definition = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, var_name);

				if (as_var->Constant) {

					if (previous_definition != Entity_Null) {
						Push_Error_Loc(file_entity_id, as_var, FMT("global constant declaration '{}' name already taken", as_var->Symbol.Symbol));
						return true;
					}

					GS_CORE_ASSERT(as_var->Assignment);

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
				else
				{
					if (previous_definition != Entity_Null) {
						Push_Error_Loc(file_entity_id, as_var, FMT("global variable declaration '{}', name already taken", as_var->Symbol.Symbol));
						return true;
					}

					Entity global_variable_entity = Create_Variable_Entity(
						var_name,
						Loc_From_Token(as_var->Symbol), file_entity.file_scope.file_id);

					global_variable_entity.flags |= EF_InComplete;
					global_variable_entity.syntax_node = as_var;

					Insert_Entity(global_variable_entity, file_entity_id);
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

				struct_entity.struct_entity.poly_morphic = as_struct_node->argument_list != nullptr;

				Entity_ID inserted_struct_entity_id = Insert_Entity(struct_entity, file_entity_id);

				if (struct_entity.struct_entity.poly_morphic)
					return false;

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

				FunctionNode* as_func_node = (FunctionNode*)statement;

				String func_name = String_Make(as_func_node->Symbol.Symbol);

				Entity_ID previous_definition = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, func_name);

				if (previous_definition != Entity_Null) {
					Push_Error_Loc(file_entity_id, as_func_node, FMT("function '{}' has its name already taken", as_func_node->Symbol.Symbol));
					return true;
				}

				Entity new_function_entity = Create_Function_Entity(func_name, Loc_From_Token(as_func_node->Symbol), file_entity.file_scope.file_id);
				new_function_entity.flags = EF_InComplete | EF_Constant;
				new_function_entity.syntax_node = as_func_node;

				if (as_func_node->foreign_directive) {
					new_function_entity.func.foreign = true;
				}

				if (as_func_node->CVariadic) {

					if (!as_func_node->foreign_directive) {
						Push_Error_Loc(file_entity_id, as_func_node, FMT("function '{}' with ! declared C variadic is not foreign", as_func_node->Symbol.Symbol));
						return true;
					}

					new_function_entity.func.c_variadic = true;
				}

				Entity_ID inserted_function_entity_id = Insert_Entity(new_function_entity, file_entity_id);

				Entity& inserted_entity = Data.entity_storage[inserted_function_entity_id];

				bool is_poly_morphic = false;

				for (size_t i = 0; i < as_func_node->argumentList->Arguments.size(); i++)
				{
					ArgumentNode* as_arg = (ArgumentNode*)as_func_node->argumentList->Arguments[i];

					Function_Parameter parameter = {};
					parameter.name = String_Make(as_arg->Symbol.Symbol);

					if (as_arg->PolyMorphic)
						is_poly_morphic = true;

					for (size_t i = 0; i < inserted_entity.func.parameters.count; i++)
					{
						if (String_Equal(inserted_entity.func.parameters[i].name, parameter.name)) {
							Push_Error_Loc(file_entity_id, as_arg, FMT("function '{}' parameter '{}' already defined", as_func_node->Symbol.Symbol, as_arg->Symbol.Symbol));
							return true;
						}
					}

					Array_Add(inserted_entity.func.parameters, parameter);
				}

				inserted_entity.func.poly_morphic = is_poly_morphic;
			}
			else if (statement->GetType() == NodeType::Library) {

				LibraryNode* as_library_node = (LibraryNode*)statement;

				String library_name = String_Make(as_library_node->Name.Symbol);

				Entity_ID previous_definition = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, library_name);

				if (previous_definition != Entity_Null) {
					Push_Error_Loc(file_entity_id, as_library_node, FMT("library '{}' has its name already taken", as_library_node->Name.Symbol));
					return true;
				}

				Entity library_entity = Create_Library_Entity(library_name, Loc_From_Token(as_library_node->Name), file_entity.file_scope.file_id);
				library_entity.library.path = String_Make(as_library_node->FileName->Symbol.Symbol);

				library_entity.library.library_node_idx = Il_Insert_Library(Data.il_program, String_Make(as_library_node->FileName->Symbol.Symbol + ".dll"));

				Insert_Entity(library_entity, file_entity_id);
			}
			else if (statement->GetType() == NodeType::AddLibrary) {
				AddLibraryNode* as_add_library_node = (AddLibraryNode*)statement;
				Data.Added_Libraries_Paths.emplace(as_add_library_node->FileName->Symbol.Symbol);
			}
			else if (statement->GetType() == NodeType::Operator)
			{
				OperatorNode* as_operator = (OperatorNode*)statement;

				String operator_name = Operator_To_String(as_operator->OPerator);

				Entity overload_entity = Create_Operator_Overload(operator_name, Loc_From_Token(as_operator->GetLocation()), file_entity.file_scope.file_id);

				overload_entity.operator_overload._operator = as_operator->OPerator;
				overload_entity.flags = EF_InComplete;
				overload_entity.syntax_node = as_operator;

				Insert_Entity(overload_entity, file_entity_id);
			}
			return false;
			});
	}

	bool Front_End::Do_Tl_Dependency_Passes()
	{
		GS_PROFILE_FUNCTION();

		return Iterate_All_Files([this](File_ID file_id, Entity_ID file_entity_id, Entity& file_entity) -> bool {

			for (size_t i = 0; i < file_entity.children.count; i++)
			{
				Entity_ID tl_file_scope_entity_id = file_entity.children[i];
				Entity& tl_entity = Data.entity_storage[tl_file_scope_entity_id];

				if (tl_entity.flags & EF_InComplete) {
					if (tl_entity.entity_type == Entity_Type::Constant) {

						GS_CORE_ASSERT(tl_entity.syntax_node);
						GS_CORE_ASSERT(tl_entity.syntax_node->GetType() == NodeType::Variable);

						VariableNode* as_var = (VariableNode*)tl_entity.syntax_node;

						if (!as_var->Assignment) {
							Push_Error_Loc(file_entity_id, as_var, FMT("global constant '{}' is not assigned a value", as_var->Symbol.Symbol));
							return true;
						}

						if (!Do_Expression_Dependecy_Pass(file_entity_id, as_var->Assignment, tl_entity.dependencies, Entity_Null)) {
							return true;
						}

						Array<Entity_ID> chain;

						if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_entity.dependencies, chain)) {

							std::string printed_chain;

							printed_chain = tl_entity.semantic_name.data;

							for (size_t i = 0; i < chain.count; i++)
							{
								printed_chain += " -> ";
								printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
							}

							printed_chain += " -> ";

							printed_chain += tl_entity.semantic_name.data;

							Entity& tail_entity = Data.entity_storage[chain[0]];

							Push_Error_Loc(file_entity_id, as_var, FMT("global constant '{}' has circular dependency: {}", as_var->Symbol.Symbol, printed_chain));
							Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

							return true;
						}

						if (as_var->Type) {

							if (!Do_Expression_Dependecy_Pass(file_entity_id, as_var->Type, tl_entity.dependencies, Entity_Null)) {
								return true;
							}

							Array<Entity_ID> chain;

							if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_entity.dependencies, chain)) {

								std::string printed_chain;

								printed_chain = tl_entity.semantic_name.data;

								for (size_t i = 0; i < chain.count; i++)
								{
									printed_chain += " -> ";
									printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
								}

								printed_chain += " -> ";

								printed_chain += tl_entity.semantic_name.data;

								Entity& tail_entity = Data.entity_storage[chain[0]];

								Push_Error_Loc(file_entity_id, as_var->Type, FMT("the type of global constant '{}' has circular dependency: {}", as_var->Symbol.Symbol, printed_chain));
								Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

								return true;
							}
						}
					}
					else if (tl_entity.entity_type == Entity_Type::Struct_Entity) {

						StructNode* as_struct_node = (StructNode*)tl_entity.syntax_node;

						if (tl_entity.struct_entity.poly_morphic)
						{
							for (size_t i = 0; i < as_struct_node->argument_list->Arguments.size(); i++)
							{
								ArgumentNode* as_arg = (ArgumentNode*)as_struct_node->argument_list->Arguments[i];

								if (!Do_Expression_Dependecy_Pass(file_entity_id, as_arg->Type, tl_entity.dependencies, tl_file_scope_entity_id))
									return true;
							}
						}
						else {

							for (size_t i = 0; i < tl_entity.children.count; i++)
							{
								Entity& struct_member_entity = Data.entity_storage[tl_entity.children[i]];
								VariableNode* as_variable_node = (VariableNode*)struct_member_entity.syntax_node;

								ASSERT(as_variable_node->Type);

								if (!Do_Expression_Dependecy_Pass(file_entity_id, as_variable_node->Type, tl_entity.dependencies, tl_file_scope_entity_id))
									return true;

								Array<Entity_ID> chain;
								if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_entity.dependencies, chain)) {

									std::string printed_chain;

									printed_chain = tl_entity.semantic_name.data;

									for (size_t i = 0; i < chain.count; i++)
									{
										printed_chain += " -> ";
										printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
									}

									printed_chain += " -> ";

									printed_chain += tl_entity.semantic_name.data;

									Entity& tail_entity = Data.entity_storage[chain[0]];

									Push_Error_Loc(file_entity_id, as_variable_node->Type, FMT("struct member '{}' type has circular dependency: {}", as_variable_node->Symbol.Symbol, printed_chain));
									Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

									return true;
								}
							}
						}
					}
					else if (tl_entity.entity_type == Entity_Type::Enum_Entity) {

						for (size_t i = 0; i < tl_entity.children.count; i++)
						{
							Entity& enum_member_entity = Data.entity_storage[tl_entity.children[i]];

							if (enum_member_entity.syntax_node->GetType() == NodeType::Variable) {

								VariableNode* as_variable_node = (VariableNode*)enum_member_entity.syntax_node;

								ASSERT(as_variable_node->Assignment);

								if (!Do_Expression_Dependecy_Pass(file_entity_id, as_variable_node->Assignment, tl_entity.dependencies, tl_file_scope_entity_id))
									return true;

								Array<Entity_ID> chain;
								if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_entity.dependencies, chain)) {

									std::string printed_chain;

									printed_chain = tl_entity.semantic_name.data;

									for (size_t i = 0; i < chain.count; i++)
									{
										printed_chain += " -> ";
										printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
									}

									printed_chain += " -> ";

									printed_chain += tl_entity.semantic_name.data;

									Entity& tail_entity = Data.entity_storage[chain[0]];

									Push_Error_Loc(file_entity_id, as_variable_node, FMT("enum member '{}' type has circular dependency: {}", as_variable_node->Symbol.Symbol, printed_chain));
									Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

									return true;
								}
							}
						}
					}
					else if (tl_entity.entity_type == Entity_Type::Function) {

						if (tl_entity.func.poly_morphic)
							continue;

						FunctionNode* as_func_node = (FunctionNode*)tl_entity.syntax_node;

						if (tl_entity.func.foreign) {

							ASSERT(as_func_node->foreign_directive);

							String foreign_library_name = String_Make(as_func_node->foreign_directive->library_name->Symbol.Symbol);

							Entity_ID foreign_library_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, foreign_library_name);

							if (foreign_library_entity_id == Entity_Null) {
								Push_Error_Loc(file_entity_id, as_func_node->foreign_directive, FMT("library '{}' is not defined", as_func_node->foreign_directive->library_name->Symbol.Symbol));
								return true;
							}

							if (Data.entity_storage[foreign_library_entity_id].entity_type != Entity_Type::Library) {
								Push_Error_Loc(file_entity_id, as_func_node->foreign_directive, FMT("'{}' is not a library", as_func_node->foreign_directive->library_name->Symbol.Symbol));
								return true;
							}

							tl_entity.func.library_entity_id = foreign_library_entity_id;
							Array_Add(tl_entity.dependencies, foreign_library_entity_id);
						}

						for (size_t i = 0; i < as_func_node->argumentList->Arguments.size(); i++)
						{
							ArgumentNode* as_arg = (ArgumentNode*)as_func_node->argumentList->Arguments[i];

							if (!Do_Expression_Dependecy_Pass(file_entity_id, as_arg->Type, tl_entity.dependencies, tl_file_scope_entity_id))
								return true;
						}

						if (as_func_node->ReturnType) {
							if (!Do_Expression_Dependecy_Pass(file_entity_id, as_func_node->ReturnType, tl_entity.dependencies, tl_file_scope_entity_id))
								return true;
						}
					}
					else if (tl_entity.entity_type == Entity_Type::Variable) {

						GS_CORE_ASSERT(tl_entity.syntax_node);
						GS_CORE_ASSERT(tl_entity.syntax_node->GetType() == NodeType::Variable);

						VariableNode* as_var = (VariableNode*)tl_entity.syntax_node;

						if (as_var->Type == nullptr && as_var->Assignment == nullptr) {
							Push_Error_Loc(file_entity_id, as_var, FMT("global variable '{}' is not assigned, type cannot be inferred!", as_var->Symbol.Symbol));
							return true;
						}

						if (as_var->Assignment) {

							if (!Do_Expression_Dependecy_Pass(file_entity_id, as_var->Assignment, tl_entity.dependencies, Entity_Null)) {
								return true;
							}
						}

						Array<Entity_ID> chain;

						if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_entity.dependencies, chain)) {

							std::string printed_chain;

							printed_chain = tl_entity.semantic_name.data;

							for (size_t i = 0; i < chain.count; i++)
							{
								printed_chain += " -> ";
								printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
							}

							printed_chain += " -> ";

							printed_chain += tl_entity.semantic_name.data;

							Entity& tail_entity = Data.entity_storage[chain[0]];

							Push_Error_Loc(file_entity_id, as_var, FMT("global constant '{}' has circular dependency: {}", as_var->Symbol.Symbol, printed_chain));
							Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

							return true;
						}

						if (as_var->Type) {

							if (!Do_Expression_Dependecy_Pass(file_entity_id, as_var->Type, tl_entity.dependencies, Entity_Null)) {
								return true;
							}

							Array<Entity_ID> chain;

							if (Check_Circular_Dependencies(tl_file_scope_entity_id, tl_entity.dependencies, chain)) {

								std::string printed_chain;

								printed_chain = tl_entity.semantic_name.data;

								for (size_t i = 0; i < chain.count; i++)
								{
									printed_chain += " -> ";
									printed_chain += Data.entity_storage[chain[i]].semantic_name.data;
								}

								printed_chain += " -> ";

								printed_chain += tl_entity.semantic_name.data;

								Entity& tail_entity = Data.entity_storage[chain[0]];

								Push_Error_Loc(file_entity_id, as_var->Type, FMT("the type of global constant '{}' has circular dependency: {}", as_var->Symbol.Symbol, printed_chain));
								Push_Error_Loc(Data.File_ID_To_Scope.at(tail_entity.definition_file), tail_entity.syntax_node, FMT("the tail"));

								return true;
							}
						}
					}
					else if (tl_entity.entity_type == Entity_Type::Operator_Overload)
					{
						OperatorNode* as_operator = (OperatorNode*)tl_entity.syntax_node;
						ASSERT(as_operator->statement->GetType() == NodeType::Identifier);

						Identifier* as_ident = (Identifier*)as_operator->statement;

						if (!Do_Expression_Dependecy_Pass(file_entity_id, as_ident, tl_entity.dependencies, tl_file_scope_entity_id))
							return true;

						String function_name = String_Make(as_ident->Symbol.Symbol);

						Entity_ID function_entity = Front_End_Data::Get_Entity_ID_By_Name(Data, file_entity_id, function_name);
						tl_entity.operator_overload.function_entity_id = function_entity;

						tl_entity.flags = EF_InComplete;
					}
				}
			}

			return false;

			});
	}

	bool Front_End::Do_Tl_Resolution_Passes()
	{
		bool all_evaluated = true;

		GS_PROFILE_FUNCTION();

		for (size_t entity_id = 0; entity_id < Data.entity_storage.count; entity_id++)
		{
			Entity& entity = Data.entity_storage[entity_id];

			Data.resolution_pass_iteration_count++;

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

				if (entity.entity_type == Entity_Type::Variable) {

					VariableNode* as_var = (VariableNode*)entity.syntax_node;

					if (as_var->Type) {
						entity.semantic_type = Evaluate_Type(as_var->Type, entity.parent);
						if (!entity.semantic_type)
							return true;
					}

					Il_Global global;
					global.name = String_Copy(entity.semantic_name);
					global.read_only = false;

					if (as_var->Assignment) {
						Eval_Result assignment_code_gen_result = Expression_Evaluate(as_var->Assignment, entity.parent, entity.semantic_type);
						if (!assignment_code_gen_result) {
							return true;
						}

						if (entity.semantic_type) {
							if (assignment_code_gen_result.expression_type != entity.semantic_type) {
								Push_Error_Loc(entity.parent, as_var, FMT("global variable '{}' type mismatch in assignment", as_var->Symbol.Symbol));
								return true;
							}
						}
						else {
							entity.semantic_type = assignment_code_gen_result.expression_type;
						}

						Array_Add(global.initializer_storage, Il_Make_Constant(assignment_code_gen_result.result, TypeSystem_Get_Type_Index(Data.type_system, entity.semantic_type)));
						global.initializer = 0;
					}

					global.type = entity.semantic_type;
					entity.var.location = Il_Insert_Global(Data.il_program, global);

					entity.var.global = true;

					entity.flags &= ~EF_InComplete;
				}
				else if (entity.entity_type == Entity_Type::Constant) {

					VariableNode* as_var = (VariableNode*)entity.syntax_node;

					if (as_var->Type) {
						entity.semantic_type = Evaluate_Type(as_var->Type, entity.parent);
						if (!entity.semantic_type)
							return true;
					}

					Eval_Result assignment_code_gen_result = Expression_Evaluate(as_var->Assignment, entity.parent, entity.semantic_type);
					if (!assignment_code_gen_result) {
						return true;
					}

					entity.flags &= ~EF_InComplete;

					if (entity.semantic_type) {
						if (assignment_code_gen_result.expression_type != entity.semantic_type) {
							Push_Error_Loc(entity.parent, as_var, FMT("global constant '{}' type mismatch in assignment", as_var->Symbol.Symbol));
							return true;
						}
					}
					else {
						entity.semantic_type = assignment_code_gen_result.expression_type;
					}

					entity.constant_value = assignment_code_gen_result.result;
				}
				else if (entity.entity_type == Entity_Type::Struct_Entity)
				{
					if (entity.struct_entity.poly_morphic) {

						entity.flags &= ~EF_InComplete;

						StructNode* as_struct_node = (StructNode*)entity.syntax_node;

						//Create_Struct_Parameter_Entity();

						Entity& file_scope_entity = Data.entity_storage[Front_End_Data::Get_File_Scope_Parent(Data, entity_id)];

						for (size_t i = 0; i < as_struct_node->argument_list->Arguments.size(); i++)
						{
							ArgumentNode* as_arg = (ArgumentNode*)as_struct_node->argument_list->Arguments[i];
							ASSERT(as_arg->Type);

							GS_Type* argument_type = Evaluate_Type(as_arg->Type, entity.parent);
							if (!argument_type) break;

							String parameter_name = String_Make(as_arg->Symbol.Symbol);

							if (entity.children_lookup.find({ parameter_name.data,parameter_name.count }) != entity.children_lookup.end()) {
								Push_Error_Loc(entity_id, as_arg, FMT("parameter name '{}' already taken", parameter_name.data));
								return true;
							}

							Entity param_entity = Create_Struct_Parameter_Entity(parameter_name, Loc_From_Token(as_arg->Symbol), file_scope_entity.file_scope.file_id);

							param_entity.struct_parameter.parameter_index = i;
							param_entity.semantic_type = argument_type;

							Struct_Parameter parameter;
							parameter.entity_id = Insert_Entity(param_entity, entity_id);
							parameter.type = argument_type;
							parameter.name = parameter_name;

							Array_Add(entity.struct_entity.parameters, parameter);
						}
						int x = 30;
					}
					else {

						GS_Struct struct_type;

						Type_Name struct_type_name;
						struct_type_name.name = entity.semantic_name;
						struct_type_name.flags = TN_Struct_Type;
						struct_type_name.size = (u64)-1;
						struct_type_name.alignment = 0;

						entity.struct_entity.type_name_id = TypeSystem_Insert_TypeName_Struct(Data.type_system, struct_type_name, struct_type);
						entity.constant_value.type = TypeSystem_Get_Basic_Type(Data.type_system, entity.struct_entity.type_name_id);

						entity.flags &= ~EF_InComplete;

						Data.typename_to_entity_id[entity.struct_entity.type_name_id] = entity_id;

						for (size_t i = 0; i < entity.children.count; i++)
						{
							Entity& struct_member_entity = Data.entity_storage[entity.children[i]];

							VariableNode* as_variable_node = (VariableNode*)struct_member_entity.syntax_node;

							struct_member_entity.semantic_type = Evaluate_Type(as_variable_node->Type, entity.parent);

							if (!struct_member_entity.semantic_type)
								return true;

							if (struct_member_entity.semantic_type->kind == Type_Dyn_Array) {
								Array_Add(struct_type.members, Data.Array_Ty);
							}
							else {
								Array_Add(struct_type.members, struct_member_entity.semantic_type);
							}

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

						// TODO: move this from here somehow
						if (String_Equal(String_Make("string"), entity.semantic_name)) {
							Data.string_Ty = TypeSystem_Get_Basic_Type(Data.type_system, entity.struct_entity.type_name_id);
							ASSERT(Data.string_Ty);
						}

						if (String_Equal(String_Make("Array"), entity.semantic_name)) {
							Data.Array_Ty = TypeSystem_Get_Basic_Type(Data.type_system, entity.struct_entity.type_name_id);
							ASSERT(Data.Array_Ty);
						}

						if (String_Equal(String_Make("Any"), entity.semantic_name)) {
							Data.Any_Ty = TypeSystem_Get_Basic_Type(Data.type_system, entity.struct_entity.type_name_id);
							ASSERT(Data.Any_Ty);
						}

						if (String_Equal(String_Make("TypeInfo"), entity.semantic_name)) {
							Data.TypeInfo_tn = entity.struct_entity.type_name_id;
							Data.TypeInfo_Ty = TypeSystem_Get_Basic_Type(Data.type_system, Data.TypeInfo_tn);
							Data.TypeInfo_Ptr_Ty = TypeSystem_Get_Pointer_Type(Data.type_system, Data.TypeInfo_Ty, 1);
							ASSERT(Data.Any_Ty);
						}
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

					Data.typename_to_entity_id[entity.enum_entity.type_name_id] = entity_id;

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
				else if (entity.entity_type == Entity_Type::Function) {

					FunctionNode* as_func_node = (FunctionNode*)entity.syntax_node;

					if (entity.func.poly_morphic)
					{
						for (size_t i = 0; i < as_func_node->argumentList->Arguments.size(); i++)
						{
							ArgumentNode* as_arg = (ArgumentNode*)as_func_node->argumentList->Arguments[i];
							ASSERT(as_arg->Type);

							entity.func.parameters[i].poly_morhpic = as_arg->PolyMorphic;
							entity.func.parameters[i].syntax_node = as_arg;
						}

						entity.flags = EF_Constant;

						continue;
					}

					GS_Type* return_type = nullptr;

					if (as_func_node->ReturnType) {
						return_type = Evaluate_Type(as_func_node->ReturnType, entity.parent);
					}
					else {
						return_type = Data.void_Ty;
					}

					ASSERT(return_type);

					Array<GS_Type*> param_types;

					for (size_t i = 0; i < as_func_node->argumentList->Arguments.size(); i++)
					{
						ArgumentNode* as_arg = (ArgumentNode*)as_func_node->argumentList->Arguments[i];
						ASSERT(as_arg->Type);

						GS_Type* argument_type = Evaluate_Type(as_arg->Type, entity.parent);
						if (!argument_type) break;

						entity.func.parameters[i].param_type = argument_type;
						Array_Add(param_types, argument_type);
					}

					GS_Type* signature = TypeSystem_Get_Proc_Type(Data.type_system, return_type, param_types);
					ASSERT(signature);
					entity.func.signature = signature;

					if (entity.func.foreign) {
						Entity& library_entity = Data.entity_storage[entity.func.library_entity_id];
						Il_IDX inserted_external_proc_idx = Il_Insert_External_Proc(Data.il_program, String_Copy(entity.semantic_name), entity.func.signature, library_entity.library.library_node_idx, entity.func.c_variadic);
						entity.func.proc_idx = inserted_external_proc_idx;
					}
					else {

						Array<GS_Type*> proc_param_types;

						auto return_type_size = TypeSystem_Get_Type_Size(Data.type_system, return_type);
						GS_Type* proc_return_type = return_type;

						if (return_type_size > 8) {
							Array_Add(proc_param_types, TypeSystem_Get_Pointer_Type(Data.type_system, return_type, 1));
							proc_return_type = Data.void_Ty;
						}

						for (size_t i = 0; i < param_types.count; i++)
						{
							GS_Type* param_type = param_types[i];

							auto param_type_size = TypeSystem_Get_Type_Size(Data.type_system, param_type);

							if (param_type_size > 8) {
								param_type = TypeSystem_Get_Pointer_Type(Data.type_system, param_type, 1);
							}

							Array_Add(proc_param_types, param_type);
						}

						GS_Type* proc_signature = TypeSystem_Get_Proc_Type(Data.type_system, proc_return_type, proc_param_types);

						Il_IDX inserted_proc_idx = Il_Insert_Proc(Data.il_program, String_Copy(entity.semantic_name), proc_signature);
						entity.func.proc_idx = inserted_proc_idx;
					}

					entity.flags &= ~EF_InComplete;
				}
				else if (entity.entity_type == Entity_Type::Operator_Overload)
				{
					OperatorNode* as_operator = (OperatorNode*)entity.syntax_node;
					Identifier* as_ident = (Identifier*)as_operator->statement;

					String function_name = String_Make(as_ident->Symbol.Symbol);

					Entity& function_entity = Data.entity_storage[entity.operator_overload.function_entity_id];

					entity.operator_overload.query_signature = TypeSystem_Get_Proc_Type(Data.type_system, Data.void_Ty, *(Array<GS_Type*>*) & function_entity.func.signature->proc.params);

					GS_Type* query_signature = entity.operator_overload.query_signature;

					String operator_as_string = Operator_To_String(entity.operator_overload._operator);
					String operator_name = String_Make(FMT("{}{}", operator_as_string.data, query_signature->type_hash));

					Entity_ID current_scope = Front_End_Data::Get_File_Scope_Parent(Data, entity_id);

					Entity_ID previous_definition_id = Front_End_Data::Get_Entity_ID_By_Name(Data, current_scope, operator_name);

					while (previous_definition_id != Entity_Null) {

						Entity& previous_definition = Data.entity_storage[previous_definition_id];
						ASSERT(previous_definition.entity_type == Entity_Type::Operator_Overload);

						if (previous_definition.operator_overload.query_signature == query_signature) {
							Push_Error_Loc(current_scope, as_operator->statement, FMT("operator '{}' is already defined with the same signature!", operator_as_string.data));
							return true;
						}

						previous_definition_id = Front_End_Data::Get_Entity_ID_By_Name(Data, current_scope, operator_name);
					}

					entity.semantic_name = operator_name;
					Data.entity_storage[current_scope].children_lookup[operator_name.data] = entity_id;

					entity.flags &= ~EF_InComplete;
				}
			}

			if (entity.flags & EF_InComplete) {
				all_evaluated = false;
			}

			if (entity_id + 1 == Data.entity_storage.count) {
				if (all_evaluated) {

				}
				else {
					all_evaluated = true;
					entity_id = 0;
				}
			}
		}

		return false;
	}

	bool Front_End::Do_CodeGen()
	{
		GS_PROFILE_FUNCTION();

		for (size_t i = 0; i < Data.entity_storage.count; i++)
		{
			Entity& entity = Data.entity_storage[i];

			if (entity.entity_type == Entity_Type::Function) {
				if (!entity.func.foreign && !entity.func.poly_morphic) {
					if (!Function_CodeGen(entity, (Entity_ID)i, entity.parent)) {
						return true;
					}
				}
				else if (entity.func.foreign) {
					Foreign_Function_CodeGen(entity, (Entity_ID)i, entity.parent);
				}
			}
		}

		if (Options.Backend == Backend_Option::LLVM_Backend) {
			if (Generate_TypeInfoTable()) {
				return true;
			}
		}

		return false;
	}

	bool Front_End::Generate_TypeInfoTable()
	{
		GS_PROFILE_FUNCTION();

		GS_Type* c_str_type = TypeSystem_Get_Pointer_Type(Data.type_system, TypeSystem_Get_Basic_Type(Data.type_system, Data.u8_tn), 1);
		ASSERT(c_str_type);

		Array<GS_Type> type_storage = Data.type_system.type_storage;

		u64 entry_count = Data.type_system.type_storage.count;

		auto table_array_ty = TypeSystem_Get_Array_Type(Data.type_system, te_Ty, entry_count);

		enum TypeInfo_Entry_Kind
		{
			TEK_BASIC = 1,
			TEK_NUMERIC = 2,
			TEK_STRUCT = 3,
			TEK_STRUCT_MEMBER = 4,
		};

		enum TypeInfo_Flags {
			TEF_BASE_TYPE = BIT(0),
			TEF_SIGNED = BIT(1),
			TEF_FLOAT = BIT(2),
		};

		auto Type_Idx = [&](GS_Type* t) {
			return TypeSystem_Get_Type_Index(Data.type_system, t);
		};

		auto Type_Size = [&](GS_Type* t) {
			return TypeSystem_Get_Type_Size(Data.type_system, t);
		};

		auto Type_Align = [&](GS_Type* t) {
			return TypeSystem_Get_Type_Alignment(Data.type_system, t);
		};

		auto Type_Flags = [&](GS_Type* t) {
			return TypeSystem_Get_Type_Flags(Data.type_system, t);
		};

		u64 members_array_size = 0;

		Array<Il_Node> table_initializer;
		Array<Il_IDX> table_array_values;

		Array<Il_Node> members_array_initializer;
		Array<Il_IDX> members_array_values;

		for (u64 i = 0; i < entry_count; i++)
		{
			Array<Il_IDX> te_member_values;

			GS_Type* entry_type = &type_storage[i];

			u64 type_info_kind = -1;
			String type_info_name = TypeSystem_Print_Type(Data.type_system, entry_type);
			u64	type_info_flags = 0;
			u64	type_info_size = Type_Size(entry_type);
			u64	type_info_alignment = Type_Align(entry_type);

			auto type_flags = Type_Flags(entry_type);

			if (type_flags & TN_Base_Type) {
				type_info_flags &= TEF_BASE_TYPE;
			}

			if (type_flags & TN_Float_Type) {
				type_info_flags &= TEF_FLOAT;
			}

			if (type_flags & ~TN_Unsigned_Type) {
				type_info_flags &= TEF_SIGNED;
			}

			if (entry_type->kind == Type_Basic) {
				if (type_flags & TN_Struct_Type) {
					type_info_kind = TEK_STRUCT;
				}
				else if (type_flags & TN_Numeric_Type) {
					type_info_kind = TEK_NUMERIC;
				}
				else {
					type_info_kind = TEK_BASIC;
				}
			}

			//Kind
			Array_Add(te_member_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, Il_Make_Constant((void*)type_info_kind, Type_Idx(Data.i64_Ty)));

			//STRING name
			Array_Add(te_member_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, Il_Make_Constant((void*)type_info_name.count, Type_Idx(Data.i64_Ty)));

			Array_Add(table_initializer, Il_Make_String(type_info_name, Type_Idx(c_str_type)));
			Array_Add(te_member_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, Il_Make_Cast(Il_Cast_Ptr2Int, Type_Idx(Data.u64_Ty), Type_Idx(c_str_type), (Il_IDX)table_initializer.count - 1));

			//u64	FLAGS
			Array_Add(te_member_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, Il_Make_Constant((void*)type_info_flags, Type_Idx(Data.i64_Ty)));

			if (type_info_kind == TEK_BASIC || type_info_kind == TEK_STRUCT || type_info_kind == TEK_NUMERIC)
			{
				u32 size_alignemnt[2] = { 0 };

				size_alignemnt[0] = type_info_size;
				size_alignemnt[1] = type_info_alignment;
				//u64	SIZE/Alignment
				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, Il_Make_Constant((void*)*(u64*)&size_alignemnt, Type_Idx(Data.i64_Ty)));
			}

			if (type_info_kind == TEK_STRUCT) {

				auto& type_name = Data.type_system.type_name_storage[entry_type->basic.type_name_id];
				GS_Struct& struct_type = Data.type_system.struct_storage[type_name.struct_id];

				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, Il_Make_Constant((void*)struct_type.members.count, Type_Idx(Data.i64_Ty)));

				Array_Add(table_initializer, Il_Make_Global_Address(struct_member_typeinfo_global_idx, Type_Idx(te_Ty)));
				Array_Add(table_initializer, Il_Make_Constant((void*)members_array_size, Type_Idx(Data.i64_Ty)));
				Array_Add(table_initializer, Il_Make_AEP(Type_Idx(te_Ty), (Il_IDX)table_initializer.count - 2, (Il_IDX)table_initializer.count - 1));
				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, Il_Make_Cast(Il_Cast_Ptr2Int, Type_Idx(Data.u64_Ty), Type_Idx(c_str_type), (Il_IDX)table_initializer.count - 1));

				ASSERT(entry_type->kind == Type_Basic);
				Entity_ID struct_entity_id = Data.typename_to_entity_id.at(entry_type->basic.type_name_id);

				if (struct_entity_id != Entity_Null) {

					Entity& struct_entity = Data.entity_storage[struct_entity_id];

					for (size_t j = 0; j < struct_entity.children.count; j++)
					{
						Entity_ID member_entity_id = struct_entity.children[j];
						Entity& member_entity = Data.entity_storage[member_entity_id];

						if (member_entity.entity_type == Entity_Type::Struct_Member_Entity) {

							Array<Il_IDX> member_te_member_values;

							//Kind
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Constant((void*)TEK_STRUCT_MEMBER, Type_Idx(Data.i64_Ty)));

							//STRING name
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Constant((void*)member_entity.semantic_name.count, Type_Idx(Data.i64_Ty)));

							Array_Add(members_array_initializer, Il_Make_String(member_entity.semantic_name, Type_Idx(c_str_type)));
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Cast(Il_Cast_Ptr2Int, Type_Idx(Data.u64_Ty), Type_Idx(c_str_type), (Il_IDX)members_array_initializer.count - 1));

							//u64	Type
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Constant((void*)Type_Idx(member_entity.semantic_type), Type_Idx(Data.i64_Ty)));

							//int	Offset
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Constant((void*)struct_type.offsets[j], Type_Idx(Data.i64_Ty)));

							Array_Add(members_array_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Struct_Init(Type_Idx(te_Ty), member_te_member_values));
							members_array_size++;
						}
					}
				}
			}

			for (size_t i = 0; i < (TE_MEM_COUNT - te_member_values.count) + 1; i++)
			{
				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, Il_Make_Constant((void*)0, Type_Idx(Data.i64_Ty)));
			}

			Array_Add(table_array_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, Il_Make_Struct_Init(Type_Idx(te_Ty), te_member_values));
		}

		Array_Add(table_initializer, Il_Make_Array_Init(Type_Idx(table_array_ty), table_array_values));

		Il_Global& table_global = Data.il_program.globals[global_type_info_table_idx];
		table_global.initializer_storage = table_initializer;
		table_global.initializer = table_initializer.count - 1;
		table_global.type = table_array_ty;

		auto members_array_ty = TypeSystem_Get_Array_Type(Data.type_system, te_Ty, members_array_size);
		Array_Add(members_array_initializer, Il_Make_Array_Init(Type_Idx(members_array_ty), members_array_values));

		Il_Global& members_array_global = Data.il_program.globals[struct_member_typeinfo_global_idx];
		members_array_global.type = members_array_ty;
		members_array_global.initializer_storage = members_array_initializer;
		members_array_global.initializer = members_array_initializer.count - 1;

		return false;
	}

	bool Front_End::Foreign_Function_CodeGen(Entity& function_entity, Entity_ID func_entity_id, Entity_ID scope_id)
	{
		ASSERT(function_entity.func.foreign);

		return false;
	}

	CodeGen_Result Front_End::Function_CodeGen(Entity& function_entity, Entity_ID func_entity_id, Entity_ID scope_id)
	{
		GS_PROFILE_FUNCTION();

		FunctionNode* as_func_node = (FunctionNode*)function_entity.syntax_node;

		function_returns = false;

		File_ID current_file = Data.entity_storage[Front_End_Data::Get_File_Scope_Parent(Data, scope_id)].file_scope.file_id;

		Il_Proc& proc = Data.il_program.procedures[function_entity.func.proc_idx];

		return_branches.count = 0;

		if (as_func_node->Symbol.Symbol == "main") {
			main_proc_idx = function_entity.func.proc_idx;
		}

		auto function_return_type = function_entity.func.signature->proc.return_type;

		u64 parameters_offset = 0;
		auto return_type_size = TypeSystem_Get_Type_Size(Data.type_system, function_return_type);

		if (return_type_size > 8) {
			parameters_offset = 1;

			GS_Type* return_type_pointer = TypeSystem_Get_Pointer_Type(Data.type_system, function_return_type, 1);
			Il_IDX return_ptr_allocation = Il_Insert_Alloca(proc, return_type_pointer);
			Il_Insert_Store(proc, return_type_pointer, return_ptr_allocation, proc.parameters[0]);
			function_entity.func.return_data_ptr_node_idx = return_ptr_allocation;
		}
		else if (return_type_size != 0) {
			function_entity.func.return_data_ptr_node_idx = Il_Insert_Alloca(proc, function_return_type);
		}

		for (size_t i = 0; i < function_entity.func.parameters.count; i++)
		{
			Function_Parameter parameter = function_entity.func.parameters[i];
			ArgumentNode* parameter_node = (ArgumentNode*)as_func_node->argumentList->Arguments[i];

			Entity parameter_variable = Create_Variable_Entity(parameter.name, Loc_From_Token(parameter_node->Symbol), current_file);
			parameter_variable.semantic_type = parameter.param_type;

			auto param_type_size = TypeSystem_Get_Type_Size(Data.type_system, parameter.param_type);

			if (param_type_size > 8)
			{
				GS_Type* param_type_pointer = TypeSystem_Get_Pointer_Type(Data.type_system, parameter.param_type, 1);
				parameter_variable.var.location = Il_Insert_Alloca(proc, param_type_pointer);
				Il_Insert_Store(proc, param_type_pointer, parameter_variable.var.location, proc.parameters[i + parameters_offset]);
				parameter_variable.var.immutable = true;
				parameter_variable.var.parameter = true;
				parameter_variable.var.code_type = param_type_pointer;
			}
			else {
				parameter_variable.var.location = Il_Insert_Alloca(proc, parameter.param_type);
				Il_Insert_Store(proc, parameter.param_type, parameter_variable.var.location, proc.parameters[i + parameters_offset]);
				parameter_variable.var.code_type = parameter.param_type;
			}

			Insert_Entity(parameter_variable, func_entity_id);
		}

		for (size_t i = 0; i < as_func_node->scope->Statements.size(); i++)
		{
			Statement* stmt = as_func_node->scope->Statements[i];

			if (!Statement_CodeGen(stmt, func_entity_id, proc)) {
				return {};
			}
		}

		if (!function_returns) {

			if (Data.void_Ty != function_entity.func.signature->proc.return_type) {
				Push_Error_Loc(scope_id, as_func_node, FMT("function '{}' does not return", as_func_node->Symbol.Symbol));
				return {};
			}
		}

		Il_IDX return_block_idx = Il_Insert_Block(proc, String_Make("final"));

		for (size_t i = 0; i < return_branches.count; i++)
		{
			Il_Node& branch_node = proc.instruction_storage[return_branches[i]];
			ASSERT(branch_node.node_type == Il_Branch);
			branch_node.br.block_idx = return_block_idx;
		}

		if (return_type_size == 0) {
			Il_Insert_Ret(proc, Data.void_Ty, -1);
		}
		else if (return_type_size <= 8) {
			Il_Insert_Ret(proc, function_return_type, Il_Insert_Load(proc, function_return_type, function_entity.func.return_data_ptr_node_idx));
		}
		else {
			Il_Insert_Ret(proc, Data.void_Ty, -1);
		}

		std::string printed_proc = Il_Print_Proc(proc);

		if (print_il)
			printed_il_stream << "\n" << printed_proc << "\n";

		//GS_CORE_TRACE("generated: {}", printed_proc);

		CodeGen_Result result;
		result.ok = true;
		return result;
	}

	CodeGen_Result Front_End::Statement_CodeGen(Statement* statement, Entity_ID scope_id, Il_Proc& proc)
	{
		NodeType node_type = statement->GetType();

		switch (node_type)
		{
		case NodeType::Identifier:
		case NodeType::NumericLiteral:
		case NodeType::BinaryExpression:
		case NodeType::Call:
		case NodeType::Cast:
		case NodeType::MemberAccess:
		case NodeType::Reference:
		case NodeType::DeReference:
		case NodeType::ArrayAccess:
			return Expression_CodeGen((Expression*)statement, scope_id, proc);
			break;
		case NodeType::Return:
		{
			terminator_encountered = true;
			function_returns = true;

			ReturnNode* as_return = (ReturnNode*)statement;

			Entity& func_entity = Data.entity_storage[Front_End_Data::Get_Func_Parent(Data, scope_id)];

			GS_Type* func_return_type = func_entity.func.signature->proc.return_type;
			ASSERT(func_return_type);

			auto func_return_type_size = TypeSystem_Get_Type_Size(Data.type_system, func_return_type);

			GS_Type* inferred_type = nullptr;

			if (func_return_type != Data.void_Ty) {
				inferred_type = func_return_type;
			}

			if (!inferred_type && as_return->Expr) {
				Push_Error_Loc(scope_id, as_return, "return value not expected");
				return {};
			}

			if (inferred_type && !as_return->Expr) {
				Push_Error_Loc(scope_id, as_return, "return value expected");
				return {};
			}

			Il_IDX value_node_idx = -1;

			if (as_return->Expr) {

				CodeGen_Result value_result = Expression_CodeGen(as_return->Expr, scope_id, proc, inferred_type);

				if (!value_result)
					return {};

				if (value_result.expression_type != func_return_type) {
					Push_Error_Loc(scope_id, as_return->Expr, "return value type mismatch");
					return {};
				}

				value_node_idx = value_result.code_node_id;
			}

			if (func_return_type_size > 8)
			{
				GS_Type* func_return_type_ptr = TypeSystem_Get_Pointer_Type(Data.type_system, func_return_type, 1);
				Il_IDX return_data_ptr_node_idx = Il_Insert_Load(proc, func_return_type_ptr, func_entity.func.return_data_ptr_node_idx);
				Il_Insert_Store(proc, func_return_type, return_data_ptr_node_idx, value_node_idx);
				Array_Add(return_branches, Il_Insert_Br(proc, 0));
			}
			else if (func_return_type_size == 0) {
				Array_Add(return_branches, Il_Insert_Br(proc, -1));
			}
			else if (func_return_type_size <= 8) {
				Il_Insert_Store(proc, func_return_type, func_entity.func.return_data_ptr_node_idx, value_node_idx);
				Array_Add(return_branches, Il_Insert_Br(proc, -1));
			}

			CodeGen_Result result = {};
			result.ok = true;
			return result;
		}
		break;
		case NodeType::Variable:
		{
			VariableNode* as_variable = (VariableNode*)statement;
			ASSERT(!as_variable->Constant);

			String variable_name = String_Make(as_variable->Symbol.Symbol);

			Entity_ID previous_declarations = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, variable_name);

			if (previous_declarations != Entity_Null) {
				Push_Error_Loc(scope_id, as_variable, FMT("variable name '{}' already taken", as_variable->Symbol.Symbol));
				return {};
			}

			GS_Type* variable_type = nullptr;

			if (as_variable->Type) {
				variable_type = Evaluate_Type(as_variable->Type, scope_id);
				if (!variable_type) return {};
			}

			Entity variable = Create_Variable_Entity(variable_name, Loc_From_Token(as_variable->Symbol), Data.entity_storage[Front_End_Data::Get_File_Scope_Parent(Data, scope_id)].file_scope.file_id);

			if (as_variable->Assignment) {

				CodeGen_Result assignment_result = Expression_CodeGen(as_variable->Assignment, scope_id, proc, variable_type);

				if (!assignment_result) return {};

				if (as_variable->Type) {
					if (assignment_result.expression_type != variable_type) {
						Push_Error_Loc(scope_id, as_variable->Assignment, FMT("variable '{}' type mismatch in assignment", as_variable->Symbol.Symbol));
						return {};
					}
				}
				else {
					variable_type = assignment_result.expression_type;
				}

				variable.var.location = Il_Insert_Alloca(proc, variable_type);

				variable.semantic_type = variable_type;

				Il_Insert_Store(proc, variable_type, variable.var.location, assignment_result.code_node_id);
			}
			else {
				variable.var.location = Il_Insert_Alloca(proc, variable_type);
				variable.semantic_type = variable_type;

				auto type_size = TypeSystem_Get_Type_Size(Data.type_system, variable_type);

				if (type_size <= 8) {
					Const_Union zero = { 0 };
					Il_Insert_Store(proc, variable_type, variable.var.location, Il_Insert_Constant(proc, zero, variable_type));
				}
				else {
					Il_Insert_Store(proc, variable_type, variable.var.location, Il_Insert_Zero_Init(proc, variable_type));
				}
			}

			Insert_Entity(variable, scope_id);

			CodeGen_Result result;
			result.ok = true;
			return result;
		}
		case NodeType::Scope: {

			ScopeNode* as_scope = (ScopeNode*)statement;

			for (u64 i = 0; i < as_scope->Statements.size(); i++)
			{
				if (!Statement_CodeGen(as_scope->Statements[i], scope_id, proc)) {
					return {};
				}
			}

			CodeGen_Result result;
			result.ok = true;
			return result;
		}
							break;
		case NodeType::If: {

			IfNode* as_if = (IfNode*)statement;

			CodeGen_Result condition_result = Expression_CodeGen(as_if->Condition, scope_id, proc, nullptr, false, true);
			if (!condition_result) return {};

			if (condition_result.expression_type != Data.bool_Ty) {

				auto expression_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, condition_result.expression_type);

				if (expression_type_flags & TN_Numeric_Type || expression_type_flags & TN_Pointer_Type || expression_type_flags & TN_Enum_Type) {
					Const_Union zero = {};
					condition_result.code_node_id = Il_Insert_Compare(proc, Il_Value_Cmp, Il_Cmp_NotEqual, condition_result.expression_type, condition_result.code_node_id, Il_Insert_Constant(proc, zero, condition_result.expression_type));
				}
				else {
					Push_Error_Loc(scope_id, as_if->Condition, FMT("expected expression to be of type bool or convertible to bool"));
					return {};
				}
			}

			Il_IDX saved_insert_point = proc.insertion_point;

			Il_IDX else_block_idx = -1;

			Il_IDX body_block_idx = Il_Insert_Block(proc, String_Make("then"));
			CodeGen_Result body_result = Statement_CodeGen(as_if->Scope, scope_id, proc);
			if (!body_result) return {};

			Il_IDX after_body_insert_point = proc.insertion_point;

			if (as_if->Else) {
				else_block_idx = Il_Insert_Block(proc, String_Make("else"));
				if (!Statement_CodeGen(as_if->Else->statement, scope_id, proc)) return {};
			}

			Il_IDX cont_block_idx = Il_Insert_Block(proc, String_Make("after_if"));

			Il_Set_Insert_Point(proc, saved_insert_point);

			if (as_if->Else) {
				Il_Insert_CBR(proc, Data.bool_Ty, condition_result.code_node_id, body_block_idx, else_block_idx);
			}
			else {
				Il_Insert_CBR(proc, Data.bool_Ty, condition_result.code_node_id, body_block_idx, cont_block_idx);
			}

			Il_Set_Insert_Point(proc, after_body_insert_point);
			Il_Insert_Br(proc, cont_block_idx);

			Il_Set_Insert_Point(proc, cont_block_idx);

			CodeGen_Result result;
			result.ok = true;
			return result;
		}
						 break;
		case NodeType::While:
		{
			WhileNode* as_while = (WhileNode*)statement;

			Il_IDX saved_insert_point = proc.insertion_point;

			Il_IDX cond_block_idx = Il_Insert_Block(proc, String_Make("cond"));

			CodeGen_Result condition_result = Expression_CodeGen(as_while->Condition, scope_id, proc, nullptr, false, true);
			if (!condition_result) return {};

			if (condition_result.expression_type != Data.bool_Ty) {

				auto expression_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, condition_result.expression_type);

				if (expression_type_flags & TN_Numeric_Type || expression_type_flags & TN_Pointer_Type) {
					Const_Union zero = {};
					condition_result.code_node_id = Il_Insert_Compare(proc, Il_Value_Cmp, Il_Cmp_NotEqual, condition_result.expression_type, condition_result.code_node_id, Il_Insert_Constant(proc, zero, condition_result.expression_type));
				}
				else {
					Push_Error_Loc(scope_id, as_while->Condition, FMT("expected expression to be of type bool or convertible to bool"));
					return {};
				}
			}

			Il_IDX body_block_idx = Il_Insert_Block(proc, String_Make("body"));
			CodeGen_Result body_result = Statement_CodeGen(as_while->Scope, scope_id, proc);
			if (!body_result) return {};

			if (terminator_encountered)
			{
				terminator_encountered = false;
			}
			else {

			}

			Il_Insert_Br(proc, cond_block_idx);

			Il_IDX cont_block_idx = Il_Insert_Block(proc, String_Make("cont"));

			Il_Set_Insert_Point(proc, cond_block_idx);
			Il_Insert_CBR(proc, Data.bool_Ty, condition_result.code_node_id, body_block_idx, cont_block_idx);

			Il_Set_Insert_Point(proc, cont_block_idx);

			CodeGen_Result result;
			result.ok = true;
			return result;
		}
		break;
		case NodeType::For:
		{
			ForNode* as_for = (ForNode*)statement;

			Il_IDX saved_insert_point = proc.insertion_point;

			Il_IDX cond_block_idx = Il_Insert_Block(proc, String_Make("condition"));
			Il_IDX body_block_idx = Il_Insert_Block(proc, String_Make("body"));
			Il_IDX loop_bottom_block_idx = Il_Insert_Block(proc, String_Make("dummmy"));

			Il_Set_Insert_Point(proc, saved_insert_point);

			Iterator_Result condition_result = Iterator_CodeGen(as_for->Condition, scope_id, proc, saved_insert_point, cond_block_idx, loop_bottom_block_idx);
			if (!condition_result) return {};

			Il_Set_Insert_Point(proc, body_block_idx);

			Entity& file_scope_entity = Data.entity_storage[Front_End_Data::Get_File_Scope_Parent(Data, scope_id)];

			Entity scope_entity = Create_Function_Scope_Entity(String_Make("@scope"), Loc_From_Token(as_for->Scope->GetLocation()), file_scope_entity.file_scope.file_id);
			scope_entity.syntax_node = as_for->Scope;
			Entity_ID inserted_scope_entity_id = Insert_Entity(scope_entity, scope_id);

			Entity it_variable = Create_Variable_Entity(String_Make("it"), Loc_From_Token(as_for->Condition->GetLocation()), file_scope_entity.file_scope.file_id);
			Entity it_index_variable = Create_Variable_Entity(String_Make("it_index"), Loc_From_Token(as_for->Condition->GetLocation()), file_scope_entity.file_scope.file_id);

			it_variable.var.location = condition_result.it_location_node;
			it_variable.semantic_type = condition_result.it_type;

			it_index_variable.var.location = condition_result.it_index_location_node;
			it_index_variable.semantic_type = condition_result.it_index_type;

			Insert_Entity(it_variable, inserted_scope_entity_id);
			Insert_Entity(it_index_variable, inserted_scope_entity_id);

			CodeGen_Result body_result = Statement_CodeGen(as_for->Scope, inserted_scope_entity_id, proc);
			if (!body_result) return {};

			Il_Block& dummy = proc.blocks[loop_bottom_block_idx];
			loop_bottom_block_idx = Il_Insert_Block(proc, String_Make("loop_bottom"));
			Il_Block& after_loop = proc.blocks[loop_bottom_block_idx];

			after_loop.instructions = dummy.instructions;
			dummy.instructions.count = 0;

			Il_Insert_Br(proc, cond_block_idx);

			Il_IDX cont_block_idx = Il_Insert_Block(proc, String_Make("after_loop"));

			Il_Set_Insert_Point(proc, cond_block_idx);
			Il_Insert_CBR(proc, Data.bool_Ty, condition_result.condition_node, body_block_idx, cont_block_idx);

			Il_Set_Insert_Point(proc, cont_block_idx);

			CodeGen_Result result;
			result.ok = true;
			return result;
		}
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return {};
	}

	CodeGen_Result Front_End::Expression_CodeGen(Expression* expression, Entity_ID scope_id, Il_Proc& proc, GS_Type* inferred_type /*= nullptr*/, bool by_reference /*= false*/, bool  is_condition/*= false*/)
	{
		NodeType node_type = expression->GetType();

		switch (node_type)
		{
		case NodeType::Identifier:
		{
			Identifier* as_ident = (Identifier*)expression;

			if (as_ident->Symbol.Symbol == "null") {

				GS_Type* null_type = inferred_type;

				if (!null_type) {
					null_type = TypeSystem_Get_Pointer_Type(Data.type_system, Data.void_Ty, 1);
				}

				CodeGen_Result result;

				Const_Union zero = { 0 };

				result.code_node_id = Il_Insert_Constant(proc, zero, null_type);
				result.ok = true;
				result.expression_type = null_type;

				return result;
			}

			if (as_ident->Symbol.Symbol == "true" || as_ident->Symbol.Symbol == "false") {

				CodeGen_Result result;

				Const_Union constant = { 0 };

				if (as_ident->Symbol.Symbol == "true")
					constant.s1 = 1;
				else
					constant.s1 = 0;

				result.code_node_id = Il_Insert_Constant(proc, constant, Data.bool_Ty);
				result.ok = true;
				result.expression_type = Data.bool_Ty;

				return result;
			}

			String name = String_Make(as_ident->Symbol.Symbol);
			Entity_ID identified_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, name);

			if (identified_entity_id == Entity_Null) {
				Push_Error_Loc(scope_id, as_ident, FMT("undefined name!: '{}'", as_ident->Symbol.Symbol));
				return {};
			}

			Entity& identified_entity = Data.entity_storage[identified_entity_id];

			ASSERT(!(identified_entity.flags & EF_InComplete));

			CodeGen_Result result;

			result.entity_reference = identified_entity_id;

			if (identified_entity.entity_type == Entity_Type::Constant || identified_entity.entity_type == Entity_Type::Type_Name_Entity) {
				result.constant = true;

				if (identified_entity.semantic_type == Data.Type_Ty) {
					Const_Union value;
					value.us8 = TypeSystem_Get_Type_Index(Data.type_system, identified_entity.constant_value.type);
					result.code_node_id = Il_Insert_Constant(proc, value, identified_entity.semantic_type);
				}
				else {
					result.code_node_id = Il_Insert_Constant(proc, identified_entity.constant_value, identified_entity.semantic_type);
				}

				result.expression_type = identified_entity.semantic_type;

			}
			else if (identified_entity.entity_type == Entity_Type::Struct_Entity) {

				if (identified_entity.entity_type == Entity_Type::Struct_Entity) {
					if (identified_entity.struct_entity.poly_morphic) {
						Push_Error_Loc(scope_id, as_ident, FMT("polymorphic struct '{}' parameter list expected", as_ident->Symbol.Symbol));
						return {};
					}
				}

				Const_Union value;
				value.us8 = TypeSystem_Get_Type_Index(Data.type_system, identified_entity.constant_value.type);
				result.code_node_id = Il_Insert_Constant(proc, value, identified_entity.semantic_type);
				result.expression_type = identified_entity.semantic_type;
			}
			else if (identified_entity.entity_type == Entity_Type::Variable) {

				Il_IDX code_location = identified_entity.var.location;

				result.immutable = identified_entity.var.immutable;

				if (identified_entity.var.parameter) {
					code_location = Il_Insert_Load(proc, identified_entity.var.code_type, code_location);
				}

				if (identified_entity.var.global) {
					code_location = Il_Insert_Global_Address(proc, identified_entity.var.location);
				}

				if (by_reference) {
					result.code_node_id = code_location;
					result.expression_type = identified_entity.semantic_type;
				}
				else {

					result.expression_type = identified_entity.semantic_type;

					if (inferred_type && identified_entity.semantic_type->kind == Type_Array) {
						if (TypeSystem_Reduce_Indirection(Data.type_system, inferred_type) == identified_entity.semantic_type->array.element_type) {
							result.code_node_id = code_location;
							result.expression_type = inferred_type;
						}
						else {
							result.code_node_id = Il_Insert_Load(proc, identified_entity.semantic_type, code_location);
							result.expression_type = identified_entity.semantic_type;
						}
					}
					else {
						result.code_node_id = Il_Insert_Load(proc, identified_entity.semantic_type, code_location);
						result.expression_type = identified_entity.semantic_type;
					}
				}
			}
			else if (identified_entity.entity_type == Entity_Type::Function) {
				result.expression_type = identified_entity.func.signature;
				result.code_node_id = Il_Insert_Proc_Address(proc, identified_entity.func.proc_idx);
			}
			else {
				ASSERT(nullptr);
			}

			result.lvalue = true;

			result.ok = true;

			return result;
		}
		break;
		case NodeType::StringLiteral:
		{
			StringLiteral* as_lit = (StringLiteral*)expression;

			GS_Type* c_str_type = TypeSystem_Get_Pointer_Type(Data.type_system, TypeSystem_Get_Basic_Type(Data.type_system, Data.u8_tn), 1);
			ASSERT(c_str_type);

			std::string processed_literal;

			for (size_t i = 0; i < as_lit->Symbol.Symbol.size(); i++)
			{
				auto c = as_lit->Symbol.Symbol[i];

				if (c == '\\') {
					if (i + 1 < as_lit->Symbol.Symbol.size()) {

						auto next_c = as_lit->Symbol.Symbol[i + 1];

						if (next_c == 'n') {
							processed_literal.push_back('\n');
						}
						else if (next_c == 'r') {
							processed_literal.push_back('\r');
						}
						else if (next_c == 't') {
							processed_literal.push_back('\t');
						}
						else {
							ASSERT(nullptr, "unknown escape code");
						}
						i++;
						continue;
					}
				}

				processed_literal.push_back(c);
			}

			String literal_value = String_Make(processed_literal);

			CodeGen_Result result;

			Il_IDX string_data_node_id = Il_Insert_String(proc, c_str_type, literal_value);

			if (inferred_type == c_str_type) {
				result.code_node_id = string_data_node_id;
				result.expression_type = c_str_type;
			}
			else {

				Il_IDX struct_members[2];

				Const_Union count_const;
				count_const.us8 = literal_value.count;

				struct_members[0] = Il_Insert_Constant(proc, count_const, TypeSystem_Get_Basic_Type(Data.type_system, Data.u64_tn));
				struct_members[1] = string_data_node_id;

				result.code_node_id = Il_Insert_Struct_Init(proc, Data.string_Ty, Array_View((Il_IDX*)struct_members, 2));
				result.expression_type = Data.string_Ty;

				if (by_reference) {
					Il_IDX lvalue_alloca = Il_Insert_Alloca(proc, Data.string_Ty);
					Il_Insert_Store(proc, Data.string_Ty, lvalue_alloca, result.code_node_id);
					result.code_node_id = lvalue_alloca;

					result.lvalue = true;
				}
			}

			result.ok = true;
			return result;
		}
		break;
		case NodeType::NumericLiteral:
		{
			NumericLiteral* as_lit = (NumericLiteral*)expression;

			GS_Type* literal_type = nullptr;

			Const_Union literal_value;

			Type_Name_Flags inferred_type_flags = 0;

			if (inferred_type) {
				inferred_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, inferred_type);
			}

			if (inferred_type_flags & TN_Numeric_Type || inferred_type_flags & TN_Bool_Type) {

				if (!(inferred_type_flags & TN_Float_Type)) {
					if (as_lit->type == NumericLiteral::Type::Float) {
						literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.float_tn);
						literal_value.f64 = as_lit->Val.Float;
					}
					else {
						literal_type = inferred_type;
						literal_value.us8 = as_lit->Val.Int;
					}
				}
				else {

					literal_type = inferred_type;

					if (as_lit->type == NumericLiteral::Type::Float) {
						literal_value.f64 = as_lit->Val.Float;
					}
					else {
						literal_value.f64 = (double)as_lit->Val.Int;
					}
				}
			}
			else {
				if (as_lit->type == NumericLiteral::Type::Float) {
					literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.float_tn);
					literal_value.f64 = as_lit->Val.Float;
				}
				else if (as_lit->type == NumericLiteral::Type::Int) {
					literal_type = TypeSystem_Get_Basic_Type(Data.type_system, Data.int_tn);
					literal_value.us8 = as_lit->Val.Int;
				}
				else {
					GS_ASSERT_UNIMPL();
				}
			}

			CodeGen_Result result;
			result.ok = true;
			result.code_node_id = Il_Insert_Constant(proc, literal_value, literal_type);
			result.expression_type = literal_type;
			return result;
		}
		case NodeType::BinaryExpression:
		{
			return BinaryExpression_CodeGen(expression, scope_id, proc, inferred_type, by_reference, is_condition);
		}
		case NodeType::NegateExpression:
		{
			NegateExpr* as_negate_expr = (NegateExpr*)expression;

			CodeGen_Result expr_result = Expression_CodeGen(as_negate_expr->What, scope_id, proc, inferred_type, false);

			if (!expr_result)
				return {};

			auto expr_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, expr_result.expression_type);

			if (!(expr_type_flags & TN_Numeric_Type)) {
				Push_Error_Loc(scope_id, as_negate_expr->What, "type is not numeric");
				return {};
			}

			Il_IDX zero = Il_Insert_Constant(proc, {}, expr_result.expression_type);

			CodeGen_Result result;
			result.ok = true;
			result.expression_type = expr_result.expression_type;
			result.code_node_id = Il_Insert_Math_Op(proc, expr_result.expression_type, Il_Sub, zero, expr_result.code_node_id);

			return result;
		}
		break;
		case NodeType::Call:
		{
			FunctionCall* as_call = (FunctionCall*)expression;

			if (as_call->callee->GetType() == NodeType::Identifier) {

				Identifier* callee_as_ident = (Identifier*)as_call->callee;

				if (callee_as_ident->Symbol.Symbol == "type_info") {

					if (as_call->Arguments.size() != 1) {
						Push_Error_Loc(scope_id, as_call, "'type_info(Type)' takes one argument");
						return {};
					}

					auto arg_node = as_call->Arguments[0];

					CodeGen_Result arg_result = Expression_CodeGen(arg_node, scope_id, proc, Data.Type_Ty);

					if (!arg_result)
						return {};

					if (arg_result.expression_type != Data.Type_Ty) {
						Push_Error_Loc(scope_id, arg_node, "'type_info()' expected argument to be of type: 'Type'");
						return {};
					}

					auto element_ptr_node_idx = Il_Insert_AEP(proc, te_Ty, Il_Insert_Global_Address(proc, global_type_info_table_idx), arg_result.code_node_id);

					CodeGen_Result result;
					result.expression_type = Data.TypeInfo_Ptr_Ty;
					result.code_node_id = Il_Insert_Cast(proc, Il_Cast_Ptr, Data.TypeInfo_Ptr_Ty, TypeSystem_Get_Pointer_Type(Data.type_system, te_Ty, 1), element_ptr_node_idx);
					result.ok = true;

					return result;
				}
			}

			Entity* func_callee_entity = nullptr;
			GS_Type* callee_signature = nullptr;

			Eval_Result eval_result = Expression_Evaluate(as_call->callee, scope_id, nullptr, false);

			if (!eval_result) return {};

			if (eval_result.entity_reference != Entity_Null)
			{
				Entity& callee_entity = Data.entity_storage[eval_result.entity_reference];

				if (callee_entity.entity_type == Entity_Type::Function) {
					if (callee_entity.func.poly_morphic) {
						return Poly_Morphic_Call(as_call, eval_result.entity_reference, scope_id, by_reference);
					}
				}

				String callee_name = callee_entity.semantic_name;
				std::string callee_name_std = std::string(callee_name.data);

				if (callee_entity.entity_type == Entity_Type::Function) {

					if (callee_entity.func.parameters.count > as_call->Arguments.size()) {
						Push_Error_Loc(scope_id, as_call, FMT("call to '{}' too few arguments (needed: {}, given: {})", callee_name_std, callee_entity.func.parameters.count, as_call->Arguments.size()));
						return {};
					}

					if (callee_entity.func.parameters.count < as_call->Arguments.size() && !callee_entity.func.c_variadic) {
						Push_Error_Loc(scope_id, as_call, FMT("call to '{}' too many arguments (needed: {}, given: {})", callee_name_std, callee_entity.func.parameters.count, as_call->Arguments.size()));
						return {};
					}

					callee_signature = callee_entity.func.signature;

					func_callee_entity = &callee_entity;
				}
			}

			Il_IDX callee_code_node_idx = -1;

			if (!func_callee_entity) {

				CodeGen_Result callee_result = Expression_CodeGen(as_call->callee, scope_id, proc, nullptr);

				if (!callee_result)
					return {};

				if (callee_result.expression_type->kind != Type_Proc) {
					Push_Error_Loc(scope_id, as_call->callee, "type is not a function type");
					return {};
				}

				callee_signature = callee_result.expression_type;

				if (as_call->Arguments.size() > callee_signature->proc.params.count) {
					Push_Error_Loc(scope_id, as_call, FMT("call too many arguments (needed: {}, given: {})", callee_signature->proc.params.count, as_call->Arguments.size()));
					return {};
				}

				if (as_call->Arguments.size() < callee_signature->proc.params.count) {
					Push_Error_Loc(scope_id, as_call, FMT("call too few arguments (needed: {}, given: {})", callee_signature->proc.params.count, as_call->Arguments.size()));
					return {};
				}

				callee_code_node_idx = callee_result.code_node_id;
			}

			Il_IDX code_node_id;

			Array<Il_Argument> arguments_code;

			auto return_type_size = TypeSystem_Get_Type_Size(Data.type_system, callee_signature->proc.return_type);

			if (return_type_size > 8) {
				code_node_id = Il_Insert_Alloca(proc, callee_signature->proc.return_type);
				Array_Add(arguments_code, Il_Argument{ code_node_id, (Type_IDX)TypeSystem_Get_Type_Index(Data.type_system, callee_signature->proc.return_type) });
			}

			for (size_t i = 0; i < as_call->Arguments.size(); i++)
			{
				Expression* arg_node = as_call->Arguments[i];

				GS_Type* parameter_type = nullptr;

				bool by_reference = false;

				if (i < callee_signature->proc.params.count) {
					parameter_type = callee_signature->proc.params[i];
					by_reference = TypeSystem_Get_Type_Size(Data.type_system, parameter_type) > 8;
				}

				CodeGen_Result arg_result = Expression_CodeGen(arg_node, scope_id, proc, parameter_type, by_reference);

				if (!arg_result) {
					return {};
				}

				if (!arg_result.lvalue && by_reference) {
					auto lvalue_alloca = Il_Insert_Alloca(proc, arg_result.expression_type);
					Il_Insert_Store(proc, arg_result.expression_type, lvalue_alloca, arg_result.code_node_id);

					arg_result.code_node_id = lvalue_alloca;
				}

				if (i < callee_signature->proc.params.count) {
					if (arg_result.expression_type != callee_signature->proc.params[i]) {
						Push_Error_Loc(scope_id, arg_node, FMT("call type mismatch in argument: given: '{}', needed: '{}'", Print_Type(arg_result.expression_type), Print_Type(callee_signature->proc.params[i])));
						return {};
					}
				}

				if (func_callee_entity) {
					if (func_callee_entity->func.c_variadic) {
						if (arg_result.expression_type == Data.float_Ty) {
							arg_result.code_node_id = Il_Insert_Cast(proc, Il_Cast_FloatExt, Data.f64_Ty, Data.float_Ty, arg_result.code_node_id);
						}
					}
				}

				Array_Add(arguments_code, Il_Argument{ arg_result.code_node_id, (Type_IDX)TypeSystem_Get_Type_Index(Data.type_system,arg_result.expression_type) });
			}

			if (return_type_size <= 8) {
				if (func_callee_entity) {
					code_node_id = Il_Insert_Call(proc, callee_signature, arguments_code, func_callee_entity->func.proc_idx);
				}
				else {
					code_node_id = Il_Insert_Call_Ptr(proc, callee_signature, arguments_code, callee_code_node_idx);
				}
			}
			else {
				Il_Insert_Call(proc, callee_signature, arguments_code, func_callee_entity->func.proc_idx);
				code_node_id = Il_Insert_Load(proc, callee_signature->proc.return_type, code_node_id);
			}

			CodeGen_Result result;
			result.expression_type = callee_signature->proc.return_type;
			result.code_node_id = code_node_id;
			result.ok = true;

			return result;
		}
		break;
		case NodeType::MemberAccess:
		{
			MemberAccess* as_member_access = (MemberAccess*)expression;

			Eval_Result eval_result = Expression_Evaluate(as_member_access->Object, scope_id, nullptr, false);

			if (!eval_result) {
				return {};
			}

			if (eval_result.entity_reference != Entity_Null)
			{
				Entity_ID referenced_entity_id = eval_result.entity_reference;

				Entity& referenced_entity = Data.entity_storage[referenced_entity_id];

				switch (referenced_entity.entity_type)
				{
				case Entity_Type::Enum_Entity:
				case Entity_Type::Constant:
				case Entity_Type::Named_File_Load:
				{
					Eval_Result eval_result = Expression_Evaluate(as_member_access, scope_id, nullptr);
					if (!eval_result)
						return {};

					Il_IDX inserted_node_idx = Il_Insert_Constant(proc, eval_result.result, eval_result.expression_type);

					CodeGen_Result result;
					result.ok = true;
					result.expression_type = eval_result.expression_type;
					result.code_node_id = inserted_node_idx;
					return result;
				}
				break;
				}
			}

			CodeGen_Result object_result = Expression_CodeGen(as_member_access->Object, scope_id, proc, inferred_type, true);
			if (!object_result)
				return {};

			bool supports_members = false;

			auto object_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, object_result.expression_type);

			if (object_result.expression_type->kind == Type_Pointer && object_result.expression_type->pointer.indirection == 1) {

				object_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, object_result.expression_type->pointer.pointee);

				object_result.code_node_id = Il_Insert_Load(proc, object_result.expression_type, object_result.code_node_id);
				object_result.expression_type = object_result.expression_type->pointer.pointee;
				supports_members = true;
			}

			if (!(object_type_flags & TN_Struct_Type) && !(object_type_flags & TN_Enum_Type) && object_result.expression_type->kind != Type_Dyn_Array) {
				supports_members = false;
			}
			else {
				supports_members = true;
			}

			if (!supports_members) {
				Push_Error_Loc(scope_id, as_member_access->Object, "type does not support members");
				return {};
			}

			if (object_result.expression_type->kind == Type_Dyn_Array) {
				object_result.expression_type = Data.Array_Ty;
			}

			Entity_ID struct_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, Data.type_system.type_name_storage[object_result.expression_type->basic.type_name_id].name);

			ASSERT(struct_entity_id != Entity_Null);
			Entity& struct_entity = Data.entity_storage[struct_entity_id];

			ASSERT(struct_entity.entity_type == Entity_Type::Struct_Entity);

			Entity* member_entity = nullptr;
			u64 member_index = -1;

			ASSERT(as_member_access->Member->GetType() == NodeType::Identifier);
			Identifier* member_as_ident = (Identifier*)as_member_access->Member;

			GS_Type* member_type = nullptr;

			String member_name = String_Make(member_as_ident->Symbol.Symbol);

			for (size_t i = 0; i < struct_entity.children.count; i++)
			{
				if (String_Equal(Data.entity_storage[struct_entity.children[i]].semantic_name, member_name)) {
					member_entity = &Data.entity_storage[struct_entity.children[i]];
					member_index = i;
					member_type = member_entity->semantic_type;
					break;
				}
			}

			if (!member_entity) {
				Push_Error_Loc(scope_id, as_member_access->Member, FMT("struct does not have member named: '{}'", member_as_ident->Symbol.Symbol));
				return {};
			}

			Il_IDX sep_node_idx = Il_Insert_SEP(proc, object_result.expression_type, member_index, object_result.code_node_id);

			CodeGen_Result result;

			if (by_reference)
			{
				result.code_node_id = sep_node_idx;
				result.lvalue = true;
			}
			else {
				result.code_node_id = Il_Insert_Load(proc, member_type, sep_node_idx);
			}

			result.ok = true;
			result.expression_type = member_type;
			result.immutable = object_result.immutable;
			return result;
		}
		break;
		case NodeType::Cast:
		{
			CastNode* as_cast_node = (CastNode*)expression;

			GS_Type* cast_to_type = Evaluate_Type(as_cast_node->Type, scope_id);

			if (!cast_to_type) {
				Push_Error_Loc(scope_id, as_cast_node->Type, FMT("cast() unknown type"));
				return {};
			}

			CodeGen_Result expr_result = Expression_CodeGen(as_cast_node->Expr, scope_id, proc, cast_to_type);
			if (!expr_result) return {};

			GS_Type* castee_type = expr_result.expression_type;

			CodeGen_Result result;
			result.ok = true;
			result.expression_type = cast_to_type;
			result.code_node_id = expr_result.code_node_id;

			if (castee_type == cast_to_type)
			{
				return result;
			}

			auto castee_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, castee_type);
			auto to_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, cast_to_type);

			bool valid_cast = false;

			if (cast_to_type->kind == castee_type->kind) {
				if (cast_to_type->kind == Type_Pointer)
				{
					result.code_node_id = Il_Insert_Cast(proc, Il_Cast_Ptr, cast_to_type, castee_type, expr_result.code_node_id);
					valid_cast = true;
				}
				else if (cast_to_type->kind == Type_Basic) {

					if (to_type_flags & TN_Numeric_Type && castee_type_flags & TN_Numeric_Type)
					{
						bool is_castee_float = castee_type_flags & TN_Float_Type;
						bool is_to_float = to_type_flags & TN_Float_Type;

						if (!is_castee_float && is_to_float) {
							result.code_node_id = Il_Insert_Cast(proc, Il_Cast_Int2Float, cast_to_type, castee_type, expr_result.code_node_id);
							valid_cast = true;
						}

						if (!is_to_float && is_castee_float) {
							result.code_node_id = Il_Insert_Cast(proc, Il_Cast_Float2Int, cast_to_type, castee_type, expr_result.code_node_id);
							valid_cast = true;
						}

						if (!is_to_float && !is_castee_float)
						{
							auto from_type_size = TypeSystem_Get_Type_Size(Data.type_system, castee_type);
							auto to_type_size = TypeSystem_Get_Type_Size(Data.type_system, cast_to_type);

							if (to_type_size == from_type_size) {
								return result;
							}

							auto int_cast_type = Il_Cast_IntTrunc;

							if (to_type_size > from_type_size) {

								if (to_type_flags & TN_Unsigned_Type)
									int_cast_type = Il_Cast_IntZExt;
								else
									int_cast_type = Il_Cast_IntSExt;
							}

							result.code_node_id = Il_Insert_Cast(proc, int_cast_type, cast_to_type, castee_type, expr_result.code_node_id);
							valid_cast = true;
						}

						if (is_to_float && is_castee_float)
						{
							auto from_type_size = TypeSystem_Get_Type_Size(Data.type_system, castee_type);
							auto to_type_size = TypeSystem_Get_Type_Size(Data.type_system, cast_to_type);

							if (to_type_size == from_type_size) {
								return result;
							}

							auto int_cast_type = Il_Cast_FloatTrunc;

							if (to_type_size > from_type_size) {
								int_cast_type = Il_Cast_FloatExt;
							}

							result.code_node_id = Il_Insert_Cast(proc, int_cast_type, cast_to_type, castee_type, expr_result.code_node_id);
							valid_cast = true;
						}
					}
				}
			}
			else if (cast_to_type->kind == Type_Pointer) {
				if (castee_type_flags & TN_Numeric_Type)
				{
					result.code_node_id = Il_Insert_Cast(proc, Il_Cast_Int2Ptr, cast_to_type, castee_type, expr_result.code_node_id);
					valid_cast = true;
				}
			}
			if (!valid_cast) {
				Push_Error_Loc(scope_id, as_cast_node->Type, FMT("invalid cast"));
				return {};
			}

			return result;
		}
		case NodeType::ArrayAccess:
		{
			ArrayAccess* as_array_access = (ArrayAccess*)expression;

			CodeGen_Result object_result = Expression_CodeGen(as_array_access->Object, scope_id, proc, inferred_type, true);
			if (!object_result)
				return {};

			if (object_result.expression_type->kind != Type_Array && object_result.expression_type->kind != Type_Pointer && object_result.expression_type->kind != Type_Dyn_Array) {
				Push_Error_Loc(scope_id, as_array_access->Object, "type doesn't support subscript operator []");
				return {};
			}

			CodeGen_Result index_result = Expression_CodeGen(as_array_access->Index, scope_id, proc, nullptr, false);
			if (!index_result)
				return {};

			if (!(TypeSystem_Get_Type_Flags(Data.type_system, index_result.expression_type) & TN_Numeric_Type)) {
				Push_Error_Loc(scope_id, as_array_access->Index, "[] index type is not numeric");
				return {};
			}

			GS_Type* array_element_type = nullptr;

			if (object_result.expression_type->kind == Type_Array)
				array_element_type = object_result.expression_type->array.element_type;
			else if (object_result.expression_type->kind == Type_Pointer) {
				array_element_type = object_result.expression_type->pointer.pointee;
				object_result.code_node_id = Il_Insert_Load(proc, object_result.expression_type, object_result.code_node_id);
			}
			else if (object_result.expression_type->kind == Type_Dyn_Array) {

				array_element_type = object_result.expression_type->dyn_array.element_type;

				Il_IDX array_data_member_node_idx = Il_Insert_SEP(proc, Data.Array_Ty, 1, object_result.code_node_id);
				Il_IDX loaded_data_member_node_idx = Il_Insert_Load(proc, Data.void_ptr_Ty, array_data_member_node_idx);
				object_result.code_node_id = Il_Insert_Cast(proc, Il_Cast_Ptr, TypeSystem_Get_Pointer_Type(Data.type_system, array_element_type, 1), Data.void_ptr_Ty, loaded_data_member_node_idx);
			}
			else {
				GS_ASSERT_UNIMPL();
			}

			CodeGen_Result result = {};

			Il_IDX aep_node_idx = Il_Insert_AEP(proc, array_element_type, object_result.code_node_id, index_result.code_node_id);

			if (by_reference) {
				result.code_node_id = aep_node_idx;
			}
			else {
				result.code_node_id = Il_Insert_Load(proc, array_element_type, aep_node_idx);
			}

			result.expression_type = array_element_type;

			result.ok = true;

			result.lvalue = true;

			return result;
		}
		case NodeType::Reference:
		{
			RefNode* as_ref = (RefNode*)expression;

			CodeGen_Result expr_result = Expression_CodeGen(as_ref->What, scope_id, proc, inferred_type, true);
			if (!expr_result)
				return {};

			CodeGen_Result result = {};
			result.ok = true;
			result.code_node_id = expr_result.code_node_id;
			result.expression_type = TypeSystem_Get_Pointer_Type(Data.type_system, expr_result.expression_type, 1);

			return result;
		}
		break;
		case NodeType::DeReference:
		{
			DeRefNode* as_de_ref = (DeRefNode*)expression;

			CodeGen_Result expr_result = Expression_CodeGen(as_de_ref->What, scope_id, proc, inferred_type, false);
			if (!expr_result)
				return {};

			if (expr_result.expression_type->kind != Type_Pointer) {
				Push_Error_Loc(scope_id, as_de_ref->What, FMT("trying to dereference a non pointer type!"));
				return {};
			}

			CodeGen_Result result = {};
			result.ok = true;
			result.expression_type = TypeSystem_Reduce_Indirection(Data.type_system, expr_result.expression_type);

			if (by_reference)
			{
				result.code_node_id = expr_result.code_node_id;
				result.lvalue = true;
			}
			else {
				result.code_node_id = Il_Insert_Load(proc, result.expression_type, expr_result.code_node_id);
			}

			return result;
		}
		case NodeType::TE_Array:
		case NodeType::TE_Func:
		case NodeType::TE_Pointer:
		{
			Eval_Result eval_result = Expression_Evaluate(expression, scope_id, nullptr);
			if (!eval_result)
				return {};

			Const_Union constant_value = {};
			constant_value.s8 = TypeSystem_Get_Type_Index(Data.type_system, eval_result.result.type);

			CodeGen_Result result;
			result.ok = true;
			result.expression_type = Data.Type_Ty;
			result.code_node_id = Il_Insert_Constant(proc, constant_value, Data.Type_Ty);
			return result;
		}
		break;
		case NodeType::SizeOf:
		{
			SizeOfNode* as_sizeof = (SizeOfNode*)expression;

			Eval_Result eval_result = Expression_Evaluate(as_sizeof->Expr, scope_id, nullptr, false);
			if (!eval_result)
				return {};

			u64 type_size = 0;

			if (eval_result.expression_type == Data.Type_Ty) {
				type_size = TypeSystem_Get_Type_Size(Data.type_system, eval_result.result.type);
			}
			else {
				type_size = TypeSystem_Get_Type_Size(Data.type_system, eval_result.expression_type);
			}

			CodeGen_Result result;
			result.ok = true;
			result.expression_type = Data.u64_Ty;
			result.code_node_id = Il_Insert_Constant(proc, (void*)type_size, Data.Type_Ty);
			return result;
		}
		break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return {};
	}

	Iterator_Result Front_End::Iterator_CodeGen(Expression* expression, Entity_ID scope_id, Il_Proc& proc, Il_IDX before_condition_block, Il_IDX condition_block, Il_IDX after_body_block)
	{

		NodeType node_type = expression->GetType();

		switch (node_type)
		{
		case NodeType::Range:
		{
			RangeNode* as_range = (RangeNode*)expression;

			Il_Set_Insert_Point(proc, before_condition_block);

			CodeGen_Result begin_result = Expression_CodeGen(as_range->Begin, scope_id, proc, nullptr);
			if (!begin_result)
				return {};

			CodeGen_Result end_result = Expression_CodeGen(as_range->End, scope_id, proc, nullptr);
			if (!end_result)
				return {};

			if (begin_result.expression_type != end_result.expression_type) {
				Push_Error_Loc(scope_id, expression, FMT("range iterator type mismatch: '{}', '{}'", Print_Type(begin_result.expression_type), Print_Type(end_result.expression_type)));
				return {};
			}

			GS_Type* range_type = begin_result.expression_type;
			auto range_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, range_type);

			if (!(range_type_flags & TN_Numeric_Type)) {
				Push_Error_Loc(scope_id, expression, FMT("range iterator type is not numeric: '{}'", Print_Type(range_type)));
				return {};
			}

			Il_IDX it_alloca = Il_Insert_Alloca(proc, range_type);
			Il_IDX it_index_alloca = Il_Insert_Alloca(proc, range_type);

			Il_Insert_Store(proc, range_type, it_alloca, begin_result.code_node_id);
			Il_Insert_Store(proc, range_type, it_index_alloca, Il_Insert_Constant(proc, (void*)0, range_type));

			Il_Set_Insert_Point(proc, condition_block);

			Il_IDX it_load = Il_Insert_Load(proc, range_type, it_alloca);
			Il_IDX cmp_node_idx = Il_Insert_Compare(proc, Il_Value_Cmp, Il_Cmp_Lesser, range_type, it_load, end_result.code_node_id);

			Il_Set_Insert_Point(proc, after_body_block);

			it_load = Il_Insert_Load(proc, range_type, it_alloca);
			Il_Insert_Store(proc, range_type, it_alloca, Il_Insert_Math_Op(proc, range_type, Il_Add, it_load, Il_Insert_Constant(proc, (void*)1, range_type)));

			Il_IDX it_index_load = Il_Insert_Load(proc, range_type, it_index_alloca);
			Il_Insert_Store(proc, range_type, it_index_alloca, Il_Insert_Math_Op(proc, range_type, Il_Add, it_index_load, Il_Insert_Constant(proc, (void*)1, range_type)));

			Iterator_Result result;
			result.ok = true;
			result.condition_node = cmp_node_idx;
			result.it_type = range_type;
			result.it_index_type = range_type;
			result.it_index_location_node = it_index_alloca;
			result.it_location_node = it_alloca;

			return result;
		}
		break;
		default:
		{
			auto expr_result = Expression_CodeGen(expression, scope_id, proc, nullptr, true);

			if (!expr_result)
				return {};

			if (expr_result.expression_type->kind != Type_Dyn_Array && expr_result.expression_type->kind != Type_Array && expr_result.expression_type != Data.string_Ty) {
				Push_Error_Loc(scope_id, expression, FMT("type is not iterable: '{}'", Print_Type(expr_result.expression_type)));
				return {};
			}

			GS_Type* it_type = nullptr;

			if (expr_result.expression_type->kind == Type_Array) {
				it_type = expr_result.expression_type->array.element_type;
			}
			else if (expr_result.expression_type->kind == Type_Dyn_Array) {
				it_type = expr_result.expression_type->dyn_array.element_type;
			}
			else if (expr_result.expression_type == Data.string_Ty) {
				it_type = Data.u8_Ty;
			}

			GS_Type* it_index_type = Data.u64_Ty;

			Il_Set_Insert_Point(proc, before_condition_block);

			Il_IDX it_alloca = Il_Insert_Alloca(proc, it_type);
			Il_IDX it_index_alloca = Il_Insert_Alloca(proc, it_index_type);
			Il_Insert_Store(proc, it_index_type, it_index_alloca, Il_Insert_Constant(proc, (void*)0, it_index_type));

			Il_Set_Insert_Point(proc, condition_block);

			auto string_ty_ptr = TypeSystem_Get_Pointer_Type(Data.type_system, Data.string_Ty, 1);
			auto array_ty_ptr = TypeSystem_Get_Pointer_Type(Data.type_system, Data.Array_Ty, 1);

			Il_IDX end = -1;
			if (expr_result.expression_type->kind == Type_Array)
				end = Il_Insert_Constant(proc, (void*)expr_result.expression_type->array.size, it_index_type);
			else if (expr_result.expression_type == Data.string_Ty) {
				Il_IDX sep = Il_Insert_SEP(proc, Data.Array_Ty, 0, Il_Insert_Cast(proc, Il_Cast_Ptr, array_ty_ptr, string_ty_ptr, expr_result.code_node_id));
				end = Il_Insert_Load(proc, it_index_type, sep);
			}
			else
			{
				end = Il_Insert_Load(proc, it_index_type, Il_Insert_SEP(proc, Data.Array_Ty, 0, expr_result.code_node_id));
			}

			Il_IDX it_index_load = Il_Insert_Load(proc, it_index_type, it_index_alloca);

			Il_IDX array_ptr;

			GS_Type* it_type_pointer = TypeSystem_Get_Pointer_Type(Data.type_system, it_type, 1);

			if (expr_result.expression_type->kind == Type_Array)
				array_ptr = expr_result.code_node_id;
			else if (expr_result.expression_type == Data.string_Ty)
			{
				array_ptr = Il_Insert_Load(proc, it_type_pointer, Il_Insert_Cast(proc, Il_Cast_Ptr, array_ty_ptr, string_ty_ptr, Il_Insert_SEP(proc, Data.Array_Ty, 1, expr_result.code_node_id)));
			}
			else
			{
				array_ptr = Il_Insert_Load(proc, it_type_pointer, Il_Insert_SEP(proc, Data.Array_Ty, 1, expr_result.code_node_id));
			}

			Il_IDX array_element_ptr = Il_Insert_AEP(proc, it_type, array_ptr, it_index_load);
			Il_Insert_Store(proc, it_type, it_alloca, Il_Insert_Load(proc, it_type, array_element_ptr));

			Il_IDX cmp_node_idx = Il_Insert_Compare(proc, Il_Value_Cmp, Il_Cmp_Lesser, it_index_type, it_index_load, end);

			Il_Set_Insert_Point(proc, after_body_block);

			it_index_load = Il_Insert_Load(proc, it_index_type, it_index_alloca);
			Il_Insert_Store(proc, it_index_type, it_index_alloca, Il_Insert_Math_Op(proc, it_index_type, Il_Add, it_index_load, Il_Insert_Constant(proc, (void*)1, it_index_type)));

			Iterator_Result result;
			result.ok = true;
			result.condition_node = cmp_node_idx;
			result.it_type = it_type;
			result.it_index_type = it_index_type;
			result.it_index_location_node = it_index_alloca;
			result.it_location_node = it_alloca;

			return result;

			return {};
		}
		break;
		}

		return {};
	}

	Eval_Result Front_End::Expression_Evaluate(Expression* expression, Entity_ID scope_id, GS_Type* inferred_type, bool const_eval)
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

			GS_Type* literal_type = nullptr;

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
		case NodeType::NegateExpression:
		{
			NegateExpr* as_negate_expr = (NegateExpr*)expression;

			Eval_Result expr_result = Expression_Evaluate(as_negate_expr->What, scope_id, inferred_type, const_eval);

			if (!expr_result)
				return {};

			auto expr_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, expr_result.expression_type);

			if (!(expr_type_flags & TN_Numeric_Type)) {
				Push_Error_Loc(scope_id, as_negate_expr->What, "type is not numeric");
				return {};
			}

			Eval_Result result;
			result.ok = true;
			result.expression_type = expr_result.expression_type;
			result.result.s8 = expr_result.result.s8;
			return result;
		}
		break;
		case NodeType::BinaryExpression:
		{
			BinaryExpression* as_bin_expr = (BinaryExpression*)expression;

			Eval_Result left_result = Expression_Evaluate(as_bin_expr->Left, scope_id, inferred_type, const_eval);
			Eval_Result right_result = Expression_Evaluate(as_bin_expr->Right, scope_id, left_result.expression_type, const_eval);

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

			if (as_ident->Symbol.Symbol == "null") {

				GS_Type* null_type = inferred_type;

				if (!null_type) {
					null_type = TypeSystem_Get_Pointer_Type(Data.type_system, Data.void_Ty, 1);
				}

				Eval_Result result = {};
				result.ok = true;
				result.expression_type = null_type;
				result.result = {};
				return result;
			}

			String name = String_Make(as_ident->Symbol.Symbol);
			Entity_ID identified_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, name);

			if (identified_entity_id == Entity_Null) {
				Push_Error_Loc(scope_id, as_ident, FMT("undefined name!: '{}'", as_ident->Symbol.Symbol));
				return {};
			}

			Entity& identified_entity = Data.entity_storage[identified_entity_id];

			ASSERT(identified_entity_id != Entity_Null);
			//ASSERT(identified_entity.semantic_type);
			///ASSERT(identified_entity.flags & EF_Constant);
			ASSERT(!(identified_entity.flags & EF_InComplete));

			if (!(identified_entity.flags & EF_Constant) && const_eval) {
				Push_Error_Loc(scope_id, as_ident, FMT("'{}' is not a constant", as_ident->Symbol.Symbol));
				return {};
			}

			Eval_Result result;
			result.ok = true;
			result.expression_type = identified_entity.semantic_type;
			result.result = identified_entity.constant_value;
			result.entity_reference = identified_entity_id;
			return result;
		}
		break;
		case NodeType::TE_Pointer:
		case NodeType::TE_Func:
		case NodeType::TE_Array:
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

			Eval_Result object_result = Expression_Evaluate(as_member_access->Object, scope_id, nullptr, const_eval);

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
					result.result = member_entity->constant_value;
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

						if (object_result.expression_type->kind == Type_Pointer && object_result.expression_type->pointer.indirection == 1) {
							object_result.expression_type = object_result.expression_type->pointer.pointee;
						}
						else {
							Push_Error_Loc(scope_id, as_member_access->Object, "type does not support members");
							return {};
						}
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
					result.expression_type = member_entity->semantic_type;
					result.result.ptr = &(&object_result.result.us1)[offset];
					return result;
				}
			}
		}
		break;
		case NodeType::DeReference:
		{
			ASSERT(const_eval == false);

			DeRefNode* as_de_ref = (DeRefNode*)expression;

			Eval_Result expr_result = Expression_Evaluate(as_de_ref->What, scope_id, nullptr, const_eval);
			if (!expr_result)
				return {};

			if (expr_result.expression_type->kind != Type_Pointer) {
				Push_Error_Loc(scope_id, as_de_ref->What, FMT("trying to dereference a non pointer type!"));
				return {};
			}

			Eval_Result result;
			result.ok = true;
			result.expression_type = TypeSystem_Reduce_Indirection(Data.type_system, expr_result.expression_type);
			return result;
		}
		break;
		case NodeType::ArrayAccess:
		{
			ASSERT(const_eval == false);

			ArrayAccess* as_array_access = (ArrayAccess*)expression;

			Eval_Result object_result = Expression_Evaluate(as_array_access->Object, scope_id, nullptr, const_eval);
			if (!object_result)
				return {};

			if (object_result.expression_type)
				if (object_result.expression_type->kind != Type_Array && object_result.expression_type->kind != Type_Pointer && object_result.expression_type->kind != Type_Dyn_Array) {
					Push_Error_Loc(scope_id, as_array_access->Object, "type doesn't support subscript operator []");
					return {};
				}

			Eval_Result index_result = Expression_Evaluate(as_array_access->Index, scope_id, nullptr, const_eval);
			if (!index_result)
				return {};

			if (!(TypeSystem_Get_Type_Flags(Data.type_system, index_result.expression_type) & TN_Numeric_Type)) {
				Push_Error_Loc(scope_id, as_array_access->Index, "[] index type is not numeric");
				return {};
			}

			GS_Type* array_element_type = nullptr;

			if (object_result.expression_type->kind == Type_Array)
				array_element_type = object_result.expression_type->array.element_type;

			if (object_result.expression_type->kind == Type_Pointer)
				array_element_type = object_result.expression_type->pointer.pointee;

			if (object_result.expression_type->kind == Type_Dyn_Array)
				array_element_type = object_result.expression_type->dyn_array.element_type;

			Eval_Result result;
			result.ok = true;
			result.expression_type = array_element_type;
			return result;
		}
		break;
		case NodeType::Cast:
		{
			CastNode* as_cast_node = (CastNode*)expression;

			GS_Type* cast_to_type = Evaluate_Type(as_cast_node->Type, scope_id);

			if (!cast_to_type) {
				Push_Error_Loc(scope_id, as_cast_node->Type, FMT("cast() unknown type"));
				return {};
			}

			Eval_Result expr_result = Expression_Evaluate(as_cast_node->Expr, scope_id, cast_to_type, const_eval);
			if (!expr_result) return {};

			GS_Type* castee_type = expr_result.expression_type;

			Eval_Result result;
			result.ok = true;
			result.expression_type = cast_to_type;
			result.result = {};

			auto castee_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, castee_type);
			auto to_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, cast_to_type);

			if (cast_to_type->kind == castee_type->kind) {
				if (cast_to_type->kind == Type_Pointer) {
					result.result.ptr = expr_result.result.ptr;
				}
				else {
					Push_Error_Loc(scope_id, as_cast_node->Type, FMT("invalid cast"));
					return {};
				}
			}
			else if (cast_to_type->kind == Type_Pointer) {
				if (castee_type->kind == Type_Basic && castee_type_flags & TN_Numeric_Type)
				{
					if (!(castee_type_flags & TN_Float_Type)) {
						if (castee_type_flags & TN_Unsigned_Type)
							result.result.ptr = (void*)expr_result.result.us8;
						else
							result.result.ptr = (void*)expr_result.result.s8;
					}
					else {
						result.result.ptr = (void*)(size_t)expr_result.result.f64;
					}
				}
				else {
					Push_Error_Loc(scope_id, as_cast_node->Type, FMT("invalid cast"));
					return {};
				}
			}
			else {
				Push_Error_Loc(scope_id, as_cast_node->Type, FMT("invalid cast"));
				return {};
			}

			return result;
		}
		case NodeType::SizeOf:
		{
			SizeOfNode* as_sizeof = (SizeOfNode*)expression;

			Eval_Result eval_result = Expression_Evaluate(as_sizeof->Expr, scope_id, nullptr, false);
			if (!eval_result)
				return {};

			u64 type_size = 0;

			if (eval_result.expression_type == Data.Type_Ty) {
				type_size = TypeSystem_Get_Type_Size(Data.type_system, eval_result.result.type);
			}
			else {
				type_size = TypeSystem_Get_Type_Size(Data.type_system, eval_result.expression_type);
			}

			Eval_Result result;
			result.ok = true;
			result.expression_type = Data.u64_Ty;
			result.result.us8 = type_size;
			return result;
		}
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return {};
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
			//ASSERT(identified_entity.flags & EF_Constant);
			ASSERT(!(identified_entity.flags & EF_InComplete));

			if (!(identified_entity.flags & EF_Constant)) {
				Push_Error_Loc(scope_id, as_ident, FMT("'{}' is not a constant", as_ident->Symbol.Symbol));
				return {};
			}

			if (identified_entity.semantic_type != Data.Type_Ty) {
				Push_Error_Loc(scope_id, as_ident, FMT("'{}' is not a type", as_ident->Symbol.Symbol));
				return {};
			}

			if (identified_entity.entity_type == Entity_Type::Struct_Entity) {
				if (identified_entity.struct_entity.poly_morphic) {
					Push_Error_Loc(scope_id, as_ident, FMT("polymorphic struct '{}' parameter list expected", as_ident->Symbol.Symbol));
					return {};
				}
			}

			return identified_entity.constant_value.type;
		}
		break;
		case NodeType::TE_Pointer: {
			PointerExpr* as_pointer = (PointerExpr*)expression;

			GS_Type* pointee_result = Evaluate_Type(as_pointer->Pointee, scope_id);

			if (!pointee_result)
				return nullptr;

			GS_Type* generated_type = TypeSystem_Get_Pointer_Type(Data.type_system, pointee_result, 1);
			return generated_type;
		}
								 break;
		case NodeType::TE_Array: {
			ArrayTypeExpr* as_array = (ArrayTypeExpr*)expression;

			GS_Type* element_type = Evaluate_Type(as_array->ElementType, scope_id);

			if (!element_type)
				return nullptr;

			if (as_array->Size) {

				Eval_Result size_eval_result = Expression_Evaluate(as_array->Size, scope_id, nullptr);
				if (!size_eval_result)
					return nullptr;

				if (!size_eval_result.expression_type) {
					Push_Error_Loc(scope_id, as_array->Size, "expected array type size to have an integral numeric type");
					return nullptr;
				}

				auto array_size_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, size_eval_result.expression_type);

				if (!(array_size_type_flags & TN_Numeric_Type) || (array_size_type_flags & TN_Float_Type)) {
					Push_Error_Loc(scope_id, as_array->Size, "expected array type size to have an integral numeric type");
					return nullptr;
				}

				if (size_eval_result.result.s8 <= 0) {
					Push_Error_Loc(scope_id, as_array->Size, FMT("expected array type size: [{}] to be greater than zero", size_eval_result.result.s8));
					return nullptr;
				}

				GS_Type* generated_type = TypeSystem_Get_Array_Type(Data.type_system, element_type, size_eval_result.result.s8);
				return generated_type;

			}
			else if (as_array->Dynamic) {
				GS_Type* generated_type = TypeSystem_Get_Dyn_Array_Type(Data.type_system, element_type);
				return generated_type;
			}
			else {
				ASSERT(nullptr);
			}
		}
							   break;
		case NodeType::TE_Func:
		{
			FuncExpr* as_func = (FuncExpr*)expression;

#define param_array_size 128

			GS_Type* param_types[param_array_size] = {};

			Array<GS_Type*> param_types_array;
			param_types_array.count = 0;
			param_types_array.capacity = param_array_size;
			param_types_array.data = param_types;

			GS_Type* return_type = Evaluate_Type(as_func->ReturnType, scope_id);

			if (!return_type)
				return nullptr;

			for (size_t i = 0; i < as_func->Arguments.size(); i++)
			{
				GS_Type* param_type = Evaluate_Type(as_func->Arguments[i], scope_id);

				if (!param_type)
					return nullptr;

				Array_Add(param_types_array, param_type);
			}

			return TypeSystem_Get_Proc_Type(Data.type_system, return_type, param_types_array);
		}
		case NodeType::Call:
		{
			FunctionCall* as_call = (FunctionCall*)expression;

			Eval_Result type_name_result = Expression_Evaluate(as_call->callee, scope_id, nullptr);

			if (!type_name_result)
				return nullptr;

			if (type_name_result.expression_type != Data.Type_Ty || type_name_result.entity_reference == Entity_Null) {
				Push_Error_Loc(scope_id, as_call->callee, "expression is not a type");
				return nullptr;
			}

			Entity& entity = Data.entity_storage[type_name_result.entity_reference];

			bool valid = false;

			if (entity.entity_type == Entity_Type::Struct_Entity) {
				if (entity.struct_entity.poly_morphic)
					valid = true;
			}

			if (!valid) {
				Push_Error_Loc(scope_id, as_call->callee, "expected a polymorhpic struct");
				return nullptr;
			}

			return nullptr;
		}
		break;
		default:
			ASSERT(nullptr);
			return nullptr;
			break;
		}
	}

	CodeGen_Result Front_End::Poly_Morphic_Call(Expression* expression, Entity_ID entity_id, Entity_ID scope_id, bool by_reference)
	{
		FunctionCall* as_call = (FunctionCall*)expression;

		Entity& entity = Data.entity_storage[entity_id];
		ASSERT(entity.entity_type == Entity_Type::Function);

		if (as_call->Arguments.size() < entity.func.parameters.count) {
			Push_Error_Loc(scope_id, expression, FMT("call to polymorhpic function '{}' too few arguments (needed: {}, given: {})", entity.semantic_name.data, entity.func.parameters.count, as_call->Arguments.size()));
			return {};
		}

		if (as_call->Arguments.size() > entity.func.parameters.count) {
			Push_Error_Loc(scope_id, expression, FMT("call to polymorhpic function '{}' too many arguments (needed: {}, given: {})", entity.semantic_name.data, as_call->Arguments.size(), entity.func.parameters.count));
			return {};
		}

		std::map<std::string_view, Const_Union> polymorphic_values;

		for (size_t i = 0; i < entity.func.parameters.count; i++)
		{
			Function_Parameter& param = entity.func.parameters[i];

			ArgumentNode* as_argument = (ArgumentNode*)param.syntax_node;

			if (param.poly_morhpic) {

				Eval_Result eval_result = Expression_Evaluate(as_call->Arguments[i], scope_id, nullptr);

				if (!eval_result)
					return {};

				polymorphic_values.emplace(as_argument->Symbol.Symbol, eval_result.result);
			}
		}

		auto file_id = Data.entity_storage[Front_End_Data::Get_File_Scope_Parent(Data, scope_id)].file_scope.file_id;

		Entity instance = Create_Function_Entity(entity.semantic_name, entity.source_location, file_id);

		return {};
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

			if (as_ident->Symbol.Symbol == "null") {
				return { true };
			}

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
		case NodeType::NegateExpression:
		{
			auto as_negate = (NegateExpr*)expr;
			return Do_Expression_Dependecy_Pass(scope_id, as_negate->What, dependencies, ignore_indirect);
		}
		break;
		case NodeType::Cast:
		{
			auto as_cast = (CastNode*)expr;

			auto expr_result = Do_Expression_Dependecy_Pass(scope_id, as_cast->Expr, dependencies, ignore_indirect);
			if (!expr_result)
				return {};

			return Do_Expression_Dependecy_Pass(scope_id, as_cast->Type, dependencies, ignore_indirect);
		}
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
			PointerExpr* as_pointer = (PointerExpr*)expr;
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
		case NodeType::ArrayAccess:
		{
			ArrayAccess* as_array_access = (ArrayAccess*)expr;
			ASSERT(as_array_access->Index);
			ASSERT(as_array_access->Object);

			Array<Entity_ID> indirect_dependencies;

			if (!Do_Expression_Dependecy_Pass(scope_id, as_array_access->Index, indirect_dependencies, ignore_indirect)) {
				return {};
			}

			if (!Do_Expression_Dependecy_Pass(scope_id, as_array_access->Object, indirect_dependencies, ignore_indirect)) {
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
		case NodeType::TE_Func:
		{
			FuncExpr* as_func = (FuncExpr*)expr;

			Array<Entity_ID> indirect_dependencies;

			if (!Do_Expression_Dependecy_Pass(scope_id, as_func->ReturnType, indirect_dependencies, ignore_indirect)) {
				return {};
			}

			for (size_t i = 0; i < as_func->Arguments.size(); i++)
			{
				if (!Do_Expression_Dependecy_Pass(scope_id, as_func->Arguments[i], indirect_dependencies, ignore_indirect)) {
					return {};
				}
			}

			for (size_t i = 0; i < indirect_dependencies.count; i++)
			{
				if (indirect_dependencies[i] != ignore_indirect) {
					Array_Add(dependencies, indirect_dependencies[i]);
				}
			}

			return { true };
		}
		case NodeType::TE_Array:
		{
			ArrayTypeExpr* as_array = (ArrayTypeExpr*)expr;
			ASSERT(as_array->ElementType);

			Array<Entity_ID> indirect_dependencies;

			if (!Do_Expression_Dependecy_Pass(scope_id, as_array->ElementType, indirect_dependencies, ignore_indirect)) {
				return {};
			}

			if (as_array->Size) {
				if (!Do_Expression_Dependecy_Pass(scope_id, as_array->Size, indirect_dependencies, ignore_indirect)) {
					return {};
				}
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
		{
			SizeOfNode* as_sizeof = (SizeOfNode*)expr;

			Array<Entity_ID> indirect_dependencies;

			if (!Do_Expression_Dependecy_Pass(scope_id, as_sizeof->Expr, indirect_dependencies, ignore_indirect)) {
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

		auto lex_start = std::chrono::high_resolution_clock::now();
		Lexer lexer = Lexer(source, file.Path);
		file.Tokens = lexer.Lex();
		auto lex_end = std::chrono::high_resolution_clock::now();
		Data.lex_time_micro_seconds += std::chrono::duration_cast<std::chrono::microseconds>(lex_end - lex_start).count();

		auto parse_start = std::chrono::high_resolution_clock::now();
		Parser parser = Parser(file.Path, file.Tokens);
		file.Syntax = parser.CreateAST();
		auto parse_end = std::chrono::high_resolution_clock::now();
		Data.parse_time_micro_seconds += std::chrono::duration_cast<std::chrono::microseconds>(parse_end - parse_start).count();

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

			parent.children_lookup[std::string_view(entity.semantic_name.data, entity.semantic_name.count)] = entity_identifier;

			Array_Add(parent.children, entity_identifier);
			entity.parent = parent_id;
		}

		Array_Add(Data.entity_storage, entity);

		//Array_Add(Data.name_to_entities[std::string_view(entity.semantic_name.data, entity.semantic_name.count)], entity_identifier);

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

		Entity file_entity = { 0 };
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

		Entity file_load_entity = { 0 };
		file_load_entity.entity_type = Entity_Type::File_Load;
		file_load_entity.semantic_name = String_Make("load " + file.Path.string());
		file_load_entity.source_location = { 0,0 };
		file_load_entity.definition_file = file_id;

		file_load_entity.file_load.loaded_file_id = loaded_file_id;

		return file_load_entity;
	}

	Entity Front_End::Create_File_Named_Load_Entity(Entity_ID file_load_entity_id, String name, Source_Loc source_location, File_ID file_id)
	{
		Entity named_file_load_entity = { 0 };

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

	Entity Front_End::Create_Function_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity func_entity = { 0 };

		func_entity.entity_type = Entity_Type::Function;
		func_entity.semantic_name = String_Copy(name);
		func_entity.source_location = source_location;
		func_entity.definition_file = file_id;
		func_entity.semantic_type = nullptr;

		return func_entity;
	}

	Entity Front_End::Create_Function_Scope_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity func_scope_entity = { 0 };

		func_scope_entity.entity_type = Entity_Type::Function_Scope;
		func_scope_entity.semantic_name = String_Copy(name);
		func_scope_entity.source_location = source_location;
		func_scope_entity.definition_file = file_id;
		func_scope_entity.semantic_type = nullptr;

		return func_scope_entity;
	}

	Entity Front_End::Create_Variable_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity var_entity = { 0 };

		var_entity.entity_type = Entity_Type::Variable;
		var_entity.semantic_name = String_Copy(name);
		var_entity.source_location = source_location;
		var_entity.definition_file = file_id;
		var_entity.semantic_type = nullptr;

		return var_entity;
	}

	Entity Front_End::Create_Library_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity lib_entity = { 0 };

		lib_entity.entity_type = Entity_Type::Library;
		lib_entity.semantic_name = String_Copy(name);
		lib_entity.source_location = source_location;
		lib_entity.definition_file = file_id;
		lib_entity.semantic_type = nullptr;

		return lib_entity;
	}

	Entity Front_End::Create_Operator_Overload(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity operator_entity = { 0 };

		operator_entity.entity_type = Entity_Type::Operator_Overload;
		operator_entity.semantic_name = name;
		operator_entity.source_location = source_location;
		operator_entity.definition_file = file_id;
		operator_entity.semantic_type = nullptr;

		return operator_entity;
	}

	Entity Front_End::Create_Struct_Parameter_Entity(String name, Source_Loc source_location, File_ID file_id)
	{
		Entity param_entity = { 0 };

		param_entity.entity_type = Entity_Type::Struct_Parameter;
		param_entity.semantic_name = name;
		param_entity.source_location = source_location;
		param_entity.definition_file = file_id;
		param_entity.semantic_type = nullptr;

		return param_entity;
	}

	CodeGen_Result Front_End::BinaryExpression_TryOverload(Expression* expression, Entity_ID scope_id, Il_Proc& proc, GS_Type* inferred_type, bool by_reference, CodeGen_Result left_result, CodeGen_Result right_result)
	{
		BinaryExpression* as_bin_expr = (BinaryExpression*)expression;

		auto left_type_size = TypeSystem_Get_Type_Size(Data.type_system, left_result.expression_type);
		auto right_type_size = TypeSystem_Get_Type_Size(Data.type_system, left_result.expression_type);

		if (left_type_size > 8) {
			left_result = Expression_CodeGen(as_bin_expr->Left, scope_id, proc, inferred_type, true);
		}

		if (right_type_size > 8) {
			right_result = Expression_CodeGen(as_bin_expr->Right, scope_id, proc, inferred_type, true);
		}

		GS_Type* query_param_types_data[2] = {};
		Array<GS_Type*> query_param_types;
		query_param_types.data = query_param_types_data;
		query_param_types.count = 2;
		query_param_types.capacity = 2;

		query_param_types[0] = left_result.expression_type;
		query_param_types[1] = right_result.expression_type;

		GS_Type* query_signature = TypeSystem_Get_Proc_Type(Data.type_system, Data.void_Ty, query_param_types);

		String operator_as_string = Operator_To_String(as_bin_expr->OPerator);
		String operator_name = String_Make(FMT("{}{}", operator_as_string.data, query_signature->type_hash));

		Entity_ID operator_entity_id = Front_End_Data::Get_Entity_ID_By_Name(Data, scope_id, operator_name);

		if (operator_entity_id == Entity_Null) {
			Push_Error_Loc(scope_id, as_bin_expr, FMT("no operator '{}' overload found for types '{}', '{}'", operator_as_string.data, Print_Type(left_result.expression_type), Print_Type(right_result.expression_type)));
			return {};
		}

		Entity& operator_entity = Data.entity_storage[operator_entity_id];

		Entity& operator_function = Data.entity_storage[operator_entity.operator_overload.function_entity_id];

		auto return_type_size = TypeSystem_Get_Type_Size(Data.type_system, operator_function.func.signature->proc.return_type);

		Il_Argument arguments_data[3] = {};
		Array<Il_Argument> arguments;
		arguments.data = arguments_data;
		arguments.capacity = 3;

		CodeGen_Result result;

		GS_Type* callee_signature_data[3] = {};
		Array<GS_Type*> callee_signature_params;
		callee_signature_params.data = callee_signature_data;
		callee_signature_params.capacity = 3;

		GS_Type* callee_signature_return_type = operator_function.func.signature->proc.return_type;

		if (return_type_size > 8) {
			result.code_node_id = Il_Insert_Alloca(proc, operator_function.func.signature->proc.return_type);
			Array_Add(callee_signature_params, TypeSystem_Get_Pointer_Type(Data.type_system, operator_function.func.signature->proc.return_type, 1));
			Array_Add(arguments, Il_Argument{ result.code_node_id, (Type_IDX)TypeSystem_Get_Type_Index(Data.type_system, operator_function.func.signature->proc.return_type) });
			callee_signature_return_type = Data.void_Ty;
		}

		Array_Add(arguments, { left_result.code_node_id });
		Array_Add(arguments, { right_result.code_node_id });

		Array_Add(callee_signature_params, { left_result.expression_type });
		Array_Add(callee_signature_params, { right_result.expression_type });

		GS_Type* call_signature = TypeSystem_Get_Proc_Type(Data.type_system, callee_signature_return_type, callee_signature_params);

		result.ok = true;
		result.expression_type = operator_function.func.signature->proc.return_type;

		Il_IDX call_node_idx = Il_Insert_Call(proc, call_signature, arguments, operator_function.func.proc_idx);

		if (return_type_size <= 8) {
			result.code_node_id = call_node_idx;
		}
		else {
			if (!by_reference)
				result.code_node_id = Il_Insert_Load(proc, result.expression_type, result.code_node_id);
		}

		return result;
	}

	CodeGen_Result Front_End::BinaryExpression_CodeGen(Expression* expression, Entity_ID scope_id, Il_Proc& proc, GS_Type* inferred_type, bool by_reference, bool is_condition)
	{
		BinaryExpression* as_bin_expr = (BinaryExpression*)expression;

		bool op_assignment =
			as_bin_expr->OPerator == Operator::AddAssign || as_bin_expr->OPerator == Operator::SubAssign ||
			as_bin_expr->OPerator == Operator::MulAssign || as_bin_expr->OPerator == Operator::DivAssign;

		bool assignment = as_bin_expr->OPerator == Operator::Assign || op_assignment;

		bool equality_compare = as_bin_expr->OPerator == Operator::Equal || as_bin_expr->OPerator == Operator::NotEqual;
		bool logical_compare = as_bin_expr->OPerator == Operator::Or || as_bin_expr->OPerator == Operator::And;
		bool bit_wise = as_bin_expr->OPerator == Operator::BitOr || as_bin_expr->OPerator == Operator::BitAnd;
		bool is_compare = as_bin_expr->OPerator == Operator::GreaterThan || as_bin_expr->OPerator == Operator::LesserThan;

		CodeGen_Result left_result = Expression_CodeGen(as_bin_expr->Left, scope_id, proc, is_compare ? nullptr : inferred_type, assignment, is_condition);
		if (!left_result) return {};
		CodeGen_Result right_result = Expression_CodeGen(as_bin_expr->Right, scope_id, proc, left_result.expression_type, false, is_condition);
		if (!right_result) return {};

		auto left_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, left_result.expression_type);
		auto right_type_flags = TypeSystem_Get_Type_Flags(Data.type_system, right_result.expression_type);

		if (!assignment) {
			bool left_overloadable = !(left_type_flags & TN_Base_Type) && !(left_type_flags & TN_Pointer_Type) && !(left_type_flags & TN_Enum_Type);
			bool right_overloadable = !(right_type_flags & TN_Base_Type) && !(right_type_flags & TN_Pointer_Type) && !(left_type_flags & TN_Enum_Type);
			if (left_overloadable || right_overloadable) {
				return BinaryExpression_TryOverload(expression, scope_id, proc, inferred_type, by_reference, left_result, right_result);
			}
		}

		if (assignment) {
			if (left_result.constant) {
				Push_Error_Loc(scope_id, as_bin_expr->Left, "trying to modify a constant");
				return {};
			}

			if (!left_result.lvalue) {
				Push_Error_Loc(scope_id, as_bin_expr->Left, "not an lvalue");
				return {};
			}
		}

		if (left_result.expression_type != right_result.expression_type) {
			Push_Error_Loc(scope_id, as_bin_expr, FMT("type mismatch: '{}', '{}'", Print_Type(left_result.expression_type), Print_Type(right_result.expression_type)));
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

		GS_Type* bin_expr_result_type = left_result.expression_type;

		auto type_flags = TypeSystem_Get_Type_Flags(Data.type_system, bin_expr_result_type);

		if (!logical_compare && !equality_compare) {
			if (!(type_flags & TN_Numeric_Type) && !(type_flags & TN_Enum_Type) && !assignment) {
				Push_Error_Loc(scope_id, as_bin_expr, "types are not numeric and no overload was found");
				return {};
			}
		}
		else {
			if (bin_expr_result_type != Data.bool_Ty && !equality_compare && !bit_wise) {
				Push_Error_Loc(scope_id, as_bin_expr, "types are not bool");
				return {};
			}
		}

		Const_Union result_value = { 0 };

		Il_IDX code_node_id = -1;

		Il_Node_Type math_op_type = (Il_Node_Type)-1;
		Il_Cmp_Type cmp_type = (Il_Cmp_Type)-1;
		bool compare = false;

		switch (as_bin_expr->OPerator)
		{
		case Operator::Add:
			math_op_type = Il_Add;
			break;
		case Operator::Subtract:
			math_op_type = Il_Sub;
			break;
		case Operator::Multiply:
			math_op_type = Il_Mul;
			break;
		case Operator::Divide:
			math_op_type = Il_Div;
			break;
		case Operator::BitAnd:
			math_op_type = Il_Bit_And;
			break;
		case Operator::BitOr:
			math_op_type = Il_Bit_Or;
			break;
		case Operator::GreaterThan:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_Greater;
			compare = true;
			break;
		case Operator::LesserThan:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_Lesser;
			compare = true;
			break;
		case Operator::GreaterThanEq:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_GreaterEqual;
			compare = true;
			break;
		case Operator::LesserThanEq:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_LesserEqual;
			compare = true;
			break;
		case Operator::Equal:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_Equal;
			compare = true;
			break;
		case Operator::NotEqual:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_NotEqual;
			compare = true;
			break;
		case Operator::And:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_And;
			compare = true;
			break;
		case Operator::Or:
			math_op_type = Il_Value_Cmp;
			cmp_type = Il_Cmp_Or;
			compare = true;
			break;

		case Operator::Assign:
		{
		}
		break;
		case Operator::AddAssign:
			math_op_type = Il_Add;
			break;
		case Operator::SubAssign:
			math_op_type = Il_Sub;
			break;
		case Operator::MulAssign:
			math_op_type = Il_Mul;
			break;
		case Operator::DivAssign:
			math_op_type = Il_Div;
			break;
		default:
			ASSERT(nullptr, "unknown operator");
			break;
		}

		if (!compare) {

			if (op_assignment) {

				CodeGen_Result byvalue_left_result = Expression_CodeGen(as_bin_expr->Left, scope_id, proc, nullptr);
				ASSERT(byvalue_left_result);
				ASSERT(byvalue_left_result.expression_type == bin_expr_result_type);

				right_result.code_node_id = Il_Insert_Math_Op(proc, bin_expr_result_type, math_op_type, byvalue_left_result.code_node_id, right_result.code_node_id);
			}
			else if (!assignment) {
				code_node_id = Il_Insert_Math_Op(proc, bin_expr_result_type, math_op_type, left_result.code_node_id, right_result.code_node_id);

				if (inferred_type == Data.bool_Ty) {
					Const_Union zero = {};
					code_node_id = Il_Insert_Compare(proc, Il_Value_Cmp, Il_Cmp_NotEqual, bin_expr_result_type, code_node_id, Il_Insert_Constant(proc, zero, bin_expr_result_type));
					bin_expr_result_type = Data.bool_Ty;
				}
			}
		}
		else {
			code_node_id = Il_Insert_Compare(proc, math_op_type, cmp_type, bin_expr_result_type, left_result.code_node_id, right_result.code_node_id);
			bin_expr_result_type = Data.bool_Ty;
		}

		if (assignment) {
			code_node_id = Il_Insert_Store(proc, bin_expr_result_type, left_result.code_node_id, right_result.code_node_id);

			if (left_result.immutable) {
				Push_Error_Loc(scope_id, as_bin_expr->Left, "value is immutable");
				return {};
			}
		}

		CodeGen_Result result;
		result.expression_type = bin_expr_result_type;
		result.code_node_id = code_node_id;
		result.ok = true;
		return result;
	}

	std::string Front_End::Print_Type(GS_Type* type)
	{
		return std::string(TypeSystem_Print_Type(Data.type_system, type).data);
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

	Entity_ID Front_End_Data::Get_Func_Parent(Front_End_Data& data, Entity_ID entity_id)
	{
		Entity& entity = data.entity_storage[entity_id];

		if (entity.entity_type == Entity_Type::Function) {
			return entity_id;
		}

		if (entity.parent == Entity_Null) {
			return entity_id;
		}
		else {

			Entity& parent = data.entity_storage[entity.parent];

			if (parent.entity_type == Entity_Type::Function) {
				return entity.parent;
			}

			return Get_Func_Parent(data, entity.parent);
		}
	}

	Entity_ID Front_End_Data::Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID scope_id, String name)
	{
		//#define MESURE_SEARCH_TIME

#ifdef MESURE_SEARCH_TIME
		auto search_start = std::chrono::high_resolution_clock::now();
#endif

		Entity_ID visited_load = Get_File_Scope_Parent(data, scope_id);
		ASSERT(data.entity_storage[visited_load].entity_type == Entity_Type::File_Scope);

		auto search_result = Get_Entity_ID_By_Name(data, scope_id, name, visited_load, Entity_Null);

#ifdef MESURE_SEARCH_TIME
		auto search_end = std::chrono::high_resolution_clock::now();
		data.entity_search_time += std::chrono::duration_cast<std::chrono::nanoseconds>(search_end - search_start).count();
#endif
		return search_result;
	}

	Entity_ID Front_End_Data::Get_Entity_ID_By_Name(Front_End_Data& data, Entity_ID scope_id, String name, Entity_ID visited_load, Entity_ID visited_parent)
	{
		ASSERT(scope_id != Entity_Null);

		Entity& scope_entity = data.entity_storage[scope_id];

		auto it = scope_entity.children_lookup.find({ name.data,name.count });

		if (it != scope_entity.children_lookup.end()) {
			return it->second;
		}

		if (visited_parent == data.global_scope_entity) return Entity_Null;

		Entity& global_scope = data.entity_storage[data.global_scope_entity];

		for (size_t i = 0; i < global_scope.children.count; i++)
		{
#ifdef MESURE_SEARCH_TIME
			data.entity_search_loop_count += 1;
#endif

			Entity_ID scope_child_id = global_scope.children[i];
			ASSERT(scope_child_id != Entity_Null);

			Entity& child_entity = data.entity_storage[scope_child_id];

			//if (strcmp(child_entity.semantic_name.data, name.data) == 0) {
			//	return scope_child_id;
			//}

			if (child_entity.entity_type == Entity_Type::File_Load) {

				Entity_ID file_load_file_scope = data.File_ID_To_Scope.at(child_entity.file_load.loaded_file_id);

				if (visited_load != file_load_file_scope) {

					Entity_ID found_in_load = Get_Entity_ID_By_Name(data, file_load_file_scope, name, visited_load, data.global_scope_entity);

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

	CodeGen_Result::operator bool()
	{
		return ok;
	}
}