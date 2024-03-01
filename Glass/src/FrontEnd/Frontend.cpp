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

		auto error_message = FMT("{}:{}:{}: {}", file_path, loc.line, loc.column, error);
		Array_Add(f.messages, Front_End_Message{ Message_Error, error_message });
	}

	void frontend_push_error(Front_End& f, Ast_Node* stmt, int file_id, String error)
	{
		frontend_push_error(f, stmt->token, f.files[file_id].path->str, error);
	}

	int find_filescope_parent(Front_End& f, int scope_id)
	{
		ASSERT(scope_id);

		if (f.scopes[scope_id].type == Scope_File)
		{
			return scope_id;
		}

		return find_filescope_parent(f, f.scopes[scope_id].parent);
	}

	void push_error_scope(Front_End& f, Ast_Node* stmt, int scope_id, String error)
	{
		int file_id = f.scopes[find_filescope_parent(f, scope_id)].file_id;
		frontend_push_error(f, stmt, file_id, error);
	}

	int insert_entity(Front_End& f, Entity entity, int scope_id = 0)
	{
		int entity_id = f.entities.count;

		if (scope_id != 0)
		{
			Scope& parent_scope = f.scopes[scope_id];
			Array_Add(parent_scope.entities, entity_id);
			parent_scope.name_to_entity[entity.name] = entity_id;
		}

		Array_Add(f.entities, entity);
		return entity_id;
	}

	int find_entity(Front_End& f, String_Atom* name, int scope_id = 0)
	{
		Scope& scope = f.scopes[scope_id];

		auto it = scope.name_to_entity.find(name);

		if (it != scope.name_to_entity.end())
			return it->second;

		if (scope.parent)
		{
			return find_entity(f, name, scope.parent);
		}

		return 0;
	}

	Entity make_entity(Entity_Kind type, String_Atom* name, Ast_Node* syntax = nullptr, int scope_id = 0, int file_id = 0)
	{
		Entity entity = {};
		entity.kind = type;
		entity.name = name;
		entity.syntax = syntax;
		entity.file_id = file_id;
		entity.scope_id = scope_id;

		return entity;
	}

	bool frontend_decl_stmt(Front_End& f, Scope& scope, int scope_id, int file_id, Ast_Node* stmt)
	{
		switch (stmt->type)
		{
		case Ast_Variable:
		{
			String_Atom* name_atom = stmt->token.name;
			int previous_decl = find_entity(f, name_atom, scope_id);

			if (previous_decl)
			{
				frontend_push_error(f, stmt, file_id, FMT("{} already declared!", name_atom->str));
				return false;
			}

			Entity_Kind entity_type = Entity_Variable;

			if (stmt->var.is_constant)
			{
				entity_type = Entity_Constant;
			}

			Entity var = make_entity(entity_type, name_atom, stmt, scope_id, file_id);
			insert_entity(f, var, scope_id);
		}
		break;
		}

		return true;
	}

	bool frontend_decl_file(Front_End& f, Scope& file_scope, int scope_id)
	{
		Ast_Node* file_node = file_scope.syntax;

		for (size_t i = 0; i < file_node->scope.stmts.count; i++)
		{
			auto stmt = file_node->scope.stmts[i];

			bool success = frontend_decl_stmt(f, file_scope, scope_id, file_scope.file_id, stmt);

			if (!success)
				return false;
		}

		return true;
	}

	void insert_dep(Front_End& f, int dep, Entity_Deps& deps)
	{
		deps.emplace(dep);
	}

	bool frontend_expr_deps(Front_End& f, Ast_Node* expr, int file_id, int scope_id, int entity_id, Entity_Deps& deps)
	{
		switch (expr->type)
		{
		case Ast_Ident:
		{
			int identified = find_entity(f, expr->token.name, scope_id);

			if (!identified)
			{
				frontend_push_error(f, expr, file_id, FMT("undeclared identifier: {}", expr->token.name->str));
				return false;
			}

			insert_dep(f, identified, deps);
		}
		break;
		case Ast_Binary:
		{
			bool success = frontend_expr_deps(f, expr->bin.lhs, file_id, scope_id, entity_id, deps);

			if (!success) {
				return false;
			}

			success = frontend_expr_deps(f, expr->bin.rhs, file_id, scope_id, entity_id, deps);

			if (!success) {

				return false;
			}
		}
		break;
		}

		return true;
	}

	bool frontend_entity_deps(Front_End& f, Scope& scope, int scope_id, int file_id, Entity& entity, int entity_id)
	{
		auto stmt = entity.syntax;

		switch (entity.kind)
		{
		case Entity_Constant:
		case Entity_Variable:
		{
			bool success = frontend_expr_deps(f, stmt->var.assignment, file_id, scope_id, entity_id, entity.deps);

			if (!success)
				return false;
		}
		break;
		}


		bool circular_deps = false;

		if (entity.deps.find(entity_id) != entity.deps.end())
		{
			circular_deps = true;
		}

		if (circular_deps)
		{
			frontend_push_error(f, stmt, file_id, FMT("{} has circular dependencies", entity.name->str));
			return false;
		}

		DBG
		(
			GS_CORE_TRACE("entity: {} depends on", entity.name->str);
		for (int dep : entity.deps)
		{
			GS_CORE_TRACE("		{}", f.entities[dep].name->str);
		}
		)

			return true;
	}

	bool frontend_file_deps(Front_End& f, Scope& file_scope, int scope_id)
	{
		for (size_t i = 0; i < file_scope.entities.count; i++)
		{
			int entity_id = file_scope.entities[i];
			frontend_entity_deps(f, file_scope, scope_id, file_scope.file_id, f.entities[entity_id], entity_id);
		}

		return true;
	}

	struct CodeGen_Result
	{
		bool success = false;
		bool is_constant;
		bool in_complete_lit;
		bool is_float_lit;
		GS_Type* type;
		Const_Union	 value;

		operator bool()
		{
			return success;
		}
	};

	CodeGen_Result make_result(bool is_constant, GS_Type* type, Const_Union value = {}, bool in_complete_lit = false, bool lit_float = false)
	{
		CodeGen_Result result;
		result.success = true;
		result.is_constant = is_constant;
		result.type = type;
		result.value = value;
		result.in_complete_lit = in_complete_lit;
		result.is_float_lit = lit_float;
		return result;
	}

	CodeGen_Result expr_resolve(Front_End& f, int scope_id, Ast_Node* expr, GS_Type* inferred_type = nullptr, bool is_top_level = false, bool const_eval = false);

	GS_Type* resolve_type(Front_End& f, int scope_id, Ast_Node* expr)
	{
		auto result = expr_resolve(f, scope_id, expr, f.Type_Ty, false, true);

		if (!result)
			return nullptr;

		if (result.type != f.Type_Ty)
		{
			push_error_scope(f, expr, scope_id, FMT("expected type"));
			return {};
		}

		return get_type_at(result.value.s4);
	}

	CodeGen_Result expr_resolve(Front_End& f, int scope_id, Ast_Node* expr, GS_Type* inferred_type, bool is_top_level, bool const_eval)
	{
		switch (expr->type)
		{
		case Ast_Ident:
		{
			int identified = find_entity(f, expr->token.name, scope_id);

			if (!identified)
			{
				push_error_scope(f, expr, scope_id, FMT("undeclared identifier: {}", expr->token.name->str));
				return {};
			}

			Entity& identified_entity = f.entities[identified];

			Const_Union value;
			GS_Type* type;

			if (identified_entity.kind == Entity_Constant)
			{
				value = identified_entity.cnst.value;
				type = identified_entity.type;
			}
			else if (identified_entity.kind == Entity_TypeName)
			{
				value.s8 = identified_entity.tn.type_name_id;
				type = f.Type_Ty;
			}
			else
			{
				push_error_scope(f, expr, scope_id, FMT("not a constant: {}", expr->token.name->str));
				return {};
			}

			return make_result(true, type, value);
		}
		break;
		case Ast_Numeric:
		{
			Const_Union value = {};
			GS_Type* type = nullptr;

			if (expr->num.is_float)
			{
				type = f.float_Ty;
				value.f64 = expr->num.floating;
			}
			else
			{
				type = f.int_Ty;
				value.s8 = expr->num.integer;
			}

			if (inferred_type)
			{
				u64 inferred_flags = get_type_flags(inferred_type);

				bool is_numeric = inferred_flags & TN_Numeric_Type;
				bool is_float = inferred_flags & TN_Float_Type;

				if (is_numeric)
				{
					if (expr->num.is_float)
					{
						if (is_float)
						{
							type = inferred_type;
						}
					}
					else
					{
						type = inferred_type;

						if (is_float)
						{
							value.f64 = (double)value.s8;
						}
					}
				}
			}

			return make_result(true, type, value, true, expr->num.is_float);
		}
		break;
		case Ast_Binary:
		{
			Const_Union value = {};
			GS_Type* type = nullptr;

			Tk_Operator op = tk_to_operator(expr->token.type);

			bool assign_op = op == Op_Assign || op == Op_AddAssign || op == Op_SubAssign || op == Op_MulAssign || op == Op_DivAssign;

			if (assign_op && !is_top_level)
			{
				push_error_scope(f, expr, scope_id, FMT("invalid expression level for: '{}'", operator_to_str(op)));
				return {};
			}

			auto lhs_result = expr_resolve(f, scope_id, expr->bin.lhs, nullptr, false, const_eval);

			if (!lhs_result)
				return {};

			auto rhs_result = expr_resolve(f, scope_id, expr->bin.rhs, nullptr, false, const_eval);

			if (!rhs_result)
				return {};

			GS_Type* most_complete_type = nullptr;

			if (rhs_result.in_complete_lit)
			{
				if (rhs_result.is_float_lit)
				{
					most_complete_type = rhs_result.type;
				}
			}
			else
			{
				most_complete_type = rhs_result.type;
			}

			if (lhs_result.in_complete_lit)
			{
				if (lhs_result.is_float_lit)
				{
					most_complete_type = lhs_result.type;
				}
			}
			else
			{
				most_complete_type = lhs_result.type;
			}

			if (!most_complete_type)
			{
				most_complete_type = inferred_type;
			}

			if (most_complete_type)
			{
				u64 inferred_type_flags = get_type_flags(most_complete_type);
				bool is_float = inferred_type_flags & TN_Float_Type;

				if (lhs_result.in_complete_lit)
				{
					if ((!lhs_result.is_float_lit) || lhs_result.is_float_lit && is_float) {
						lhs_result.type = most_complete_type;

						if (is_float && !lhs_result.is_float_lit)
						{
							lhs_result.value.f64 = (double)lhs_result.value.s8;
						}
					}
				}

				if (rhs_result.in_complete_lit)
				{
					if ((!rhs_result.is_float_lit) || rhs_result.is_float_lit && is_float) {
						rhs_result.type = most_complete_type;
					}

					if (is_float && !rhs_result.is_float_lit)
					{
						rhs_result.value.f64 = (double)rhs_result.value.s8;
					}
				}
			}

			if (lhs_result.type != rhs_result.type)
			{
				push_error_scope(f, expr, scope_id, FMT("type mismatch: '{}' {} '{}'", print_type(lhs_result.type), operator_to_str(op), print_type(rhs_result.type)));
				return {};
			}

			type = lhs_result.type;
			auto type_flags = get_type_flags(type);

			bool numeric_op = op == Op_Add || op == Op_Sub || op == Op_Mul || op == Op_Div;
			bool comparative_op = op == Op_Eq || op == Op_NotEq;

			bool is_integer = !(type_flags & TN_Float_Type);
			bool is_signed = !(type_flags & TN_Unsigned_Type);
			bool is_numeric = type_flags & TN_Numeric_Type;

			if (numeric_op && !is_numeric)
			{
				push_error_scope(f, expr, scope_id, FMT("expected numeric types in expression: '{}' {} '{}'", print_type(lhs_result.type), operator_to_str(op), print_type(rhs_result.type)));
				return {};
			}

			if (comparative_op && (!is_numeric && f.bool_Ty != type && f.Type_Ty != type))
			{
				push_error_scope(f, expr, scope_id, FMT("expected types in expression to be comparable : '{}' {} '{}'", print_type(lhs_result.type), operator_to_str(op), print_type(rhs_result.type)));
				return {};
			}

			bool fold = lhs_result.in_complete_lit && rhs_result.in_complete_lit;

			if (fold)
			{
				switch (op)
				{
				case Op_Add:
				{
					if (is_integer)
					{
						value.s8 = lhs_result.value.s8 + rhs_result.value.s8;
					}
					else
					{
						value.f64 = lhs_result.value.f64 + rhs_result.value.f64;
					}
				}
				break;
				case Op_Sub:
				{
					if (is_integer)
					{
						value.s8 = lhs_result.value.s8 - rhs_result.value.s8;
					}
					else
					{
						value.f64 = lhs_result.value.f64 - rhs_result.value.f64;
					}
				}
				break;
				case Op_Mul:
					if (is_integer)
					{
						value.s8 = lhs_result.value.s8 * rhs_result.value.s8;
					}
					else
					{
						value.f64 = lhs_result.value.f64 * rhs_result.value.f64;
					}
					break;
				case Op_Div:

					if (is_integer)
					{
						if (is_signed)
							value.s8 = lhs_result.value.s8 / rhs_result.value.s8;
						else
							value.us8 = lhs_result.value.us8 / rhs_result.value.us8;
					}
					else
					{
						value.f64 = lhs_result.value.f64 / rhs_result.value.f64;
					}
					break;
				case Op_Eq:

					if (is_integer)
					{
						value.us1 = lhs_result.value.us8 == rhs_result.value.us8;
					}
					else
					{
						value.us1 = lhs_result.value.f64 == rhs_result.value.f64;
					}
					break;
				case Op_NotEq:
					if (is_integer)
					{
						value.us1 = lhs_result.value.us8 != rhs_result.value.us8;
					}
					else
					{
						value.us1 = lhs_result.value.f64 != rhs_result.value.f64;
					}
					break;
				default:
					break;
				}
			}

			if (comparative_op)
			{
				type = f.bool_Ty;
			}

			if (fold)
			{
				return make_result(true, type, value, true, !is_integer);
			}
			else
			{
				return make_result(true, type, value);
			}
		}
		break;
		}

		return {};
	}

	bool frontend_entity_resolve(Front_End& f, Scope& scope, int scope_id, int file_id, Entity& entity, int entity_id)
	{
		Ast_Node* stmt = entity.syntax;

		switch (entity.kind)
		{
		case Entity_Constant:
		{
			GS_Type* type = nullptr;

			if (stmt->var.type)
			{
				auto type_result = resolve_type(f, scope_id, stmt->var.type);

				if (!type_result)
					return false;

				type = type_result;
			}

			auto result = expr_resolve(f, scope_id, stmt->var.assignment, type, false, true);

			if (!result)
				return false;

			if (type)
			{
				if (type != result.type)
				{
					push_error_scope(f, stmt, scope_id, FMT("assignment type mismatch: {} :: {}", print_type(type), print_type(result.type)));
					return {};
				}
			}

			entity.type = result.type;
			entity.cnst.value = result.value;
			entity.flags |= Flag_Complete;
		}
		break;
		}

		return true;
	}

	bool frontend_file_resolve(Front_End& f, Scope& file_scope, int scope_id)
	{
		bool all_resolved = true;

		for (size_t i = 0; i < file_scope.entities.count; i++)
		{
			int entity_id = file_scope.entities[i];
			Entity& entity = f.entities[entity_id];

			if (entity.flags & Flag_Complete)
				continue;

			bool has_unresolved = false;

			for (int dep : entity.deps)
			{
				if (!(f.entities[dep].flags & Flag_Complete))
				{
					has_unresolved = true;
					break;
				}
			}

			if (has_unresolved)
			{
				all_resolved = false;
			}
			else
			{
				bool success = frontend_entity_resolve(f, file_scope, scope_id, file_scope.file_id, f.entities[entity_id], entity_id);

				if (!success)
					return false;
			}

			if (!all_resolved)
			{
				i = 0;
				all_resolved = true;
			}
		}

		return true;
	}

	bool frontend_loop(Front_End& f)
	{
		Scope& global_scope = f.scopes[1];

		for (size_t i = 0; i < global_scope.children.count; i++)
		{
			int scope_id = global_scope.children[i];
			Scope& scope = f.scopes[scope_id];

			if (scope.type != Scope_File)
				continue;

			bool success = frontend_decl_file(f, scope, scope_id);
			if (!success)
				return false;
		}

		for (size_t i = 0; i < global_scope.children.count; i++)
		{
			int scope_id = global_scope.children[i];
			Scope& scope = f.scopes[scope_id];

			if (scope.type != Scope_File)
				continue;

			bool success = frontend_file_deps(f, scope, scope_id);
			if (!success)
				return false;
		}

		for (size_t i = 0; i < global_scope.children.count; i++)
		{
			int scope_id = global_scope.children[i];
			Scope& scope = f.scopes[scope_id];

			if (scope.type != Scope_File)
				continue;

			bool success = frontend_file_resolve(f, scope, scope_id);

			if (!success)
				return false;
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

		success = true;

		return success;
	}

	GS_Type* insert_base_type_entity(Front_End& f, const char* name, u64 flags, u64 size, u64 alignment)
	{
		Entity tn_entity = make_entity(Entity_TypeName, get_atom(name), nullptr, 1);
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
		Entity tn_entity = make_entity(Entity_TypeName, get_atom(name), nullptr, 1);

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

		f.Type_Ty = insert_base_type_entity(f, "Type", TN_Base_Type, 8, 8);

		f.bool_Ty = insert_base_type_entity(f, "bool", TN_Base_Type, 1, 1);

		f.i32_Ty = insert_base_type_entity(f, "i32", TN_Numeric_Type | TN_Base_Type, 4, 4);
		f.u32_Ty = insert_base_type_entity(f, "u32", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 4, 4);
		f.i64_Ty = insert_base_type_entity(f, "i64", TN_Numeric_Type | TN_Base_Type, 8, 8);
		f.u64_Ty = insert_base_type_entity(f, "u64", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 8, 8);

		f.f32_Ty = insert_base_type_entity(f, "f32", TN_Numeric_Type | TN_Base_Type | TN_Float_Type, 4, 4);
		f.f64_Ty = insert_base_type_entity(f, "f64", TN_Numeric_Type | TN_Base_Type | TN_Float_Type, 8, 8);

		f.int_Ty = insert_base_type_alias_entity(f, "int", f.i32_Ty->basic.type_name_id);
		f.float_Ty = insert_base_type_alias_entity(f, "float", f.f32_Ty->basic.type_name_id);
	}

	void frontend_compile(Front_End& f)
	{
		frontend_init(f);

		if (f.opts.Files.size() != 1) {
			frontend_push_error(f, String_Make("expected 1 file as argument"));
			return;
		}

		fs_path first_file_path = f.opts.Files[0];

		bool result = frontend_do_load(f, first_file_path, std::filesystem::current_path());

		if (!result)
			return;

		result = frontend_loop(f);

		if (!result)
			return;
	}
}
