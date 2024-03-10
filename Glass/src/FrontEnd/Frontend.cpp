#include "pch.h"

#include "FrontEnd/Frontend.h"
#include "FrontEnd/Lexer.h"
#include "BackEnd/MC_Gen.h"
#include "BackEnd/LLVM_Converter.h"
#include "microsoft_craziness.h"

#define FMT(...) String_Make(fmt::format(__VA_ARGS__))

#define DBG(x) x

namespace Glass
{
	void frontend_push_error(Front_End& f, String error)
	{
		Array_Add(f.messages, Front_End_Message{ Message_Error, error });
	}

	void frontend_push_error(Front_End& f, String error, String path, Source_Loc location)
	{
		Array_Add(f.messages, Front_End_Message{ Message_Error, error });
	}

	void frontend_push_error(Front_End& f, Tk& token, String file_path, String error)
	{
		Source_Loc loc;
		loc.line = token.line + 1;
		loc.column = token.start;

		if (token.type == Tk_NumericLiteral || token.type == Tk_HexLiteral)
		{

		}

		auto error_message = FMT("{}:{}:{}: {}", file_path, loc.line, loc.column, error);
		Array_Add(f.messages, Front_End_Message{ Message_Error, error_message });
	}

	void frontend_push_error(Front_End& f, Tk tk, int file_id, String error)
	{
		frontend_push_error(f, tk, f.files[file_id].path->str, error);
	}

	void frontend_push_error(Front_End& f, Ast_Node* stmt, int file_id, String error)
	{
		frontend_push_error(f, stmt->token, f.files[file_id].path->str, error);
	}

	void push_error_scope(Front_End& f, Ast_Node* stmt, int scope_id, String error)
	{
		int file_id = f.scopes[find_filescope_parent(f, scope_id)].file_id;
		frontend_push_error(f, stmt, file_id, error);
	}

	void push_error_scope(Front_End& f, Tk tk, int scope_id, String error)
	{
		int file_id = f.scopes[find_filescope_parent(f, scope_id)].file_id;
		frontend_push_error(f, tk, file_id, error);
	}

	int find_filescope_parent(Front_End& f, int scope_id)
	{
		if (!scope_id) return 0;

		if (f.scopes[scope_id].type == Scope_File)
		{
			return scope_id;
		}

		return find_filescope_parent(f, f.scopes[scope_id].parent);
	}

	Entity make_entity(Entity_Kind type, String_Atom* name, Ast_Node* syntax /*= nullptr*/)
	{
		Entity entity = {};
		entity.kind = type;
		entity.name = name;
		entity.syntax = syntax;

		return entity;
	}

	int insert_entity(Front_End& f, Entity entity, int scope_id)
	{
		int entity_id = f.entities.count;

		entity.scope_id = scope_id;

		int file_scope = find_filescope_parent(f, scope_id);

		if (file_scope)
		{
			entity.file_id = get_scope(f, file_scope).file_id;
		}

		if (scope_id != 0)
		{
			Scope& parent_scope = f.scopes[scope_id];
			Array_Add(parent_scope.entities, entity_id);
			parent_scope.name_to_entity[entity.name] = entity_id;
		}

		Array_Add(f.entities, entity);
		return entity_id;
	}

	int find_entity(Front_End& f, String_Atom* name, int scope_id)
	{
		int file_scope_entity_id = find_filescope_parent(f, scope_id);
		return find_entity(f, name, scope_id, file_scope_entity_id, false);
	}

	int find_entity(Front_End& f, String_Atom* name, int scope_id, int ignore, bool ignore_global)
	{
		Scope& scope = f.scopes[scope_id];

		auto it = scope.name_to_entity.find(name);

		if (it != scope.name_to_entity.end())
			return it->second;

		if (scope.parent)
		{
			return find_entity(f, name, scope.parent, ignore, ignore_global);
		}

		if (!ignore_global)
		{
			for (size_t i = 0; i < scope.entities.count; i++)
			{
				Entity& entity = get_entity(f, scope.entities[i]);
				if (entity.kind == Entity_Load)
					if (entity.load.file_scope_id != ignore)
					{
						int result = find_entity(f, name, entity.load.file_scope_id, ignore, true);

						if (result)
						{
							return result;
						}
					}
			}
		}

		return 0;
	}

	int insert_scope(Front_End& f, Scope_Type type, int parent, int file_id, int entity_id, Ast_Node* syntax)
	{
		int scope_id = f.scopes.count;

		Scope* scope = Array_Add(f.scopes, {});
		scope->type = type;
		scope->file_id = file_id;
		scope->syntax = syntax;
		scope->parent = parent;

		Array_Add(f.scopes[parent].children, scope_id);

		return scope_id;
	}

	Scope& get_scope(Front_End& f, int scope_id)
	{
		ASSERT(scope_id);
		return f.scopes[scope_id];
	}

	Entity& get_entity(Front_End& f, int entity_id)
	{
		ASSERT(entity_id);
		return f.entities[entity_id];
	}

	void flatten_statement(Ast_Node* stmt, Array<Ast_Node*>)
	{

	}

	bool load_pass(Front_End& f, int scope_id, int file_id, Ast_Node* stmt)
	{
		switch (stmt->type)
		{
		case Ast_Directive_Load:
		{
			fs_path file_path = fs_path(stmt->token.name->str.data);
			fs_path search_path = fs_path(f.files[file_id].path->str.data);

			file_path = normalizePath(file_path);

			if (!std::filesystem::exists(file_path)) {

				fs_path relative = std::filesystem::relative(file_path, search_path);

				if (!std::filesystem::exists(relative))
				{
					push_error_scope(f, stmt, scope_id, FMT("file not found!: {}", file_path.string()));
					return false;
				}

				file_path = relative;
			}

			if (!frontend_do_load(f, file_path, search_path)) {
				return false;
			}
		}
		}

		return true;
	}

	int frontend_do_load(Front_End& f, fs_path file_path, fs_path search_path)
	{
		file_path = normalizePath(file_path);

		if (!std::filesystem::exists(file_path)) {

			fs_path relative = std::filesystem::relative(file_path, search_path);

			if (!std::filesystem::exists(relative))
			{
				frontend_push_error(f, FMT("file {} not found!", file_path.string()));
				return 0;
			}

			file_path = relative;
		}

		fs_path file_path_abs = std::filesystem::absolute(file_path);

		String_Atom* file_path_atom = get_atom(String_Make(file_path.string()));
		String_Atom* file_path_abs_atom = get_atom(String_Make(file_path_abs.string()));

		auto it = f.filename_to_id.find(file_path_abs_atom);
		if (it != f.filename_to_id.end()) {
			return it->second;
		}

		int file_id = f.files.count;
		Comp_File* file = Array_Add(f.files, {});

		file->path = file_path_atom;
		file->path_abs = file_path_abs_atom;

		String file_source = read_entire_file(file_path);
		String file_path_str = String_Make(file_path.string());

		bool success;
		file->syntax = parse_string(f, file_path_str, file_source, success);

		if (!success)
		{
			return false;
		}

		f.filename_to_id[file_path_abs_atom] = file_id;

		int scope_id = f.scopes.count;

		file->scope_id = scope_id;

		Scope* scope = Array_Add(f.scopes, {});
		scope->type = Scope_File;
		scope->file_id = file_id;
		scope->syntax = file->syntax;
		scope->parent = 1;

		Array_Add(f.scopes[scope->parent].children, scope_id);

		f.file_to_scope[file_id] = scope_id;

		Entity load_entity = make_entity(Entity_Load, get_atom(""), nullptr);
		load_entity.load.file_id = file_id;
		load_entity.load.file_scope_id = scope_id;
		insert_entity(f, load_entity, 1);

		for (size_t i = 0; i < file->syntax->scope.stmts.count; i++)
		{
			if (!load_pass(f, scope_id, file_id, file->syntax->scope.stmts[i])) {
				return false;
			}
		}

		success = true;

		return success;
	}

	GS_Type* insert_base_type_entity(Front_End& f, const char* name, u64 flags, u64 size, u64 alignment)
	{
		Entity tn_entity = make_entity(Entity_TypeName, get_atom(name), nullptr);
		Type_Name tn;
		tn.alignment = alignment;
		tn.size = size;
		tn.flags = flags;
		tn.name = String_Make(name);

		int type_name_id = insert_typename(tn);

		tn_entity.tn.type_name_id = type_name_id;
		tn_entity.type = f.Type_Ty;
		tn_entity.flags = Flag_Complete;

		insert_entity(f, tn_entity, 1);

		return get_type(type_name_id);
	}

	GS_Type* insert_base_type_alias_entity(Front_End& f, const char* name, int type_name_id)
	{
		Entity tn_entity = make_entity(Entity_TypeName, get_atom(name));

		tn_entity.tn.type_name_id = type_name_id;
		tn_entity.type = f.Type_Ty;
		tn_entity.flags = Flag_Complete;

		insert_entity(f, tn_entity, 1);

		return get_type(type_name_id);
	}

	void frontend_init(Front_End& f)
	{
		parser_init();
		init_typesystem();

		f.scopes = Array_Reserved<Scope>(1000 * 100);
		f.entities = Array_Reserved<Entity>(1000 * 100);

		Array_Add(f.files, {});
		Array_Add(f.scopes, {});
		Array_Add(f.entities, {});

		Scope* global_scope = Array_Add(f.scopes, {});
		global_scope->type = Scope_Global;
		global_scope->file_id = 0;
		global_scope->parent = 0;
		global_scope->syntax = nullptr;

		f.keyword_Array = get_atom("Array");
		f.keyword_string = get_atom("string");

		f.Type_Ty = insert_base_type_entity(f, "Type", TN_Base_Type, 8, 8);

		f.bool_Ty = insert_base_type_entity(f, "bool", TN_Base_Type, 1, 1);

		f.void_Ty = insert_base_type_entity(f, "void", TN_Base_Type, 0, 0);

		f.i8_Ty = insert_base_type_entity(f, "i8", TN_Numeric_Type | TN_Base_Type, 1, 1);
		f.u8_Ty = insert_base_type_entity(f, "u8", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 1, 1);
		f.i16_Ty = insert_base_type_entity(f, "i16", TN_Numeric_Type | TN_Base_Type, 2, 2);
		f.u16_Ty = insert_base_type_entity(f, "u16", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 2, 2);
		f.i32_Ty = insert_base_type_entity(f, "i32", TN_Numeric_Type | TN_Base_Type, 4, 4);
		f.u32_Ty = insert_base_type_entity(f, "u32", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 4, 4);
		f.i64_Ty = insert_base_type_entity(f, "i64", TN_Numeric_Type | TN_Base_Type, 8, 8);
		f.u64_Ty = insert_base_type_entity(f, "u64", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 8, 8);

		f.f32_Ty = insert_base_type_entity(f, "f32", TN_Numeric_Type | TN_Base_Type | TN_Float_Type, 4, 4);
		f.f64_Ty = insert_base_type_entity(f, "f64", TN_Numeric_Type | TN_Base_Type | TN_Float_Type, 8, 8);

		f.int_Ty = insert_base_type_alias_entity(f, "int", f.i32_Ty->basic.type_name_id);
		f.float_Ty = insert_base_type_alias_entity(f, "float", f.f32_Ty->basic.type_name_id);

		get_ts().void_Ty = f.void_Ty;
		get_ts().void_ptr_Ty = get_pointer_type(f.void_Ty, 1);

		get_ts().u8_Ty = f.u8_Ty;
		get_ts().u16_Ty = f.u16_Ty;
		get_ts().u32_Ty = f.u32_Ty;
		get_ts().u64_Ty = f.u64_Ty;

		get_ts().i8_Ty = f.i8_Ty;
		get_ts().i16_Ty = f.i16_Ty;
		get_ts().i32_Ty = f.i32_Ty;
		get_ts().i64_Ty = f.i64_Ty;

		get_ts().f32_Ty = f.f32_Ty;
		get_ts().f64_Ty = f.f64_Ty;

		get_ts().float_Ty = f.float_Ty;
		get_ts().int_Ty = f.int_Ty;

		Il_Program_Init(f.program);

		f.c_str_Ty = f.u8_Ty->get_pointer();
	}

	void frontend_generate_output(Front_End& f)
	{
		GS_PROFILE_FUNCTION();

		bool llvm = f.opts.Backend == Backend_Option::LLVM_Backend;

		std::string linked_objects;

		if (llvm)
		{
			GS_PROFILE_SCOPE("run llvm converter");

			LLVM_Converter_Spec lc_spec;
			lc_spec.output_path = String_Make(".bin/llvm.obj");

			LLVM_Converter llvm_converter = LLVM_Converter_Make(lc_spec, &f.program);
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

			MC_Gen_Spec mc_gen_spec = {};
			mc_gen_spec.output_path = String_Make(".bin/il.obj");

			MC_Gen mc_generator = MC_Gen_Make(mc_gen_spec, &f.program);
			MC_Gen_Run(mc_generator);

			linked_objects = "./.bin/il.obj";

		}

		bool disassemble = f.opts.Dissassemble;

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

		std::string linker_options = "/nologo /ignore:4210 /NODEFAULTLIB /IMPLIB:C /DEBUG:FULL /SUBSYSTEM:CONSOLE /INCREMENTAL:NO vcruntime.lib kernel32.lib ucrt.lib libcmt.lib";

		std::string program_libraries;

		for (auto& path : f.added_library_paths)
		{
			program_libraries.append(path->str.data);
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

		//GS_CORE_INFO("running linker: {}", linker_command);

		{
			GS_PROFILE_SCOPE("run msvc-linker");
			system(linker_command.c_str());
		}

		//free_resources(&find_result);
	}

	void frontend_compile(Front_End& f)
	{
		frontend_init(f);

		if (f.opts.Files.size() != 1) {
			frontend_push_error(f, String_Make("expected 1 file as argument"));
			return;
		}

		fs_path base_file_path = "Library/Base.glass";

		bool result = frontend_do_load(f, base_file_path, std::filesystem::current_path());

		if (!result)
			return;

		fs_path first_file_path = f.opts.Files[0];

		result = frontend_do_load(f, first_file_path, std::filesystem::current_path());

		if (!result)
			return;

		{
			std::string printed_program = il_print_program(f.program);
			std::ofstream print_out(".bin/program.il");
			print_out << printed_program;
		}

		get_ts().Array_Ty = f.Array_Ty;

		if (!result)
			return;

		frontend_generate_output(f);

		if (!result)
			return;
	}
}
