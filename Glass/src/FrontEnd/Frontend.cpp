#include "pch.h"

#include "FrontEnd/Frontend.h"
#include "FrontEnd/Lexer.h"
#include "BackEnd/LLVM_Converter.h"
#include "BackEnd/Machine.h"
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

	void flatten_syntax(Ast_Node** reference, Array<Flat_Node>& flat_ast, int scope_id)
	{
		Ast_Node* node = *reference;

		switch (node->type)
		{
		case Ast_Ident:
		case Ast_Numeric:
			Array_Add(flat_ast, { reference, scope_id });
			break;
		case Ast_Binary:
		{
			flatten_syntax(&node->bin.lhs, flat_ast, scope_id);
			flatten_syntax(&node->bin.rhs, flat_ast, scope_id);
			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Call:
		{
			for (size_t i = 0; i < node->call.args.count; i++)
			{
				flatten_syntax(&node->call.args[i], flat_ast, scope_id);
			}

			flatten_syntax(&node->call.callee, flat_ast, scope_id);

			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_String:
		{
			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Pointer:
		{
			flatten_syntax(&node->un.expr, flat_ast, scope_id);
			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Array_Type:
		{
			flatten_syntax(&node->array_type.elem, flat_ast, scope_id);

			if (!node->array_type.dynamic)
			{
				flatten_syntax(&node->array_type.size, flat_ast, scope_id);
			}

			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Variable:
		{
			if (!node->var.is_constant)
			{
				if (node->var.type)
					flatten_syntax(&node->var.type, flat_ast, scope_id);

				if (node->var.assignment)
					flatten_syntax(&node->var.assignment, flat_ast, scope_id);

				Array_Add(flat_ast, { reference, scope_id });
			}
		}
		break;
		case Ast_Member:
		{
			flatten_syntax(&node->mem.expr, flat_ast, scope_id);
			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Cast:
		{
			flatten_syntax(&node->bin.lhs, flat_ast, scope_id);
			flatten_syntax(&node->bin.rhs, flat_ast, scope_id);
			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Ref:
		case Ast_DeRef:
		{
			flatten_syntax(&node->un.expr, flat_ast, scope_id);
			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Char:
			Array_Add(flat_ast, { reference, scope_id });
			break;
		case Ast_Scope:
		{
			for (size_t i = 0; i < node->scope.stmts.count; i++)
			{
				flatten_syntax(&node->scope.stmts[i], flat_ast, node->scope.scope_id);
			}
		}
		break;
		case Ast_If:
		case Ast_While:
		case Ast_For:
			flatten_syntax(&node->cond.condition, flat_ast, scope_id);
			Array_Add(flat_ast, { reference, scope_id });
			flatten_syntax(&node->cond.body, flat_ast, scope_id);
			break;
		case Ast_Type_Info:
		{
			flatten_syntax(&node->un.expr, flat_ast, scope_id);
			Array_Add(flat_ast, { reference, scope_id });
		}
		break;
		case Ast_Function:
		case Ast_Struct:
			break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}
	}

	struct Checked_Expr
	{
		GS_Type* type;
		Const_Union value;
	};

	void insert_dep(Front_End& f, int dep, Entity_Deps& deps)
	{
		deps.emplace(dep);
	}

	auto try_promote(Front_End& f, GS_Type* to_type, Expr_Value& expr_value)
	{
		u64 type_flags = get_type_flags(to_type);
		bool is_float = type_flags & TN_Float_Type;

		if (expr_value.is_unsolid && !expr_value.is_unsolid_float && to_type == f.bool_Ty)
		{
			expr_value.type = to_type;
		}

		if (expr_value.is_unsolid && (type_flags & TN_Numeric_Type)) {

			if (!(!is_float && expr_value.is_unsolid_float))
			{
				expr_value.type = to_type;

				if (!expr_value.is_unsolid_float && is_float) {
					expr_value.value.f64 = (double)expr_value.value.s8;
					return true;
				}
			}
		}

		if (expr_value.is_unsolid && expr_value.type == f.string_Ty) {
			static GS_Type* cstr_type = f.u8_Ty->get_pointer();
			if (to_type == cstr_type) {
				expr_value.is_unsolid = false;
				expr_value.type = cstr_type;
			}
		}

		return false;
	};

	bool prewalk_stmt(Front_End& f, Ast_Node* stmt, int scope_id, int file_id)
	{
		auto already_decalared_error = [&]()
		{
			String_Atom* name = stmt->token.name;

			int previous_declaration = find_entity(f, stmt->token.name, scope_id);

			if (previous_declaration)
			{
				push_error_scope(f, stmt, scope_id, FMT("already declared: '{}'", name->str));
				return false;
			}

			return true;
		};

		switch (stmt->type)
		{
		case Ast_Directive_Add_Library:
		{
			f.added_library_paths.emplace(stmt->token.name);
		}
		break;
		case Ast_Variable:
		{
			Scope_Type current_scope_type = get_scope(f, scope_id).type;

			if (!stmt->var.is_constant && current_scope_type == Scope_Function) {
				return true;
			}
			else if (!stmt->var.is_constant && current_scope_type == Scope_Struct) {

				if (!already_decalared_error()) {
					return false;
				}

				Entity struct_data_member = make_entity(Entity_Struct_Member, stmt->token.name, stmt);

				if (stmt->var.type)
					flatten_syntax(&stmt->var.type, struct_data_member.flat_syntax, scope_id);

				if (stmt->var.assignment)
					flatten_syntax(&stmt->var.assignment, struct_data_member.flat_syntax, scope_id);

				insert_entity(f, struct_data_member, scope_id);
			}
			else if (stmt->var.is_constant)
			{
				if (!already_decalared_error()) {
					return false;
				}

				if (stmt->var.assignment->type == Ast_Directive_Library)
				{
					Entity lib_directive = make_entity(Entity_Library, stmt->token.name, stmt);
					lib_directive.library.file_name = stmt->var.assignment->token.name;
					insert_entity(f, lib_directive, scope_id);
					return true;
				}

				Entity constant = make_entity(Entity_Constant, stmt->token.name, stmt);

				if (stmt->var.type)
					flatten_syntax(&stmt->var.type, constant.flat_syntax, scope_id);

				flatten_syntax(&stmt->var.assignment, constant.flat_syntax, scope_id);
				insert_entity(f, constant, scope_id);
			}
			else
			{
				GS_ASSERT_UNIMPL();
			}
		}
		break;
		case Ast_Struct:
		{
			if (!already_decalared_error()) {
				return false;
			}

			int entity_id = insert_entity(f, make_entity(Entity_Struct, stmt->token.name, stmt), scope_id);
			int struct_scope_id = insert_scope(f, Scope_Struct, scope_id, file_id, entity_id, nullptr);

			Entity& struct_entity = get_entity(f, entity_id);

			Type_Name tn = {};
			tn.flags = TN_Struct_Type;
			tn.name = stmt->token.name->str;

			struct_entity._struct.typename_id = insert_typename(tn);
			struct_entity._struct.scope_id = struct_scope_id;

			if (stmt->token.name == f.keyword_string) {
				f.string_Ty = get_type(struct_entity._struct.typename_id);
				f.string_entity_id = entity_id;
			}

			if (stmt->token.name == f.keyword_Array) {
				f.Array_Ty = get_type(struct_entity._struct.typename_id);
				f.Array_entity_id = entity_id;
			}

			if (stmt->token.name == f.keyword_TypeInfo) {
				f.TypeInfo_Ty = get_type(struct_entity._struct.typename_id);
				f.TypeInfo_entity_id = entity_id;
			}

			for (size_t i = 0; i < stmt->_struct.body.stmts.count; i++)
			{
				prewalk_stmt(f, stmt->_struct.body.stmts[i], struct_scope_id, file_id);
			}

			Scope& my_scope = get_scope(f, struct_scope_id);

			for (size_t i = 0; i < my_scope.entities.count; i++)
			{
				int member_entity_id = my_scope.entities[i];
				Entity& member_entity = get_entity(f, member_entity_id);

				if (member_entity.kind == Entity_Struct_Member) {
					insert_dep(f, member_entity_id, struct_entity.deps);
				}
			}
		}
		break;
		case Ast_Ident:
		{
			Scope_Type current_scope_type = get_scope(f, scope_id).type;

			if (current_scope_type == Scope_Enum)
			{
				if (!already_decalared_error()) {
					return false;
				}

				int enum_entity_id = f.scope_id_to_enum.at(scope_id);

				Entity& enum_entity = get_entity(f, enum_entity_id);

				int entity_id = insert_entity(f, make_entity(Entity_Enum_Member, stmt->token.name, stmt), scope_id);
				Entity& entity = get_entity(f, entity_id);

				entity.enum_mem.enum_entity_id = enum_entity_id;

				entity.enum_mem.previous_member = enum_entity.enm.previous_member;

				if (enum_entity.enm.previous_member)
					insert_dep(f, enum_entity.enm.previous_member, entity.deps);

				enum_entity.enm.previous_member = entity_id;
			}
		}
		break;
		case Ast_Enum:
		{
			if (!already_decalared_error()) {
				return false;
			}

			int entity_id = insert_entity(f, make_entity(Entity_Enum, stmt->token.name, stmt), scope_id);
			int enum_scope_id = insert_scope(f, Scope_Enum, scope_id, file_id, entity_id, nullptr);

			f.scope_id_to_enum[enum_scope_id] = entity_id;

			Entity& entity = get_entity(f, entity_id);

			entity.enm.underlying_type_id = f.u64_Ty->basic.type_name_id;
			GS_Type* underlying_type = f.u64_Ty;
			entity.enm.underlying_type = underlying_type;

			Type_Name tn = {};
			tn.flags = TN_Enum_Type | get_type_flags(underlying_type);
			tn.name = stmt->token.name->str;
			tn.size = underlying_type->size();
			tn.alignment = get_type_alignment(underlying_type);

			entity.enm.typename_id = insert_typename(tn);
			entity.enm.scope_id = enum_scope_id;

			entity.enm.type = get_type(entity.enm.typename_id);

			for (size_t i = 0; i < stmt->_enum.body.stmts.count; i++)
			{
				prewalk_stmt(f, stmt->_enum.body.stmts[i], enum_scope_id, file_id);
			}

			Scope& my_scope = get_scope(f, enum_scope_id);

			for (size_t i = 0; i < my_scope.entities.count; i++)
			{
				int member_entity_id = my_scope.entities[i];
				Entity& member_entity = get_entity(f, member_entity_id);

				if (member_entity.kind == Entity_Enum_Member) {
					insert_dep(f, member_entity_id, entity.deps);
				}
			}
		}
		break;
		case Ast_Function:
		{
			if (!already_decalared_error()) {
				return false;
			}

			int entity_id = insert_entity(f, make_entity(Entity_Function, stmt->token.name, stmt), scope_id);
			int func_scope_id = insert_scope(f, Scope_Function, scope_id, file_id, entity_id, nullptr);

			Entity& entity = get_entity(f, entity_id);

			entity.fn.scope_id = func_scope_id;

			if (stmt->fn.return_type)
				flatten_syntax(&stmt->fn.return_type, entity.flat_syntax, func_scope_id);

			entity.fn.return_ast_count = entity.flat_syntax.count;

			for (size_t i = 0; i < stmt->fn.parameters.count; i++)
			{
				Ast_Node* parameter = stmt->fn.parameters[i];

				int previous_declaration = find_entity(f, parameter->token.name, func_scope_id);

				if (previous_declaration)
				{
					push_error_scope(f, parameter, scope_id, FMT("already declared: '{}'", parameter->token.name->str));
					return false;
				}

				int param_entity_id = insert_entity(f, make_entity(Entity_Variable, parameter->token.name, parameter), func_scope_id);
				Entity& param_entity = get_entity(f, param_entity_id);

				if (parameter->var.type)
				{
					flatten_syntax(&parameter->var.type, param_entity.flat_syntax, func_scope_id);
				}

				insert_dep(f, param_entity_id, entity.deps);
				Array_Add(entity.fn.parameters, param_entity_id);
			}

			entity.fn.c_varargs = stmt->fn.c_varargs;

			if (stmt->fn.has_body)
			{
				for (size_t i = 0; i < stmt->fn.body.stmts.count; i++)
				{
					prewalk_stmt(f, stmt->fn.body.stmts[i], func_scope_id, file_id);
				}

				for (size_t i = 0; i < stmt->fn.body.stmts.count; i++)
				{
					flatten_syntax(&stmt->fn.body.stmts[i], entity.flat_syntax, func_scope_id);
				}
			}
		}
		break;
		case Ast_Scope:
		{
			int new_scope_id = insert_scope(f, Scope_Function, scope_id, file_id, 0, nullptr);

			stmt->scope.scope_id = new_scope_id;

			for (size_t i = 0; i < stmt->scope.stmts.count; i++)
			{
				prewalk_stmt(f, stmt->scope.stmts[i], new_scope_id, file_id);
			}
		}
		break;
		case Ast_If:
		case Ast_While:
		{
			prewalk_stmt(f, stmt->cond.body, scope_id, file_id);
		}
		break;
		case Ast_For:
		{
			int new_scope_id = insert_scope(f, Scope_Function, scope_id, file_id, 0, nullptr);
			stmt->cond.scope_id = new_scope_id;
			prewalk_stmt(f, stmt->cond.body, new_scope_id, file_id);
		}
		break;
		case Ast_Directive_Load:
		case Ast_Binary:
		case Ast_Call:
		case Ast_Type_Info:
			break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return true;
	}

	bool type_check_call(Front_End& f, Ast_Node** reference, int scope_id, Expr_Val_Map& expr_values, int index)
	{
		Ast_Node* node = *reference;
		Ast_Node_Call& call = node->call;

		Expr_Value& callee = expr_values[call.callee];

		ASSERT(callee.referenced_entity);

		Entity* fn = &get_entity(f, callee.referenced_entity);
		int fn_id = callee.referenced_entity;
		GS_Type* signature = fn->fn.signature;

		bool c_varargs = false;

		if (fn)
		{
			c_varargs = fn->fn.c_varargs;
		}

		if (call.args.count < signature->proc.params.count)
		{
			push_error_scope(f, node, scope_id, FMT("too few argument for call: (needed: {}, given: {}): {}", signature->proc.params.count, call.args.count, print_type(signature)));
			return {};
		}

		if (call.args.count > signature->proc.params.count && !c_varargs)
		{
			push_error_scope(f, node->call.args[node->call.args.count - 1], scope_id, FMT("too many argument for call: (needed: {}, given: {}): {}", signature->proc.params.count, call.args.count, print_type(signature)));
			return {};
		}

		for (int i = 0; i < call.args.count; i++)
		{
			Ast_Node* argument = call.args[i];
			Expr_Value& value = expr_values[argument];

			if (i < signature->proc.params.count) {

				GS_Type* param_type = signature->proc.params[i];

				try_promote(f, param_type, value);

				if (value.type != param_type) {
					push_error_scope(f, argument, scope_id, FMT("argument type mismatch: (needed: '{}', given: '{}')", print_type(param_type), print_type(value.type)));
					return false;
				}
			}
		}

		return true;
	}

	GS_Type* expr_get_type(Front_End& f, int scope_id, Ast_Node* expr, Expr_Value& value)
	{
		if (value.type != f.Type_Ty)
		{
			push_error_scope(f, expr, scope_id, FMT("expected expression to be a type"));
			return false;
		}

		return get_type_at(value.value.s8);
	}

	bool type_check_pass(Front_End& f, Array<Flat_Node>& flat_ast, int scope_id, Expr_Val_Map& expr_values, int& index, Entity_Deps& deps, bool const_eval)
	{
		for (; index < flat_ast.count; index++)
		{
			Flat_Node ref = flat_ast[index];
			Ast_Node* node = *ref.reference;

			scope_id = ref.scope_id;

			Expr_Value& my_value = expr_values[node];

			switch (node->type)
			{
			case Ast_Ident:
			{
				int ident_entity_id = find_entity(f, node->token.name, scope_id);

				if (!ident_entity_id)
				{
					push_error_scope(f, node, scope_id, FMT("undeclared identifier: '{}'", node->token.name->str));
					return false;
				}

				Entity& ident_entity = get_entity(f, ident_entity_id);

				bool is_incomplete = !(ident_entity.flags & Flag_Complete);

				if (ident_entity.kind == Entity_Function) {
					is_incomplete = !ident_entity.fn.header_complete;
				}

				if (is_incomplete)
				{
					DBG(GS_CORE_TRACE("  wait on '{}', id: {}", node->token.name->str, ident_entity_id));
					insert_dep(f, ident_entity_id, deps);
					return true;
				}

				my_value.referenced_entity = ident_entity_id;

				if (ident_entity.kind == Entity_Variable)
				{
					my_value.type = ident_entity.type;

					if (const_eval)
					{
						push_error_scope(f, node, scope_id, FMT("variable is not constant: '{}'", node->token.name->str));
						return false;
					}
				}
				else if (ident_entity.kind == Entity_Constant)
				{
					my_value.type = ident_entity.type;
					my_value.value = ident_entity.cnst.value;
				}
				else if (ident_entity.kind == Entity_TypeName)
				{
					my_value.type = f.Type_Ty;
					my_value.value.s4 = get_type_index(get_type(ident_entity.tn.type_name_id));
				}
				else if (ident_entity.kind == Entity_Struct)
				{
					my_value.type = f.Type_Ty;
					my_value.value.s4 = get_type_index(get_type(ident_entity._struct.typename_id));
				}
				else if (ident_entity.kind == Entity_Enum)
				{
					my_value.type = f.Type_Ty;
					my_value.value.s4 = get_type_index(get_type(ident_entity.enm.typename_id));
				}
				else if (ident_entity.kind == Entity_Function)
				{
					my_value.type = f.Type_Ty;
					my_value.value.s4 = get_type_index(ident_entity.fn.signature);
				}
				else
				{
					GS_ASSERT_UNIMPL();
				}
			}
			break;
			case Ast_Numeric:
			{
				my_value.is_unsolid = true;

				if (node->num.is_float)
				{
					my_value.value.f64 = node->num.floating;
					my_value.is_unsolid_float = true;
					my_value.type = f.float_Ty;
				}
				else
				{
					my_value.value.s8 = node->num.integer;
					my_value.type = f.int_Ty;
				}
			}
			break;
			case Ast_Binary:
			{
				Const_Union value = {};

				Tk_Operator op = tk_to_operator(node->token.type);

				GS_Type* type = nullptr;

				Expr_Value& lhs_value = expr_values[node->bin.lhs];
				Expr_Value& rhs_value = expr_values[node->bin.rhs];

				GS_Type* most_complete_type = nullptr;

				if (lhs_value.is_unsolid) {
					if (lhs_value.is_unsolid_float)
						most_complete_type = lhs_value.type;
				}
				else
				{
					most_complete_type = lhs_value.type;
				}

				if (rhs_value.is_unsolid) {
					if (rhs_value.is_unsolid_float)
						most_complete_type = rhs_value.type;
				}
				else
				{
					most_complete_type = rhs_value.type;
				}

				if (most_complete_type)
				{
					try_promote(f, most_complete_type, lhs_value);
					try_promote(f, most_complete_type, rhs_value);
				}

				if (lhs_value.type != rhs_value.type) {
					push_error_scope(f, node, scope_id, FMT("type mismatch: '{}' {} '{}'", print_type(lhs_value.type), operator_to_str(op), print_type(rhs_value.type)));
					return {};
				}

				bool is_range = op == Op_Range;

				type = lhs_value.type;
				auto type_flags = get_type_flags(type);

				bool is_assign = op == Op_Assign;

				bool numeric_op = op == Op_Add || op == Op_Sub || op == Op_Mul || op == Op_Div;
				bool comparative_op = op == Op_Eq || op == Op_NotEq || op == Op_Lesser || op == Op_Greater;

				bool is_integer = !(type_flags & TN_Float_Type);
				bool is_signed = !(type_flags & TN_Unsigned_Type);
				bool is_numeric = type_flags & TN_Numeric_Type;

				if ((numeric_op || is_range) && !is_numeric) {
					push_error_scope(f, node, scope_id, FMT("types must be numeric: '{}' {} '{}'", print_type(lhs_value.type), operator_to_str(op), print_type(rhs_value.type)));
					return false;
				}

				if (is_assign || is_range)
				{
				}
				else
				{
					switch (op)
					{
					case Op_Add:
					{
						if (is_integer)
						{
							value.s8 = lhs_value.value.s8 + rhs_value.value.s8;
						}
						else
						{
							value.f64 = lhs_value.value.f64 + rhs_value.value.f64;
						}
					}
					break;
					case Op_Sub:
					{
						if (is_integer)
						{
							value.s8 = lhs_value.value.s8 - rhs_value.value.s8;
						}
						else
						{
							value.f64 = lhs_value.value.f64 - rhs_value.value.f64;
						}
					}
					break;
					case Op_Mul:
						if (is_integer)
						{
							value.s8 = lhs_value.value.s8 * rhs_value.value.s8;
						}
						else
						{
							value.f64 = lhs_value.value.f64 * rhs_value.value.f64;
						}
						break;
					case Op_Div:

						if (is_integer)
						{
							if (is_signed)
								value.s8 = lhs_value.value.s8 / rhs_value.value.s8;
							else
								value.us8 = lhs_value.value.us8 / rhs_value.value.us8;
						}
						else
						{
							value.f64 = lhs_value.value.f64 / rhs_value.value.f64;
						}
						break;
					case Op_Eq:

						if (is_integer)
						{
							value.us1 = lhs_value.value.us8 == rhs_value.value.us8;
						}
						else
						{
							value.us1 = lhs_value.value.f64 == rhs_value.value.f64;
						}
						break;
					case Op_NotEq:
						if (is_integer)
						{
							value.us1 = lhs_value.value.us8 != rhs_value.value.us8;
						}
						else
						{
							value.us1 = lhs_value.value.f64 != rhs_value.value.f64;
						}
						break;
					case Op_Lesser:
						if (is_integer)
						{
							value.us1 = lhs_value.value.us8 < rhs_value.value.us8;
						}
						else
						{
							value.us1 = lhs_value.value.f64 < rhs_value.value.f64;
						}
						break;
					case Op_Greater:
						if (is_integer)
						{
							value.us1 = lhs_value.value.us8 > rhs_value.value.us8;
						}
						else
						{
							value.us1 = lhs_value.value.f64 > rhs_value.value.f64;
						}
						break;
					default:
						GS_ASSERT_UNIMPL();
						break;
					}
				}

				my_value.value = value;
				my_value.type = type;

				if (comparative_op)
				{
					my_value.type = f.bool_Ty;
				}

				if (lhs_value.is_unsolid && rhs_value.is_unsolid && numeric_op)
				{
					my_value.is_unsolid = true;
					my_value.is_unsolid_float = !is_integer;
				}
			}
			break;
			case Ast_Pointer:
			{
				Expr_Value& expr = expr_values[node->un.expr];

				if (expr.type != f.Type_Ty) {
					push_error_scope(f, node->un.expr, scope_id, FMT("expression must be of type 'Type': {}", print_type(expr.type)));
					return {};
				}

				my_value.value.s4 = get_type_index(get_type_at(expr.value.s4)->get_pointer());
				my_value.type = f.Type_Ty;
			}
			break;
			case Ast_Array_Type:
			{
				Expr_Value& elem = expr_values[node->array_type.elem];

				if (elem.type != f.Type_Ty) {
					push_error_scope(f, node->array_type.elem, scope_id, FMT("expression must be of type 'Type': {}", print_type(elem.type)));
					return {};
				}

				if (node->array_type.dynamic) {
					my_value.value.s4 = get_type_index(get_type_at(elem.value.s4)->get_dynarray());
				}
				else {

					Expr_Value& size = expr_values[node->array_type.size];

					if (!(get_type_flags(size.type) & TN_Numeric_Type)) {
						push_error_scope(f, node->array_type.size, scope_id, FMT("expression must have numeric type: {}", print_type(size.type)));
						return {};
					}

					if (size.value.s8 <= 0) {
						push_error_scope(f, node->array_type.size, scope_id, FMT(" array size must be greater than zero {}", size.value.s8));
						return false;
					}

					my_value.value.s4 = get_type_index(get_type_at(elem.value.s4)->get_array(size.value.s8));
				}

				my_value.type = f.Type_Ty;
			}
			break;
			case Ast_String:
			{
				my_value.type = f.string_Ty;
				my_value.is_unsolid = true;
			}
			break;
			case Ast_Call:
			{
				if (!type_check_call(f, ref.reference, scope_id, expr_values, index)) {
					return false;
				}
			}
			break;
			case Ast_Variable:
			{
				String_Atom* name = node->token.name;

				int previous_declaration = find_entity(f, node->token.name, scope_id);

				if (previous_declaration)
				{
					push_error_scope(f, node, scope_id, FMT("already declared: '{}'", name->str));
					return false;
				}

				int entity_id = insert_entity(f, make_entity(Entity_Variable, name, node), scope_id);
				Entity& entity = get_entity(f, entity_id);

				if (node->var.type) {

					entity.type = expr_get_type(f, scope_id, node->var.type, expr_values[node->var.type]);

					if (!entity.type)
						return false;
				}

				if (node->var.assignment) {

					Expr_Value& assignment = expr_values[node->var.assignment];

					if (!entity.type)
					{
						entity.type = assignment.type;
					}
					else {

						try_promote(f, entity.type, assignment);

						if (entity.type != assignment.type)
						{
							push_error_scope(f, entity.syntax->var.type, scope_id, FMT("type mismatch in assignment '{}' = '{}'", print_type(entity.type), print_type(assignment.type)));
							return false;
						}
					}
				}

				node->var.entity_id = entity_id;

				entity.flags = Flag_Complete;
			}
			break;
			case Ast_Member:
			{
				ASSERT(!const_eval);

				Ast_Node_Member& mem = node->mem;
				Expr_Value& expr = expr_values[mem.expr];

				Entity* entity = nullptr;

				if (expr.referenced_entity)
					entity = &get_entity(f, expr.referenced_entity);

				bool name_spaced = false;

				if (entity && entity->kind != Entity_Variable)
				{
					name_spaced = true;
				}

				if (name_spaced)
				{
					my_value.member.is_constant = true;

					int search_scope_id = 0;

					switch (entity->kind)
					{
					case Entity_Enum:
						search_scope_id = entity->enm.scope_id;
						break;
					case Entity_Struct:
						search_scope_id = entity->_struct.scope_id;
						break;
					default:
						push_error_scope(f, node->mem.expr, scope_id, FMT("'{}' does not support members", entity->name->str));
						return false;
					}

					int member = find_entity(f, mem.member.name, search_scope_id);

					if (!member)
					{
						push_error_scope(f, mem.member, scope_id, FMT("'{}' does have member named: '{}'", entity->name->str, mem.member.name->str));
						return false;
					}

					Entity& member_entity = get_entity(f, member);

					switch (member_entity.kind)
					{
					case Entity_Enum_Member:
					{
						my_value.value = member_entity.enum_mem.value;
						my_value.type = member_entity.type;
					}
					break;
					default:
						GS_ASSERT_UNIMPL();
					}

					my_value.referenced_entity = member;
				}
				else
				{
					int code_id = -1;

					GS_Type* type = expr.type;

					if (expr.type->kind == Type_Pointer && expr.type->pointer.indirection == 1) {
						type = expr.type->pointer.pointee;
						my_value.member.is_ptr_access = true;
					}

					if (get_type_flags(type) & TN_Struct_Type)
					{
						int struct_entity_id = f.typename_to_struct.at(type->basic.type_name_id);
						int struct_scope_id = get_entity(f, struct_entity_id)._struct.scope_id;

						int member = find_entity(f, mem.member.name, struct_scope_id);

						if (!member)
						{
							push_error_scope(f, mem.member, scope_id, FMT("struct '{}' does have member named: '{}'", print_type(type), mem.member.name->str));
							return {};
						}

						Entity& member_entity = get_entity(f, member);

						if (member_entity.kind != Entity_Struct_Member)
						{
							push_error_scope(f, mem.member, scope_id, FMT("struct '{}' member: '{}' is not a data member", print_type(type), mem.member.name->str));
							return {};
						}

						my_value.type = member_entity.type;
						my_value.value.s4 = member_entity.struct_mem.index;
					}
					else
					{
						push_error_scope(f, node->mem.expr, scope_id, FMT("type does not support members: '{}'", print_type(type)));
						return {};
					}
				}
			}
			break;
			case Ast_Cast:
			{
				ASSERT(!const_eval);

				Ast_Node_Binary& bin = node->bin;

				Expr_Value& type = expr_values[bin.lhs];
				Expr_Value& expr = expr_values[bin.rhs];

				GS_Type* to_type = expr_get_type(f, scope_id, bin.lhs, expr_values[bin.lhs]);
				GS_Type* from_type = expr.type;

				bool to_is_pointer = to_type->kind == Type_Pointer;
				bool from_is_pointer = from_type->kind == Type_Pointer;

				if (to_is_pointer && from_is_pointer)
				{
					my_value.type = to_type;
					my_value.cast_type = Il_Cast_Ptr;
				}
				else
				{
					GS_ASSERT_UNIMPL();
				}
			}
			break;
			case Ast_Ref:
			{
				ASSERT(!const_eval);

				Ast_Node_Unary& un = node->un;

				Expr_Value& expr = expr_values[un.expr];

				my_value.type = expr.type->get_pointer();
			}
			break;
			case Ast_Char:
			{
				std::string processed_literal;

				for (size_t i = 0; i < node->token.name->str.count; i++)
				{
					auto c = node->token.name->str[i];

					if (c == '\\') {
						if (i + 1 < node->token.name->str.count) {

							auto next_c = node->token.name->str[i + 1];

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

				my_value.value.s4 = processed_literal[0];

				my_value.type = f.u8_Ty;
				my_value.is_unsolid = true;
			}
			break;
			case Ast_If:
			case Ast_While:
			{
				Expr_Value& expr = expr_values[node->cond.condition];

				try_promote(f, f.bool_Ty, expr);

				if (expr.type != f.bool_Ty) {
					push_error_scope(f, node->cond.condition, scope_id, FMT("expression must be of type 'bool': {}", print_type(expr.type)));
					return false;
				}
			}
			break;
			case Ast_For:
			{
				Expr_Value& cond = expr_values[node->cond.condition];

				String_Atom* it_name = get_atom("it");
				String_Atom* it_index_name = get_atom("it_index");

				my_value._for.it = insert_entity(f, make_entity(Entity_Variable, it_name, nullptr), node->cond.scope_id);
				my_value._for.it_index = insert_entity(f, make_entity(Entity_Variable, it_index_name, nullptr), node->cond.scope_id);

				if (node->cond.condition->token.type == Tk_Range)
				{
					my_value._for.it_type = cond.type;
					my_value._for.it_index_type = cond.type;
				}
				else if (cond.type->kind == Type_Array || cond.type->kind == Type_Dyn_Array || cond.type == f.string_Ty)
				{
					if (cond.type->kind == Type_Array)
						my_value._for.it_type = cond.type->array.element_type;
					else if (cond.type->kind == Type_Dyn_Array)
						my_value._for.it_type = cond.type->dyn_array.element_type;
					else if (cond.type == f.string_Ty)
						my_value._for.it_type = f.c_str_Ty;

					my_value._for.it_index_type = f.u64_Ty;
				}
				else {
					push_error_scope(f, node->cond.condition, scope_id, FMT("type '{}' is not iterable", print_type(cond.type)));
					return false;
				}

				Entity& it = get_entity(f, my_value._for.it);
				Entity& it_index = get_entity(f, my_value._for.it_index);

				it.flags = Flag_Complete;
				it_index.flags = Flag_Complete;

				it.type = my_value._for.it_type;
				it_index.type = my_value._for.it_index_type;
			}
			break;
			case Ast_Type_Info:
			{
				ASSERT(!const_eval);

				if (!(get_entity(f, f.TypeInfo_entity_id).flags & Flag_Complete)) {
					insert_dep(f, f.TypeInfo_entity_id, deps);
					return true;
				}

				Expr_Value& expr = expr_values[node->un.expr];

				if (expr.type != f.Type_Ty)
				{
					push_error_scope(f, node->un.expr, scope_id, FMT("expected expression to be a type"));
					return false;
				}

				my_value.type = f.TypeInfo_Ty->get_pointer();
			}
			break;
			default:
				GS_ASSERT_UNIMPL();
				break;
			}
		}

		return true;
	}

	struct Iterator_Result
	{
		int it_code_id;
		int it_index_code_id;
		GS_Type* it_type;
		GS_Type* it_index_type;
	};

	bool code_gen_pass(Front_End& f, int scope_id, Ast_Node* node, Expr_Val_Map& expr_values, Il_Proc& proc, bool lval = false);

	Iterator_Result iterator_code_gen(Front_End& f, int scope_id, Ast_Node* node, Expr_Val_Map& expr_values, Il_Proc& proc, int before_condition_block, int condition_block, int after_body_block)
	{
		Expr_Value& my_value = expr_values[node];

		switch (node->type)
		{
		case Ast_Binary:
		{
			il_set_insert_point(proc, before_condition_block);

			code_gen_pass(f, scope_id, node->bin.lhs, expr_values, proc, true);
			code_gen_pass(f, scope_id, node->bin.rhs, expr_values, proc);

			Expr_Value& lhs = expr_values[node->bin.lhs];
			Expr_Value& rhs = expr_values[node->bin.rhs];

			Expr_Value& condition = expr_values[node];

			GS_Type* range_type = condition.type;
			auto range_type_flags = get_type_flags(range_type);

			Il_IDX it_alloca = il_insert_alloca(proc, range_type);
			Il_IDX it_index_alloca = il_insert_alloca(proc, range_type);

			il_insert_store(proc, range_type, it_alloca, lhs.code_id);
			il_insert_store(proc, range_type, it_index_alloca, il_insert_constant(proc, (void*)0, range_type));

			il_set_insert_point(proc, condition_block);

			Il_IDX it_load = il_insert_load(proc, range_type, it_alloca);
			Il_IDX cmp_node_idx = il_insert_compare(proc, Il_Value_Cmp, Il_Cmp_Lesser, range_type, it_load, rhs.code_id);

			il_set_insert_point(proc, after_body_block);

			it_load = il_insert_load(proc, range_type, it_alloca);
			il_insert_store(proc, range_type, it_alloca, il_insert_math_op(proc, range_type, Il_Add, it_load, il_insert_constant(proc, (void*)1, range_type)));

			Il_IDX it_index_load = il_insert_load(proc, range_type, it_index_alloca);
			il_insert_store(proc, range_type, it_index_alloca, il_insert_math_op(proc, range_type, Il_Add, it_index_load, il_insert_constant(proc, (void*)1, range_type)));

			my_value.code_id = cmp_node_idx;

			Iterator_Result result;
			result.it_type = range_type;
			result.it_index_type = range_type;
			result.it_index_code_id = it_index_alloca;
			result.it_code_id = it_alloca;

			return result;
		}
		break;
		default:
		{
			code_gen_pass(f, scope_id, node, expr_values, proc, true);

			Expr_Value& expr = expr_values[node];

			GS_Type* it_type = nullptr;

			if (expr.type->kind == Type_Array) {
				it_type = expr.type->array.element_type;
			}
			else if (expr.type->kind == Type_Dyn_Array) {
				it_type = expr.type->dyn_array.element_type;
			}
			else if (expr.type == f.string_Ty) {
				it_type = f.u8_Ty;
			}

			GS_Type* it_index_type = f.u64_Ty;

			il_set_insert_point(proc, before_condition_block);

			Il_IDX it_alloca = il_insert_alloca(proc, it_type);
			Il_IDX it_index_alloca = il_insert_alloca(proc, it_index_type);
			il_insert_store(proc, it_index_type, it_index_alloca, il_insert_constant(proc, (void*)0, it_index_type));

			il_set_insert_point(proc, condition_block);

			auto string_ty_ptr = f.string_Ty->get_pointer();
			auto array_ty_ptr = f.Array_Ty->get_pointer();

			Il_IDX end = -1;
			if (expr.type->kind == Type_Array)
				end = il_insert_constant(proc, (void*)expr.type->array.size, it_index_type);
			else if (expr.type == f.string_Ty) {
				Il_IDX sep = il_insert_sep(proc, f.Array_Ty, 0, il_insert_cast(proc, Il_Cast_Ptr, array_ty_ptr, string_ty_ptr, expr.code_id));
				end = il_insert_load(proc, it_index_type, sep);
			}
			else
			{
				end = il_insert_load(proc, it_index_type, il_insert_sep(proc, f.Array_Ty, 0, expr.code_id));
			}

			Il_IDX it_index_load = il_insert_load(proc, it_index_type, it_index_alloca);

			Il_IDX array_ptr;

			GS_Type* it_type_pointer = it_type->get_pointer();

			if (expr.type->kind == Type_Array)
				array_ptr = expr.code_id;
			else if (expr.type == f.string_Ty)
			{
				array_ptr = il_insert_load(proc, it_type_pointer, il_insert_cast(proc, Il_Cast_Ptr, array_ty_ptr, string_ty_ptr, il_insert_sep(proc, f.Array_Ty, 1, expr.code_id)));
			}
			else
			{
				array_ptr = il_insert_load(proc, it_type_pointer, il_insert_sep(proc, f.Array_Ty, 1, expr.code_id));
			}

			Il_IDX array_element_ptr = il_inser_aep(proc, it_type, array_ptr, it_index_load);
			il_insert_store(proc, it_type, it_alloca, il_insert_load(proc, it_type, array_element_ptr));

			Il_IDX cmp_node_idx = il_insert_compare(proc, Il_Value_Cmp, Il_Cmp_Lesser, it_index_type, it_index_load, end);

			il_set_insert_point(proc, after_body_block);

			it_index_load = il_insert_load(proc, it_index_type, it_index_alloca);
			il_insert_store(proc, it_index_type, it_index_alloca, il_insert_math_op(proc, it_index_type, Il_Add, it_index_load, il_insert_constant(proc, (void*)1, it_index_type)));

			my_value.code_id = cmp_node_idx;

			Iterator_Result result;
			result.it_type = it_type;
			result.it_index_type = it_index_type;
			result.it_code_id = it_alloca;
			result.it_index_code_id = it_index_alloca;

			return result;
		}
		break;
		}

		return {};
	}

	bool code_gen_pass(Front_End& f, int scope_id, Ast_Node* node, Expr_Val_Map& expr_values, Il_Proc& proc, bool lval)
	{
		Expr_Value& my_value = expr_values[node];

		switch (node->type)
		{
		case Ast_String:
		{
			std::string processed_literal;

			for (size_t i = 0; i < node->token.name->str.count; i++)
			{
				auto c = node->token.name->str[i];

				if (c == '\\') {
					if (i + 1 < node->token.name->str.count) {

						auto next_c = node->token.name->str[i + 1];

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

			int string_code_id = il_insert_string(proc, type, literal_value);

			if (my_value.type != f.c_str_Ty)
			{
				type = f.string_Ty;

				Il_IDX struct_members[2];

				Const_Union count_const;
				count_const.us8 = literal_value.count;

				struct_members[0] = il_insert_constant(proc, count_const, f.u64_Ty);
				struct_members[1] = string_code_id;

				string_code_id = il_insert_si(proc, type, Array_View((Il_IDX*)struct_members, 2));

				if (lval) {
					Il_IDX lvalue_alloca = il_insert_alloca(proc, type);
					il_insert_store(proc, type, lvalue_alloca, string_code_id);
					string_code_id = lvalue_alloca;
				}
			}

			my_value.code_id = string_code_id;
		}
		break;
		case Ast_Call:
		{
			Ast_Node_Call& call = node->call;
			Expr_Value& callee = expr_values[call.callee];

			ASSERT(callee.referenced_entity);

			Entity* fn = &get_entity(f, callee.referenced_entity);
			int fn_id = callee.referenced_entity;
			GS_Type* signature = fn->fn.signature;

			Array<int> arguments;
			Array<GS_Type*> argument_types;
			Array<GS_Type*> code_argument_types;

			for (int i = 0; i < call.args.count; i++)
			{
				Ast_Node* argument = call.args[i];

				Expr_Value& value = expr_values[argument];

				code_gen_pass(f, scope_id, argument, expr_values, proc, is_type_aggr(value.type));

				if (is_type_aggr(value.type)) {
					Array_Add(code_argument_types, value.type->get_pointer());
					Array_Add(arguments, value.code_id);
				}
				else
				{
					Array_Add(code_argument_types, value.type);
					Array_Add(arguments, value.code_id);
				}


				Array_Add(argument_types, value.type);
			}

			my_value.code_id = il_insert_call(proc, get_proc_type(signature->proc.return_type, code_argument_types), arguments, fn->fn.proc_id);
		}
		break;
		case Ast_Numeric:
		case Ast_Char:
		{
			my_value.code_id = il_insert_constant(proc, my_value.value, my_value.type);
		}
		break;
		case Ast_Binary:
		{
			Tk_Operator op = tk_to_operator(node->token.type);

			Expr_Value& lhs = expr_values[node->bin.lhs];
			Expr_Value& rhs = expr_values[node->bin.rhs];

			if (op == Op_Assign)
			{
				code_gen_pass(f, scope_id, node->bin.lhs, expr_values, proc, true);
				code_gen_pass(f, scope_id, node->bin.rhs, expr_values, proc);

				Expr_Value& lhs = expr_values[node->bin.lhs];
				Expr_Value& rhs = expr_values[node->bin.rhs];

				il_insert_store(proc, my_value.type, lhs.code_id, rhs.code_id);
			}
			else if (my_value.is_unsolid) {
				my_value.code_id = il_insert_constant(proc, my_value.value, my_value.type);
			}
			else
			{
				bool comparative_op = op == Op_Eq || op == Op_NotEq || op == Op_Lesser || op == Op_Greater;

				code_gen_pass(f, scope_id, node->bin.lhs, expr_values, proc);
				code_gen_pass(f, scope_id, node->bin.rhs, expr_values, proc);

				if (!comparative_op)
				{
					Il_Node_Type op_type;

					switch (op)
					{
					case Op_Add: op_type = Il_Add; break;
					case Op_Sub: op_type = Il_Sub; break;
					case Op_Mul: op_type = Il_Mul; break;
					case Op_Div: op_type = Il_Div; break;
					default:
						GS_ASSERT_UNIMPL();
						break;
					}

					my_value.code_id = il_insert_math_op(proc, my_value.type, op_type, lhs.code_id, rhs.code_id);
				}
				else
				{
					Il_Cmp_Type comp_type;

					switch (op)
					{
					case Op_Eq: comp_type = Il_Cmp_Equal; break;
					case Op_NotEq: comp_type = Il_Cmp_NotEqual; break;
					case Op_Lesser: comp_type = Il_Cmp_Lesser; break;
					case Op_Greater: comp_type = Il_Cmp_Greater; break;
					default:
						GS_ASSERT_UNIMPL();
						break;
					}

					my_value.code_id = il_insert_compare(proc, Il_Value_Cmp, comp_type, lhs.type, lhs.code_id, rhs.code_id);
				}
			}
		}
		break;
		case Ast_Variable:
		{
			if (node->var.is_constant)
				return true;

			int entity_id = node->var.entity_id;
			Entity& entity = get_entity(f, entity_id);
			entity.var.code_id = il_insert_alloca(proc, entity.type);

			if (node->var.assignment) {
				code_gen_pass(f, scope_id, node->var.assignment, expr_values, proc);
				Expr_Value& assignment = expr_values[node->var.assignment];
				il_insert_store(proc, entity.type, entity.var.code_id, assignment.code_id);
			}
			else {
				if (get_type_flags(entity.type) & TN_Struct_Type || entity.type->kind == Type_Array) {
					il_insert_store(proc, entity.type, entity.var.code_id, il_insert_zero_init(proc, entity.type));
				}
				else
				{
					il_insert_store(proc, entity.type, entity.var.code_id, il_insert_constant(proc, 0, entity.type));
				}
			}
		}
		break;
		case Ast_Ident:
		{
			Entity& entity = get_entity(f, my_value.referenced_entity);

			if (entity.kind == Entity_Variable)
			{
				my_value.code_id = entity.var.code_id;

				if (entity.var.big_parameter)
				{
					my_value.code_id = il_insert_load(proc, my_value.type->get_pointer(), my_value.code_id);
				}

				if (!lval) {
					my_value.code_id = il_insert_load(proc, my_value.type, my_value.code_id);
				}
			}
			else if (entity.kind == Entity_Constant)
			{
				my_value.code_id = il_insert_constant(proc, my_value.value, my_value.type);
			}
			else if (entity.kind == Entity_Enum || entity.kind == Entity_Struct || entity.kind == Entity_TypeName)
			{
				my_value.code_id = il_insert_constant(proc, my_value.value, my_value.type);
			}
			else
			{
				GS_ASSERT_UNIMPL();
			}
		}
		break;
		case Ast_Member:
		{
			if (my_value.member.is_constant)
			{
				my_value.code_id = il_insert_constant(proc, my_value.value, my_value.type);
			}
			else
			{
				Expr_Value& expr = expr_values[node->mem.expr];
				code_gen_pass(f, scope_id, node->mem.expr, expr_values, proc, !my_value.member.is_ptr_access);

				GS_Type* type = expr.type;

				if (my_value.member.is_ptr_access)
				{
					type = expr.type->pointer.pointee;
				}

				int code_id = il_insert_sep(proc, type, my_value.value.s4, expr.code_id);

				if (!lval) {
					code_id = il_insert_load(proc, my_value.type, code_id);
				}

				my_value.code_id = code_id;
			}
		}
		break;
		case Ast_Ref:
		{
			code_gen_pass(f, scope_id, node->un.expr, expr_values, proc, true);
			my_value.code_id = expr_values[node->un.expr].code_id;
		}
		break;
		case Ast_Cast:
		{
			code_gen_pass(f, scope_id, node->bin.rhs, expr_values, proc);

			Expr_Value& expr = expr_values[node->bin.rhs];

			GS_Type* to_type = my_value.type;
			GS_Type* from_type = expr.type;

			if (to_type == from_type || my_value.cast_type == Il_Cast_Ptr)
			{
				my_value.code_id = expr.code_id;
			}
			else
			{
				my_value.code_id = il_insert_cast(proc, expr.cast_type, to_type, from_type, expr.code_id);
			}
		}
		break;
		case Ast_Function:
		case Ast_Struct:
			break;
		case Ast_Scope:
		{
			for (size_t i = 0; i < node->scope.stmts.count; i++)
			{
				code_gen_pass(f, node->scope.scope_id, node->scope.stmts[i], expr_values, proc);
			}
		}
		break;
		case Ast_If:
		{
			code_gen_pass(f, scope_id, node->cond.condition, expr_values, proc);

			Expr_Value& condition = expr_values[node->cond.condition];

			Il_IDX saved_insert_point = proc.insertion_point;

			Il_IDX else_block_idx = -1;

			Il_IDX body_block_idx = il_insert_block(proc, String_Make("then"));
			code_gen_pass(f, scope_id, node->cond.body, expr_values, proc);

			Il_IDX after_body_insert_point = proc.insertion_point;

			/*
			if (as_if->Else) {
				else_block_idx = Il_Insert_Block(proc, String_Make("else"));
				if (!Statement_CodeGen(as_if->Else->statement, scope_id, proc)) return {};
			}*/

			Il_IDX cont_block_idx = il_insert_block(proc, String_Make("after_if"));

			il_set_insert_point(proc, saved_insert_point);

			/*if (as_if->Else) {
				il_insert_cbr(proc, Data.bool_Ty, condition_result.code_node_id, body_block_idx, else_block_idx);
			}
			else {*/
			il_insert_cbr(proc, f.bool_Ty, condition.code_id, body_block_idx, cont_block_idx);
			//}

			il_set_insert_point(proc, after_body_insert_point);
			il_insert_br(proc, cont_block_idx);

			il_set_insert_point(proc, cont_block_idx);
		}
		break;
		case Ast_While:
		{
			Il_IDX saved_insert_point = proc.insertion_point;

			Il_IDX cond_block_idx = il_insert_block(proc, String_Make("cond"));

			code_gen_pass(f, scope_id, node->cond.condition, expr_values, proc);
			Expr_Value& condition = expr_values[node->cond.condition];

			Il_IDX body_block_idx = il_insert_block(proc, String_Make("body"));
			code_gen_pass(f, scope_id, node->cond.body, expr_values, proc);

			il_insert_br(proc, cond_block_idx);

			Il_IDX cont_block_idx = il_insert_block(proc, String_Make("cont"));

			il_set_insert_point(proc, cond_block_idx);
			il_insert_cbr(proc, f.bool_Ty, condition.code_id, body_block_idx, cont_block_idx);

			il_set_insert_point(proc, cont_block_idx);
		}
		break;
		case Ast_For:
		{
			Il_IDX saved_insert_point = proc.insertion_point;

			Il_IDX cond_block_idx = il_insert_block(proc, String_Make("condition"));
			Il_IDX body_block_idx = il_insert_block(proc, String_Make("body"));
			Il_IDX loop_bottom_block_idx = il_insert_block(proc, String_Make("dummmy"));

			Expr_Value& condition = expr_values[node->cond.condition];

			il_set_insert_point(proc, saved_insert_point);

			Iterator_Result condition_result = iterator_code_gen(f, scope_id, node->cond.condition, expr_values, proc, saved_insert_point, cond_block_idx, loop_bottom_block_idx);

			il_set_insert_point(proc, body_block_idx);

			Entity& it = get_entity(f, my_value._for.it);
			Entity& it_index = get_entity(f, my_value._for.it_index);

			it.var.code_id = condition_result.it_code_id;
			it_index.var.code_id = condition_result.it_index_code_id;

			code_gen_pass(f, scope_id, node->cond.body, expr_values, proc);

			Il_Block& dummy = proc.blocks[loop_bottom_block_idx];
			loop_bottom_block_idx = il_insert_block(proc, String_Make("loop_bottom"));
			Il_Block& after_loop = proc.blocks[loop_bottom_block_idx];

			after_loop.instructions = dummy.instructions;
			dummy.instructions.count = 0;

			il_insert_br(proc, cond_block_idx);

			Il_IDX cont_block_idx = il_insert_block(proc, String_Make("after_loop"));

			il_set_insert_point(proc, cond_block_idx);
			il_insert_cbr(proc, f.bool_Ty, condition.code_id, body_block_idx, cont_block_idx);

			il_set_insert_point(proc, cont_block_idx);
		}
		break;
		case Ast_Type_Info:
		{
			code_gen_pass(f, scope_id, node->un.expr, expr_values, proc);
			my_value.code_id = il_inser_aep(proc, f.typeinfo_entry, il_insert_global_address(proc, f.typeinfo_table_global), expr_values[node->un.expr].code_id);
		}
		break;
		case Ast_Pointer:
		case Ast_Array_Type:
		{
			my_value.code_id = il_insert_constant(proc, my_value.value, f.Type_Ty);
		}
		break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return true;
	}

	bool type_check_entity(Front_End& f, Entity& entity, int entity_id, int scope_id)
	{
		Ast_Node* stmt = entity.syntax;

		switch (entity.kind)
		{
		case Entity_Constant:
		{
			Array<Flat_Node>& flat_ast = entity.flat_syntax;

			Expr_Val_Map& expr_values = *(Expr_Val_Map*)&entity.expr_values;

			if (entity.progress)
			{
				GS_CORE_TRACE("constant '{}' resume eval: progress: {}", entity.name->str, entity.progress);
			}

			bool success = type_check_pass(f, flat_ast, scope_id, expr_values, entity.progress, entity.deps, true);

			if (!success)
				return false;

			if (entity.progress < flat_ast.count - 1) {
				return true;
			}

			if (entity.syntax->var.type)
			{
				Expr_Value type = expr_values[entity.syntax->var.type];

				if (type.type != f.Type_Ty)
				{
					push_error_scope(f, entity.syntax->var.type, scope_id, FMT("expected expression to be a type"));
					return false;
				}

				entity.type = get_type_at(type.value.s8);
			}

			Expr_Value value = expr_values[entity.syntax->var.assignment];

			if (!entity.type) {
				entity.type = value.type;
			}
			else
			{
				try_promote(f, entity.type, value);

				if (entity.type != value.type) {
					push_error_scope(f, entity.syntax->var.type, scope_id, FMT("type mismatch in assignment '{}' :: '{}'", print_type(entity.type), print_type(value.type)));
					return false;
				}
			}

			entity.cnst.value = value.value;
			entity.flags = Flag_Complete;

			DBG(
				GS_CORE_TRACE("checked constant {}: value: {}, type: {}", entity.name->str, entity.cnst.value.s8, print_type(entity.type));
			);
		}
		break;
		case Entity_Variable:
		{
			Array<Flat_Node>& flat_ast = entity.flat_syntax;

			Expr_Val_Map& expr_values = *(Expr_Val_Map*)&entity.expr_values;

			if (entity.progress)
			{
				GS_CORE_TRACE("variable '{}' resume eval: progress: {}", entity.name->str, entity.progress);
			}

			bool success = type_check_pass(f, flat_ast, scope_id, expr_values, entity.progress, entity.deps, true);

			if (!success)
				return false;

			if (entity.progress <= flat_ast.count - 1) {
				return true;
			}

			if (entity.syntax->var.type)
			{
				Expr_Value type = expr_values[entity.syntax->var.type];

				if (type.type != f.Type_Ty)
				{
					push_error_scope(f, entity.syntax->var.type, scope_id, FMT("expected expression to be a type"));
					return false;
				}

				entity.type = get_type_at(type.value.s8);
			}

			entity.flags = Flag_Complete;

			DBG(
				GS_CORE_TRACE("variable {}: type: {}", entity.name->str, print_type(entity.type));
			);
		}
		break;
		case Entity_Struct_Member:
		{
			Array<Flat_Node>& flat_ast = entity.flat_syntax;
			Expr_Val_Map& expr_values = *(Expr_Val_Map*)&entity.expr_values;

			if (entity.progress)
			{
				GS_CORE_TRACE("struct data member '{}' resume eval: progress: {}", entity.name->str, entity.progress);
			}

			bool success = type_check_pass(f, flat_ast, scope_id, expr_values, entity.progress, entity.deps, true);

			if (!success)
				return false;

			if (entity.progress < flat_ast.count) {
				return true;
			}

			if (entity.syntax->var.type)
			{
				Expr_Value type = expr_values[entity.syntax->var.type];

				if (type.type != f.Type_Ty)
				{
					push_error_scope(f, entity.syntax->var.type, scope_id, FMT("expected expression to be a type"));
					return false;
				}

				entity.type = get_type_at(type.value.s8);
			}

			entity.flags = Flag_Complete;

			DBG(
				GS_CORE_TRACE("checked data member {}: type: {}", entity.name->str, print_type(entity.type));
			);
		}
		break;
		case Entity_Struct:
		{
			Scope& my_scope = get_scope(f, entity._struct.scope_id);

			Array<GS_Type*> member_types;

			int index = 0;
			for (size_t i = 0; i < my_scope.entities.count; i++)
			{
				int member_entity_id = my_scope.entities[i];
				Entity& member_entity = get_entity(f, member_entity_id);

				if (member_entity.kind == Entity_Struct_Member) {
					Array_Add(member_types, member_entity.type);
					member_entity.struct_mem.index = index;
					index++;
				}
			}

			f.typename_to_struct[entity._struct.typename_id] = entity_id;
			insert_struct(entity._struct.typename_id, member_types);

			entity.flags = Flag_Complete;

			DBG(

				std::string members;

			for (size_t i = 0; i < member_types.count; i++)
			{
				members += print_type(member_types[i]).data;
				members += " ,";
			}

			GS_CORE_TRACE("checked struct '{}' = {}", entity.name->str, members);
			);
		}
		break;
		case Entity_Enum:
		{
			Scope& my_scope = get_scope(f, entity._struct.scope_id);
			entity.flags = Flag_Complete;
		}
		break;
		case Entity_Enum_Member:
		{
			if (entity.enum_mem.previous_member)
			{
				Entity& previous_member = get_entity(f, entity.enum_mem.previous_member);
				entity.enum_mem.value.s8 = previous_member.enum_mem.value.s8 + 1;
			}
			else
			{
				entity.enum_mem.value.s8 = 0;
				//GS_ASSERT_UNIMPL();
			}

			entity.type = get_entity(f, entity.enum_mem.enum_entity_id).enm.type;

			entity.flags = Flag_Complete;
		}
		break;
		case Entity_Function:
		{
			Scope& my_scope = get_scope(f, entity.fn.scope_id);

			if (stmt->fn.foreign)
			{
				int library = find_entity(f, stmt->fn.foreign, scope_id);

				if (!library)
				{
					push_error_scope(f, stmt, scope_id, FMT("undeclared identifier: '{}'", stmt->fn.foreign->str));
					return false;
				}
				else if (get_entity(f, library).kind != Entity_Library)
				{
					push_error_scope(f, stmt, scope_id, FMT("expected name to be a library: '{}'", stmt->fn.foreign->str));
					return false;
				}

				entity.fn.foreign = library;
			}

			if (entity.progress)
			{
				GS_CORE_TRACE("function '{}' resume: progress: {}", entity.name->str, entity.progress);
			}

			if (!entity.fn.header_complete)
			{
				if (stmt->fn.return_type)
				{
					Array<Flat_Node> return_flat_syntax = entity.flat_syntax;
					return_flat_syntax.count = entity.fn.return_ast_count;

					bool success = type_check_pass(f, return_flat_syntax, entity.fn.scope_id, entity.expr_values, entity.progress, entity.deps, true);

					if (!success)
						return false;

					if (entity.progress < return_flat_syntax.count) {
						return true;
					}

					Expr_Value checked_return = entity.expr_values[stmt->fn.return_type];

					if (checked_return.type != f.Type_Ty) {
						push_error_scope(f, stmt->fn.return_type, scope_id, FMT("expected 'Type' instead got '{}'", print_type(checked_return.type)));
						return false;
					}

					entity.fn.return_type = get_type_at(checked_return.value.s4);
				}
				else
				{
					entity.fn.return_type = f.void_Ty;
				}

				Array<GS_Type*> param_types;
				Array<GS_Type*> code_param_types;

				for (size_t i = 0; i < entity.fn.parameters.count; i++)
				{
					int param_entity_id = entity.fn.parameters[i];
					Entity& param_entity = get_entity(f, param_entity_id);
					Array_Add(param_types, param_entity.type);

					if (is_type_aggr(param_entity.type))
					{
						Array_Add(code_param_types, param_entity.type->get_pointer());
					}
					else
					{
						Array_Add(code_param_types, param_entity.type);
					}
				}

				entity.fn.signature = get_proc_type(entity.fn.return_type, param_types);

				GS_Type* code_signature = get_proc_type(entity.fn.return_type, code_param_types);

				if (entity.fn.foreign) {
					entity.fn.proc_id = il_insert_external_proc(f.program, entity.name->str, code_signature, -1, entity.fn.c_varargs);
				}
				else
				{
					entity.fn.proc_id = il_insert_proc(f.program, entity.name->str, code_signature, entity.fn.c_varargs);
				}

				entity.fn.header_complete = true;

				DBG(
					GS_CORE_TRACE("checked function header {}: type: {}", entity.name->str, print_type(entity.fn.signature));
				);
			}

			if (!entity.fn.foreign)
			{
				bool success = type_check_pass(f, entity.flat_syntax, entity.fn.scope_id, entity.expr_values, entity.progress, entity.deps, false);

				if (!success)
					return false;

				if (entity.progress < entity.flat_syntax.count) {
					return true;
				}

				Il_Proc& proc = f.program.procedures[entity.fn.proc_id];

				for (size_t i = 0; i < entity.fn.parameters.count; i++)
				{
					int parameter_id = entity.fn.parameters[i];
					Entity& parameter = get_entity(f, parameter_id);

					GS_Type* code_type = parameter.type;

					int input_id = proc.parameters[i];
					int alloca_id = 0;

					if (is_type_aggr(parameter.type))
					{
						if (parameter.type->size() <= 8)
						{
							input_id = il_insert_load(proc, code_type, input_id);
						}
						else {
							code_type = parameter.type->get_pointer();
							parameter.var.big_parameter = true;
						}

						alloca_id = il_insert_alloca(proc, code_type);

						il_insert_store(proc, code_type, alloca_id, input_id);
					}
					else
					{
						alloca_id = il_insert_alloca(proc, code_type);
						il_insert_store(proc, code_type, alloca_id, input_id);
					}

					parameter.var.code_type = code_type;
					parameter.var.code_id = alloca_id;
				}

				for (size_t i = 0; i < stmt->fn.body.stmts.count; i++)
				{
					if (!code_gen_pass(f, entity.fn.scope_id, stmt->fn.body.stmts[i], entity.expr_values, proc)) {
						return false;
					}
				}

				il_insert_ret(proc, f.void_Ty, -1);
			}

			entity.flags = Flag_Complete;
		}
		break;
		case Entity_Load:
		case Entity_Library:
			entity.flags = Flag_Complete;
			break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return true;
	}

	bool type_check_pass(Front_End& f)
	{
		for (int file_id = 1; file_id < f.files.count; file_id++)
		{
			Comp_File& file = f.files[file_id];

			for (size_t i = 0; i < file.syntax->scope.stmts.count; i++)
			{
				Ast_Node* tl_stmt = file.syntax->scope.stmts[i];

				if (!prewalk_stmt(f, tl_stmt, f.file_to_scope.at(file_id), file_id))
				{
					return false;
				}
			}
		}

		bool all_resolved = false;

		while (!all_resolved)
		{
			all_resolved = true;

			bool progress = false;

			for (int entity_id = 1; entity_id < f.entities.count; entity_id++)
			{
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
					bool success = type_check_entity(f, entity, entity_id, entity.scope_id);

					if (!success)
						return false;

					if (!(entity.flags & Flag_Complete))
						all_resolved = false;
					else
						progress = true;
				}
			}

			if (!progress)
			{
				GS_CORE_WARN("circular dep detected!");
				all_resolved = true;
			}
		}

		return true;
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
		f.keyword_TypeInfo = get_atom("TypeInfo");

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

		{
			Il_Global struct_member_typeinfo_global;
			struct_member_typeinfo_global.type = f.void_Ty;
			struct_member_typeinfo_global.name = String_Make("__TypeInfo_Members_Array__");
			struct_member_typeinfo_global.initializer = -1;

			f.typeinfo_member_array_global = il_insert_global(f.program, struct_member_typeinfo_global);

			Il_Global type_info_table_global;
			type_info_table_global.type = f.void_Ty;
			type_info_table_global.name = String_Make("__TypeInfo_Table__");
			type_info_table_global.initializer = -1;

			f.typeinfo_table_global = il_insert_global(f.program, type_info_table_global);

			GS_Struct te_struct;

			Type_Name struct_type_name;
			struct_type_name.name = String_Make("TypeInfoEntry");
			struct_type_name.flags = TN_Struct_Type;
			struct_type_name.size = 64;
			struct_type_name.alignment = 0;

			auto te_type_name_id = insert_typename_struct(struct_type_name, te_struct);
			f.typeinfo_entry = get_type(te_type_name_id);

#define TE_MEM_COUNT 8

			for (size_t i = 0; i < TE_MEM_COUNT; i++)
			{
				Array_Add(te_struct.members, f.u64_Ty);
				Array_Add(te_struct.offsets, (u64)-1);
			}

			GS_Struct_Data_Layout te_data_layout = struct_compute_align_size_offsets(te_struct.members);

			for (size_t i = 0; i < te_struct.offsets.count; i++)
			{
				te_struct.offsets[i] = te_data_layout.offsets[i];
			}

			get_ts().type_name_storage[te_type_name_id].size = te_data_layout.size;
			get_ts().type_name_storage[te_type_name_id].alignment = te_data_layout.alignment;
			get_ts().struct_storage[get_ts().type_name_storage[te_type_name_id].struct_id] = te_struct;
		}
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

			Machine_Gen generator = {};
			code_generator_init(generator, f.program, String_Make("./.bin/il.obj"));
			code_generator_run(generator);

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

	bool generate_typeinfo_table(Front_End& f)
	{
		GS_PROFILE_FUNCTION();

		GS_Type* c_str_type = f.c_str_Ty;

		Type_System& ts = get_ts();

		Array<GS_Type> type_storage = ts.type_storage;

		u64 entry_count = ts.type_storage.count;

		GS_Type* te = f.typeinfo_entry;

		auto table_array_ty = te->get_array(entry_count);

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
			String type_info_name = print_type(entry_type);
			u64	type_info_flags = 0;
			u64	type_info_size = entry_type->size();
			u64	type_info_alignment = get_type_alignment(entry_type);

			auto type_flags = get_type_flags(entry_type);

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
			Array_Add(table_initializer, il_make_const((void*)type_info_kind, get_type_index(f.i64_Ty)));

			//STRING name
			Array_Add(te_member_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, il_make_const((void*)type_info_name.count, get_type_index(f.i64_Ty)));

			Array_Add(table_initializer, Il_Make_String(type_info_name, get_type_index(c_str_type)));
			Array_Add(te_member_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, Il_Make_Cast(Il_Cast_Ptr2Int, get_type_index(f.u64_Ty), get_type_index(c_str_type), (Il_IDX)table_initializer.count - 1));

			//u64	FLAGS
			Array_Add(te_member_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, il_make_const((void*)type_info_flags, get_type_index(f.i64_Ty)));

			if (type_info_kind == TEK_BASIC || type_info_kind == TEK_STRUCT || type_info_kind == TEK_NUMERIC)
			{
				u32 size_alignemnt[2] = { 0 };

				size_alignemnt[0] = type_info_size;
				size_alignemnt[1] = type_info_alignment;
				//u64	SIZE/Alignment
				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, il_make_const((void*)*(u64*)&size_alignemnt, get_type_index(f.i64_Ty)));
			}

			if (type_info_kind == TEK_STRUCT) {

				auto& type_name = ts.type_name_storage[entry_type->basic.type_name_id];
				GS_Struct& struct_type = ts.struct_storage[type_name.struct_id];

				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, il_make_const((void*)struct_type.members.count, get_type_index(f.i64_Ty)));

				Array_Add(table_initializer, Il_Make_Global_Address(f.typeinfo_member_array_global, get_type_index(entry_type)));
				Array_Add(table_initializer, il_make_const((void*)members_array_size, get_type_index(f.i64_Ty)));
				Array_Add(table_initializer, Il_Make_AEP(get_type_index(te), (Il_IDX)table_initializer.count - 2, (Il_IDX)table_initializer.count - 1));
				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, Il_Make_Cast(Il_Cast_Ptr2Int, get_type_index(f.u64_Ty), get_type_index(c_str_type), (Il_IDX)table_initializer.count - 1));

				ASSERT(entry_type->kind == Type_Basic);
				int struct_entity_id = f.typename_to_struct[entry_type->basic.type_name_id];

				if (struct_entity_id != 0) {

					Entity& struct_entity = get_entity(f, struct_entity_id);
					Scope& struct_scope = get_scope(f, struct_entity._struct.scope_id);

					for (size_t j = 0; j < struct_scope.entities.count; j++)
					{
						int member_entity_id = struct_scope.entities[j];
						Entity& member_entity = get_entity(f, member_entity_id);

						if (member_entity.kind == Entity_Struct_Member) {

							Array<Il_IDX> member_te_member_values;

							//Kind
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, il_make_const((void*)TEK_STRUCT_MEMBER, get_type_index(f.i64_Ty)));

							//STRING name
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, il_make_const((void*)member_entity.name->str.count, get_type_index(f.i64_Ty)));

							Array_Add(members_array_initializer, Il_Make_String(member_entity.name->str, get_type_index(c_str_type)));
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Cast(Il_Cast_Ptr2Int, get_type_index(f.u64_Ty), get_type_index(c_str_type), (Il_IDX)members_array_initializer.count - 1));

							//u64	Type
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, il_make_const((void*)get_type_index(member_entity.type), get_type_index(f.i64_Ty)));

							//int	Offset
							Array_Add(member_te_member_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, il_make_const((void*)struct_type.offsets[j], get_type_index(f.i64_Ty)));

							Array_Add(members_array_values, (Il_IDX)members_array_initializer.count);
							Array_Add(members_array_initializer, Il_Make_Struct_Init(get_type_index(te), member_te_member_values));
							members_array_size++;
						}
					}
				}
			}

			for (size_t i = 0; i < (TE_MEM_COUNT - te_member_values.count) + 1; i++)
			{
				Array_Add(te_member_values, (Il_IDX)table_initializer.count);
				Array_Add(table_initializer, il_make_const((void*)0, get_type_index(f.i64_Ty)));
			}

			Array_Add(table_array_values, (Il_IDX)table_initializer.count);
			Array_Add(table_initializer, Il_Make_Struct_Init(get_type_index(te), te_member_values));
		}

		Array_Add(table_initializer, Il_Make_Array_Init(get_type_index(table_array_ty), table_array_values));

		Il_Global& table_global = f.program.globals[f.typeinfo_table_global];
		table_global.initializer_storage = table_initializer;
		table_global.initializer = table_initializer.count - 1;
		table_global.type = table_array_ty;

		auto members_array_ty = te->get_array(members_array_size);
		Array_Add(members_array_initializer, Il_Make_Array_Init(get_type_index(members_array_ty), members_array_values));

		Il_Global& members_array_global = f.program.globals[f.typeinfo_member_array_global];
		members_array_global.type = members_array_ty;
		members_array_global.initializer_storage = members_array_initializer;
		members_array_global.initializer = members_array_initializer.count - 1;

		return false;
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

		result = true;

		fs_path first_file_path = f.opts.Files[0];

		result = frontend_do_load(f, first_file_path, std::filesystem::current_path());

		if (!result)
			return;

		result = type_check_pass(f);

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

		generate_typeinfo_table(f);

		frontend_generate_output(f);

		if (!result)
			return;
	}
}
