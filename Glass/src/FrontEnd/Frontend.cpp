#include "pch.h"

#include "FrontEnd/Frontend.h"
#include "FrontEnd/Lexer.h"
#include "BackEnd/MC_Gen.h"
#include "BackEnd/LLVM_Converter.h"
#include "microsoft_craziness.h"

#define FMT(...) String_Make(fmt::format(__VA_ARGS__))

#define DBG(x)

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
		ASSERT(scope_id);

		if (f.scopes[scope_id].type == Scope_File)
		{
			return scope_id;
		}

		return find_filescope_parent(f, f.scopes[scope_id].parent);
	}

	CodeGen_Result make_result(bool is_constant, GS_Type* type, Const_Union value, bool in_complete_lit, bool lit_float, int code_id, int entity)
	{
		CodeGen_Result result;
		result.success = true;
		result.is_constant = is_constant;
		result.type = type;
		result.value = value;
		result.in_complete_lit = in_complete_lit;
		result.is_float_lit = lit_float;
		result.code_id = code_id;
		result.entity_id = entity;
		return result;
	}

	Entity make_entity(Entity_Kind type, String_Atom* name, Ast_Node* syntax, int scope_id, int file_id)
	{
		Entity entity = {};
		entity.kind = type;
		entity.name = name;
		entity.syntax = syntax;
		entity.scope_id = scope_id;
		entity.file_id = file_id;

		return entity;
	}

	int insert_entity(Front_End& f, Entity entity, int scope_id)
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

	void insert_dep(Front_End& f, int dep, Entity_Deps& deps)
	{
		deps.emplace(dep);
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

	bool frontend_decl_stmt(Front_End& f, int scope_id, int file_id, Ast_Node* stmt)
	{
		switch (stmt->type)
		{
		case Ast_Directive_Add_Library:
		{
			f.added_library_paths.emplace(stmt->token.name);
		}
		break;
		case Ast_Variable:
		{
			String_Atom* name_atom = stmt->token.name;
			int previous_decl = find_entity(f, name_atom, scope_id);

			Scope& scope = get_scope(f, scope_id);

			if (previous_decl)
			{
				frontend_push_error(f, stmt, file_id, FMT("{} already declared!", name_atom->str));
				return false;
			}

			if (stmt->var.is_constant)
			{
				if (stmt->var.assignment->type == Ast_Directive_Library)
				{
					Entity lib_directive = make_entity(Entity_Library, name_atom, stmt, scope_id, file_id);
					lib_directive.library.file_name = stmt->var.assignment->token.name;
					insert_entity(f, lib_directive, scope_id);
					return true;
				}
			}

			if (scope.type == Scope_Function && !stmt->var.is_constant)
			{
				return true;
			}

			if (scope.type == Scope_Struct && !stmt->var.is_constant)
			{
				Entity_Kind entity_type = Entity_Struct_Member;

				Entity member = make_entity(entity_type, name_atom, stmt, scope_id, file_id);
				insert_entity(f, member, scope_id);

				return true;
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
		case Ast_Function:
		{
			String_Atom* name_atom = stmt->token.name;
			int previous_decl = find_entity(f, name_atom, scope_id);

			if (previous_decl)
			{
				frontend_push_error(f, stmt, file_id, FMT("{} already declared!", name_atom->str));
				return false;
			}

			Entity func = make_entity(Entity_Function, name_atom, stmt, scope_id, file_id);

			func.fn.c_varargs = stmt->fn.c_varargs;

			if (stmt->fn.foreign)
			{
				int library = find_entity(f, stmt->fn.foreign, scope_id);

				if (!library)
				{
					frontend_push_error(f, stmt, file_id, FMT("undeclared identifier: '{}'", stmt->fn.foreign->str));
					return false;
				}
				else if (get_entity(f, library).kind != Entity_Library)
				{
					frontend_push_error(f, stmt, file_id, FMT("expected name to be a library: '{}'", stmt->fn.foreign->str));
					return false;
				}

				func.fn.foreign = library;
			}

			int func_scope_id = insert_scope(f, Scope_Function, scope_id, file_id, 0, stmt);

			for (size_t i = 0; i < stmt->fn.parameters.count; i++)
			{
				Ast_Node* param = stmt->fn.parameters[i];

				String_Atom* name_atom = param->token.name;
				int previous_decl = find_entity(f, name_atom, func_scope_id);

				if (previous_decl)
				{
					frontend_push_error(f, param, file_id, FMT("{} already declared!", name_atom->str));
					return false;
				}

				Entity var = make_entity(Entity_Variable, name_atom, param, func_scope_id, file_id);
				var.var.parameter = true;
				int entity_id = insert_entity(f, var, func_scope_id);
				Array_Add(func.fn.parameters, entity_id);
			}

			int entity_id = insert_entity(f, func, scope_id);
			get_scope(f, func_scope_id).entity_id = entity_id;
			get_entity(f, entity_id).fn.scope_id = func_scope_id;

			for (size_t i = 0; i < stmt->fn.body.stmts.count; i++)
			{
				Ast_Node* scope_member = stmt->fn.body.stmts[i];
				bool result = frontend_decl_stmt(f, func_scope_id, file_id, scope_member);

				if (!result)
					return false;
			}
		}
		break;
		case Ast_Struct:
		{
			String_Atom* name_atom = stmt->token.name;
			int previous_decl = find_entity(f, name_atom, scope_id);

			if (previous_decl)
			{
				frontend_push_error(f, stmt, file_id, FMT("{} already declared!", name_atom->str));
				return false;
			}

			Entity _struct = make_entity(Entity_Struct, name_atom, stmt, scope_id, file_id);

			Type_Name tn = {};
			tn.flags = TN_Struct_Type;
			tn.name = _struct.name->str;

			_struct._struct.typename_id = insert_typename(tn);

			if (name_atom == f.keyword_Array)
			{
				f.Array_Ty = get_type(_struct._struct.typename_id);
			}

			if (name_atom == f.keyword_string)
			{
				f.string_Ty = get_type(_struct._struct.typename_id);
			}

			int entity_id = insert_entity(f, _struct, scope_id);
			int struct_scope_id = insert_scope(f, Scope_Struct, scope_id, file_id, entity_id, stmt);
			get_entity(f, entity_id)._struct.scope_id = struct_scope_id;

			for (size_t i = 0; i < stmt->_struct.body.stmts.count; i++)
			{
				Ast_Node* scope_member = stmt->_struct.body.stmts[i];
				bool result = frontend_decl_stmt(f, struct_scope_id, file_id, scope_member);

				if (!result)
					return false;
			}
		}
		case Ast_Directive_Load:
		case Ast_Call:
		case Ast_Binary:
		case Ast_Member:
		case Ast_Array_Type:
		case Ast_Return:
			break;
		default:
			GS_ASSERT_UNIMPL();
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

			bool success = frontend_decl_stmt(f, scope_id, file_scope.file_id, stmt);

			if (!success)
				return false;
		}

		return true;
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
		case Ast_Pointer:
		case Ast_Numeric:
		case Ast_Func_Type:
		case Ast_Array_Type:
			break;
		default:
			GS_ASSERT_UNIMPL();
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
			if (scope.type == Scope_Function && !stmt->var.is_constant)
			{
				return true;
			}

			if (stmt->var.type)
			{
				bool success = frontend_expr_deps(f, stmt->var.type, file_id, scope_id, entity_id, entity.deps);

				if (!success)
					return false;
			}

			if (stmt->var.assignment)
			{
				bool success = frontend_expr_deps(f, stmt->var.assignment, file_id, scope_id, entity_id, entity.deps);

				if (!success)
					return false;
			}
		}
		break;
		case Entity_Struct_Member:
		{
			if (stmt->var.type)
			{
				bool success = frontend_expr_deps(f, stmt->var.type, file_id, scope_id, entity_id, entity.deps);

				if (!success)
					return false;
			}

			if (stmt->var.assignment)
			{
				bool success = frontend_expr_deps(f, stmt->var.assignment, file_id, scope_id, entity_id, entity.deps);

				if (!success)
					return false;
			}
		}
		break;
		case Entity_Function:
		{
			if (stmt->fn.return_type)
			{
				bool success = frontend_expr_deps(f, stmt->fn.return_type, file_id, scope_id, entity_id, entity.deps);
				if (!success) return false;
			}

			for (size_t i = 0; i < stmt->fn.parameters.count; i++)
			{
				Ast_Node* param = stmt->fn.parameters[i];

				bool success = frontend_expr_deps(f, param->var.type, file_id, scope_id, entity_id, entity.deps);
				if (!success) return false;
			}
		}
		break;
		case Entity_Struct:
		{
			Scope& struct_scope = get_scope(f, entity._struct.scope_id);

			for (size_t i = 0; i < struct_scope.entities.count; i++)
			{
				int member_id = struct_scope.entities[i];
				Entity& member_entity = get_entity(f, member_id);

				if (member_entity.kind == Entity_Struct_Member)
				{
					insert_dep(f, member_id, entity.deps);
				}
			}
		}
		break;
		case Entity_Library:
		case Entity_TypeName:
		case Entity_Load:
			break;
		default:
			GS_ASSERT_UNIMPL();
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

	CodeGen_Result expr_resolve_assign(Front_End& f, int scope_id, Ast_Node* expr, GS_Type* inferred_type /*= nullptr*/, bool is_top_level /*= false*/, bool const_eval /*= false*/, Il_Proc* proc /*= nullptr*/, bool by_reference)
	{
		Tk_Operator op = tk_to_operator(expr->token.type);

		CodeGen_Result lhs_result = expr_resolve(f, scope_id, expr->bin.lhs, nullptr, true, false, proc, true);

		if (!lhs_result)
			return {};

		CodeGen_Result rhs_result = expr_resolve(f, scope_id, expr->bin.rhs, lhs_result.type, true, false, proc, false);

		if (!rhs_result)
			return {};

		if (lhs_result.type != rhs_result.type)
		{
			push_error_scope(f, expr, scope_id, FMT("type mismatch: '{}' {} '{}'", print_type(lhs_result.type), operator_to_str(op), print_type(rhs_result.type)));
			return {};
		}

		GS_Type* operation_type = lhs_result.type;

		switch (op)
		{
		case Op_AddAssign:
		case Op_SubAssign:
		case Op_MulAssign:
		case Op_DivAssign:
		{
			Il_Node_Type math_op_type;

			switch (op)
			{
			case Op_AddAssign:
				math_op_type = Il_Add;
				break;
			case Op_SubAssign:
				math_op_type = Il_Sub;
				break;
			case Op_MulAssign:
				math_op_type = Il_Mul;
				break;
			case Op_DivAssign:
				math_op_type = Il_Div;
			}

			CodeGen_Result by_value_lhs_result = expr_resolve(f, scope_id, expr->bin.lhs, nullptr, true, false, proc, false);

			if (!by_value_lhs_result)
				return {};

			rhs_result.code_id = il_insert_math_op(*proc, operation_type, math_op_type, by_value_lhs_result.code_id, rhs_result.code_id);
		}
		break;
		case Op_Assign:
			break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		il_insert_store(*proc, operation_type, lhs_result.code_id, rhs_result.code_id);

		return make_result(false, nullptr, {});
	}

	CodeGen_Result expr_resolve(Front_End& f, int scope_id, Ast_Node* expr, GS_Type* inferred_type, bool is_top_level, bool const_eval, Il_Proc* proc, bool by_reference)
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

			if (identified_entity.kind == Entity_Constant || identified_entity.kind == Entity_TypeName || identified_entity.kind == Entity_Struct)
			{
				if (identified_entity.kind == Entity_Constant)
				{
					value = identified_entity.cnst.value;
					type = identified_entity.type;
				}
				else if (identified_entity.kind == Entity_TypeName)
				{
					value.s8 = get_type_index(get_type(identified_entity.tn.type_name_id));
					type = f.Type_Ty;
				}
				else if (identified_entity.kind == Entity_Struct)
				{
					value.s8 = get_type_index(get_type(identified_entity._struct.typename_id));
					type = f.Type_Ty;
				}

				if (proc)
				{
					int code_id = il_insert_constant(*proc, value, type);
					return make_result(true, type, value, false, false, code_id, identified);
				}
				else
				{
					return make_result(true, type, value, false, false, identified);
				}
			}
			else if (identified_entity.kind == Entity_Function)
			{
				return make_result(true, identified_entity.type, {}, false, false, -1, identified);
			}
			else if (identified_entity.kind == Entity_Variable)
			{
				int code_id = -1;

				if (by_reference)
				{
					code_id = identified_entity.var.code_id;
				}
				else
				{
					code_id = il_insert_load(*proc, identified_entity.type, identified_entity.var.code_id);
				}

				return make_result(true, identified_entity.type, {}, false, false, code_id, identified);
			}
			else
			{
				push_error_scope(f, expr, scope_id, FMT("not a constant: {}", expr->token.name->str));
				return {};
			}
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

			if (proc)
			{
				int code_id = il_insert_constant(*proc, value, type);
				return make_result(true, type, value, true, expr->num.is_float, code_id);
			}
			else
			{
				return make_result(true, type, value, true, expr->num.is_float);
			}
		}
		case Ast_String:
		{
			std::string processed_literal;

			for (size_t i = 0; i < expr->token.name->str.count; i++)
			{
				auto c = expr->token.name->str[i];

				if (c == '\\') {
					if (i + 1 < expr->token.name->str.count) {

						auto next_c = expr->token.name->str[i + 1];

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

			GS_Type* type = f.c_str_Ty;

			int string_code_id = il_insert_string(*proc, type, literal_value);

			if (inferred_type != f.c_str_Ty)
			{
				type = f.string_Ty;

				Il_IDX struct_members[2];

				Const_Union count_const;
				count_const.us8 = literal_value.count;

				struct_members[0] = il_insert_constant(*proc, count_const, f.u64_Ty);
				struct_members[1] = string_code_id;

				string_code_id = il_insert_si(*proc, type, Array_View((Il_IDX*)struct_members, 2));

				if (by_reference) {
					Il_IDX lvalue_alloca = il_insert_alloca(*proc, type);
					il_insert_store(*proc, type, lvalue_alloca, string_code_id);
					string_code_id = lvalue_alloca;
				}
			}

			return make_result(false, type, {}, false, false, string_code_id);
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

			if (assign_op)
			{
				return expr_resolve_assign(f, scope_id, expr, inferred_type, is_top_level, const_eval, proc, false);
			}

			auto lhs_result = expr_resolve(f, scope_id, expr->bin.lhs, nullptr, false, const_eval, proc);

			if (!lhs_result)
				return {};

			auto rhs_result = expr_resolve(f, scope_id, expr->bin.rhs, nullptr, false, const_eval, proc);

			if (!rhs_result)
				return {};

			GS_Type* most_complete_type = nullptr;

			if (rhs_result.in_complete_lit)
			{
				if (rhs_result.is_float_lit && !inferred_type)
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
				if (lhs_result.is_float_lit && !inferred_type)
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
					if ((!lhs_result.is_float_lit) || (lhs_result.is_float_lit && is_float)) {

						lhs_result.type = most_complete_type;

						if (is_float && !lhs_result.is_float_lit)
						{
							lhs_result.value.f64 = (double)lhs_result.value.s8;
						}

						if (proc)
						{
							proc->instruction_storage[lhs_result.code_id].type_idx = get_type_index(most_complete_type);
						}
					}
				}

				if (rhs_result.in_complete_lit)
				{
					if ((!rhs_result.is_float_lit) || (rhs_result.is_float_lit && is_float)) {
						rhs_result.type = most_complete_type;

						if (is_float && !rhs_result.is_float_lit)
						{
							rhs_result.value.f64 = (double)rhs_result.value.s8;
						}

						if (proc)
						{
							proc->instruction_storage[rhs_result.code_id].type_idx = get_type_index(most_complete_type);
						}
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

			bool evaluate = fold || const_eval;

			if (evaluate)
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

			Il_Node_Type il_op_type;
			Il_Cmp_Type il_cmp_type;

			if (proc)
			{
				switch (op)
				{
				case Op_Add: il_op_type = Il_Add; break;
				case Op_Sub: il_op_type = Il_Sub; break;
				case Op_Mul: il_op_type = Il_Mul; break;
				case Op_Div: il_op_type = Il_Div; break;
				case Op_Eq: il_op_type = Il_Value_Cmp; break;
				case Op_NotEq: il_op_type = Il_Value_Cmp; break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}

			GS_Type* result_type = type;

			if (comparative_op)
			{
				result_type = f.bool_Ty;

				switch (op)
				{
				case Op_Eq: il_cmp_type = Il_Cmp_Equal; break;
				case Op_NotEq: il_cmp_type = Il_Cmp_NotEqual; break;
				default: GS_ASSERT_UNIMPL(); break;
				}
			}

			if (fold)
			{
				if (proc)
				{
					int code_id = il_insert_constant(*proc, value, result_type);
					return make_result(true, result_type, value, true, !is_integer, code_id);
				}
				else {
					return make_result(true, result_type, value, true, !is_integer);
				}
			}
			else
			{
				if (proc)
				{
					int code_id = -1;

					if (!comparative_op) {
						code_id = il_insert_math_op(*proc, type, il_op_type, lhs_result.code_id, rhs_result.code_id);
					}
					else
					{
						code_id = il_insert_compare(*proc, il_op_type, il_cmp_type, type, lhs_result.code_id, rhs_result.code_id);
					}

					return make_result(true, result_type, value, false, false, code_id);
				}
				else {
					return make_result(true, result_type, value);
				}
			}
		}
		break;
		case Ast_Pointer:
		{
			Const_Union value = {};

			auto result = expr_resolve(f, scope_id, expr->un.expr, nullptr, false, true);

			if (!result)
				return {};

			if (result.type != f.Type_Ty)
			{
				push_error_scope(f, expr->un.expr, scope_id, FMT("expression must be of type 'Type': {}", print_type(result.type)));
				return {};
			}

			value.s8 = get_type_index(get_type_at(result.value.s4)->get_pointer());

			if (proc)
			{
				int code_id = il_insert_constant(*proc, value, f.Type_Ty);
				return make_result(true, f.Type_Ty, value, false, false, code_id);
			}
			else
			{
				return make_result(true, f.Type_Ty, value);
			}
		}
		break;
		case Ast_Array_Type:
		{
			Const_Union value = {};

			auto result = expr_resolve(f, scope_id, expr->array_type.elem, nullptr, false, true);

			if (!result)
				return {};

			if (result.type != f.Type_Ty)
			{
				push_error_scope(f, expr->un.expr, scope_id, FMT("expression must be of type 'Type': {}", print_type(result.type)));
				return {};
			}

			if (expr->array_type.dynamic)
			{
				value.s8 = get_type_index(get_type_at(result.value.s4)->get_dynarray());
			}
			else if (expr->array_type.size)
			{
				CodeGen_Result size_result = expr_resolve(f, scope_id, expr->array_type.size, nullptr, false, true);

				if (!size_result)
					return {};

				if (!size_result.is_constant)
				{
					push_error_scope(f, expr->array_type.size, scope_id, FMT("expression is not constant"));
					return {};
				}

				if ((get_type_flags(size_result.type) & TN_Float_Type) || !(get_type_flags(size_result.type) & TN_Numeric_Type))
				{
					push_error_scope(f, expr->array_type.size, scope_id, FMT("expression must evaluate to an integer"));
					return {};
				}

				if (size_result.value.s8 < 0)
				{
					push_error_scope(f, expr->array_type.size, scope_id, FMT("size must be greater thn zero: {}", size_result.value.s8));
					return {};
				}

				value.s8 = get_type_index(get_type_at(result.value.s4)->get_array(size_result.value.s8));
			}
			else
			{
				GS_ASSERT_UNIMPL();
			}

			if (proc)
			{
				int code_id = il_insert_constant(*proc, value, f.Type_Ty);
				return make_result(true, f.Type_Ty, value, false, false, code_id);
			}
			else
			{
				return make_result(true, f.Type_Ty, value);
			}
		}
		break;
		case Ast_Func_Type:
		{
			Const_Union value = {};

			Array<GS_Type*> param_types;

			for (size_t i = 0; i < expr->func_type.params.count; i++)
			{
				Ast_Node* param = expr->func_type.params[i];

				auto result = resolve_type(f, scope_id, expr->func_type.params[i]);

				if (!result)
					return {};

				Array_Add(param_types, result);
			}

			auto return_type = resolve_type(f, scope_id, expr->func_type.return_type);

			if (!return_type)
				return {};

			value.s8 = get_type_index(get_proc_type(return_type, param_types));

			if (proc)
			{
				int code_id = il_insert_constant(*proc, value, f.Type_Ty);
				return make_result(true, f.Type_Ty, value, false, false, code_id);
			}
			else
			{
				return make_result(true, f.Type_Ty, value);
			}
		}
		break;
		case Ast_Call:
		{
			CodeGen_Result result = expr_resolve(f, scope_id, expr->call.callee, nullptr, false, false, proc);

			if (!result)
				return {};

			int called_function = 0;
			int called_function_proc_idx = -1;
			Entity* called_function_entity = nullptr;

			GS_Type* signature = nullptr;

			if (result.entity_id)
			{
				Entity& entity = get_entity(f, result.entity_id);

				if (entity.kind == Entity_Function)
				{
					called_function = result.entity_id;
					called_function_proc_idx = entity.fn.proc_idx;
					called_function_entity = &entity;
					signature = entity.fn.signature;
				}
			}

			bool c_varargs = false;

			if (called_function_entity)
			{
				c_varargs = called_function_entity->fn.c_varargs;
			}

			if (expr->call.args.count < signature->proc.params.count)
			{
				push_error_scope(f, expr, scope_id, FMT("too few argument for call: (needed: {}, given: {})", signature->proc.params.count, expr->call.args.count));
				return {};
			}

			if (expr->call.args.count > signature->proc.params.count && !c_varargs)
			{
				push_error_scope(f, expr->call.args[expr->call.args.count - 1], scope_id, FMT("too many argument for call: (needed: {}, given: {})", signature->proc.params.count, expr->call.args.count));
				return {};
			}

			Array<int> argument_code_ids;
			Array<GS_Type*> code_argument_types;
			GS_Type* code_return_type = signature->proc.return_type;

			bool aggr_return = is_type_aggr(signature->proc.return_type);

			int return_alloca = 0;

			if (aggr_return)
			{
				return_alloca = il_insert_alloca(*proc, signature->proc.return_type);
				Array_Add(argument_code_ids, return_alloca);
				Array_Add(code_argument_types, signature->proc.return_type->get_pointer());
				code_return_type = f.void_Ty;
			}

			for (size_t i = 0; i < expr->call.args.count; i++)
			{
				GS_Type* param_type = nullptr;

				if (i < signature->proc.params.count)
				{
					param_type = signature->proc.params[i];
				}

				Ast_Node* argument = expr->call.args[i];

				CodeGen_Result result = expr_resolve(f, scope_id, argument, param_type, false, false, proc);

				if (!result)
					return {};

				if (param_type)
				{
					if (result.type != param_type)
					{
						push_error_scope(f, argument, scope_id, FMT("argument type mismatch: (needed: {}, given: {})", print_type(param_type), print_type(result.type)));
						return {};
					}
				}

				if (c_varargs && (get_type_flags(result.type) & TN_Float_Type) && result.type->size() != 8)
				{
					result.type = f.f64_Ty;
					result.code_id = il_insert_cast(*proc, Il_Cast_FloatExt, f.f64_Ty, result.type, result.code_id);
				}

				Array_Add(argument_code_ids, result.code_id);
				Array_Add(code_argument_types, result.type);
			}

			int code_id = 0;

			if (called_function_entity)
			{
				GS_Type* code_signature = get_proc_type(code_return_type, code_argument_types);
				code_id = il_insert_call(*proc, code_signature, argument_code_ids, called_function_entity->fn.proc_idx);

				if (aggr_return)
				{
					code_id = return_alloca;

					if (!by_reference)
					{
						code_id = il_insert_load(*proc, signature->proc.return_type, return_alloca);
					}
				}
			}

			return make_result(false, signature->proc.return_type, {}, false, false, code_id);
		}
		break;
		case Ast_Member:
		{
			CodeGen_Result result = expr_resolve(f, scope_id, expr->mem.expr, nullptr, false, false, proc, true);

			if (!result)
				return {};

			// || result.type->kind == Type_Array || result.type->kind == Type_Dyn_Array

			int code_id = -1;

			if (result.type && get_type_flags(result.type) & TN_Struct_Type)
			{
				int struct_entity_id = f.typename_to_struct.at(result.type->basic.type_name_id);
				int struct_scope_id = get_entity(f, struct_entity_id)._struct.scope_id;

				int member = find_entity(f, expr->mem.member.name, struct_scope_id);

				if (!member)
				{
					push_error_scope(f, expr, scope_id, FMT("struct '{}' does have member named: '{}'", print_type(result.type), expr->mem.member.name->str));
					return {};
				}

				Entity& member_entity = get_entity(f, member);

				if (member_entity.kind != Entity_Struct_Member)
				{
					push_error_scope(f, expr->mem.member, scope_id, FMT("struct '{}' member: '{}' is not a data member", print_type(result.type), expr->mem.member.name->str));
					return {};
				}

				code_id = il_insert_sep(*proc, result.type, member_entity.struct_mem.index, result.code_id);

				if (!by_reference)
				{
					code_id = il_insert_load(*proc, member_entity.type, code_id);
				}

				return make_result(false, member_entity.type, {}, false, false, code_id);
			}
			else
			{
				push_error_scope(f, expr, scope_id, FMT("type does not support members: '{}'", print_type(result.type)));
				return {};
			}
		}
		break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return {};
	}

	bool frontend_entity_resolve(Front_End& f, int scope_id, Entity& entity, int entity_id)
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
		case Entity_Variable:
		{
			if (!entity.var.global)
				return true;
		}
		break;
		case Entity_Function:
		{
			Array<GS_Type*> parameter_types;
			Array<GS_Type*> code_parameter_types;

			GS_Type* return_type = f.void_Ty;
			GS_Type* code_return_type = f.void_Ty;

			if (stmt->fn.return_type)
			{
				return_type = resolve_type(f, scope_id, stmt->fn.return_type);

				if (!return_type)
					return false;
			}

			bool aggr_return = is_type_aggr(return_type);

			if (aggr_return)
			{
				Array_Add(code_parameter_types, return_type->get_pointer());
			}
			else
			{
				code_return_type = return_type;
			}

			for (int i = 0; i < stmt->fn.parameters.count; i++)
			{
				Ast_Node* parameter = stmt->fn.parameters[i];

				auto param_type = resolve_type(f, scope_id, parameter->var.type);

				if (!param_type)
					return false;

				Entity& param_entity = get_entity(f, entity.fn.parameters[i]);

				param_entity.type = param_type;
				param_entity.var.parameter_code_idx = i + (int)aggr_return;

				GS_Type* code_type = param_type;

				if (is_type_aggr(param_type))
				{
					code_type = param_type->get_pointer();
				}

				param_entity.var.code_type = code_type;

				Array_Add(code_parameter_types, code_type);
				Array_Add(parameter_types, param_type);
			}

			GS_Type* proc_type = get_proc_type(return_type, parameter_types);
			GS_Type* code_proc_type = get_proc_type(code_return_type, code_parameter_types);

			entity.fn.signature = proc_type;
			entity.fn.code_proc_signature = code_proc_type;

			if (entity.fn.foreign)
			{
				entity.fn.proc_idx = il_insert_external_proc(f.program, entity.name->str, code_proc_type, -1, entity.fn.c_varargs);
			}
			else
			{
				entity.fn.proc_idx = il_insert_proc(f.program, entity.name->str, code_proc_type, false);
			}

			entity.flags |= Flag_Complete;
		}
		break;
		case Entity_Struct:
		{
			Scope& struct_scope = get_scope(f, entity._struct.scope_id);

			Array<GS_Type*> member_types;

			u64 member_index = 0;

			for (size_t i = 0; i < struct_scope.entities.count; i++)
			{
				int member_id = struct_scope.entities[i];
				Entity& member_entity = get_entity(f, member_id);

				if (member_entity.kind == Entity_Struct_Member)
				{
					member_entity.struct_mem.index = member_index;
					Array_Add(member_types, member_entity.type);
					member_index++;
				}
			}

			f.typename_to_struct[entity._struct.typename_id] = entity_id;

			insert_struct(entity._struct.typename_id, member_types);

			entity.flags |= Flag_Complete;
		}
		break;
		case Entity_Struct_Member:
		{
			if (entity.syntax->var.type)
			{
				entity.type = resolve_type(f, scope_id, entity.syntax->var.type);

				if (!entity.type)
					return nullptr;
			}

			if (stmt->var.assignment)
			{
				auto result = expr_resolve(f, scope_id, stmt->var.assignment, entity.type, false, true);

				if (!result)
					return false;

				entity.struct_mem.initializer = result.value;
				entity.struct_mem.has_initializer = true;

				if (!entity.syntax->var.type)
				{
					entity.type = result.type;
				}
			}

			if (entity.type->size() <= 0) {
				push_error_scope(f, entity.syntax, scope_id, FMT("struct member '{}' type size is zero and is not allowed", entity.name->str));
				return {};
			}

			entity.flags |= Flag_Complete;
		}
		break;
		case Entity_TypeName:
		case Entity_Library:
		case Entity_Load:
			break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return true;
	}

	bool frontend_stmt_codegen(Front_End& f, Ast_Node* stmt, int func_entity_id, int scope_id, Il_Proc& proc)
	{
		switch (stmt->type)
		{
		case Ast_Variable:
		{
			if (stmt->var.is_constant)
			{
				return true;
			}

			String_Atom* name_atom = stmt->token.name;
			int previous_decl = find_entity(f, name_atom, scope_id);

			Scope& scope = get_scope(f, scope_id);

			if (previous_decl)
			{
				push_error_scope(f, stmt, scope_id, FMT("{} already declared!", name_atom->str));
				return {};
			}

			GS_Type* type = nullptr;

			int assignment_code_id = -1;

			if (stmt->var.type)
			{
				type = resolve_type(f, scope_id, stmt->var.type);

				if (!type)
					return {};
			}

			if (stmt->var.assignment)
			{
				CodeGen_Result result = expr_resolve(f, scope_id, stmt->var.assignment, type, false, false, &proc);

				if (!result)
					return {};

				if (type)
				{
					if (result.type != type)
					{
						push_error_scope(f, stmt, scope_id, FMT("type mismatch in assignment: needed: '{}', given: '{}'", print_type(type), print_type(result.type)));
						return {};
					}
				}

				type = result.type;
				assignment_code_id = result.code_id;
			}

			int alloca_code_id = il_insert_alloca(proc, type);

			int assignment = -1;

			if (assignment_code_id != -1)
			{
				assignment = assignment_code_id;
			}
			else
			{
				if (is_type_aggr(type))
				{
					assignment = il_insert_zero_init(proc, type);
				}
				else
				{
					assignment = il_insert_constant(proc, 0, type);
				}
			}

			il_insert_store(proc, type, alloca_code_id, assignment);

			Entity var = make_entity(Entity_Variable, name_atom, stmt, scope_id, get_scope(f, scope_id).file_id);
			var.type = type;
			var.var.code_id = alloca_code_id;

			insert_entity(f, var, scope_id);
		}
		break;
		case Ast_Ident:
		case Ast_Binary:
		case Ast_Call:
		case Ast_Member:
		{
			return expr_resolve(f, scope_id, stmt, nullptr, true, false, &proc);
		}
		break;
		case Ast_Return:
		{
			Ast_Node* expr = stmt->un.expr;

			Entity& func = get_entity(f, func_entity_id);
			auto func_ret_type = func.fn.signature->proc.return_type;

			if (!expr && func_ret_type != f.void_Ty) {
				push_error_scope(f, stmt, scope_id, String_Make("return value expected"));
				return false;
			}

			if (expr && func_ret_type == f.void_Ty) {
				push_error_scope(f, expr, scope_id, String_Make("return value not expected"));
				return false;
			}

			if (expr)
			{
				bool is_aggr = is_type_aggr(func_ret_type);

				CodeGen_Result result = expr_resolve(f, scope_id, expr, func_ret_type, false, false, &proc);

				if (!result)
					return false;

				if (func_ret_type != result.type)
				{
					push_error_scope(f, expr, scope_id, FMT("return type mismatch: (needed: '{}', given: '{}')", print_type(func_ret_type), print_type(result.type)));
					return false;
				}

				if (is_aggr)
				{
					il_insert_store(proc, func_ret_type,
						il_insert_load(proc, func_ret_type->get_pointer(), func.fn.return_variable_id)
						, result.code_id);
				}
				else
				{
					il_insert_store(proc, func_ret_type, func.fn.return_variable_id, result.code_id);
				}

				if (!result)
					return false;
			}
		}
		break;
		break;
		case Ast_Struct:
		case Ast_Function:
			break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return true;
	}

	bool frontend_function_codegen(Front_End& f, Entity& entity, int entity_id)
	{
		int scope_id = entity.scope_id;
		Ast_Node* stmt = entity.syntax;

		Il_Proc& proc = f.program.procedures[entity.fn.proc_idx];

		auto ret_type = entity.fn.signature->proc.return_type;
		bool aggr_return = is_type_aggr(ret_type);

		if (ret_type == f.void_Ty)
		{
		}
		else if (aggr_return)
		{
			entity.fn.return_variable_id = il_insert_alloca(proc, ret_type->get_pointer());
			il_insert_store(proc, ret_type->get_pointer(), entity.fn.return_variable_id, proc.parameters[0]);
		}
		else {
			entity.fn.return_variable_id = il_insert_alloca(proc, ret_type);
		}

		for (size_t i = 0; i < entity.fn.parameters.count; i++)
		{
			int param = entity.fn.parameters[i];
			Entity& param_entity = get_entity(f, param);
			param_entity.var.code_id = il_insert_alloca(proc, param_entity.var.code_type);
			int parameter_node_id = proc.parameters[param_entity.var.parameter_code_idx];
			il_insert_store(proc, param_entity.var.code_type, param_entity.var.code_id, parameter_node_id);
		}

		for (size_t i = 0; i < stmt->fn.body.stmts.count; i++)
		{
			bool success = frontend_stmt_codegen(f, stmt->fn.body.stmts[i], entity_id, entity.fn.scope_id, proc);

			if (!success)
				return success;
		}

		int return_block_idx = il_insert_block(proc, String_Make("return_block"));

		for (size_t i = 0; i < entity.fn.return_branches.count; i++)
		{
			proc.instruction_storage[entity.fn.return_branches[i]].br.block_idx = return_block_idx;
		}

		if (ret_type == f.void_Ty || aggr_return)
		{
			il_insert_ret(proc, f.void_Ty, -1);
		}
		else
		{
			il_insert_ret(proc, ret_type, il_insert_load(proc, ret_type, entity.fn.return_variable_id));
		}

		return true;
	}

	bool frontend_entity_codegen(Front_End& f, Entity& entity, int entity_id)
	{
		int scope_id = entity.scope_id;

		if (entity.kind == Entity_Function)
		{
			if (!entity.fn.foreign)
			{
				bool success = frontend_function_codegen(f, entity, entity_id);

				if (!success)
					return false;
			}
		}

		return true;
	}

	bool frontend_codegen(Front_End& f)
	{
		for (int i = 0; i < f.entities.count; i++)
		{
			Entity& entity = f.entities[i];

			if (entity.kind == Entity_Function) {

				bool success = frontend_entity_codegen(f, entity, i);

				if (!success)
					return false;
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

		for (size_t i = 1; i < f.entities.count; i++)
		{
			int entity_id = i;
			Entity& entity = f.entities[entity_id];

			int scope_id = entity.scope_id;
			Scope& scope = get_scope(f, scope_id);

			bool success = frontend_entity_deps(f, scope, scope_id, scope.file_id, entity, entity_id);
			if (!success)
				return false;
		}

		bool all_resolved = true;

		for (size_t i = 1; i < f.entities.count; i++)
		{
			int entity_id = i;
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
				continue;
			}
			else
			{
				Entity& entity = f.entities[entity_id];
				bool success = frontend_entity_resolve(f, entity.scope_id, entity, entity_id);

				if (!success)
					return false;
			}

			if (!all_resolved)
			{
				i = 0;
				all_resolved = true;
			}
		}

		bool success = frontend_codegen(f);

		if (!success)
			return false;

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

		Entity load_entity = make_entity(Entity_Load, get_atom(""), nullptr, 1, 0);
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
		Entity tn_entity = make_entity(Entity_TypeName, get_atom(name), nullptr, 1, 0);
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

		f.keyword_Array = get_atom("Array");
		f.keyword_string = get_atom("string");

		f.Type_Ty = insert_base_type_entity(f, "Type", TN_Base_Type, 8, 8);

		f.bool_Ty = insert_base_type_entity(f, "bool", TN_Base_Type, 1, 1);

		f.void_Ty = insert_base_type_entity(f, "void", TN_Base_Type, 0, 0);

		f.i8_Ty = insert_base_type_entity(f, "i8", TN_Numeric_Type | TN_Base_Type, 4, 4);
		f.u8_Ty = insert_base_type_entity(f, "u8", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 4, 4);
		f.i16_Ty = insert_base_type_entity(f, "i16", TN_Numeric_Type | TN_Base_Type, 4, 4);
		f.u16_Ty = insert_base_type_entity(f, "u16", TN_Numeric_Type | TN_Base_Type | TN_Unsigned_Type, 4, 4);
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

		result = frontend_loop(f);

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
