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
	enum Tk_Operator
	{
		Op_Invalid,

		Op_Add = Tk_Plus,
		Op_Sub = Tk_Minus,
		Op_Mul = Tk_Star,
		Op_Div = Tk_Slash,

		Op_AddAssign = Tk_PlusAssign,
		Op_SubAssign = Tk_MinusAssign,
		Op_MulAssign = Tk_StarAssign,
		Op_DivAssign = Tk_SlashAssign,

		Op_Eq = Tk_Equal,
		Op_NotEq = Tk_NotEqual,

		Op_Assign = Tk_Assign,
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
		case Op_Assign: return String_Make("=");
		default:
			GS_ASSERT_UNIMPL();
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

	struct Entity_TN
	{
		int type_name_id;
	};

	struct Entity_Const
	{
		Const_Union value;
	};

	using Entity_Deps = std::unordered_set<int>;

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

		union
		{
			Entity_TN tn;
			Entity_Const cnst;
		};
	};

	enum Scope_Type
	{
		Scope_Invalid,
		Scope_Global,
		Scope_File,
	};

	struct Scope
	{
		Scope_Type type;
		Ast_Node* syntax;
		int file_id;
		int parent;
		Array<int> children;
		Array<int> entities;
		std::unordered_map<String_Atom*, int> name_to_entity;
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
	};

	void frontend_push_error(Front_End& f, String error);
	void frontend_push_error(Front_End& f, Tk& token, String file_path, String error);
	void frontend_push_error(Front_End& f, Ast_Node* stmt, int file_id, String error);

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
