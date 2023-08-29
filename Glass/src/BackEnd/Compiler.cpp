#include "pch.h"

#include "Application.h"

#include "BackEnd/Compiler.h"
#include "FrontEnd/AstCopier.h"
#include "FrontEnd/AstPolymorpher.h"
#include "Interpeter/Interpeter.h"
#include "FrontEnd/Parser.h"
#include "FrontEnd/Lexer.h"

#define NULL_ID (u64)-1
#define MSG_LOC(x) PushMessage(CompilerMessage{ PrintTokenLocation((x->GetLocation())), MessageType::Error })
#define AST_LOC(x) PrintTokenLocation((x->GetLocation()))

#define FMT_WARN(x, ...) PushMessage(CompilerMessage{ fmt::format(x, __VA_ARGS__), MessageType::Warning})

namespace Glass
{
	TypeSystem* TypeSystem::m_Instance = nullptr;

	Compiler::Compiler(std::vector<CompilerFile*> files) : m_Files(files)
	{
	}

	void Compiler::InitTypeSystem()
	{
		m_Metadata.RegisterType(IR_void, "void", 0);

		m_Metadata.RegisterType(IR_float, "float", 4);
		m_Metadata.RegisterType(IR_int, "int", 4);

		m_Metadata.RegisterType(IR_i8, "i8", 1);
		m_Metadata.RegisterType(IR_i16, "i16", 2);
		m_Metadata.RegisterType(IR_i32, "i32", 4);
		m_Metadata.RegisterType(IR_i64, "i64", 8);

		m_Metadata.RegisterType(IR_u8, "u8", 1);
		m_Metadata.RegisterType(IR_u16, "u16", 2);
		m_Metadata.RegisterType(IR_u32, "u32", 4);
		m_Metadata.RegisterType(IR_u64, "u64", 8);

		m_Metadata.RegisterType(IR_f32, "f32", 4);
		m_Metadata.RegisterType(IR_f64, "f64", 8);

		m_Metadata.RegisterType(IR_bool, "bool", 1);

		m_Metadata.RegisterType(IR_type, "Type", 8);


		{
			m_Metadata.GetTypeFlags(IR_int) |= FLAG_BASE_TYPE;

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

			m_Metadata.GetTypeFlags(IR_type) |= FLAG_BASE_TYPE;
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

		TypeSystem::Init(m_Metadata);

		const fs_path base_file_path = "Library/Base.glass";

		std::ifstream in(base_file_path);
		std::stringstream base_buffer;
		base_buffer << in.rdbuf();

		std::string base_source = base_buffer.str();

		CompilerFile base_file = CompilerFile(10, base_source, base_file_path);

		Lexer lexer = Lexer(base_source, base_file_path);

		base_file.SetTokens(lexer.Lex());

		Parser parser = Parser(base_file);

		ModuleFile* base_Ast = parser.CreateAST();

		auto generate_base_struct = [this](StructNode* structNode, u64 type_id, u64 struct_id) {

			Token& struct_name = structNode->Name;
			auto& struct_members = structNode->m_Members;

			StructMetadata struct_metadata;
			struct_metadata.Name = struct_name;

			for (VariableNode* member : struct_members)
			{
				MemberMetadata member_metadata;
				member_metadata.Name = member->Symbol;

				auto type = TypeExpressionGetType(member->Type);

				member_metadata.Tipe = TSToLegacy(type);

				if (member_metadata.Tipe.ID == (u64)-1) {
					MSG_LOC(member);

					//@Todo Add type print helper and use it here

					FMT_WARN("struct '{}' member '{}' is of undefined type 'TODO'", struct_name.Symbol, member->Symbol.Symbol);
					return;
				}

				struct_metadata.Members.push_back(member_metadata);
			}

			m_Metadata.RegisterStruct(struct_id, type_id, struct_metadata);
		};

		for (Statement* stmt : base_Ast->GetStatements()) {
			NodeType node_type = stmt->GetType();

			if (node_type == NodeType::StructNode) {

				StructNode* stmt_Struct = (StructNode*)stmt;

				if (stmt_Struct->Name.Symbol == "Any") {
					generate_base_struct(stmt_Struct, IR_any, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "Array") {
					generate_base_struct(stmt_Struct, IR_array, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo") {
					generate_base_struct(stmt_Struct, IR_typeinfo, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Member") {
					generate_base_struct(stmt_Struct, IR_typeinfo_member, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Struct") {
					generate_base_struct(stmt_Struct, IR_typeinfo_struct, GetStructID());
				}
			}
			if (node_type == NodeType::Enum) {

				EnumNode* stmt_Enum = (EnumNode*)stmt;

				if (stmt_Enum->Name.Symbol == "TypeInfo_Flags") {
					EnumCodeGen(stmt_Enum, IR_typeinfo_flags);
				}
			}
		}

		m_TypeIDCounter = IR_typeinfo;
	}

	IRTranslationUnit* Compiler::CodeGen()
	{
		//////////////////////////////////////////////////////////////////////////
		//		@PUSH_SCOPE @GLOBAL
		m_Metadata.PushContextGlobal();
		//////////////////////////////////////////////////////////////////////////

		InitTypeSystem();

		IRTranslationUnit* tu = IR(IRTranslationUnit());
		for (CompilerFile* file : m_Files)
		{
			ModuleFile* module_file = file->GetAST();

			IRFile* ir_file = IR(IRFile());
			ir_file->File_Name = file->GetPath().filename().string();
			ir_file->Directory = file->GetPath().parent_path().string();

			for (const Statement* stmt : module_file->GetStatements())
			{
				auto stmt_code = StatementCodeGen(stmt);
				if (stmt_code != nullptr)
				{
					ir_file->Instructions.push_back(stmt_code);
				}
			}

			tu->Instructions.push_back(ir_file);

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

		//@Debugging
		RegisterDBGLoc(statement);

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

			auto return_type_ts = TypeExpressionGetType(fn_decl->ReturnType);

			Type return_type = TSToLegacy(return_type_ts);

			std::vector<ArgumentMetadata> args;

			for (const Statement* a : fn_decl->GetArgList()->GetArguments())
			{
				const ArgumentNode* decl_arg = (ArgumentNode*)a;

				ArgumentMetadata fmt_arg;

				fmt_arg.Name = decl_arg->Symbol.Symbol;
				fmt_arg.Tipe = TSToLegacy(TypeExpressionGetType(decl_arg->Type));

				args.push_back(fmt_arg);
			}

			m_Metadata.RegisterFunction(ID, fn_decl->Symbol.Symbol, return_type, args, fn_decl->CVariadic);

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
		return_type.ID = IR_void;

		if (functionNode->ReturnType)
		{
			auto return_type_ts = TypeExpressionGetType(functionNode->ReturnType);
			return_type = TSToLegacy(return_type_ts);
		}

		m_Metadata.RegisterFunction(poly_func_id, functionNode->Symbol.Symbol, return_type);

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

			ArgumentNode* arg = (ArgumentNode*)a;

			RegisterVariable(arg_address_ssa, arg->Symbol.Symbol);

			arg_address_ssa->Type = IR_u64;
			arg_address_ssa->Pointer = false;

			IRAddressOf address_of;

			{
				address_of.SSA = IR(IRARGValue(arg_address_ssa->ID));
			}

			arg_address_ssa->Value = IR(address_of);

			TypeStorage* argument_type = TypeExpressionGetType(arg->Type);
			Glass::Type legacy_argument_type;

			if (!arg->Variadic) {

				legacy_argument_type = TSToLegacy(argument_type);
			}
			else {
				legacy_argument_type.ID = IR_array;
			}

			if (!argument_type)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(arg->Type->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("argument '{}' is of undefined type '{}', at '{}' function definition",
														arg->Symbol.Symbol, PrintType(legacy_argument_type), functionNode->DefinitionTk.Symbol),
											MessageType::Warning });
				return nullptr;
			}

			VariableMetadata var_metadata = {
				arg->Symbol,
				nullptr,
				legacy_argument_type,
			};

			if (!arg->Variadic)
			{
				arg_ssa->Type = var_metadata.Tipe.ID;
			}
			else
			{
				arg_ssa->Type = IR_array;
			}

			arg_ssa->Pointer = legacy_argument_type.Pointer;

			m_Metadata.RegisterVariableMetadata(arg_address_ssa->ID, var_metadata);

			IRF->Arguments.push_back(arg_ssa);

			ArgumentMetadata arg_metadata;

			arg_metadata.Name = arg->Symbol.Symbol;

			arg_metadata.Tipe.Pointer = TypeSystem::IndirectionCount(argument_type);
			arg_metadata.Tipe.ID = argument_type->BaseID;

			arg_metadata.Variadic = arg->Variadic;
			arg_metadata.SSAID = arg_address_ssa->ID;

			args_metadata.push_back(arg_metadata);
		}

		m_Metadata.RegisterFunction(IRF->ID, functionNode->Symbol.Symbol, return_type, args_metadata, false, functionNode->Symbol);

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

		TypeStorage* Type = nullptr;

		u64 allocation_type = 0;
		u64 allocation_pointer = 0;

		if (variableNode->Type != nullptr) {
			Type = TypeExpressionGetType(variableNode->Type);
			u64 assignment_type_id = Type->BaseID;
			SetLikelyConstantType(assignment_type_id);

		}

		if (variableNode->Assignment != nullptr)
		{
			value = GetExpressionByValue(variableNode->Assignment);
			if (!value) {
				return nullptr;
			}
		}

		ResetLikelyConstantType();

		if (variableNode->Type == nullptr) {
			TypeStorage* assignment_type = m_Metadata.GetExprType(value->SSA);
			Type = assignment_type;
		}

		if (!Type) {
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->Type->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable is of unknown type '@TODO'"), MessageType::Warning });
			return nullptr;
		}


		if (Type->Kind == TypeStorageKind::Array)
		{
			allocation_type = IR_array;
		}
		else {
			allocation_type = Type->BaseID;
		}

		allocation_pointer = TypeSystem::IndirectionCount(Type);

		auto alloca = IR(IRAlloca((u32)allocation_type, (u32)allocation_pointer));

		IRSSA* variable_address_ssa = CreateIRSSA();
		variable_address_ssa->Type = Type->BaseID;
		variable_address_ssa->Pointer = 1;

		variable_address_ssa->Value = alloca;

		RegisterVariable(variable_address_ssa, variableNode->Symbol.Symbol);

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			Type,
			TSToLegacy(Type),
			false,
			false,
			nullptr,
			variable_address_ssa };

		m_Metadata.RegisterVariableMetadata(variable_address_ssa->ID, var_metadata);
		m_Metadata.RegExprType(variable_address_ssa->ID, Type);

		//@Debugging
		alloca->VarMetadata = m_Metadata.GetVariableMetadata(variable_address_ssa->ID);

		if (value)
		{
			IRStore* assignment_store = IR(IRStore());

			assignment_store->AddressSSA = variable_address_ssa->ID;
			assignment_store->Data = value;
			assignment_store->Pointer = TSToLegacy(Type).Pointer;
			assignment_store->Type = TSToLegacy(Type).ID;

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

		if (variableNode->Assignment != nullptr)
		{
			MSG_LOC(variableNode->Assignment);
			FMT_WARN("assignment for global variables is not supported yet");
			return nullptr;
		}

		IRSSA* StorageSSA = CreateIRSSA();

		TypeStorage* Type = TypeExpressionGetType(variableNode->Type);

		StorageSSA->Type = Type->BaseID;

		if (StorageSSA->Type == NULL_ID) {
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->Type->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable is of unknown type '@TODO'"), MessageType::Warning });
			return nullptr;
		}

		StorageSSA->Value = nullptr;

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			Type,
			TSToLegacy(Type),
			false,
			true,
			StorageSSA,
			nullptr };

		m_Metadata.RegisterGlobalVariable(glob_id, variableNode->Symbol.Symbol);
		m_Metadata.RegisterVariableMetadata(glob_id, var_metadata);

		IRGlobalDecl* global = IR(IRGlobalDecl());
		global->GlobID = glob_id;
		global->Type = Type;

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

		for (VariableNode* member_node : struct_members)
		{
			MemberMetadata member_metadata;
			member_metadata.Name = member_node->Symbol;

			member_metadata.Tipe = TSToLegacy(TypeExpressionGetType(member_node->Type));

			if (member_metadata.Tipe.ID == (u64)-1) {
				MSG_LOC(member_node);
				FMT_WARN("struct '{}' member '{}' is of undefined type '{}'", struct_name.Symbol, member_node->Symbol.Symbol);
				return nullptr;
			}

			struct_metadata.Members.push_back(member_metadata);
		}

		u64 type_id = GetTypeID();
		u64 struct_id = GetStructID();

		m_Metadata.RegisterStruct(struct_id, type_id, struct_metadata);

		m_Metadata.GetTypeFlags(type_id) |= TypeFlag::FLAG_STRUCT_TYPE;

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

	IRInstruction* Compiler::EnumCodeGen(const EnumNode* enumNode, u64 type_id /*= (u64)-1*/)
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

		if (type_id == NULL_ID) {
			type_id = GetEnumID();
		}

		m_Metadata.RegisterEnum(GetEnumID(), type_id, metadata);

		return nullptr;
	}

	IRInstruction* Compiler::IfCodeGen(const IfNode* ifNode)
	{
		IRSSAValue* condition = nullptr;

		auto cond = GetExpressionByValue(ifNode->Condition);

		if (!cond)
			return nullptr;

		auto condition_type = m_Metadata.GetExprType(cond->SSA);

		if (condition_type->BaseID != IR_bool)
		{
			SetLikelyConstantType(m_Metadata.GetSSA(cond->SSA)->Type);

			//@Hack
			NumericLiteral lit;
			lit.Val.Int = 0;
			lit.type = NumericLiteral::Type::Int;

			auto zero = (IRSSAValue*)NumericLiteralCodeGen(IR(lit));

			ResetLikelyConstantType();

			auto greater = IR(IRGreater(cond, zero));

			auto convert_to_bool = CreateIRSSA();
			convert_to_bool->Type = IR_bool;
			convert_to_bool->Value = greater;

			condition = IR(IRSSAValue(convert_to_bool->ID));
		}
		else {
			condition = cond;
		}

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

			if (!first_inst) {
				continue;
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

		if (!condition)
			return false;

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

			if (!inst) {
				continue;
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
		case NodeType::TypeExpression:
			return TypeExpressionCodeGen((TypeExpression*)expression);
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

			m_Metadata.RegExprType(ssa_val.SSA, LegacyToTS(metadata->Tipe));

			return (IRInstruction*)IR(ssa_val);
		}
		else if (symbol_type == SymbolType::GlobVariable) {

			u64 glob_id = m_Metadata.GetGlobalVariable(identifier->Symbol.Symbol);

			auto ssa = CreateIRSSA();
			ssa->Type = IR_u64;
			IRGlobalAddress* glob_address = IR(IRGlobalAddress(glob_id));

			ssa->Value = glob_address;

			const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(glob_id);

			m_Metadata.RegExprType(ssa->ID, metadata->Type);

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
			m_Metadata.RegExprType(ssa_value->SSA, TypeSystem::GetBasic(Value->Type));
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
			m_Metadata.RegExprType(ssa_val.SSA, TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u8), type.Pointer));
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

		TypeStorage* left_type = m_Metadata.GetExprType(A->SSA);
		TypeStorage* right_type = m_Metadata.GetExprType(B->SSA);

		IRSSA* IRssa = CreateIRSSA();

		IRssa->Type = left_type->BaseID;

		if ((left_type->Kind != TypeStorageKind::Pointer) && (right_type->Kind != TypeStorageKind::Pointer))
		{
			TypeFlags left_type_flags = m_Metadata.GetTypeFlags(left_type->BaseID);
			TypeFlags right_type_flags = m_Metadata.GetTypeFlags(right_type->BaseID);

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
						op_to_word(binaryExpr->OPerator), PrintType(TSToLegacy(left_type)), PrintType(TSToLegacy(right_type))),
					MessageType::Warning });
			};

			bool right_numeric_type = right_type_flags & FLAG_NUMERIC_TYPE;
			bool left_numeric_type = left_type_flags & FLAG_NUMERIC_TYPE;

			bool type_comparison = left_type->BaseID == IR_type && right_type->BaseID == IR_type;

			if (!(right_numeric_type && left_numeric_type) && !type_comparison)
			{
				OperatorQuery op_query;

				op_query.TypeArguments.push_back(TSToLegacy(left_type));
				op_query.TypeArguments.push_back(TSToLegacy(right_type));

				u64 op_func_id = m_Metadata.GetOperator(binaryExpr->OPerator, op_query);

				bool flipped = false;

				if (op_func_id == (u64)-1)
				{

					op_query.TypeArguments.clear();

					op_query.TypeArguments.push_back(TSToLegacy(left_type));
					op_query.TypeArguments.push_back(TSToLegacy(right_type));

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

					m_Metadata.RegExprType(op_result->SSA, left_type);

					return op_result;
				}
			}
		}

		u64 result_type = left_type->BaseID;

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
			result_type = IR_bool;
		}
		break;
		case Operator::NotEqual:
		{
			IRssa->Value = IR(IRNOTEQ(A, B));
			result_type = IR_bool;
		}
		break;
		case Operator::GreaterThan:
		{
			IRssa->Value = IR(IRGreater(A, B));
			result_type = IR_bool;
		}
		break;
		case Operator::LesserThan:
		{
			IRssa->Value = IR(IRLesser(A, B));
			result_type = IR_bool;
		}
		break;
		case Operator::GreaterThanEq:
		{
			IRssa->Value = IR(IRGreater(A, B));
			result_type = IR_bool;
		}
		break;
		case Operator::LesserThanEq:
		{
			IRssa->Value = IR(IRLesser(A, B));
			result_type = IR_bool;
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

		((IRBinOp*)IRssa->Value)->Type = result_type;

		IRSSAValue* ssa_value = IR(IRSSAValue());

		ssa_value->SSA = IRssa->ID;

		m_Metadata.RegExprType(ssa_value->SSA, TypeSystem::GetBasic(result_type));

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

				u64 var_ssa_id = GetVariableSSA(identifier_left->Symbol.Symbol);
				IRSSA* var_ssa = m_Metadata.GetSSA(var_ssa_id);

				const auto metadata = m_Metadata.GetVariableMetadata(var_ssa_id);

				SetLikelyConstantIntegerType(metadata->Tipe.ID);

				IRSSAValue* right_val = (IRSSAValue*)GetExpressionByValue(binaryExpr->Right);

				IRStore* store = IR(IRStore());
				{
					store->Data = right_val;
					store->AddressSSA = var_ssa_id;
					store->Type = metadata->Tipe.ID;
					store->Pointer = metadata->Tipe.Pointer;
				}

				ResetLikelyConstantType();

				return store;
			}
		}
		if (left->GetType() == NodeType::MemberAccess)
		{
			IRSSAValue* member_access = (IRSSAValue*)ExpressionCodeGen(left);
			TypeStorage* member_access_expr_type = m_Metadata.GetExprType(member_access->SSA);

			SetLikelyConstantIntegerType(member_access_expr_type->BaseID);

			IRSSAValue* right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			if (!member_access || !right_ssa) {
				return nullptr;
			}

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = member_access->SSA;
				store->Type = member_access_expr_type->BaseID;
				store->Pointer = TSToLegacy(member_access_expr_type).Pointer;
			}

			ResetLikelyConstantType();

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
				store->Type = m_Metadata.GetExprType(right_ssa->SSA)->BaseID;
			}
			return store;
		}
		if (left->GetType() == NodeType::DeReference)
		{
			auto left_ssa = (IRSSAValue*)ExpressionCodeGen(left);
			auto right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			if (!left_ssa || !right_ssa) {
				return nullptr;
			}

			auto expr_type = m_Metadata.GetExprType(right_ssa->SSA);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = left_ssa->SSA;
				store->Type = expr_type->BaseID;
				store->Pointer = TypeSystem::IndirectionCount(expr_type) - 1;
			}
			return store;
		}
		return nullptr;
	}

	IRInstruction* Compiler::FunctionCallCodeGen(const FunctionCall* call)
	{
		if (call->Function.Symbol == "type_info") {
			return TypeInfoCodeGen(call);
		}

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

				TypeStorage* expr_type = m_Metadata.GetExprType(expr_code->SSA);

				PolyMorphicType poly_type;
				poly_type.ID = metadata->GetPolyMorphID(metadata->Arguments[i].Name);

				overloads.TypeArguments.push_back({ poly_type, TSToLegacy(expr_type) });
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
					SetLikelyConstantType(decl_arg->Tipe.ID);

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

					ResetLikelyConstantType();

					if (argument_code == nullptr)
						return nullptr;

					TypeStorage* type = m_Metadata.GetExprType(argument_code->SSA);

					if (decl_arg->Tipe.ID != IR_any && !decl_arg->Variadic)
					{
						bool type_mismatch = !CheckTypeConversion(decl_arg->Tipe.ID, type->BaseID);

						if (type_mismatch)
						{

							PushMessage(CompilerMessage{ PrintTokenLocation(call->Arguments[i]->GetLocation()), MessageType::Error });
							PushMessage(CompilerMessage{ "type mismatch in function call", MessageType::Warning });
							PushMessage(CompilerMessage{ fmt::format("needed a '{}' instead got '{}'",
																	PrintType(decl_arg->Tipe),
																	PrintType(TSToLegacy(type))),

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

			m_Metadata.RegExprType(ir_ssa_val->SSA, LegacyToTS(metadata->ReturnType));

			return ir_ssa_val;
		}
		else {
			auto void_ssa = CreateIRSSA();
			void_ssa->Type = IR_void;
			void_ssa->Value = IR(ir_call);
			return nullptr;
		}
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

			const Glass::Type& obj_expr_type = TSToLegacy(m_Metadata.GetExprType(obj_ssa_value->SSA));

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

			m_Metadata.RegExprType(address_ssa->ID, LegacyToTS(result_type));

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

		SetLikelyConstantType(IR_u64);
		IRSSAValue* result = (IRSSAValue*)NumericLiteralCodeGen(AST(node));
		ResetLikelyConstantType();

		m_Metadata.RegExprType(result->SSA, TypeSystem::GetBasic(metadata->Name.Symbol));

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

				TypeStorage* expr_type = m_Metadata.GetExprType(expr_value->SSA);

				if (expr_type->Kind == TypeStorageKind::Pointer) {
					size = 8;
				}
				else if (expr_type->Kind == TypeStorageKind::DynArray) {
					size = 16;
				}
				else {
					size = m_Metadata.GetTypeSize(expr_type->BaseID);
				}
			}
		}
		else {

			IRSSAValue* expr_value = (IRSSAValue*)ExpressionCodeGen(size_of->Expr);

			TypeStorage* expr_type = m_Metadata.GetExprType(expr_value->SSA);

			if (expr_type->Kind == TypeStorageKind::Pointer) {
				size = 8;
			}
			else if (expr_type->Kind == TypeStorageKind::DynArray) {
				size = 16;
			}
			else {
				size = m_Metadata.GetTypeSize(expr_type->BaseID);
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

		if (!object || !index) {
			return nullptr;
		}

		auto obj_expr_type = m_Metadata.GetExprType(object->SSA);

		if (!TypeSystem::IsPointer(obj_expr_type) && !TypeSystem::IsArray(obj_expr_type))
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
			ir_array_Access->Type = obj_expr_type->BaseID;

			array_access_ssa->Type = obj_expr_type->BaseID;
			array_access_ssa->Value = ir_array_Access;

			obj_expr_type = TypeSystem::ReduceIndirection((TSPtr*)obj_expr_type);

			m_Metadata.RegExprType(array_access_ssa->ID, obj_expr_type);

			return IR(IRSSAValue(array_access_ssa->ID));
		}

		return nullptr;
	}

	IRInstruction* Compiler::TypeofCodeGen(const TypeOfNode* typeof)
	{
		u64 GlobalTypeInfoArrayIndex = -1;

		u64 type_id = -1;
		u64 pointer = -1;
		u64 array = -1;

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
			}

			if (symbol_type == SymbolType::Enum)
			{
				const auto metadata = m_Metadata.GetEnum(type_ident->Symbol.Symbol);
				type_id = m_Metadata.GetType(type_ident->Symbol.Symbol);
				pointer = 0;
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
			type_id = m_Metadata.GetExprType(code->SSA)->BaseID;
		}

		IRSSA* ssa = CreateIRSSA();

		IRTypeOf type_of;

		Glass::Type type_info_query;
		type_info_query.ID = type_id;

		type_of.Type = LegacyToTS(type_info_query);

		ssa->Value = IR(type_of);

		ssa->Type = IR_typeinfo;
		ssa->Pointer = true;

		Type type;
		type.ID = IR_typeinfo;
		type.Pointer = 1;

		m_Metadata.RegExprType(ssa->ID, LegacyToTS(type));

		return IR(IRSSAValue(ssa->ID));
	}

	IRInstruction* Compiler::CastCodeGen(const CastNode* cast)
	{
		auto expr_value = (IRSSAValue*)GetExpressionByValue(cast->Expr);

		if (!expr_value)
			return nullptr;

		auto new_ssa = CreateIRSSA();

		auto cast_type = TypeExpressionGetType(cast->Type);

		new_ssa->Type = cast_type->BaseID;
		new_ssa->Pointer = TypeSystem::IndirectionCount(cast_type);
		new_ssa->Value = IR(IRPointerCast(new_ssa->Type, new_ssa->Pointer, expr_value->SSA));

		m_Metadata.RegExprType(new_ssa->ID, cast_type);

		return IR(IRSSAValue(new_ssa->ID));
	}


	IRInstruction* Compiler::NullCodeGen()
	{
		auto null_ptr_ssa = CreateIRSSA();
		null_ptr_ssa->Type = IR_void;
		null_ptr_ssa->Pointer = 30;
		null_ptr_ssa->Value = IR(IRNullPtr(IR_void, 1));

		m_Metadata.RegExprType(null_ptr_ssa->ID, TypeSystem::GetPtr(TypeSystem::GetBasic(IR_void), 1));

		return IR(IRSSAValue(null_ptr_ssa->ID));
	}

	IRInstruction* Compiler::TypeInfoCodeGen(const FunctionCall* type_info_call)
	{
		GS_CORE_ASSERT(type_info_call->Function.Symbol == "type_info");

		if (type_info_call->Arguments.empty()) {
			MSG_LOC(type_info_call);
			FMT_WARN("'type_info' takes one argument of type 'Type'");
			return nullptr;
		}
		if (type_info_call->Arguments.size() > 1) {
			MSG_LOC(type_info_call);
			FMT_WARN("'type_info' too many arguments");
			FMT_WARN("'type_info' takes one argument of type 'Type'");
			return nullptr;
		}

		IRSSAValue* type_arg_value = GetExpressionByValue(type_info_call->Arguments[0]);

		if (!type_arg_value) {
			return nullptr;
		}

		TypeStorage* type_arg_type = m_Metadata.GetExprType(type_arg_value->SSA);

		if (type_arg_type != TypeSystem::GetBasic(IR_type)) {
			MSG_LOC(type_info_call);
			FMT_WARN("'type_info' type argument type mismatch");
			FMT_WARN("'type_info' needed a 'Type' instead got a {}", PrintType(TSToLegacy(type_arg_type)));
			return nullptr;
		}

		auto result_type = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_typeinfo), 1);

		IRSSA* type_info_ssa = CreateIRSSA();
		type_info_ssa->Type = result_type->BaseID;
		type_info_ssa->Value = IR(IRTypeInfo(type_arg_value->SSA));

		m_Metadata.RegExprType(type_info_ssa->ID, result_type);
		return IR(IRSSAValue(type_info_ssa->ID));
	}

	IRInstruction* Compiler::RefCodeGen(const RefNode* refNode)
	{
		IRSSAValue* referee = (IRSSAValue*)ExpressionCodeGen(refNode->What);

		if (!referee) {
			return nullptr;
		}

		auto referee_type = m_Metadata.GetExprType(referee->SSA);

		TSPtr* result_type = (TSPtr*)TypeSystem::IncreaseIndirection(referee_type);

		IRSSA* rvalue_ptr_storage = CreateIRSSA();
		rvalue_ptr_storage->Type = result_type->BaseID;
		rvalue_ptr_storage->Pointer = result_type->Indirection;
		rvalue_ptr_storage->Value = referee;

		m_Metadata.RegExprType(rvalue_ptr_storage->ID, result_type);

		return IR(IRSSAValue(rvalue_ptr_storage->ID));
	}

	//@DeDef
	//Its located here mainly for type checking
	//the llvm backend nor the c backend need any changes to the value of the pointer
	//because we use store and load everything is referred to by address
	//we just have to select either to load or to store by AssignmentCodeGen or GetExpressionByValue

	IRInstruction* Compiler::DeRefCodeGen(const DeRefNode* deRefNode)
	{
		IRSSAValue* expr_value = (IRSSAValue*)GetExpressionByValue(deRefNode->What);

		if (!expr_value)
			return nullptr;

		TypeStorage* exprType = m_Metadata.GetExprType(expr_value->SSA);

		u16 indirection_count = TypeSystem::IndirectionCount(exprType);

		if (indirection_count == 0) {
			MSG_LOC(deRefNode);
			FMT_WARN("trying to dereference a non pointer value, the type of said value is '{}'", PrintType(TSToLegacy(exprType)));
			return nullptr;
		}

		m_Metadata.RegExprType(expr_value->SSA, exprType);

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

			auto expr_type = m_Metadata.GetExprType(ir_address->SSA);

			if (TypeSystem::IsArray(expr_type)) {
				return ir_address;
			}

			IRSSA* value_ssa = CreateIRSSA();

			IRLoad load;
			load.SSAddress = ir_address->SSA;
			load.Type = expr_type->BaseID;

			if (TypeSystem::IsPointer(expr_type))
			{
				load.Pointer = true;
			}

			value_ssa->Value = IR(load);
			value_ssa->Type = load.Type;
			value_ssa->Pointer = TypeSystem::IsPointer(m_Metadata.GetExprType(ir_address->SSA));

			m_Metadata.RegExprType(value_ssa->ID, m_Metadata.GetExprType(ir_address->SSA));

			return IR(IRSSAValue(value_ssa->ID));
		}
		break;
		case NodeType::ArrayAccess:
		{
			auto ir_address = (IRSSAValue*)ArrayAccessCodeGen((ArrayAccess*)expr);

			if (!ir_address) {
				return nullptr;
			}

			auto expr_type = m_Metadata.GetExprType(ir_address->SSA);

			if (!TypeSystem::IsPointer(expr_type) && !TypeSystem::IsArray(expr_type))
			{
				IRSSA* value_ssa = CreateIRSSA();

				IRLoad load;

				load.SSAddress = ir_address->SSA;
				load.Type = expr_type->BaseID;

				value_ssa->Value = IR(load);
				value_ssa->Type = load.Type;
				value_ssa->Pointer = TSToLegacy(expr_type).Pointer;

				m_Metadata.RegExprType(value_ssa->ID, expr_type);

				return IR(IRSSAValue(value_ssa->ID));
			}

			return ir_address;
		}
		break;
		case NodeType::Identifier:
		{
			Identifier* identifier = (Identifier*)expr;

			if (identifier->Symbol.Symbol == "null") {
				return (IRSSAValue*)NullCodeGen();
			}

			SymbolType symbol_type = m_Metadata.GetSymbolType(identifier->Symbol.Symbol);

			if (symbol_type == SymbolType::Function) {
				return (IRSSAValue*)ExpressionCodeGen(expr);
			}

			if (symbol_type == SymbolType::Type)
			{
				return TypeValueCodeGen(TypeSystem::GetBasic(identifier->Symbol.Symbol));
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
					load.Pointer = metadata->Tipe.Pointer > 0;

					IRSSA* ssa = CreateIRSSA();

					IRSSA* var_ssa = m_Metadata.GetSSA(load.SSAddress);

					load.Type = metadata->Tipe.ID;

					ssa->Type = metadata->Tipe.ID;
					ssa->Value = IR(load);
					ssa->Pointer = metadata->Tipe.Pointer > 0;

					m_Metadata.RegExprType(ssa->ID, LegacyToTS(metadata->Tipe));

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

				auto expr_type = m_Metadata.GetExprType(ir_address->SSA);

				IRLoad load;
				load.SSAddress = ir_address->SSA;

				IRSSA* ssa = CreateIRSSA();

				load.Type = expr_type->BaseID;
				load.Pointer = TypeSystem::IsPointer(expr_type);

				ssa->Type = expr_type->BaseID;
				ssa->Value = IR(load);
				ssa->Pointer = TypeSystem::IndirectionCount(expr_type);

				m_Metadata.RegExprType(ssa->ID, expr_type);

				return IR(IRSSAValue(ssa->ID));
			}
		}
		break;
		case NodeType::DeReference:
		{
			auto ir_address = (IRSSAValue*)ExpressionCodeGen(expr);

			if (!ir_address)
				return nullptr;

			auto expr_type = m_Metadata.GetExprType(ir_address->SSA);
			auto new_type = TypeSystem::ReduceIndirection((TSPtr*)expr_type);

			u16 new_type_ind_count = (u64)TypeSystem::IndirectionCount(new_type);

			IRLoad* ir_load = IR(IRLoad());
			ir_load->Type = expr_type->BaseID;
			ir_load->Pointer = new_type_ind_count;
			ir_load->SSAddress = ir_address->SSA;

			auto load_ssa = CreateIRSSA();

			load_ssa->Type = expr_type->BaseID;
			load_ssa->Pointer = new_type_ind_count;
			load_ssa->Value = ir_load;

			m_Metadata.RegExprType(load_ssa->ID, new_type);

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
		IRSSAValue* expr_result = (IRSSAValue*)GetExpressionByValue(expr);

		if (expr_result == nullptr)
			return nullptr;

		auto expr_type = m_Metadata.GetExprType(expr_result->SSA);

		if (expr_type->BaseID == IR_any)
		{
			return expr_result;
		}

		IRSSA* any_ssa = CreateIRSSA();
		any_ssa->Type = m_Metadata.GetType("Any");

		any_ssa->Value = IR(IRAny(expr_result->SSA, expr_type));

		m_Metadata.RegExprType(any_ssa->ID, expr_type);

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
		if (decl_arg->Tipe.ID == IR_any) {

			std::vector<IRAny> anys;
			anys.reserve(arguments.size());

			for (Expression* arg : arguments) {

				IRSSAValue* arg_ssa_val = (IRSSAValue*)ExpressionCodeGen(arg);

				if (!arg_ssa_val) {
					return nullptr;
				}

				auto arg_type = m_Metadata.GetExprType(arg_ssa_val->SSA);

				anys.push_back(IRAny(arg_ssa_val->SSA, arg_type));
			}

			auto any_array_ir_ssa = CreateIRSSA();
			any_array_ir_ssa->Type = IR_array;
			any_array_ir_ssa->Value = IR(IRAnyArray(anys));;

			m_Metadata.RegExprType(any_array_ir_ssa->ID, TypeSystem::GetBasic(IR_array));

			return IR(IRSSAValue(any_array_ir_ssa->ID));
		}

		GS_CORE_ASSERT(0, "Non any var args are yet to be implemented");

		return nullptr;
	}


	IRSSAValue* Compiler::TypeExpressionCodeGen(TypeExpression* type_expr)
	{
		auto type = TypeExpressionGetType(type_expr);
		return TypeValueCodeGen(type);
	}

	IRSSAValue* Compiler::TypeValueCodeGen(TypeStorage* type)
	{
		auto expr_type = TypeSystem::GetBasic(IR_type);

		auto type_expr_value_ssa = CreateIRSSA();

		type_expr_value_ssa->Type = expr_type->BaseID;
		type_expr_value_ssa->Value = IR(IRTypeValue(type));

		m_Metadata.RegExprType(type_expr_value_ssa->ID, expr_type);

		return IR(IRSSAValue(type_expr_value_ssa->ID));
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

		SSA->SetDBGLoc(m_CurrentDBGLoc);

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

			//arg->Type->PolyMorphic = false;
		}

		IRFunction* ir_func = CreateIRFunction(new_function);

		metadata->PendingPolymorphInstantiations.push_back({ ir_func, new_function });

		return ir_func;
	}

	TypeStorage* Compiler::TypeExpressionGetType(TypeExpression* type_expr)
	{
		u16 indirection = 0;
		TypeStorage* type = nullptr;

		if (type_expr->GetType() == NodeType::TE_Pointer) {
			indirection = ((TypeExpressionPointer*)type_expr)->Indirection;
			type_expr = ((TypeExpressionPointer*)type_expr)->Pointee;
		}

		if (type_expr->GetType() == NodeType::TE_TypeName) {
			type = TypeSystem::GetBasic(((TypeExpressionTypeName*)type_expr)->Symbol.Symbol);
		}

		if (indirection) {
			type = TypeSystem::GetPtr(type, indirection);
		}

		return type;
	}

	Glass::Type Compiler::TSToLegacy(TypeStorage* type)
	{
		Glass::Type legacy;

		legacy.ID = type->BaseID;

		if (type->Kind == TypeStorageKind::Pointer) {
			legacy.Pointer = ((TSPtr*)type)->Indirection;
		}

		if (type->Kind == TypeStorageKind::DynArray) {
			legacy.Array = ((TSDynArray*)type);
		}

		return legacy;
	}


	TypeStorage* Compiler::LegacyToTS(const Glass::Type& type)
	{
		TypeStorage* new_ts_type = TypeSystem::GetBasic(type.ID);

		if (type.Pointer) {
			new_ts_type = TypeSystem::GetPtr(new_ts_type, type.Pointer);
		}

		if (type.Array) {
			new_ts_type = TypeSystem::GetDynArray(new_ts_type);
		}

		return new_ts_type;
	}
}