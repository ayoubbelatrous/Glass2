#include "pch.h"

#include "Application.h"

#include "BackEnd/Compiler.h"
#include "FrontEnd/AstCopier.h"
#include "FrontEnd/AstPolymorpher.h"
#include "Interpeter/Interpeter.h"

#define NULL_ID (u64)-1
#define MSG_LOC(x) PushMessage(CompilerMessage{ PrintTokenLocation((x->GetLocation())), MessageType::Error })
#define AST_LOC(x) PrintTokenLocation((x->GetLocation()))

#define FMT_WARN(x, ...) PushMessage(CompilerMessage{ fmt::format(x, __VA_ARGS__), MessageType::Warning})

namespace Glass
{
	Compiler::Compiler(std::vector<CompilerFile*> files) : m_Files(files)
	{
		m_Metadata.RegisterType((u64)IRType::IR_void, "void", 0);

		m_Metadata.RegisterType((u64)IRType::IR_float, "float", 4);
		m_Metadata.RegisterType((u64)IRType::IR_int, "int", 4);

		m_Metadata.RegisterType((u64)IRType::IR_i8, "i8", 1);
		m_Metadata.RegisterType((u64)IRType::IR_i16, "i16", 2);
		m_Metadata.RegisterType((u64)IRType::IR_i32, "i32", 4);
		m_Metadata.RegisterType((u64)IRType::IR_i64, "i64", 8);

		m_Metadata.RegisterType((u64)IRType::IR_u8, "u8", 1);
		m_Metadata.RegisterType((u64)IRType::IR_u16, "u16", 2);
		m_Metadata.RegisterType((u64)IRType::IR_u32, "u32", 4);
		m_Metadata.RegisterType((u64)IRType::IR_u64, "u64", 8);

		m_Metadata.RegisterType((u64)IRType::IR_f32, "f32", 4);
		m_Metadata.RegisterType((u64)IRType::IR_f64, "f64", 8);

		m_Metadata.RegisterType((u64)IRType::IR_bool, "bool", 1);

		m_Metadata.GetTypeFlags(IR_int) |= FLAG_BASE_TYPE;

		{
			m_Metadata.GetTypeFlags(IR_i8) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_i16) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_i32) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_i64) |= FLAG_BASE_TYPE;

			m_Metadata.GetTypeFlags(IR_u8) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_u16) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_u32) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_u64) |= FLAG_BASE_TYPE;

			m_Metadata.GetTypeFlags(IR_float) |= FLAG_BASE_TYPE;

			m_Metadata.GetTypeFlags(IR_f32) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_f64) |= FLAG_BASE_TYPE;

			m_Metadata.GetTypeFlags(IR_void) |= FLAG_BASE_TYPE;
			m_Metadata.GetTypeFlags(IR_bool) |= FLAG_BASE_TYPE;
		}

		{
			m_Metadata.GetTypeFlags(IR_int) |= FLAG_NUMERIC_TYPE;

			m_Metadata.GetTypeFlags(IR_i8) |= FLAG_NUMERIC_TYPE;
			m_Metadata.GetTypeFlags(IR_i16) |= FLAG_NUMERIC_TYPE;
			m_Metadata.GetTypeFlags(IR_i32) |= FLAG_NUMERIC_TYPE;
			m_Metadata.GetTypeFlags(IR_i64) |= FLAG_NUMERIC_TYPE;

			m_Metadata.GetTypeFlags(IR_u8) |= FLAG_NUMERIC_TYPE | FLAG_UNSIGNED_TYPE;
			m_Metadata.GetTypeFlags(IR_u16) |= FLAG_NUMERIC_TYPE | FLAG_UNSIGNED_TYPE;
			m_Metadata.GetTypeFlags(IR_u32) |= FLAG_NUMERIC_TYPE | FLAG_UNSIGNED_TYPE;
			m_Metadata.GetTypeFlags(IR_u64) |= FLAG_NUMERIC_TYPE | FLAG_UNSIGNED_TYPE;

			m_Metadata.GetTypeFlags(IR_float) |= FLAG_NUMERIC_TYPE | FLAG_FLOATING_TYPE;

			m_Metadata.GetTypeFlags(IR_f32) |= FLAG_NUMERIC_TYPE | FLAG_FLOATING_TYPE;
			m_Metadata.GetTypeFlags(IR_f64) |= FLAG_NUMERIC_TYPE | FLAG_FLOATING_TYPE;

			m_Metadata.GetTypeFlags(IR_bool) |= FLAG_NUMERIC_TYPE | FLAG_UNSIGNED_TYPE;
		}

		{

			EnumMetadata type_flags_metadata;
			type_flags_metadata.Name.Symbol = "TypeInfo_Flags";

			{
				EnumMemberMetadata member;
				member.Name = "BASE_TYPE";
				member.Value = TI_BASE_TYPE;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			{
				EnumMemberMetadata member;
				member.Name = "NUMERIC_TYPE";
				member.Value = TI_NUMERIC_TYPE;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			{
				EnumMemberMetadata member;
				member.Name = "UNSIGNED_TYPE";
				member.Value = TI_UNSIGNED_TYPE;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			{
				EnumMemberMetadata member;
				member.Name = "FLOATING_TYPE";
				member.Value = TI_FLOATING_TYPE;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			{
				EnumMemberMetadata member;
				member.Name = "STRUCT";
				member.Value = TI_STRUCT;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			{
				EnumMemberMetadata member;
				member.Name = "STRUCT_MEMBER";
				member.Value = TI_STRUCT_MEMBER;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			{
				EnumMemberMetadata member;
				member.Name = "ENUM_TYPE";
				member.Value = TI_ENUM;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			{
				EnumMemberMetadata member;
				member.Name = "FUNCTION";
				member.Value = TI_FUNCTION;

				type_flags_metadata.InsertMember(member.Name, member);
			}

			m_Metadata.RegisterEnum(GetEnumID(), IR_typeinfo_flags, type_flags_metadata);
		}

		{
			StructMetadata any_Metadata;
			any_Metadata.Name.Symbol = "Any";

			// type
			{
				MemberMetadata type_member_metadata;
				type_member_metadata.Name.Symbol = "type";
				type_member_metadata.Tipe.ID = IR_u64; //@TODO: Change to Type when we add first class types
				type_member_metadata.Tipe.Pointer = 0;

				any_Metadata.Members.push_back(type_member_metadata);
			}

			// data
			{
				MemberMetadata data_member_metadata;
				data_member_metadata.Name.Symbol = "data";
				data_member_metadata.Tipe.ID = IR_void;
				data_member_metadata.Tipe.Pointer = 1;

				any_Metadata.Members.push_back(data_member_metadata);
			}

			m_Metadata.RegisterStruct(GetStructID(), IR_any, any_Metadata);
		}

		{
			StructMetadata array_info_Metadata;
			array_info_Metadata.Name.Symbol = "Array";

			// type
			{
				MemberMetadata count_member_metadata;
				count_member_metadata.Name.Symbol = "count";
				count_member_metadata.Tipe.ID = IR_u64;
				count_member_metadata.Tipe.Pointer = 0;

				array_info_Metadata.Members.push_back(count_member_metadata);
			}

			// data
			{
				MemberMetadata data_member_metadata;
				data_member_metadata.Name.Symbol = "data";
				data_member_metadata.Tipe.ID = IR_void;
				data_member_metadata.Tipe.Pointer = 1;

				array_info_Metadata.Members.push_back(data_member_metadata);
			}

			m_Metadata.RegisterStruct(GetStructID(), IR_array, array_info_Metadata);
		}

		{

			StructMetadata type_info_member_Metadata;
			type_info_member_Metadata.Name.Symbol = "TypeInfo_Member";

			// id
			{
				MemberMetadata id_member_metadata;
				id_member_metadata.Name.Symbol = "id";
				id_member_metadata.Tipe.ID = IR_u64;
				id_member_metadata.Tipe.Pointer = 0;

				type_info_member_Metadata.Members.push_back(id_member_metadata);
			}

			// name
			{
				MemberMetadata name_member_metadata;
				name_member_metadata.Name.Symbol = "name";
				name_member_metadata.Tipe.ID = IR_u8;
				name_member_metadata.Tipe.Pointer = 1;

				type_info_member_Metadata.Members.push_back(name_member_metadata);
			}

			// size
			{
				MemberMetadata size_member_metadata;
				size_member_metadata.Name.Symbol = "size";
				size_member_metadata.Tipe.ID = IR_u64;
				size_member_metadata.Tipe.Pointer = 0;

				type_info_member_Metadata.Members.push_back(size_member_metadata);
			}

			// size
			{
				MemberMetadata size_member_metadata;
				size_member_metadata.Name.Symbol = "flags";
				size_member_metadata.Tipe.ID = IR_u64;
				size_member_metadata.Tipe.Pointer = 0;

				type_info_member_Metadata.Members.push_back(size_member_metadata);
			}

			// member_name
			{
				MemberMetadata mem_name_member_metadata;
				mem_name_member_metadata.Name.Symbol = "member_name";
				mem_name_member_metadata.Tipe.ID = IR_u8;
				mem_name_member_metadata.Tipe.Pointer = 1;

				type_info_member_Metadata.Members.push_back(mem_name_member_metadata);
			}

			m_Metadata.RegisterStruct(GetStructID(), IR_typeinfo_member, type_info_member_Metadata);
		}

		{

			StructMetadata type_info_struct_Metadata;
			type_info_struct_Metadata.Name.Symbol = "TypeInfo_Struct";

			// id
			{
				MemberMetadata id_member_metadata;
				id_member_metadata.Name.Symbol = "id";
				id_member_metadata.Tipe.ID = IR_u64;
				id_member_metadata.Tipe.Pointer = 0;

				type_info_struct_Metadata.Members.push_back(id_member_metadata);
			}

			// name
			{
				MemberMetadata name_member_metadata;
				name_member_metadata.Name.Symbol = "name";
				name_member_metadata.Tipe.ID = IR_u8;
				name_member_metadata.Tipe.Pointer = 1;

				type_info_struct_Metadata.Members.push_back(name_member_metadata);
			}

			// size
			{
				MemberMetadata size_member_metadata;
				size_member_metadata.Name.Symbol = "size";
				size_member_metadata.Tipe.ID = IR_u64;
				size_member_metadata.Tipe.Pointer = 0;

				type_info_struct_Metadata.Members.push_back(size_member_metadata);
			}

			// members
			{
				MemberMetadata members_member_metadata;
				members_member_metadata.Name.Symbol = "members";
				members_member_metadata.Tipe.ID = IR_typeinfo_member;
				members_member_metadata.Tipe.Pointer = 1;
				members_member_metadata.Tipe.Array = 1;

				type_info_struct_Metadata.Members.push_back(members_member_metadata);
			}

			m_Metadata.RegisterStruct(GetStructID(), IR_typeinfo_struct, type_info_struct_Metadata);
		}

		{

			StructMetadata type_info_Metadata;
			type_info_Metadata.Name.Symbol = "TypeInfo";

			// id
			{
				MemberMetadata id_member_metadata;
				id_member_metadata.Name.Symbol = "id";
				id_member_metadata.Tipe.ID = IR_u64;
				id_member_metadata.Tipe.Pointer = 0;

				type_info_Metadata.Members.push_back(id_member_metadata);
			}

			// name
			{
				MemberMetadata name_member_metadata;
				name_member_metadata.Name.Symbol = "name";
				name_member_metadata.Tipe.ID = IR_u8;
				name_member_metadata.Tipe.Pointer = 1;

				type_info_Metadata.Members.push_back(name_member_metadata);
			}

			// size
			{
				MemberMetadata size_member_metadata;
				size_member_metadata.Name.Symbol = "size";
				size_member_metadata.Tipe.ID = IR_u64;
				size_member_metadata.Tipe.Pointer = 0;

				type_info_Metadata.Members.push_back(size_member_metadata);
			}

			// flags
			{
				MemberMetadata flags_member_metadata;
				flags_member_metadata.Name.Symbol = "flags";
				flags_member_metadata.Tipe.ID = IR_typeinfo_flags;
				flags_member_metadata.Tipe.Pointer = 0;

				type_info_Metadata.Members.push_back(flags_member_metadata);
			}

			m_Metadata.RegisterStruct(GetStructID(), IR_typeinfo, type_info_Metadata);
		}

		m_TypeIDCounter = IR_typeinfo;
	}

	IRTranslationUnit* Compiler::CodeGen()
	{
		//////////////////////////////////////////////////////////////////////////
		//		@PUSH_SCOPE @GLOBAL
		m_Metadata.PushContextGlobal();
		//////////////////////////////////////////////////////////////////////////

		IRTranslationUnit* tu = IR(IRTranslationUnit());
		for (CompilerFile* file : m_Files)
		{
			ModuleFile* module_file = file->GetAST();
			for (const Statement* stmt : module_file->GetStatements())
			{
				auto stmt_code = StatementCodeGen(stmt);
				if (stmt_code != nullptr)
				{
					tu->Instructions.push_back(stmt_code);
				}
			}
			m_CurrentFile++;
		}

		for (auto& func : m_Metadata.m_Functions)
		{
			FunctionMetadata& metadata = func.second;
			if (metadata.PolyMorphic)
			{
				for (auto [irf, function_node] : metadata.PendingPolymorphInstantiations)
				{
					IRFunction* ir_function = (IRFunction*)FunctionCodeGen(function_node);
					irf->Instructions = ir_function->Instructions;
					irf->Arguments = ir_function->Arguments;

					m_Metadata.m_Functions[irf->ID] = *m_Metadata.GetFunctionMetadata(ir_function->ID);
				}
			}
		}

		auto ir_data = PoPIRData();

		std::vector<IRInstruction*> instructions;

		for (auto entry : ir_data)
		{
			instructions.push_back(entry);
		}

		for (auto entry : tu->Instructions)
		{
			instructions.push_back(entry);
		}

		tu->Instructions = instructions;

		for (auto& func : m_Metadata.m_Functions)
		{
			FunctionMetadata& metadata = func.second;
			if (metadata.PolyMorphic)
			{

				for (auto& [overload, irf] : metadata.PolyMorhOverLoads)
				{
					tu->Instructions.push_back(irf);
				}
			}
		}

		return tu;
	}

	IRInstruction* Compiler::StatementCodeGen(const Statement* statement)
	{
		NodeType Type = statement->GetType();

		switch (Type)
		{
		case NodeType::Foreign:
			return ForeignCodeGen((ForeignNode*)statement);
			break;
		case NodeType::Operator:
			return OperatorCodeGen((OperatorNode*)statement);
			break;
		case NodeType::Identifier:
		case NodeType::NumericLiteral:
		case NodeType::BinaryExpression:
		case NodeType::StringLiteral:
		case NodeType::Call:
		case NodeType::MemberAccess:
		case NodeType::ArrayAccess:
		case NodeType::Cast:
		case NodeType::SizeOf:
		case NodeType::DeReference:
			return ExpressionCodeGen((Expression*)statement);
			break;
		case NodeType::Function:
			return FunctionCodeGen((FunctionNode*)statement);
			break;
		case NodeType::Variable:
			return VariableCodeGen((VariableNode*)statement);
			break;
		case NodeType::Return:
			return ReturnCodeGen((ReturnNode*)statement);
			break;
		case NodeType::StructNode:
			return StructCodeGen((StructNode*)statement);
			break;
		case NodeType::Enum:
			return EnumCodeGen((EnumNode*)statement);
			break;
		case NodeType::If:
			return IfCodeGen((IfNode*)statement);
			break;
		case NodeType::While:
			return WhileCodeGen((WhileNode*)statement);
			break;
		case NodeType::Break:
		{
			return IR(IRBreak());
		}
		break;
		default:
			return nullptr;
			break;
		}
	}

	IRInstruction* Compiler::ForeignCodeGen(const ForeignNode* frn)
	{
		NodeType tipe = frn->statement->GetType();

		if (tipe == NodeType::Function)
		{
			FunctionNode* fn_decl = (FunctionNode*)frn->statement;

			if (tipe != NodeType::Function)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(frn->statement->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("#foreign directive only supports functions at the moment"), MessageType::Warning });
			}

			u64 ID = GetFunctionID();

			Type return_type;
			return_type.ID = m_Metadata.GetType(fn_decl->ReturnType->Symbol.Symbol);
			return_type.Pointer = fn_decl->ReturnType->Pointer;

			std::vector<ArgumentMetadata> args;

			for (const Statement* a : fn_decl->GetArgList()->GetArguments())
			{
				const VariableNode* decl_arg = (VariableNode*)a;

				ArgumentMetadata fmt_arg;

				fmt_arg.Name = decl_arg->Symbol.Symbol;
				fmt_arg.Tipe.Pointer = decl_arg->Type->Pointer;
				fmt_arg.Tipe.ID = m_Metadata.GetType(decl_arg->Type->Symbol.Symbol);
				fmt_arg.Tipe.Array = decl_arg->Type->Array;

				args.push_back(fmt_arg);
			}

			m_Metadata.RegisterFunction(ID, fn_decl->Symbol.Symbol, return_type, args, fn_decl->Variadic);

			FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(ID);
			metadata->Foreign = true;
		}
		else if (tipe == NodeType::StructNode)
		{
			u64 type_id = GetTypeID();
			u64 struct_id = GetStructID();

			StructNode* strct_decl = (StructNode*)frn->statement;

			m_Metadata.RegisterType(type_id, strct_decl->Name.Symbol, 0);

			StructMetadata struct_metadata;

			struct_metadata.Name = strct_decl->Name;
			struct_metadata.Foreign = true;

			m_Metadata.RegisterStruct(struct_id, type_id, struct_metadata);
		}
		return nullptr;
	}

	IRInstruction* Compiler::OperatorCodeGen(const OperatorNode* op_node)
	{
		auto stmt_type = op_node->statement->GetType();

		if (stmt_type == NodeType::Function)
		{
			IRFunction* IRF = (IRFunction*)FunctionCodeGen((FunctionNode*)op_node->statement);

			const FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(IRF->ID);

			OperatorQuery op_query;

			for (const ArgumentMetadata& arg : metadata->Arguments)
			{
				op_query.TypeArguments.push_back(arg.Tipe);
			}

			m_Metadata.RegisterOperator(op_node->OPerator, op_query, IRF->ID);

			return IRF;
		}

		if (stmt_type == NodeType::Identifier)
		{

			Identifier* identifier = (Identifier*)op_node->statement;

			if (m_Metadata.GetSymbolType(identifier->Symbol.Symbol) != SymbolType::Function)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(op_node->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("operator directive function '{}()' is not defined", identifier->Symbol.Symbol), MessageType::Warning });
				return nullptr;
			}
			u64 function_id = m_Metadata.GetFunctionMetadata(identifier->Symbol.Symbol);

			const FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(function_id);

			OperatorQuery op_query;

			for (const ArgumentMetadata& arg : metadata->Arguments)
			{
				op_query.TypeArguments.push_back(arg.Tipe);
			}

			m_Metadata.RegisterOperator(op_node->OPerator, op_query, function_id);

			return nullptr;
		}

		PushMessage(CompilerMessage{ PrintTokenLocation(op_node->GetLocation()), MessageType::Error });
		PushMessage(CompilerMessage{ "Expected a function name or a function definition after operator directive", MessageType::Warning });

		return nullptr;
	}

	IRInstruction* Compiler::FunctionCodeGen(FunctionNode* functionNode)
	{
		ResetSSAIDCounter();

		u64 poly_func_id = GetFunctionID();

		Type return_type;
		return_type.ID = (u64)IRType::IR_void;

		if (functionNode->ReturnType)
		{
			return_type.ID = m_Metadata.GetType(functionNode->ReturnType->Symbol.Symbol);
			return_type.Pointer = functionNode->ReturnType->Pointer;
			return_type.Array = functionNode->ReturnType->Array;
		}

		m_Metadata.RegisterFunction(poly_func_id, functionNode->Symbol.Symbol, return_type);

		bool poly_morphic = false;

		{
			const auto& arg_list = functionNode->GetArgList()->GetArguments();

			for (const auto arg : arg_list)
			{
				auto var = (VariableNode*)arg;
				if (var->Type->PolyMorphic)
				{
					poly_morphic = true;
					break;
				}
			}
		}

		if (poly_morphic)
		{
			FunctionMetadata* func_metadata = m_Metadata.GetFunctionMetadata(poly_func_id);

			func_metadata->PolyMorphic = true;

			auto& arg_list = functionNode->GetArgList()->GetArguments();

			std::vector<ArgumentMetadata> args_metadata;

			for (auto arg : arg_list)
			{
				auto var = (VariableNode*)arg;

				ArgumentMetadata arg_metadata;

				arg_metadata.Name = var->Symbol.Symbol;

				if (var->Type->PolyMorphic)
				{
					arg_metadata.PolyMorphic = true;
					arg_metadata.PolyMorhID = func_metadata->GetPolyMorphID(var->Type->Symbol.Symbol);
				}
				else
				{
					arg_metadata.Tipe.ID = m_Metadata.GetType(var->Type->Symbol.Symbol);
				}

				arg_metadata.Tipe.Pointer = var->Type->Pointer;

				args_metadata.push_back(arg_metadata);

				func_metadata->Arguments = args_metadata;
			}

			func_metadata->FunctionAST = functionNode;

			return nullptr;
		}

		std::vector<ArgumentMetadata> args_metadata;

		IRFunction* IRF = CreateIRFunction(functionNode);

		PoPIRSSA();

		//////////////////////////////////////////////////////////////////////////
		//		@PUSH_SCOPE
		m_Metadata.PushContext(ContextScopeType::FUNC);
		//////////////////////////////////////////////////////////////////////////

		for (auto a : functionNode->GetArgList()->GetArguments())
		{

			IRSSA* arg_ssa = IR(IRSSA());
			arg_ssa->ID = m_SSAIDCounter;

			IRSSA* arg_address_ssa = CreateIRSSA();

			VariableNode* var = (VariableNode*)a;

			RegisterVariable(arg_address_ssa, var->Symbol.Symbol);

			arg_address_ssa->Type = IR_u64;
			arg_address_ssa->Pointer = false;

			IRAddressOf address_of;

			{
				address_of.SSA = IR(IRARGValue(arg_address_ssa->ID));
			}

			arg_address_ssa->Value = IR(address_of);

			u64 arg_type = m_Metadata.GetType(var->Type->Symbol.Symbol);

			if (arg_type == (u64)-1)
			{

				PushMessage(CompilerMessage{ PrintTokenLocation(var->Type->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("argument '{}' is of undefined type '{}', at '{}' function definition",
														var->Symbol.Symbol, var->Type->Symbol.Symbol, functionNode->DefinitionTk.Symbol),
											MessageType::Warning });

				return nullptr;
			}

			VariableMetadata var_metadata = {
				var->Symbol,
				Type{
					arg_type,
					var->Type->Array || var->Type->Variadic,
					var->Type->Pointer,
				},
			};

			if (!var->Type->Variadic)
			{
				arg_ssa->Type = var_metadata.Tipe.ID;
			}
			else
			{
				arg_ssa->Type = IR_array;
			}

			arg_ssa->Pointer = var->Type->Pointer;

			m_Metadata.RegisterVariableMetadata(arg_address_ssa->ID, var_metadata);

			IRF->Arguments.push_back(arg_ssa);

			ArgumentMetadata arg_metadata;

			arg_metadata.Name = var->Symbol.Symbol;
			arg_metadata.Tipe.Pointer = arg_ssa->Pointer;
			arg_metadata.Tipe.ID = var_metadata.Tipe.ID;
			arg_metadata.Variadic = var->Type->Variadic;
			arg_metadata.SSAID = arg_address_ssa->ID;

			args_metadata.push_back(arg_metadata);
		}

		m_Metadata.RegisterFunction(IRF->ID, functionNode->Symbol.Symbol, return_type, args_metadata);

		for (const Statement* stmt : functionNode->GetStatements())
		{
			IRInstruction* code = StatementCodeGen(stmt);

			if (!code)
				continue;

			auto SSAs = PoPIRSSA();

			for (auto ssa : SSAs)
			{
				IRF->Instructions.push_back(ssa);
			}

			if (code->GetType() != IRNodeType::SSAValue) {
				//We Discard function scope level r values

				if (code != nullptr) {
					IRF->Instructions.push_back(code);
				}
			}

			if (stmt->GetType() == NodeType::Scope) {
				std::vector<IRInstruction*> scope_code_stack = ScopeCodeGen((ScopeNode*)stmt);

				for (auto scope_code : scope_code_stack) {
					IRF->Instructions.push_back(scope_code);
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//		@POP_SCOPE
		m_Metadata.PopContext();
		//////////////////////////////////////////////////////////////////////////

		m_Metadata.m_CurrentFunction++;

		return (IRInstruction*)IRF;
	}

	IRInstruction* Compiler::VariableCodeGen(const VariableNode* variableNode)
	{
		if (ContextGlobal()) {
			return GlobalVariableCodeGen(variableNode);
		}

		const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(variableNode->Symbol.Symbol));

		if (metadata != nullptr)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable '{}' is already defined", variableNode->Symbol.Symbol), MessageType::Warning });
			PushMessage(CompilerMessage{ fmt::format("Defined At!", variableNode->Symbol.Symbol), MessageType::Info });
			PushMessage(CompilerMessage{ "\t" + PrintTokenLocation(metadata->Name), MessageType::Info });
			return nullptr;
		}

		IRSSAValue* value = nullptr;

		Glass::Type Type;

		u64 allocation_type = 0;
		u64 allocation_pointer = 0;

		if (variableNode->Type != nullptr) {

			u64 assignment_type_id = m_Metadata.GetType(variableNode->Type->Symbol.Symbol);
			u64 assignment_type_flags = m_Metadata.GetTypeFlags(assignment_type_id);

			if (assignment_type_flags & FLAG_NUMERIC_TYPE)
			{
				if (!(assignment_type_flags & FLAG_FLOATING_TYPE)) {
					SetLikelyConstantIntegerType(assignment_type_id);
				}
				else if (assignment_type_flags & FLAG_FLOATING_TYPE) {
					SetLikelyConstantFloatType(assignment_type_id);
				}
			}
		}

		if (variableNode->Assignment != nullptr)
		{
			value = GetExpressionByValue(variableNode->Assignment);

			if (!value) {
				return nullptr;
			}
		}

		if (variableNode->Type != nullptr) {
			ResetLikelyConstantIntegerType();
			ResetLikelyConstantFloatType();
		}

		if (variableNode->Type == nullptr) {
			const Glass::Type& assignment_type = m_Metadata.GetExprType(value->SSA);
			Type = assignment_type;

			if (Type.Array)
			{
				allocation_type = IR_array;
			}
		}
		else {

			Type.ID = m_Metadata.GetType(variableNode->Type->Symbol.Symbol);
			Type.Pointer = variableNode->Type->Pointer;
			Type.Array = variableNode->Type->Array;

			if (variableNode->Type->Array)
			{
				allocation_type = IR_array;
			}
			else
			{
				allocation_type = Type.ID;

				if (allocation_type == NULL_ID) {
					PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->Type->GetLocation()), MessageType::Error });
					PushMessage(CompilerMessage{ fmt::format("variable is of unknown type '{}'", variableNode->Type->Symbol.Symbol), MessageType::Warning });
					return nullptr;
				}
			}
			if (!variableNode->Type->Array)
			{
				for (u64 i = 0; i < variableNode->Type->Pointer; i++)
				{
					allocation_pointer++;
				}
			}
		}

		IRSSA* variable_address_ssa = CreateIRSSA();
		variable_address_ssa->Type = Type.ID;
		variable_address_ssa->Pointer = 1;

		variable_address_ssa->Value = IR(IRAlloca((u32)allocation_type, (u32)allocation_pointer));

		RegisterVariable(variable_address_ssa, variableNode->Symbol.Symbol);

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			Type,
			false,
			false,
			nullptr,
			variable_address_ssa };

		m_Metadata.RegisterVariableMetadata(variable_address_ssa->ID, var_metadata);
		m_Metadata.RegExprType(variable_address_ssa->ID, var_metadata.Tipe);

		if (value)
		{
			IRStore* assignment_store = IR(IRStore());

			assignment_store->AddressSSA = variable_address_ssa->ID;
			assignment_store->Data = value;
			assignment_store->Pointer = Type.Pointer;
			assignment_store->Type = Type.ID;

			return assignment_store;
		}

		return nullptr;
	}

	IRInstruction* Compiler::GlobalVariableCodeGen(const VariableNode* variableNode)
	{
		const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(variableNode->Symbol.Symbol));

		if (metadata != nullptr)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable '{}' is already defined", variableNode->Symbol.Symbol), MessageType::Warning });
			PushMessage(CompilerMessage{ fmt::format("Defined At!", variableNode->Symbol.Symbol), MessageType::Info });
			PushMessage(CompilerMessage{ "\t" + PrintTokenLocation(metadata->Name), MessageType::Info });
			return nullptr;
		}

		u64 glob_id = GetGlobalID();

		Glass::Type Type;

		if (variableNode->Assignment != nullptr)
		{
			MSG_LOC(variableNode->Assignment);
			FMT_WARN("assignment for global variables is not supported yet");
			return nullptr;
		}

		IRSSA* StorageSSA = CreateIRSSA();

		Type.ID = m_Metadata.GetType(variableNode->Type->Symbol.Symbol);
		Type.Pointer = variableNode->Type->Pointer;
		Type.Array = variableNode->Type->Array;

		if (variableNode->Type->Array)
		{
			StorageSSA->Type = IR_array;
		}
		else
		{
			StorageSSA->Type = Type.ID;

			if (StorageSSA->Type == NULL_ID) {
				PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->Type->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("variable is of unknown type '{}'", variableNode->Type->Symbol.Symbol), MessageType::Warning });
				return nullptr;
			}
		}
		if (!variableNode->Type->Array)
		{
			for (u64 i = 0; i < variableNode->Type->Pointer; i++)
			{
				StorageSSA->Pointer++;
			}
		}

		StorageSSA->Value = nullptr;

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			Type,
			false,
			true,
			StorageSSA,
			nullptr };

		m_Metadata.RegisterGlobalVariable(glob_id, variableNode->Symbol.Symbol);
		m_Metadata.RegisterVariableMetadata(glob_id, var_metadata);

		//m_Metadata.RegExprType(IRssa->ID, var_metadata.Tipe);

		IRGlobalDecl* global = IR(IRGlobalDecl());
		global->GlobID = glob_id;
		global->Type = var_metadata.Tipe.ID;
		global->Pointer = var_metadata.Tipe.Pointer;
		global->Array = var_metadata.Tipe.Array;

		return global;
	}

	IRInstruction* Compiler::ReturnCodeGen(const ReturnNode* returnNode)
	{
		IRReturn ret;

		auto expr = (IRSSAValue*)GetExpressionByValue(returnNode->Expr);

		ret.Value = expr;

		return IR(ret);
	}

	IRInstruction* Compiler::StructCodeGen(const StructNode* structNode)
	{
		auto& struct_name = structNode->Name;
		auto& struct_members = structNode->m_Members;

		StructMetadata struct_metadata;
		struct_metadata.Name = struct_name;

		for (VariableNode* member : struct_members)
		{

			MemberMetadata member_metadata;
			member_metadata.Name = member->Symbol;
			member_metadata.Tipe.ID = m_Metadata.GetType(member->Type->Symbol.Symbol);
			member_metadata.Tipe.Pointer = member->Type->Pointer;
			member_metadata.Tipe.Array = member->Type->Array;

			if (member_metadata.Tipe.ID == (u64)-1) {
				MSG_LOC(member);
				FMT_WARN("struct '{}' member '{}' is of undefined type '{}'", struct_name.Symbol, member->Symbol.Symbol, member->Type->Symbol.Symbol);
				return nullptr;
			}

			struct_metadata.Members.push_back(member_metadata);
		}

		u64 type_id = GetTypeID();
		u64 struct_id = GetStructID();

		m_Metadata.RegisterStruct(struct_id, type_id, struct_metadata);

		IRStruct ir_struct;

		ir_struct.TypeID = type_id;
		ir_struct.ID = struct_id;

		u64 member_id_counter = 0;

		for (MemberMetadata& member : struct_metadata.Members)
		{
			IRStructMember ir_member;

			ir_member.ID = member_id_counter;
			ir_member.TypeID = member.Tipe.ID;
			ir_member.Pointer = member.Tipe.Pointer;
			ir_member.Array = member.Tipe.Array;

			ir_struct.Members.push_back(IR(ir_member));

			member_id_counter++;
		}

		return IR(ir_struct);
	}

	IRInstruction* Compiler::EnumCodeGen(const EnumNode* enumNode)
	{
		{
			SymbolType symbol_type = m_Metadata.GetSymbolType(enumNode->Name.Symbol);

			if (symbol_type == SymbolType::Enum) {
				const EnumMetadata* previous = m_Metadata.GetEnum(enumNode->Name.Symbol);

				MSG_LOC(enumNode);
				FMT_WARN("enum '{}' is already defined At: {}", enumNode->Name.Symbol, PrintTokenLocation(previous->Name));

				return nullptr;
			}

			if (symbol_type != SymbolType::None) {
				MSG_LOC(enumNode);
				FMT_WARN("enum '{}' name is already taken", enumNode->Name.Symbol);

				return nullptr;
			}
		}


		EnumMetadata metadata;

		metadata.Name = enumNode->Name;

		u64 IOTA = 0;

		if (enumNode->Members.size() > 64) {
			MSG_LOC(enumNode);
			FMT_WARN("enum #flags '{}' exceeded maximum number of members which is 64,\nif you wish to add more remove #flags directive and unlock 18,446,744,073,709,551,551 more members", enumNode->Name.Symbol);
			return nullptr;
		}

		for (Identifier* identifier : enumNode->Members) {
			EnumMemberMetadata member_metadata;

			member_metadata.Name = identifier->Symbol.Symbol;

			if (enumNode->Flags) {
				member_metadata.Value = 1ULL << IOTA;
			}
			else {
				member_metadata.Value = IOTA;
			}

			if (metadata.GetMember(member_metadata.Name)) {
				MSG_LOC(enumNode);
				MSG_LOC(identifier);
				FMT_WARN("enum '{}' .member already declared before", enumNode->Name.Symbol);
				return nullptr;
			}

			metadata.InsertMember(member_metadata.Name, member_metadata);

			IOTA++;
		}

		m_Metadata.RegisterEnum(GetEnumID(), GetTypeID(), metadata);

		return nullptr;
	}

	IRInstruction* Compiler::IfCodeGen(const IfNode* ifNode)
	{
		IRSSAValue* condition = GetExpressionByValue(ifNode->Condition);

		if (!condition)
			return nullptr;

		IRIf IF;
		IF.SSA = condition->SSA;

		PushScope();

		for (const Statement* stmt : ifNode->Scope->GetStatements())
		{
			auto first_inst = StatementCodeGen(stmt);

			auto ir_ssa_stack = PoPIRSSA();

			for (auto inst : ir_ssa_stack)
			{
				IF.Instructions.push_back(inst);
			}

			IF.Instructions.push_back(first_inst);
		}

		PopScope();

		return IR(IF);
	}

	IRInstruction* Compiler::WhileCodeGen(const WhileNode* whileNode)
	{
		PushScope();
		IRSSAValue* condition = GetExpressionByValue(whileNode->Condition);
		std::vector<IRSSA*> condition_ssas = PoPIRSSA();
		PopScope();

		IRWhile WHILE;
		WHILE.SSA = condition->SSA;

		WHILE.ConditionBlock = condition_ssas;

		PushScope();

		for (const Statement* stmt : whileNode->Scope->GetStatements())
		{
			auto inst = StatementCodeGen(stmt);

			auto ir_ssa_stack = PoPIRSSA();

			for (auto ssa_inst : ir_ssa_stack)
			{
				WHILE.Instructions.push_back(ssa_inst);
			}

			WHILE.Instructions.push_back(inst);
		}

		PopScope();

		return IR(WHILE);
	}

	IRInstruction* Compiler::ExpressionCodeGen(const Expression* expression)
	{
		NodeType Type = expression->GetType();

		switch (Type)
		{
		case NodeType::Identifier:
			return IdentifierCodeGen((Identifier*)expression);
			break;
		case NodeType::NumericLiteral:
			return NumericLiteralCodeGen((NumericLiteral*)expression);
			break;
		case NodeType::StringLiteral:
			return StringLiteralCodeGen((StringLiteral*)expression);
			break;
		case NodeType::BinaryExpression:
			return BinaryExpressionCodeGen((BinaryExpression*)expression);
			break;
		case NodeType::Call:
			return FunctionCallCodeGen((FunctionCall*)expression);
			break;
		case NodeType::MemberAccess:
			return MemberAccessCodeGen((MemberAccess*)expression);
			break;
		case NodeType::ArrayAccess:
			return ArrayAccessCodeGen((ArrayAccess*)expression);
			break;
		case NodeType::Reference:
			return RefCodeGen((RefNode*)expression);
			break;
		case NodeType::DeReference:
			return DeRefCodeGen((DeRefNode*)expression);
			break;
		case NodeType::TypeOf:
			return TypeofCodeGen((TypeOfNode*)expression);
			break;
		case NodeType::Cast:
			return CastCodeGen((CastNode*)expression);
			break;
		case NodeType::SizeOf:
			return SizeOfCodeGen((SizeOfNode*)expression);
			break;
		}

		return nullptr;
	}

	IRInstruction* Compiler::IdentifierCodeGen(const Identifier* identifier)
	{
		SymbolType symbol_type = m_Metadata.GetSymbolType(identifier->Symbol.Symbol);

		if (symbol_type == SymbolType::Function) {
			return FunctionRefCodegen(identifier);
		}

		if (symbol_type == SymbolType::None) {
			MSG_LOC(identifier);
			FMT_WARN("undefined name: '{}'", identifier->Symbol.Symbol);
			return nullptr;
		}

		if (symbol_type == SymbolType::Variable)
		{
			const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(identifier->Symbol.Symbol));

			if (metadata == nullptr)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(identifier->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("variable '{}' is not defined", identifier->Symbol.Symbol), MessageType::Warning });
				return nullptr;
			}

			u64 ID = GetVariableSSA(identifier->Symbol.Symbol);

			IRSSA* ssa = GetSSA(ID);

			IRSSAValue ssa_val;

			ssa_val.SSA = GetVariableSSA(identifier->Symbol.Symbol);

			m_Metadata.RegExprType(ssa_val.SSA, metadata->Tipe);

			return (IRInstruction*)IR(ssa_val);
		}
		else if (symbol_type == SymbolType::GlobVariable) {

			u64 glob_id = m_Metadata.GetGlobalVariable(identifier->Symbol.Symbol);

			auto ssa = CreateIRSSA();
			ssa->Type = IR_u64;
			IRGlobalAddress* glob_address = IR(IRGlobalAddress(glob_id));

			ssa->Value = glob_address;

			const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(glob_id);

			m_Metadata.RegExprType(ssa->ID, metadata->Tipe);

			return IR(IRSSAValue(ssa->ID));
		}

		return nullptr;
	}

	IRInstruction* Compiler::NumericLiteralCodeGen(const NumericLiteral* numericLiteral)
	{
		IRSSA* IRssa = CreateIRSSA();

		IRssa->Value = IR(IRCONSTValue());

		IRCONSTValue* Value = (IRCONSTValue*)IRssa->Value;

		if (numericLiteral->type == NumericLiteral::Type::Float)
		{
			IRssa->Type = GetLikelyConstantFloatType();
			memcpy(&Value->Data, &numericLiteral->Val.Float, sizeof(double));
		}
		if (numericLiteral->type == NumericLiteral::Type::Int)
		{
			IRssa->Type = GetLikelyConstantIntegerType();
			memcpy(&Value->Data, &numericLiteral->Val.Int, sizeof(i64));
		}

		Value->Type = IRssa->Type;

		IRSSAValue* ssa_value = IR(IRSSAValue());

		ssa_value->SSA = IRssa->ID;

		{
			Type type;
			type.ID = IRssa->Type;
			m_Metadata.RegExprType(ssa_value->SSA, type);
		}

		return ssa_value;
	}

	IRInstruction* Compiler::StringLiteralCodeGen(const StringLiteral* stringLiteral)
	{
		IRData* data = CreateIRData();

		for (char c : stringLiteral->Symbol.Symbol)
		{
			data->Data.push_back(c);
		}

		auto ir_ssa = CreateIRSSA();

		ir_ssa->Type = IR_u8;
		ir_ssa->Pointer = true;
		ir_ssa->Value = IR(IRDataValue(0, data->ID));

		IRSSAValue ssa_val;

		ssa_val.SSA = ir_ssa->ID;

		{
			Type type;
			type.ID = IR_u8;
			type.Pointer = 1;
			m_Metadata.RegExprType(ssa_val.SSA, type);
		}

		return IR(ssa_val);
	}

	IRInstruction* Compiler::BinaryExpressionCodeGen(const BinaryExpression* binaryExpr)
	{
		if (binaryExpr->OPerator == Operator::Assign)
		{
			return AssignmentCodeGen(binaryExpr);
		}

		IRSSAValue* A = nullptr;
		IRSSAValue* B = nullptr;

		A = GetExpressionByValue(binaryExpr->Left);
		B = GetExpressionByValue(binaryExpr->Right);

		if (!A || !B)
		{
			return nullptr;
		}

		const Glass::Type& left_type = m_Metadata.GetExprType(A->SSA);
		const Glass::Type& right_type = m_Metadata.GetExprType(B->SSA);

		IRSSA* IRssa = CreateIRSSA();

		IRssa->Type = left_type.ID;

		if ((left_type.Pointer == 0) && (right_type.Pointer == 0))
		{
			TypeFlags left_type_flags = m_Metadata.GetTypeFlags(left_type.ID);
			TypeFlags right_type_flags = m_Metadata.GetTypeFlags(right_type.ID);

			auto op_to_word = [&](Operator op) -> std::string
			{
				switch (op)
				{
				case Operator::Add:
					return "Additions";
					break;
				case Operator::Subtract:
					return "Subtractions";
					break;
				case Operator::Multiply:
					return "Multiplications";
					break;
				case Operator::Divide:
					return "Divisions";
					break;
				case Operator::Not:
					return "Boolean Operators";
					break;
				case Operator::Equal:
				case Operator::NotEqual:
				case Operator::GreaterThan:
				case Operator::LesserThan:
				case Operator::GreaterThanEq:
				case Operator::LesserThanEq:
					return "Comparisons";
					break;
				default:
					return fmt::format("@FIXME: {}() {}:{}", __FUNCTION__, __FILE__, __LINE__);
					break;
				}
			};

			auto no_op_error = [&]()
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(binaryExpr->OperatorToken), MessageType::Error });
				PushMessage(CompilerMessage{
					fmt::format(
						"No {} were defined between '{}' and '{}'",
						op_to_word(binaryExpr->OPerator), PrintType(left_type), PrintType(right_type)),
					MessageType::Warning });
			};

			bool right_numeric_type = right_type_flags & FLAG_NUMERIC_TYPE;
			bool left_numeric_type = left_type_flags & FLAG_NUMERIC_TYPE;

			if (!(right_numeric_type && left_numeric_type))
			{

				OperatorQuery op_query;

				op_query.TypeArguments.push_back(left_type);
				op_query.TypeArguments.push_back(right_type);

				u64 op_func_id = m_Metadata.GetOperator(binaryExpr->OPerator, op_query);

				bool flipped = false;

				if (op_func_id == (u64)-1)
				{

					op_query.TypeArguments.clear();

					op_query.TypeArguments.push_back(right_type);
					op_query.TypeArguments.push_back(left_type);

					op_func_id = m_Metadata.GetOperator(binaryExpr->OPerator, op_query);

					flipped = true;

					if (op_func_id == (u64)-1)
					{
						no_op_error();
						return nullptr;
					}
				}

				{
					//@TODO: Generate Proper Call

					const auto op_func_metadata = m_Metadata.GetFunctionMetadata(op_func_id);

					Expression* lhs = binaryExpr->Left;
					Expression* rhs = binaryExpr->Right;

					FunctionCall call;

					if (!flipped)
					{
						call.Arguments.push_back(lhs);
						call.Arguments.push_back(rhs);
					}
					else
					{
						call.Arguments.push_back(rhs);
						call.Arguments.push_back(lhs);
					}

					call.Function.Symbol = op_func_metadata->Name;

					IRSSAValue* op_result = (IRSSAValue*)ExpressionCodeGen(AST(call));

					m_Metadata.RegExprType(op_result->SSA, op_func_metadata->ReturnType);

					return op_result;
				}
			}
		}

		switch (binaryExpr->OPerator)
		{
		case Operator::Add:
		{
			IRADD IROp;

			IROp.SSA_A = A;
			IROp.SSA_B = B;

			IRssa->Value = IR(IROp);
		}
		break;
		case Operator::Subtract:
		{
			IRSUB IROp;

			IROp.SSA_A = A;
			IROp.SSA_B = B;

			IRssa->Value = IR(IROp);
		}
		break;
		case Operator::Multiply:
		{
			IRMUL IROp;

			IROp.SSA_A = A;
			IROp.SSA_B = B;

			IRssa->Value = IR(IROp);
		}
		break;
		case Operator::Divide:
		{
			IRDIV IROp;

			IROp.SSA_A = A;
			IROp.SSA_B = B;

			IRssa->Value = IR(IROp);
		}
		break;
		case Operator::Equal:
		{
			IRssa->Value = IR(IREQ(A, B));
		}
		break;
		case Operator::NotEqual:
		{
			IRssa->Value = IR(IRNOTEQ(A, B));
		}
		break;
		case Operator::GreaterThan:
		{
			IRssa->Value = IR(IRGreater(A, B));
		}
		break;
		case Operator::LesserThan:
		{
			IRssa->Value = IR(IRLesser(A, B));
		}
		break;
		case Operator::GreaterThanEq:
		{
			IRssa->Value = IR(IRGreater(A, B));
		}
		break;
		case Operator::LesserThanEq:
		{
			IRssa->Value = IR(IRLesser(A, B));
		}
		break;
		case Operator::BitAnd:
		{
			IRssa->Value = IR(IRBitAnd(A, B));
		}
		break;
		case Operator::BitOr:
		{
			IRssa->Value = IR(IRBitOr(A, B));
		}
		break;
		default:
			return nullptr;
			break;
		}

		IRSSAValue* ssa_value = IR(IRSSAValue());

		ssa_value->SSA = IRssa->ID;

		m_Metadata.RegExprType(ssa_value->SSA, left_type);

		return ssa_value;
	}

	IRInstruction* Compiler::AssignmentCodeGen(const BinaryExpression* binaryExpr)
	{
		auto left = binaryExpr->Left;
		auto right = binaryExpr->Right;

		if (left->GetType() == NodeType::Identifier)
		{
			Identifier* identifier_left = (Identifier*)binaryExpr->Left;

			SymbolType symbol_type = m_Metadata.GetSymbolType(identifier_left->Symbol.Symbol);

			if (symbol_type == SymbolType::GlobVariable)
			{
				IRSSAValue* right_val = (IRSSAValue*)GetExpressionByValue(binaryExpr->Right);

				u64 glob_id = m_Metadata.GetGlobalVariable(identifier_left->Symbol.Symbol);

				auto ssa = CreateIRSSA();
				ssa->Type = IR_u64;
				ssa->Value = IR(IRGlobalAddress(glob_id));

				const Glass::Type& type = m_Metadata.GetVariableMetadata(glob_id)->Tipe;

				IRStore* store = IR(IRStore());
				{
					store->Data = right_val;
					store->AddressSSA = ssa->ID;
					store->Type = type.ID;
					store->Pointer = type.Pointer;
				}

				return store;
			}

			if (symbol_type == SymbolType::Variable) {
				IRSSAValue* right_val = (IRSSAValue*)GetExpressionByValue(binaryExpr->Right);

				u64 var_ssa_id = GetVariableSSA(identifier_left->Symbol.Symbol);
				IRSSA* var_ssa = m_Metadata.GetSSA(var_ssa_id);

				const auto metadata = m_Metadata.GetVariableMetadata(var_ssa_id);

				IRStore* store = IR(IRStore());
				{
					store->Data = right_val;
					store->AddressSSA = var_ssa_id;
					store->Type = metadata->Tipe.ID;
					store->Pointer = metadata->Tipe.Pointer;
				}

				return store;
			}
		}
		if (left->GetType() == NodeType::MemberAccess)
		{
			IRSSAValue* member_access = (IRSSAValue*)ExpressionCodeGen(left);
			IRSSAValue* right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			const Glass::Type& member_access_expr_type = m_Metadata.GetExprType(member_access->SSA);

			if (!member_access || !right_ssa) {
				return nullptr;
			}

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = member_access->SSA;
				store->Type = member_access_expr_type.ID;
				store->Pointer = member_access_expr_type.Pointer;
			}

			return store;
		}
		if (left->GetType() == NodeType::ArrayAccess)
		{
			auto left_ssa = (IRSSAValue*)ExpressionCodeGen(left);
			auto right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = left_ssa->SSA;
				store->Type = m_Metadata.GetExprType(right_ssa->SSA).ID;
			}
			return store;
		}
		if (left->GetType() == NodeType::DeReference)
		{
			auto left_ssa = (IRSSAValue*)ExpressionCodeGen(left);
			auto right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = left_ssa->SSA;
				store->Type = m_Metadata.GetExprType(right_ssa->SSA).ID;
			}
			return store;
		}
		return nullptr;
	}

	IRInstruction* Compiler::FunctionCallCodeGen(const FunctionCall* call)
	{
		u64 IRF = m_Metadata.GetFunctionMetadata(call->Function.Symbol);
		FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(IRF);

		if (metadata == nullptr)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("trying to call a undefined function '{}'", call->Function.Symbol), MessageType::Warning });
			return nullptr;
		}
		else
		{
			if (metadata->Arguments.size() != 0)
			{

				if (metadata->Arguments[metadata->Arguments.size() - 1].Variadic == false)
				{
					if ((call->Arguments.size() > metadata->Arguments.size()) && !metadata->Variadic)
					{
						PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()), MessageType::Error });
						PushMessage(CompilerMessage{ fmt::format("too many arguments for '{}()' function call ", call->Function.Symbol), MessageType::Warning });
						return nullptr;
					}
					if ((call->Arguments.size() < metadata->Arguments.size()))
					{
						PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()), MessageType::Error });
						PushMessage(CompilerMessage{ fmt::format("too few arguments for '{}()' function call ", call->Function.Symbol), MessageType::Warning });
						return nullptr;
					}
				}
			}
		}

		std::vector<IRSSAValue*> argument_expr_results;

		if (metadata->PolyMorphic)
		{
			// 			PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()),MessageType::Error });
			// 			PushMessage(CompilerMessage{ fmt::format("calls to polymorphic '{}' function is unsupported",call->Function.Symbol),MessageType::Warning });
			// 			return nullptr;

			PolyMorphOverloads overloads;

			auto& args = call->Arguments;

			for (size_t i = 0; i < args.size(); i++)
			{
				auto expr_code = GetExpressionByValue(args[i]);

				if (!expr_code)
				{
					return nullptr;
				}

				argument_expr_results.push_back(expr_code);

				if (!metadata->Arguments[i].PolyMorphic)
				{
					continue;
				}

				const Glass::Type& expr_type = m_Metadata.GetExprType(expr_code->SSA);

				PolyMorphicType poly_type;
				poly_type.ID = metadata->GetPolyMorphID(metadata->Arguments[i].Name);

				overloads.TypeArguments.push_back({ poly_type, expr_type });
			}
			const IRFunction* overload = GetPolyMorphOverLoad(IRF, overloads);

			IRF = overload->ID;

			if (overload == nullptr)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("failed to instantiate polymorphic function '{}'", call->Function.Symbol), MessageType::Warning });
				return nullptr;
			}
		}

		IRFunctionCall ir_call;

		ir_call.FuncID = IRF;

		if (metadata->PolyMorphic)
		{
			for (size_t i = 0; i < call->Arguments.size(); i++)
			{
				ir_call.Arguments.push_back(IR(IRSSAValue(argument_expr_results[i]->SSA)));
			}
		}
		else
		{
			for (size_t i = 0; i < call->Arguments.size(); i++)
			{
				const ArgumentMetadata* decl_arg = metadata->GetArgument(i);

				IRInstruction* arg = nullptr;

				IRSSAValue* argument_code = nullptr;

				if (decl_arg != nullptr)
				{
					if (!decl_arg->Variadic) {
						if (decl_arg->Tipe.ID == IR_any) {
							argument_code = PassAsAny(call->Arguments[i]);
						}
						else {
							argument_code = GetExpressionByValue(call->Arguments[i]);
						}
					}
					else {
						argument_code = PassAsVariadicArray(i, call->Arguments, decl_arg);
					}

					if (argument_code == nullptr)
						return nullptr;

					const Glass::Type& type = m_Metadata.GetExprType(argument_code->SSA);

					if (decl_arg->Tipe.ID != IR_any && !decl_arg->Variadic)
					{
						bool type_mismatch = !CheckTypeConversion(decl_arg->Tipe.ID, type.ID);

						if (type_mismatch)
						{

							PushMessage(CompilerMessage{ PrintTokenLocation(call->Arguments[i]->GetLocation()), MessageType::Error });
							PushMessage(CompilerMessage{ "type mismatch in function call", MessageType::Warning });
							PushMessage(CompilerMessage{ fmt::format("needed a '{}' instead got '{}'",
																	PrintType(decl_arg->Tipe),
																	PrintType(type)),

														MessageType::Info });
							PushMessage(CompilerMessage{ fmt::format("In place of function argument '{}'", decl_arg->Name),
														MessageType::Info });
						}
					}

					arg = argument_code;
				}
				else
				{
					arg = GetExpressionByValue(call->Arguments[i]);
				}

				ir_call.Arguments.push_back(arg);
				if (decl_arg != nullptr)
				{
					if (decl_arg->Variadic)
					{
						break;
					}
				}
			}
		}

		bool has_return = metadata->ReturnType.Pointer;

		has_return |= metadata->ReturnType.ID != IR_void;

		if (has_return)
		{
			auto ir_ssa = CreateIRSSA();

			ir_ssa->Type = metadata->ReturnType.ID;
			ir_ssa->Value = IR(ir_call);
			ir_ssa->Pointer = metadata->ReturnType.Pointer;

			IRSSAValue* ir_ssa_val = IR(IRSSAValue(ir_ssa->ID));

			m_Metadata.RegExprType(ir_ssa_val->SSA, metadata->ReturnType);

			return ir_ssa_val;
		}

		return IR(ir_call);
	}


	IRInstruction* Compiler::MemberAccessCodeGen(const MemberAccess* memberAccess)
	{
		if (memberAccess->Object->GetType() == NodeType::Identifier) {

			SymbolType symbol_type = m_Metadata.GetSymbolType(((Identifier*)memberAccess->Object)->Symbol.Symbol);

			if (symbol_type == SymbolType::Enum) {
				return (IRSSAValue*)EnumMemberAccessCodeGen(memberAccess);
			}
		}

		u64 struct_id = 0;
		u64 member_id = 0;
		u64 object_ssa_id = 0;
		bool reference_access = false;

		Glass::Type result_type;

		IRSSAValue* obj_ssa_value = (IRSSAValue*)ExpressionCodeGen(memberAccess->Object);

		if (!obj_ssa_value)
			return nullptr;

		const StructMetadata* struct_metadata = nullptr;

		{
			object_ssa_id = obj_ssa_value->SSA;

			const Glass::Type& obj_expr_type = m_Metadata.GetExprType(obj_ssa_value->SSA);

			//@Note this is here because variables are unique in the sense that they always are a pointer or a double pointer as int
			if (memberAccess->Object->GetType() == NodeType::Identifier) {
				reference_access = obj_expr_type.Pointer;
			}
			else if (memberAccess->Object->GetType() != NodeType::MemberAccess) {
				//Handle temporaries that are not pointers
				//Normally they are not modifiable however if they are pointers they can be, so if want to read the temporary value so we do need to reference it
				if (obj_expr_type.Pointer == 0) {
					auto temporary_reference = CreateIRSSA();
					temporary_reference->Type = obj_expr_type.ID;
					temporary_reference->Pointer = 1;
					temporary_reference->Value = IR(IRAddressOf(obj_ssa_value));

					object_ssa_id = temporary_reference->ID;
				}
			}

			struct_id = m_Metadata.GetStructIDFromType(obj_expr_type.ID);

			if (obj_expr_type.Array) {
				struct_id = m_Metadata.GetStructIDFromType(IR_array);
			}

			if (struct_id == NULL_ID) {
				MSG_LOC(memberAccess->Member);
				FMT_WARN("The type '{}' is not a struct and does not support members", PrintType(obj_expr_type));
				return nullptr;
			}

			struct_metadata = m_Metadata.GetStructMetadata(struct_id);
			GS_CORE_ASSERT(struct_metadata, "'struct_metadata' Must Not Be Null");
		}

		{
			auto member_node_type = memberAccess->Member->GetType();


			GS_CORE_ASSERT(
				member_node_type == NodeType::Identifier,
				"'MemberAccess.Member' Must Be an Identifier");

			Identifier* as_identifier = (Identifier*)memberAccess->Member;

			member_id = struct_metadata->FindMember(as_identifier->Symbol.Symbol);

			if (member_id == NULL_ID) {

				MSG_LOC(memberAccess->Member);
				FMT_WARN("'{}' is not defined as a member from type '{}'",
					as_identifier->Symbol.Symbol, struct_metadata->Name.Symbol);
				return nullptr;
			}

			result_type = struct_metadata->Members[member_id].Tipe;
		}


		IRMemberAccess ir_mem_access;

		{
			ir_mem_access.StructID = struct_id;
			ir_mem_access.ObjectSSA = object_ssa_id;
			ir_mem_access.MemberID = member_id;
			ir_mem_access.ReferenceAccess = reference_access;

			auto address_ssa = CreateIRSSA();

			address_ssa->Value = IR(ir_mem_access);
			address_ssa->Type = IR_u64;

			address_ssa->Reference = true;
			address_ssa->ReferenceType = result_type.ID;

			m_Metadata.RegExprType(address_ssa->ID, result_type);

			return IR(IRSSAValue(address_ssa->ID));
		}
	}

	IRInstruction* Compiler::EnumMemberAccessCodeGen(const MemberAccess* memberAccess)
	{
		Identifier* Object = (Identifier*)memberAccess->Object;
		Identifier* Member = (Identifier*)memberAccess->Member;

		const EnumMetadata* metadata = m_Metadata.GetEnum(Object->Symbol.Symbol);
		GS_CORE_ASSERT(metadata, "Can't be null at this point");

		u64 Value = 0;

		if (const EnumMemberMetadata* member = metadata->GetMember(Member->Symbol.Symbol)) {
			Value = member->Value;
		}
		else {
			MSG_LOC(Member);
			FMT_WARN("enum '{}' has no member named '{}', defined at '{}'", metadata->Name.Symbol, Member->Symbol.Symbol, PrintTokenLocation(metadata->Name));
			return nullptr;
		}

		//@Gross
		NumericLiteral node;
		node.Val.Int = Value;
		node.type = NumericLiteral::Type::Int;

		IRSSAValue* result = (IRSSAValue*)NumericLiteralCodeGen(AST(node));

		Glass::Type Type;
		Type.ID = m_Metadata.GetType(metadata->Name.Symbol);

		Type.Array = 0;
		Type.Pointer = 0;

		m_Metadata.RegExprType(result->SSA, Type);

		return result;
	}

	IRInstruction* Compiler::SizeOfCodeGen(const SizeOfNode* size_of)
	{
		u64 size = 0;

		if (size_of->Expr->GetType() == NodeType::Identifier) {

			Identifier* identifier = (Identifier*)size_of->Expr;

			SymbolType symbol_type = m_Metadata.GetSymbolType(identifier->Symbol.Symbol);

			if (symbol_type == SymbolType::None) {
				MSG_LOC(identifier);
				FMT_WARN("undefined name '{}' inside sizeof", identifier->Symbol.Symbol);
				return nullptr;
			}

			if (symbol_type == SymbolType::Type) {
				size = m_Metadata.GetTypeSize(m_Metadata.GetType(identifier->Symbol.Symbol));
			}
			else {
				IRSSAValue* expr_value = (IRSSAValue*)ExpressionCodeGen(size_of->Expr);

				const Glass::Type& expr_type = m_Metadata.GetExprType(expr_value->SSA);

				if (expr_type.Pointer) {
					size = 8;
				}
				else if (expr_type.Array) {
					size = 16;
				}
				else {
					size = m_Metadata.GetTypeSize(expr_type.ID);
				}
			}
		}
		else {

			IRSSAValue* expr_value = (IRSSAValue*)ExpressionCodeGen(size_of->Expr);

			const Glass::Type& expr_type = m_Metadata.GetExprType(expr_value->SSA);

			if (expr_type.Pointer) {
				size = 8;
			}
			else if (expr_type.Array) {
				size = 16;
			}
			else {
				size = m_Metadata.GetTypeSize(expr_type.ID);
			}
		}

		//@Gross
		NumericLiteral SizeNode;
		SizeNode.Val.Int = size;
		SizeNode.type = NumericLiteral::Type::Int;

		return NumericLiteralCodeGen(AST(SizeNode));
	}

	IRInstruction* Compiler::FunctionRefCodegen(const Identifier* func)
	{
		u64 func_id = m_Metadata.GetFunctionMetadata(func->Symbol.Symbol);

		GS_CORE_ASSERT(func_id != NULL_ID, "Function must exist at this point");

		auto ssa = CreateIRSSA();
		ssa->Value = IR(IRFuncPtr(func_id));
		ssa->Type = IR_void;
		ssa->Pointer = 1;

		return IR(IRSSAValue(ssa->ID));
	}

	std::vector<IRInstruction*> Compiler::ScopeCodeGen(const ScopeNode* scope)
	{
		m_Metadata.PushContext(ContextScopeType::FUNC);

		std::vector<IRInstruction*> Instructions;

		for (auto stmt : scope->GetStatements()) {

			auto code = StatementCodeGen(stmt);


			auto SSAs = PoPIRSSA();

			for (auto ssa : SSAs)
			{
				Instructions.push_back(ssa);
			}

			if (code != nullptr) {
				Instructions.push_back(code);
			}
			else {
				if (stmt->GetType() == NodeType::Scope) {
					std::vector<IRInstruction*> scope_code_stack = ScopeCodeGen((ScopeNode*)stmt);
					for (auto scope_code : scope_code_stack) {
						Instructions.push_back(scope_code);
					}
				}
			}
		}

		m_Metadata.PopContext();

		return Instructions;
	}

	IRInstruction* Compiler::ArrayAccessCodeGen(const ArrayAccess* arrayAccess)
	{
		auto object = (IRSSAValue*)GetExpressionByValue(arrayAccess->Object);
		auto index = (IRSSAValue*)GetExpressionByValue(arrayAccess->Index);

		auto obj_expr_type = m_Metadata.GetExprType(object->SSA);

		if (!obj_expr_type.Pointer && !obj_expr_type.Array)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(arrayAccess->Object->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ "type of expression must be a pointer type in order to be accessed by the [] operator", MessageType::Warning });
		}

		if (UseArrayAccessInstruction) {

			//@Todo add support for non pointer arrays

			IRSSA* array_access_ssa = CreateIRSSA();

			IRArrayAccess* ir_array_Access = IR(IRArrayAccess());

			ir_array_Access->ArrayAddress = object->SSA;
			ir_array_Access->ElementSSA = index->SSA;
			ir_array_Access->Type = obj_expr_type.ID;


			array_access_ssa->Type = obj_expr_type.ID;
			array_access_ssa->Value = ir_array_Access;

			obj_expr_type.Pointer--;
			m_Metadata.RegExprType(array_access_ssa->ID, obj_expr_type);

			return IR(IRSSAValue(array_access_ssa->ID));
		}

		return nullptr;
	}

	IRInstruction* Compiler::TypeofCodeGen(const TypeOfNode* typeof)
	{
		u64 type_id = 0;
		u64 type_size = 0;

		u64 variable_id = 0;
		bool pointer = false;

		if (typeof->What->GetType() == NodeType::Identifier)
		{

			Identifier* type_ident = (Identifier*)typeof->What;

			SymbolType symbol_type = m_Metadata.GetSymbolType(type_ident->Symbol.Symbol);

			if (symbol_type == SymbolType::None)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(type_ident->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ "typeof() undefined expression inside '()' parentheses", MessageType::Warning });
			}

			if (symbol_type == SymbolType::Type)
			{
				type_id = m_Metadata.GetType(type_ident->Symbol.Symbol);
			}

			if (symbol_type == SymbolType::Variable)
			{
				const auto metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariableSSA(type_ident->Symbol.Symbol));
				type_id = metadata->Tipe.ID;
				pointer = metadata->Tipe.Pointer;
				variable_id = metadata->DataSSA->ID;
			}

			if (symbol_type == SymbolType::Enum)
			{
				const auto metadata = m_Metadata.GetEnum(type_ident->Symbol.Symbol);
				type_id = m_Metadata.GetType(type_ident->Symbol.Symbol);
				pointer = 0;
				variable_id = 0;
			}

			if (symbol_type == SymbolType::Function)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(type_ident->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ "typeof() doesn't support functions for now", MessageType::Warning });
			}
		}
		else
		{
			IRSSAValue* code = (IRSSAValue*)ExpressionCodeGen(typeof->What);
			type_id = m_Metadata.GetExprType(code->SSA).ID;
		}

		type_size = m_Metadata.GetTypeSize(type_id);

		TypeInfoType type_info_type;
		TypeInfo* type_info = nullptr;

		{
			u64 struct_id = m_Metadata.GetStructIDFromType(type_id);

			if (struct_id != (u64)-1)
			{

				const StructMetadata* struct_metadata = m_Metadata.GetStructMetadata(struct_id);

				TypeInfoStruct* type_info_struct = IR(TypeInfoStruct());

				type_info_struct->id = type_id;
				type_info_struct->name = m_Metadata.GetType(type_id);
				type_info_struct->pointer = pointer;
				type_info_struct->size = type_size;
				type_info_struct->flags |= TI_STRUCT;

				for (const MemberMetadata& member : struct_metadata->Members)
				{

					TypeInfoMember member_type_info;
					member_type_info.id = member.Tipe.ID;
					member_type_info.name = m_Metadata.GetType(member.Tipe.ID);

					member_type_info.member_name = member.Name.Symbol;
					member_type_info.pointer = m_Metadata.GetTypeSize(member.Tipe.Pointer);
					member_type_info.size = m_Metadata.GetTypeSize(member.Tipe.ID);

					member_type_info.flags |= TI_STRUCT_MEMBER;

					type_info_struct->members.push_back(member_type_info);
				}

				type_info_type = TypeInfoType::Struct;
				type_info = (TypeInfo*)type_info_struct;
			}
			else
			{

				TypeInfo* type_info_basic = IR(TypeInfo());

				type_info_basic->id = type_id;
				type_info_basic->name = m_Metadata.GetType(type_id);
				type_info_basic->pointer = pointer;
				type_info_basic->size = type_size;
				type_info_basic->flags = 0;

				type_info_type = TypeInfoType::Base;
				type_info = type_info_basic;
			}
		}

		IRSSA* ssa = CreateIRSSA();

		IRTypeOf type_of;

		type_of.typeInfoType = type_info_type;
		type_of.Type = type_info;

		ssa->Value = IR(type_of);

		ssa->Type = IR_typeinfo;
		ssa->Pointer = true;

		Type type;
		type.ID = IR_typeinfo;
		type.Pointer = 1;

		m_Metadata.RegExprType(ssa->ID, type);

		return IR(IRSSAValue(ssa->ID));
	}

	IRInstruction* Compiler::CastCodeGen(const CastNode* cast)
	{
		auto expr_value = (IRSSAValue*)GetExpressionByValue(cast->Expr);

		auto new_ssa = CreateIRSSA();

		new_ssa->Type = m_Metadata.GetType(cast->Type->Symbol.Symbol);
		new_ssa->Pointer = cast->Type->Pointer;
		new_ssa->Value = IR(IRPointerCast(new_ssa->Type, new_ssa->Pointer, expr_value->SSA));

		Glass::Type new_type;
		new_type.ID = new_ssa->Type;
		new_type.Pointer = new_ssa->Pointer;

		m_Metadata.RegExprType(new_ssa->ID, new_type);

		return IR(IRSSAValue(new_ssa->ID));
	}

	IRInstruction* Compiler::RefCodeGen(const RefNode* refNode)
	{
		IRSSAValue* expr_value = (IRSSAValue*)ExpressionCodeGen(refNode->What);
		const Glass::Type& exprType = m_Metadata.GetExprType(expr_value->SSA);

		IRAsAddress* ssa_value = IR(IRAsAddress(expr_value->SSA));

		IRSSA* ir_ssa = CreateIRSSA();
		ir_ssa->Type = exprType.ID;
		ir_ssa->Pointer = exprType.Pointer + 1;
		ir_ssa->Value = ssa_value;

		Glass::Type expr_type = exprType;
		expr_type.Pointer--;

		m_Metadata.RegExprType(ir_ssa->ID, expr_type);

		return IR(IRSSAValue(ir_ssa->ID));
	}

	//@DeDef
	//Its located here mainly for type checking
	//the llvm backend nor the c backend need any changes to the value of the pointer
	//because we use store and load everything is referred to by address
	//we just have to select either to load or to store by AssignmentCodeGen or GetExpressionByValue

	IRInstruction* Compiler::DeRefCodeGen(const DeRefNode* deRefNode)
	{
		IRSSAValue* expr_value = (IRSSAValue*)ExpressionCodeGen(deRefNode->What);
		const Glass::Type& exprType = m_Metadata.GetExprType(expr_value->SSA);

		Glass::Type expr_type = exprType;
		expr_type.Pointer--;

		m_Metadata.RegExprType(expr_value->SSA, expr_type);

		return expr_value;
	}

	IRSSAValue* Compiler::GetExpressionByValue(const Expression* expr)
	{
		switch (expr->GetType())
		{
		case NodeType::MemberAccess:
		{
			MemberAccess* member_access = (MemberAccess*)expr;

			if (member_access->Object->GetType() == NodeType::Identifier) {

				SymbolType symbol_type = m_Metadata.GetSymbolType(((Identifier*)member_access->Object)->Symbol.Symbol);

				if (symbol_type == SymbolType::Enum) {
					return (IRSSAValue*)EnumMemberAccessCodeGen(member_access);
				}
			}

			auto ir_address = (IRSSAValue*)MemberAccessCodeGen(member_access);

			if (ir_address == nullptr)
				return nullptr;

			const Glass::Type& expr_type = m_Metadata.GetExprType(ir_address->SSA);

			if (expr_type.Array) {
				return ir_address;
			}

			IRSSA* value_ssa = CreateIRSSA();

			IRLoad load;
			load.SSAddress = ir_address->SSA;
			load.Type = expr_type.ID;

			if (expr_type.Pointer)
			{
				load.ReferencePointer = true;
			}

			value_ssa->Value = IR(load);
			value_ssa->Type = load.Type;
			value_ssa->Pointer =
				m_Metadata.GetExprType(ir_address->SSA).Pointer == 1 || m_Metadata.GetExprType(ir_address->SSA).Array;

			m_Metadata.RegExprType(value_ssa->ID, m_Metadata.GetExprType(ir_address->SSA));

			return IR(IRSSAValue(value_ssa->ID));
		}
		break;
		case NodeType::ArrayAccess:
		{
			auto ir_address = (IRSSAValue*)ArrayAccessCodeGen((ArrayAccess*)expr);

			auto& expr_type = m_Metadata.GetExprType(ir_address->SSA);

			if (!expr_type.Pointer && !expr_type.Array)
			{
				IRSSA* value_ssa = CreateIRSSA();

				IRLoad load;

				load.SSAddress = ir_address->SSA;
				load.Type = expr_type.ID;

				value_ssa->Value = IR(load);
				value_ssa->Type = load.Type;
				value_ssa->Pointer = expr_type.Pointer;

				m_Metadata.RegExprType(value_ssa->ID, expr_type);

				return IR(IRSSAValue(value_ssa->ID));
			}

			return ir_address;
		}
		break;
		case NodeType::Identifier:
		{
			Identifier* identifier = (Identifier*)expr;

			SymbolType symbol_type = m_Metadata.GetSymbolType(identifier->Symbol.Symbol);

			if (symbol_type == SymbolType::Function) {
				return (IRSSAValue*)ExpressionCodeGen(expr);
			}

			if (symbol_type == SymbolType::Type)
			{
				u64 type_id = m_Metadata.GetType(identifier->Symbol.Symbol);

				NumericLiteral Node;
				Node.Val.Int = type_id;
				Node.type = NumericLiteral::Type::Int;

				return (IRSSAValue*)ExpressionCodeGen(AST(Node));
			}
			else if (symbol_type == SymbolType::Variable)
			{
				auto ir_address = (IRSSAValue*)IdentifierCodeGen(identifier);

				if (!ir_address)
				{
					return nullptr;
				}

				const auto metadata = m_Metadata.GetVariableMetadata(ir_address->SSA);
				if (!metadata->Tipe.Array)
				{
					IRLoad load;
					load.SSAddress = ir_address->SSA;
					load.ReferencePointer = metadata->Tipe.Pointer > 0;

					IRSSA* ssa = CreateIRSSA();

					IRSSA* var_ssa = m_Metadata.GetSSA(load.SSAddress);

					load.Type = metadata->Tipe.ID;

					ssa->Type = metadata->Tipe.ID;
					ssa->Value = IR(load);
					ssa->Pointer = metadata->Tipe.Pointer > 0;

					ssa->Reference = true;
					ssa->ReferenceType = metadata->Tipe.ID;
					ssa->PointerReference = metadata->Tipe.Pointer > 0;

					m_Metadata.RegExprType(ssa->ID, metadata->Tipe);

					return IR(IRSSAValue(ssa->ID));
				}
				else
				{
					return ir_address;
				}
			}
			else {

				auto ir_address = (IRSSAValue*)IdentifierCodeGen(identifier);

				if (!ir_address)
					return nullptr;

				const Glass::Type& expr_type = m_Metadata.GetExprType(ir_address->SSA);

				IRLoad load;
				load.SSAddress = ir_address->SSA;

				IRSSA* ssa = CreateIRSSA();

				load.Type = expr_type.ID;
				load.ReferencePointer = expr_type.Pointer;

				ssa->Type = expr_type.ID;
				ssa->Value = IR(load);
				ssa->Pointer = expr_type.Pointer;

				m_Metadata.RegExprType(ssa->ID, expr_type);

				return IR(IRSSAValue(ssa->ID));
			}
		}
		break;
		case NodeType::DeReference:
		{
			auto ir_address = (IRSSAValue*)ExpressionCodeGen(expr);
			const Glass::Type& expr_type = m_Metadata.GetExprType(ir_address->SSA);

			IRLoad* ir_load = IR(IRLoad());
			ir_load->Type = expr_type.ID;
			ir_load->ReferencePointer = expr_type.Pointer;
			ir_load->SSAddress = ir_address->SSA;

			auto load_ssa = CreateIRSSA();

			load_ssa->Type = expr_type.ID;
			load_ssa->Pointer = expr_type.Pointer;
			load_ssa->Value = ir_load;

			return IR(IRSSAValue(load_ssa->ID));
		}
		break;
		default:
			return (IRSSAValue*)ExpressionCodeGen(expr);
			break;
		}
	}

	IRSSAValue* Compiler::PassAsAny(const Expression* expr)
	{
		IRSSAValue* expr_result = (IRSSAValue*)ExpressionCodeGen(expr);

		if (expr_result == nullptr)
			return nullptr;

		const Glass::Type& expr_type = m_Metadata.GetExprType(expr_result->SSA);

		if (expr_type.ID == IR_any)
		{
			return expr_result;
		}

		IRSSA* any_ssa = CreateIRSSA();
		any_ssa->Type = m_Metadata.GetType("Any");

		any_ssa->Value = IR(IRAny(expr_result->SSA, expr_type.ID));

		Glass::Type result_type;
		result_type.ID = any_ssa->Type;

		m_Metadata.RegExprType(any_ssa->ID, result_type);

		auto ir_load_ssa = CreateIRSSA();

		IRLoad* ir_load = IR(IRLoad());
		ir_load->SSAddress = any_ssa->ID;
		ir_load->Type = IR_any;

		ir_load_ssa->Value = ir_load;
		ir_load_ssa->Type = IR_any;

		m_Metadata.RegExprType(ir_load_ssa->ID, expr_type);

		return IR(IRSSAValue(ir_load_ssa->ID));
	}

	IRSSAValue* Compiler::PassAsVariadicArray(u64 start, const std::vector<Expression*>& arguments, const ArgumentMetadata* decl_arg)
	{
		Glass::Type expr_type;
		expr_type.ID = IR_array;

		if (decl_arg->Tipe.ID == IR_any) {

			std::vector<IRAny> anys;
			anys.reserve(arguments.size());

			for (Expression* arg : arguments) {

				IRSSAValue* arg_ssa_val = (IRSSAValue*)ExpressionCodeGen(arg);
				const Glass::Type& arg_type = m_Metadata.GetExprType(arg_ssa_val->SSA);

				anys.push_back(IRAny(arg_ssa_val->SSA, arg_type.ID));
			}

			auto any_array_ir_ssa = CreateIRSSA();
			any_array_ir_ssa->Type = IR_array;
			any_array_ir_ssa->Value = IR(IRAnyArray(anys));;

			m_Metadata.RegExprType(any_array_ir_ssa->ID, expr_type);

			return IR(IRSSAValue(any_array_ir_ssa->ID));
		}

		GS_CORE_ASSERT(0, "Non any var args are yet to be implemented");

		return nullptr;
	}

	IRFunction* Compiler::CreateIRFunction(const FunctionNode* functionNode)
	{
		IRFunction* IRF = IR(IRFunction());
		IRF->ID = GetFunctionID();

		return IRF;
	}

	IRSSA* Compiler::CreateIRSSA()
	{
		IRSSA* SSA = IR(IRSSA());
		SSA->ID = m_SSAIDCounter;

		m_SSAIDCounter++;

		PushIRSSA(SSA);

		return SSA;
	}

	IRData* Compiler::CreateIRData()
	{
		IRData* Data = IR(IRData());
		Data->ID = m_DATAIDCounter;

		m_DATAIDCounter++;

		PushIRData(Data);

		return Data;
	}

	IRFunction* Compiler::CreatePolyMorhOverload(u64 ID, const PolyMorphOverloads& overloads)
	{
		FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(ID);

		ASTCopier copier = ASTCopier(metadata->FunctionAST);
		FunctionNode* new_function = (FunctionNode*)copier.Copy();

		std::unordered_map<std::string, std::string> replacements;

		std::string mangled_name_args;

		for (const auto& [polyID, arg] : overloads.TypeArguments)
		{
			replacements[metadata->PolyMorphicIDToTypeNames[polyID.ID]] = m_Metadata.GetType(arg.ID);
			mangled_name_args += m_Metadata.GetType(arg.ID);
		}

		ASTPolyMorpher poly_morpher = ASTPolyMorpher(new_function, replacements);
		poly_morpher.Poly();

		std::string function_name = fmt::format("{}__{}", new_function->Symbol.Symbol, mangled_name_args);

		new_function->Symbol.Symbol = function_name;

		auto& args = new_function->GetArgList()->GetArguments();

		for (Statement* a : args)
		{
			VariableNode* arg = (VariableNode*)a;

			arg->Type->PolyMorphic = false;
		}

		IRFunction* ir_func = CreateIRFunction(new_function);

		metadata->PendingPolymorphInstantiations.push_back({ ir_func, new_function });

		return ir_func;
	}
}