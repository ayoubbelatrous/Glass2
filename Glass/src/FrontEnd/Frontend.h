#pragma once

#include "Base/String.h"
#include "Base/Array.h"
#include "Base/Hash.h"
#include "Application.h"
#include "FrontEnd/Parser.h"
#include "FrontEnd/Type_System.h"
#include "FrontEnd/Syntax.h"
#include "BackEnd/Il.h"
#include <unordered_set>

namespace Glass
{
	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Timer::Reset()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		float Timer::Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
		}

		float Timer::ElapsedMillis()
		{
			return Elapsed() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

	enum Tk_Operator
	{
		Op_Invalid,

		Op_Add = Tk_Plus,
		Op_Sub = Tk_Minus,
		Op_Mul = Tk_Star,
		Op_Div = Tk_Slash,
		Op_Mod = Tk_Modulo,

		Op_BitAnd = Tk_Ampersand,
		Op_BitOr = Tk_Pipe,

		Op_AddAssign = Tk_PlusAssign,
		Op_SubAssign = Tk_MinusAssign,
		Op_MulAssign = Tk_StarAssign,
		Op_DivAssign = Tk_SlashAssign,

		Op_Eq = Tk_Equal,
		Op_NotEq = Tk_NotEqual,
		Op_Greater = Tk_CloseAngular,
		Op_Lesser = Tk_OpenAngular,

		Op_Assign = Tk_Assign,

		Op_Range = Tk_Range,

		Op_And = Tk_And,
		Op_Or = Tk_Or,
	};

	inline Tk_Operator tk_to_operator(Tk_Type type) {
		return (Tk_Operator)type;
	}

	inline String operator_to_str(Tk_Operator op)
	{
		ASSERT(op != Op_Invalid);

		switch (op)
		{
		case Op_Add: return String_Make("+");
		case Op_Sub: return String_Make("-");
		case Op_Mul: return String_Make("*");
		case Op_Div: return String_Make("/");
		case Op_AddAssign: return String_Make("+=");
		case Op_SubAssign: return String_Make("-=");
		case Op_MulAssign: return String_Make("*=");
		case Op_DivAssign: return String_Make("/=");
		case Op_Eq: return String_Make("==");
		case Op_NotEq: return String_Make("!=");
		case Op_Lesser: return String_Make("<");
		case Op_Greater: return String_Make(">");
		case Op_Assign: return String_Make("=");
		case Op_Range: return String_Make("..");
		case Op_BitAnd: return String_Make("&");
		case Op_BitOr: return String_Make("|");
		case Op_Mod: return String_Make("%");
		default:
			ASSERT_UNIMPL();
			break;
		}

		return String_Make("invalid operator");
	}

	struct Source_Loc
	{
		u32 line;
		u32 column;
	};

	enum Front_End_Message_Type
	{
		Message_Info,
		Message_Warning,
		Message_Error,
	};

	struct Front_End_Message
	{
		Front_End_Message_Type type;
		String msg;
	};

	enum Entity_Kind
	{
		Entity_TypeName,
		Entity_Constant,
		Entity_Variable,
		Entity_Function,
		Entity_Poly_Function,
		Entity_Struct,
		Entity_Poly_Struct,
		Entity_Struct_Member,
		Entity_Enum,
		Entity_Enum_Member,
		Entity_Load,
		Entity_Library,
		Entity_Operator
	};

	enum Entity_Flags
	{
		Flag_Complete = BIT(0)
	};

	inline Entity_Flags operator|(Entity_Flags a, Entity_Flags b)
	{
		return static_cast<Entity_Flags>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline void operator|=(Entity_Flags& a, Entity_Flags b)
	{
		(int&)a |= static_cast<int>(b);
	}

	inline void operator&=(Entity_Flags& a, Entity_Flags b)
	{
		(int&)a &= static_cast<int>(b);
	}

	inline Entity_Flags operator&(Entity_Flags a, Entity_Flags b)
	{
		return static_cast<Entity_Flags>(static_cast<int>(a) & static_cast<int>(b));
	}

	using Entity_Deps = std::unordered_set<int>;

	struct Flat_Node
	{
		Ast_Node** reference;
		int scope_id;
	};

	struct Expr_Value
	{
		GS_Type* type;
		Const_Union value;
		int referenced_entity;
		int code_id;

		bool is_constant : 1;
		bool is_unsolid : 1;
		bool is_unsolid_float : 1;
		bool is_unsolid_null : 1;
		bool lvalue : 1;
		bool promote_to_bool : 1;

		union
		{
			Il_Cast_Type cast_type;
			struct Checked_For
			{
				GS_Type* it_type;
				GS_Type* it_index_type;
				int it;
				int it_index;
			} _for;

			struct Checked_Member
			{
				bool is_constant;
				bool is_ptr_access;
				GS_Type* struct_type;
			} member;

			struct Checked_Call
			{
				bool is_proc_call;
				int proc_id;
				GS_Type* callee_signature;
				bool varargs;
				GS_Type* varargs_type;
				bool is_poly_struct;
			} call;

			struct Checked_Bin
			{
				bool is_overload;
				int proc_idx;
			} bin;
		};
	};

	using Expr_Val_Map = std::unordered_map<Ast_Node*, Expr_Value>;

	struct Entity_TN
	{
		int type_name_id;
	};

	struct Entity_Const
	{
		Const_Union value;
	};

	struct Entity_Ld
	{
		int file_id;
		int file_scope_id;
	};

	struct Entity_Func
	{
		int scope_id;
		int proc_id;
		GS_Type* signature;
		GS_Type* return_type;
		int return_var_id;
		bool c_varargs;
		int foreign;
		Array<int> parameters;
		bool has_varargs;
		int var_arg_parameter;
		bool header_complete;
		int return_ast_count;
	};

	struct Entity_Oper
	{
		int fn_entity_id;
		Tk_Operator op;
		Array<GS_Type*> parameters;
		GS_Type* result_type;
	};

	struct Poly_Decl
	{
		String_Atom* name;
		bool		 is_type;
		int			 parameter_index;
	};

	struct Poly_Instance
	{
		Array_UI<std::tuple<String_Atom*, GS_Type*>> overloads;
		int entity_id;
	};

	struct Entity_Poly_Func
	{
		Array_UI<Poly_Decl> poly_declarations;
		Array_UI<Poly_Instance> instances;
		bool has_varargs;
		int parameter_count;
	};

	struct Parameter_Pair
	{
		GS_Type* type;
		String_Atom* name;
	};

	struct Entity_Strct
	{
		int scope_id;
		int typename_id;
		int instance_of;
		int instance_id;
	};

	struct Poly_Struct_Instance
	{
		Array<Const_Union> overloads;
		int entity_id;
	};

	struct Entity_Poly_Strct
	{
		Array<int> parameter_ids;
		Array<Parameter_Pair> parameters;
		Array<Poly_Struct_Instance> instances;
	};

	struct Entity_Enm
	{
		int scope_id;
		int typename_id;
		GS_Type* type;
		GS_Type* underlying_type;
		int underlying_type_id;
		int previous_member;
	};

	struct Entity_Strct_Member
	{
		int index;
		Const_Union initializer;
		bool		has_initializer;
	};

	struct Entity_Enm_Member
	{
		int index;
		Const_Union value;
		int previous_member;
		int enum_entity_id;
	};

	struct Entity_Lib
	{
		String_Atom* file_name;
	};

	struct Entity_Var
	{
		bool global;
		int code_id;
		GS_Type* code_type;
		bool big_parameter;
		bool parameter;
	};

	struct Entity
	{
		Entity_Kind  kind;
		Entity_Flags flags;
		String_Atom* name;
		int			 scope_id;
		int			 file_id;
		Ast_Node* syntax;

		GS_Type* type;

		Entity_Deps deps;

		Expr_Val_Map expr_values;
		Array<Flat_Node> flat_syntax;

		int progress;

		union
		{
			Entity_TN			tn;
			Entity_Const		cnst;
			Entity_Func			fn;
			Entity_Poly_Func	poly_fn;
			Entity_Strct		_struct;
			Entity_Poly_Strct	poly_struct;
			Entity_Enm			enm;
			Entity_Strct_Member struct_mem;
			Entity_Enm_Member   enum_mem;
			Entity_Ld			load;
			Entity_Lib			library;
			Entity_Var			var;
			Entity_Oper			op;
		};
	};

	enum Scope_Type
	{
		Scope_Invalid,
		Scope_Global,
		Scope_File,
		Scope_Function,
		Scope_Struct,
		Scope_Enum,
	};

	struct Entity_List
	{
		int entity_id;
		Array<int> entities;
	};

	struct Scope
	{
		Scope_Type type;
		Ast_Node* syntax;
		int file_id;
		int parent;
		int entity_id;
		Array<int> children;
		Array<int> entities;
		std::unordered_map<String_Atom*, Entity_List> name_to_entity;
	};

	struct Comp_File
	{
		String_Atom* path;
		String_Atom* path_abs;
		Ast_Node* syntax;
		int scope_id;
	};

	struct Front_End
	{
		ApplicationOptions opts;
		Array<Front_End_Message> messages;

		Array<Scope> scopes;
		Array<Comp_File> files;
		Array<Entity> entities;

		std::unordered_map<String_Atom*, int> filename_to_id;
		std::unordered_map<int, int> file_to_scope;
		std::unordered_map<int, int> typename_to_struct;
		std::unordered_map<int, int> scope_id_to_enum;

		GS_Type* i8_Ty;
		GS_Type* u8_Ty;
		GS_Type* i16_Ty;
		GS_Type* u16_Ty;
		GS_Type* i32_Ty;
		GS_Type* i64_Ty;
		GS_Type* u32_Ty;
		GS_Type* u64_Ty;

		GS_Type* f32_Ty;
		GS_Type* f64_Ty;

		GS_Type* int_Ty;
		GS_Type* float_Ty;

		GS_Type* Type_Ty;

		GS_Type* bool_Ty;

		GS_Type* void_Ty;

		GS_Type* c_str_Ty;

		Il_Program program;

		std::unordered_set<String_Atom*> added_library_paths;

		GS_Type* Array_Ty;
		GS_Type* string_Ty;
		GS_Type* Any_Ty;
		String_Atom* keyword_Array;
		String_Atom* keyword_string;
		String_Atom* keyword_TypeInfo;
		String_Atom* keyword_Any;

		int typeinfo_member_array_global;
		int typeinfo_table_global;
		GS_Type* typeinfo_entry;
		GS_Type* TypeInfo_Ty;

		int TypeInfo_entity_id;
		int Array_entity_id;
		int string_entity_id;
	};

	void frontend_push_error(Front_End& f, String error);
	void frontend_push_error(Front_End& f, Tk& token, String file_path, String error);
	void frontend_push_error(Front_End& f, Ast_Node* stmt, int file_id, String error);

	int find_filescope_parent(Front_End& f, int scope_id);

	int insert_entity(Front_End& f, Entity entity, int scope_id = 0);

	int find_entity(Front_End& f, String_Atom* name, int scope_id = 0);
	int find_entity(Front_End& f, String_Atom* name, int scope_id, int ignore, bool ignore_global);

	Scope& get_scope(Front_End& f, int scope_id);
	Entity& get_entity(Front_End& f, int entity_id);

	Entity make_entity(Entity_Kind type, String_Atom* name, Ast_Node* syntax = nullptr);

	int frontend_do_load(Front_End& f, fs_path file_path, fs_path search_path);

	void frontend_compile(Front_End& f);

	inline String read_entire_file(fs_path& path)
	{
		std::ifstream t(path);
		std::stringstream buffer;
		buffer << t.rdbuf();
		return String_Make(buffer.str());
	}

	inline std::string normalizePath(const std::string& messyPath) {
		std::filesystem::path path(messyPath);
		std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
		std::string npath = canonicalPath.make_preferred().string();
		return npath;
	}

	inline fs_path normalizePath(const fs_path& messyPath) {
		fs_path path(messyPath);
		fs_path canonicalPath = std::filesystem::weakly_canonical(path);
		return canonicalPath.make_preferred();
	}
}
