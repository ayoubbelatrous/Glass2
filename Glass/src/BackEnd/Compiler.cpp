#include "pch.h"

#include "Application.h"

#include "BackEnd/Compiler.h"
#include "FrontEnd/AstCopier.h"
#include "FrontEnd/AstPolymorpher.h"
#include "Interpeter/Interpeter.h"
#include "FrontEnd/Parser.h"
#include "FrontEnd/Lexer.h"

#define UN_WRAP(x) if ((x) == nullptr) {return nullptr;}

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

				if (!type) {
					//@Todo Add type print helper and use it here
					MSG_LOC(member);
					FMT_WARN("struct '{}' member '{}' is of undefined type 'TODO'", struct_name.Symbol, member->Symbol.Symbol);
					return;
				}

				member_metadata.Type = type;

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
		LoadLoop();

		//////////////////////////////////////////////////////////////////////////
		//		@PUSH_SCOPE @GLOBAL
		m_Metadata.PushContextGlobal();
		//////////////////////////////////////////////////////////////////////////

		InitTypeSystem();

		LibraryPass();
		FirstPass();

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

		return tu;
	}


	void Compiler::LoadLoop()
	{
		IRTranslationUnit* tu = IR(IRTranslationUnit());
		for (CompilerFile* file : m_Files)
		{
			ModuleFile* module_file = file->GetAST();

			for (Statement* stmt : module_file->GetStatements()) {
				if (stmt->GetType() == NodeType::Load) {
					LoadCodeGen((LoadNode*)stmt);
				}
			}

			m_CurrentFile++;
		}

		m_CurrentFile = 0;
	}

	void Compiler::LibraryPass()
	{
		m_CurrentFile = 0;
		for (CompilerFile* file : m_Files) {

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();

				switch (tl_type)
				{
				case NodeType::Library: {

					LibraryNode* library_node = (LibraryNode*)stmt;

					const Library* previous_library = m_Metadata.GetLibrary(library_node->Name.Symbol);

					if (previous_library) {
						MSG_LOC(library_node);
						FMT_WARN("Library '{}' already declared at '{}'", library_node->Name.Symbol, PrintTokenLocation(previous_library->Name));
						return;
					}

					Library library;
					library.Name = library_node->Name;
					library.Value = library_node->FileName;

					GS_CORE_ASSERT(library.Value);

					m_Metadata.InsertLibrary(library);
				}
									  break;
				}
			}

			m_CurrentFile++;
		}
		m_CurrentFile = 0;
	}

	void Compiler::FirstPass()
	{
		m_CurrentFile = 0;
		for (CompilerFile* file : m_Files) {

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();

				switch (tl_type)
				{
				case NodeType::Enum:
					HandleTopLevelEnum((EnumNode*)stmt);
					break;
				}
			}

			m_CurrentFile++;
		}
		m_CurrentFile = 0;
		for (CompilerFile* file : m_Files) {

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();

				switch (tl_type)
				{
				case NodeType::StructNode:
					HandleTopLevelStruct((StructNode*)stmt);
					break;
				}
			}

			m_CurrentFile++;
		}
		m_CurrentFile = 0;
		for (CompilerFile* file : m_Files) {

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();

				switch (tl_type)
				{
				case NodeType::Function:
					HandleTopLevelFunction((FunctionNode*)stmt);
					break;
				}
			}

			m_CurrentFile++;
		}
		m_CurrentFile = 0;
		for (CompilerFile* file : m_Files) {

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();
				if (tl_type == NodeType::Foreign) {
					ForeignCodeGen((ForeignNode*)stmt);
				}
			}

			m_CurrentFile++;
		}
		m_CurrentFile = 0;
		for (CompilerFile* file : m_Files) {

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();
				if (tl_type == NodeType::Operator) {
					OperatorCodeGen((OperatorNode*)stmt);
				}
			}

			m_CurrentFile++;
		}
		m_CurrentFile = 0;

		m_CurrentFile = 0;
		for (CompilerFile* file : m_Files) {

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();

				switch (tl_type)
				{
				case NodeType::StructNode:
					StructCodeGen((StructNode*)stmt);
					break;
				}
			}

			m_CurrentFile++;
		}
		m_CurrentFile = 0;

		SizingLoop();
	}

	void Compiler::SizingLoop()
	{
		auto resolve_depended_types = [this](StructMetadata& structure) {

			bool size_complete = true;

			for (MemberMetadata& member : structure.Members) {

				if (!member.SizeComplete) {

					if (member.Type->Kind != TypeStorageKind::Base) {
						member.SizeComplete = true;
					}
					else {

						u64 struct_id = m_Metadata.GetStructIDFromType(member.Type->BaseID);

						if (struct_id != NULL_ID) {

							auto member_type_struct = m_Metadata.GetStructMetadata(struct_id);

							GS_CORE_ASSERT(member_type_struct);

							if (member_type_struct->SizeComplete) {
								member.SizeComplete = true;
							}
						}
						else {
							member.SizeComplete = true;
						}
					}
				}

				size_complete = size_complete && member.SizeComplete;
			}

			if (size_complete) {
				m_Metadata.m_TypeSizes[structure.TypeID] = m_Metadata.ComputeStructSize(&structure);
			}

			structure.SizeComplete = size_complete;
		};

		//m_Metadata.RegisterStruct(structure.TypeID, m_Metadata.GetStructIDFromType(member.Type->BaseID), struct_metadata);

		u64 number_of_loops = 0;

		while (true) {

			u64 incomplete_count = 0;

			for (auto& [id, structure] : m_Metadata.m_StructMetadata) {
				if (!structure.SizeComplete) {
					incomplete_count++;
					resolve_depended_types(structure);
				}
			}

			if (incomplete_count == 0)
				break;

			number_of_loops++;
		}

		GS_CORE_WARN("Sizing Pass Took {} Loops", number_of_loops);
	}

	void Compiler::HandleTopLevelFunction(FunctionNode* fnNode)
	{
		FunctionMetadata* previous = m_Metadata.GetFunctionMetadata(m_Metadata.GetFunctionMetadata(fnNode->Symbol.Symbol));

		std::vector<ArgumentMetadata> arguments;

		bool poly_morphic = false;

		std::vector<ArgumentNode*> ASTArguments;

		for (const Statement* a : fnNode->GetArgList()->GetArguments())
		{
			ArgumentNode* parameter = (ArgumentNode*)a;
			poly_morphic |= parameter->PolyMorphic;
			poly_morphic |= parameter->Type->GetType() == NodeType::TE_Dollar;

			ASTArguments.push_back(parameter);
		}

		TypeStorage* return_type = nullptr;

		if (!poly_morphic) {

			if (fnNode->ReturnType == nullptr) {
				return_type = TypeSystem::GetVoid();
			}
			else {
				return_type = TypeExpressionGetType(fnNode->ReturnType);
			}

			for (const Statement* a : fnNode->GetArgList()->GetArguments())
			{
				const ArgumentNode* parameter = (ArgumentNode*)a;

				ArgumentMetadata argument;
				argument.Name = parameter->Symbol.Symbol;
				argument.Type = TypeExpressionGetType(parameter->Type);

				arguments.push_back(argument);
			}
		}

		std::vector<TypeStorage*> signature_arguments;
		for (auto& arg : arguments) {
			signature_arguments.push_back(arg.Type);
		}

		FunctionMetadata metadata;

		metadata.PolyMorphic = poly_morphic;

		if (!poly_morphic) {
			metadata.Signature = TypeSystem::GetFunction(signature_arguments, return_type);
			metadata.Arguments = arguments;
			metadata.ReturnType = return_type;
		}

		metadata.HasBody = false;
		metadata.Symbol = fnNode->Symbol;
		metadata.Ast = fnNode;

		metadata.ASTArguments = ASTArguments;
		metadata.ASTReturnType = fnNode->ReturnType;

		if (previous == nullptr) {
			m_Metadata.RegisterFunction(GetFunctionID(), metadata);
			return;
		}

		if (metadata.Signature == previous->Signature) {
			MSG_LOC(fnNode);
			FMT_WARN("function '{}' redefined with the same signature", fnNode->Symbol.Symbol);
			return;
		}
		else {
			FunctionMetadata* overload = previous->FindOverload((TSFunc*)metadata.Signature);
			if (overload) {
				MSG_LOC(fnNode);
				FMT_WARN("function '{}' redefined with the same signature", fnNode->Symbol.Symbol);
				return;
			}
		}

		previous->AddOverload(metadata);
	}

	void Compiler::HandleTopLevelStruct(StructNode* strct)
	{
		u64 type_id = GetTypeID();
		u64 struct_id = GetStructID();

		m_Metadata.RegisterType(type_id, strct->Name.Symbol, 0);

		StructMetadata struct_metadata;
		struct_metadata.Name = strct->Name;

		m_Metadata.RegisterStruct(struct_id, type_id, struct_metadata);
	}

	void Compiler::HandleTopLevelEnum(EnumNode* enmNode)
	{
		EnumMetadata metadata;
		metadata.Name = enmNode->Name;

		u64 enum_id = GetEnumID();
		u64 type_id = GetTypeID();

		m_Metadata.RegisterEnum(enum_id, type_id, metadata);
		EnumCodeGen(enmNode);
	}

	void Compiler::LoadCodeGen(LoadNode* loadNode)
	{
		std::string fileName = ((StringLiteral*)loadNode->FileName)->Symbol.Symbol;

		auto current_file_path = m_Files[m_CurrentFile]->GetPath();
		current_file_path.remove_filename();

		fs_path relative_path = current_file_path / fileName;

		if (!std::filesystem::exists(relative_path)) {
			MSG_LOC(loadNode);
			FMT_WARN("Cannot find path specified in #load directive, Path is: \"{}\"", fileName);
			return;
		}

		CompilerFile* file = CompilerFile::GenerateCompilerFile(relative_path);

		Lexer lexer = Lexer(file->GetSource(), relative_path);
		file->SetTokens(lexer.Lex());

		Parser parser = Parser(*file);
		file->SetAST(parser.CreateAST());

		std::vector<CompilerFile*> files_copy;
		files_copy.push_back(file);

		for (auto f : m_Files) {
			files_copy.push_back(f);
		}

		m_Files = files_copy;
	}

	IRInstruction* Compiler::StatementCodeGen(const Statement* statement)
	{
		NodeType Type = statement->GetType();

		//@Debugging
		RegisterDBGLoc(statement);

		switch (Type)
		{
			// 		case NodeType::Foreign:
			// 			return ForeignCodeGen((ForeignNode*)statement);
			// 			break;
						// 		case NodeType::Operator:
						// 			return OperatorCodeGen((OperatorNode*)statement);
						// 			break;
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
		case NodeType::Range:
		case NodeType::NegateExpression:
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
			// 		case NodeType::StructNode:
			// 			return StructCodeGen((StructNode*)statement);
			// 			break;
						// 		case NodeType::Enum:
						// 			return EnumCodeGen((EnumNode*)statement);
						// 			break;
		case NodeType::If:
			return IfCodeGen((IfNode*)statement);
			break;
		case NodeType::While:
			return WhileCodeGen((WhileNode*)statement);
			break;
		case NodeType::For:
			return ForCodeGen((ForNode*)statement);
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

			TypeStorage* return_type = nullptr;

			if (fn_decl->ReturnType) {

				return_type = TypeExpressionGetType(fn_decl->ReturnType);

				if (!return_type) {
					MSG_LOC(fn_decl->ReturnType);
					MSG_LOC(frn);
					FMT_WARN("unknown return type");
					return nullptr;
				}
			}
			else {
				return_type = TypeSystem::GetVoid();
			}

			std::vector<ArgumentMetadata> args;

			for (const Statement* a : fn_decl->GetArgList()->GetArguments())
			{
				const ArgumentNode* decl_arg = (ArgumentNode*)a;

				ArgumentMetadata fmt_arg;

				fmt_arg.Name = decl_arg->Symbol.Symbol;
				fmt_arg.Type = TypeExpressionGetType(decl_arg->Type);
				args.push_back(fmt_arg);
			}

			std::vector<TypeStorage*> signature_arguments;
			for (auto& arg : args) {
				signature_arguments.push_back(arg.Type);
			}
			auto signature = TypeSystem::GetFunction(signature_arguments, return_type);

			FunctionMetadata metadata;
			metadata.Foreign = true;
			metadata.Signature = signature;
			metadata.Arguments = args;
			metadata.Variadic = fn_decl->CVariadic;
			metadata.HasBody = false;
			metadata.ReturnType = return_type;
			metadata.Symbol = fn_decl->Symbol;

			m_Metadata.RegisterFunction(ID, metadata);

			return nullptr;
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
			return nullptr;
		}
		else if (tipe == NodeType::Variable) {
			return GlobalVariableCodeGen((VariableNode*)frn->statement, true);
		}

		GS_CORE_ASSERT(0, "Un-supported foreign entity");

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
				op_query.TypeArguments.push_back(TSToLegacy(arg.Type));
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
				op_query.TypeArguments.push_back(TSToLegacy(arg.Type));
			}

			m_Metadata.RegisterOperator(op_node->OPerator, op_query, function_id);

			return nullptr;
		}

		PushMessage(CompilerMessage{ PrintTokenLocation(op_node->GetLocation()), MessageType::Error });
		PushMessage(CompilerMessage{ "Expected a function name or a function definition after operator directive", MessageType::Warning });

		return nullptr;
	}

	IRInstruction* Compiler::FunctionCodeGen(FunctionNode* fnNode)
	{
		ResetSSAIDCounter();

		IRFunction* IRF = IR(IRFunction());

		IRF->ID = m_Metadata.GetFunctionMetadata(fnNode->Symbol.Symbol);
		auto metadata = m_Metadata.GetFunctionMetadata(IRF->ID);

		if (metadata->PolyMorphic) {
			return IRF;
		}

		TypeStorage* return_type;
		return_type = TypeSystem::GetBasic(IR_void);

		if (fnNode->ReturnType)
		{
			return_type = TypeExpressionGetType(fnNode->ReturnType);

			if (!return_type) {
				MSG_LOC(fnNode);
				MSG_LOC(fnNode->ReturnType);
				FMT_WARN("function '{}' has undefined return type", fnNode->Symbol.Symbol);
			}
		}

		SetExpectedReturnType(return_type);

		std::vector<ArgumentMetadata> args_metadata;

		PoPIRSSA();

		//////////////////////////////////////////////////////////////////////////
		//		@PUSH_SCOPE
		m_Metadata.PushContext(ContextScopeType::FUNC);
		//////////////////////////////////////////////////////////////////////////

		for (auto a : fnNode->GetArgList()->GetArguments())
		{
			IRSSA* arg_ssa = IR(IRSSA());
			arg_ssa->ID = m_SSAIDCounter;

			IRSSA* arg_address_ssa = CreateIRSSA();

			ArgumentNode* arg = (ArgumentNode*)a;

			RegisterVariable(arg_address_ssa, arg->Symbol.Symbol);

			IRAddressOf address_of;

			{
				address_of.SSA = IR(IRARGValue(arg_address_ssa->ID));
			}

			arg_address_ssa->Value = IR(address_of);

			TypeStorage* argument_type = TypeExpressionGetType(arg->Type);

			TypeStorage* argument_variable_type = nullptr;

			if (!arg->Variadic) {

				argument_variable_type = argument_type;
			}
			else {
				argument_variable_type = TypeSystem::GetBasic(IR_array);
			}

			if (!argument_type)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(arg->Type->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("argument '{}' is of undefined type '{}', at '{}' function definition",
														arg->Symbol.Symbol, PrintType(argument_type), fnNode->DefinitionTk.Symbol),
											MessageType::Warning });
				return nullptr;
			}

			VariableMetadata var_metadata = {
				arg->Symbol,
				argument_variable_type,
			};

			m_Metadata.RegisterVariableMetadata(arg_address_ssa->ID, var_metadata);

			IRF->Arguments.push_back(arg_ssa);

			ArgumentMetadata arg_metadata;

			arg_metadata.Name = arg->Symbol.Symbol;

			arg_metadata.Type = argument_type;

			arg_metadata.Variadic = arg->Variadic;
			arg_metadata.SSAID = arg_address_ssa->ID;

			args_metadata.push_back(arg_metadata);
		}

		std::vector<TypeStorage*> signature_arguments;
		for (auto& arg : args_metadata) {
			signature_arguments.push_back(arg.Type);
		}

		auto signature = TypeSystem::GetFunction(signature_arguments, return_type);

		if (metadata->Signature != signature) {
			metadata = metadata->FindOverload((TSFunc*)signature);
			IRF->Overload = metadata->Signature;
			if (metadata == nullptr) {
				return nullptr;
			}
		}

		metadata->Arguments = args_metadata;
		metadata->HasBody = true;

		for (const Statement* stmt : fnNode->GetStatements())
		{
			IRInstruction* code = StatementCodeGen(stmt);

			auto SSAs = PoPIRSSA();

			for (auto ssa : SSAs)
			{
				IRF->Instructions.push_back(ssa);
			}

			if (!code)
				continue;

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
		if (variableNode->Constant) {
			return ConstantCodeGen(variableNode);
		}

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

		TypeStorage* VariableType = nullptr;

		if (variableNode->Type != nullptr) {
			VariableType = TypeExpressionGetType(variableNode->Type);

			if (!VariableType) {
				return nullptr;
			}

			u64 assignment_type_id = VariableType->BaseID;
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

		TypeStorage* assignment_type = nullptr;

		if (variableNode->Assignment) {
			assignment_type = m_Metadata.GetExprType(value->SSA);
		}

		if (variableNode->Type == nullptr) {
			VariableType = assignment_type;
		}

		if (!VariableType) {
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->Type->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable is of unknown type"), MessageType::Warning });
			return nullptr;
		}

		if (variableNode->Assignment) {
			if (!TypeSystem::StrictPromotion(assignment_type, VariableType)) {
				MSG_LOC(variableNode->Assignment);
				MSG_LOC(variableNode);
				FMT_WARN("variable assigned to incompatible type: '{}' and the variable type is: '{}'", PrintType(assignment_type), PrintType(VariableType));
				return nullptr;
			}
		}

		auto alloca = IR(IRAlloca(VariableType));

		IRSSA* variable_address_ssa = CreateIRSSA();
		variable_address_ssa->Value = alloca;

		RegisterVariable(variable_address_ssa, variableNode->Symbol.Symbol);

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			VariableType,
			false,
			false,
			nullptr,
			variable_address_ssa };

		m_Metadata.RegisterVariableMetadata(variable_address_ssa->ID, var_metadata);
		m_Metadata.RegExprType(variable_address_ssa->ID, VariableType);

		//@Debugging
		alloca->VarMetadata = m_Metadata.GetVariableMetadata(variable_address_ssa->ID);

		if (value)
		{
			IRStore* assignment_store = IR(IRStore());

			assignment_store->AddressSSA = variable_address_ssa->ID;
			assignment_store->Data = value;
			assignment_store->Type = VariableType;

			return assignment_store;
		}

		return nullptr;
	}

	IRInstruction* Compiler::ConstantCodeGen(const VariableNode* variableNode)
	{
		auto previous_symbol_type = m_Metadata.GetSymbolType(variableNode->Symbol.Symbol);

		if (previous_symbol_type != SymbolType::None) {
			MSG_LOC(variableNode);
			FMT_WARN("name of constant '{}' already taken", variableNode->Symbol.Symbol);
		}

		if (!variableNode->Assignment) {
			MSG_LOC(variableNode);
			FMT_WARN("expected constant '{}' to be assigned", variableNode->Symbol.Symbol);
		}

		IRInstruction* assignment = GetExpressionByValue(variableNode->Assignment);
		UN_WRAP(assignment);

		{
			GS_CORE_ASSERT(assignment->GetType() == IRNodeType::SSAValue);
			IRSSA* assignment_register = GetSSA(((IRSSAValue*)assignment)->SSA);

			if (assignment_register->Value->GetType() != IRNodeType::ConstValue) {
				MSG_LOC(variableNode->Assignment);
				FMT_WARN("expected constant '{}' to be assigned to a constant value", variableNode->Symbol.Symbol);
				FMT_WARN("NOTE: we currently only support numeric literals as constant assignments", variableNode->Symbol.Symbol);
			}

			assignment = assignment_register->Value;
		}

		TypeStorage* constant_type = TypeExpressionGetType(variableNode->Type);

		if (!constant_type) {
			MSG_LOC(variableNode->Assignment);
			FMT_WARN("constant '{}' is of a un-known type", variableNode->Symbol.Symbol);
		}

		ConstantDecl constant;
		constant.Name = variableNode->Symbol;
		constant.Type = constant_type;
		constant.Value = assignment;

		m_Metadata.InsertConstant(variableNode->Symbol.Symbol, constant);

		return nullptr;
	}

	IRInstruction* Compiler::ConstantValueCodeGen(const ConstantDecl* constant)
	{
		GS_CORE_ASSERT(constant);
		GS_CORE_ASSERT(constant->Value);
		return CreateIRSSA(constant->Value, constant->Type);
	}

	IRInstruction* Compiler::GlobalVariableCodeGen(const VariableNode* variableNode, bool foreign /*= false*/)
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

		TypeStorage* Type = TypeExpressionGetType(variableNode->Type);

		if (Type == nullptr) {
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->Type->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable is of unknown type '@TODO'"), MessageType::Warning });
			return nullptr;
		}

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			Type,
			false,
			true,
			foreign,
			nullptr,
			nullptr };

		m_Metadata.RegisterGlobalVariable(glob_id, variableNode->Symbol.Symbol);
		m_Metadata.RegisterVariableMetadata(glob_id, var_metadata);

		IRGlobalDecl* global = IR(IRGlobalDecl());
		global->GlobID = glob_id;
		global->Type = Type;

		if (variableNode->Assignment != nullptr)
		{
			if (variableNode->Assignment->GetType() != NodeType::NumericLiteral) {
				MSG_LOC(variableNode->Assignment);
				FMT_WARN("const expression are not suppoerted yet you can only use numeric literals as an initializer for global variables");
				return nullptr;
			}

			if (foreign) {
				MSG_LOC(variableNode->Assignment);
				FMT_WARN("foreign global variables cannot have initializers");
				return nullptr;
			}

			auto as_numeric_literal = (NumericLiteral*)variableNode->Assignment;

			global->Initializer = IR(IRCONSTValue());

			if (as_numeric_literal->type == NumericLiteral::Type::Int) {
				memcpy(&global->Initializer->Data, &as_numeric_literal->Val.Int, sizeof(i64));
			}
			else if (as_numeric_literal->type == NumericLiteral::Type::Float) {
				memcpy(&global->Initializer->Data, &as_numeric_literal->Val.Int, sizeof(double));
			}

			global->Initializer->Type = Type->BaseID;
		}

		return global;
	}

	IRInstruction* Compiler::ReturnCodeGen(const ReturnNode* returnNode)
	{
		IRReturn ret;

		SetLikelyConstantType(GetExpectedReturnType()->BaseID);
		auto expr = (IRSSAValue*)GetExpressionByValue(returnNode->Expr);
		ResetLikelyConstantType();

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

			member_metadata.Type = TypeExpressionGetType(member_node->Type);

			if (!member_metadata.Type) {
				MSG_LOC(member_node);
				FMT_WARN("struct '{}' member '{}' is of undefined type 'TODO'", struct_name.Symbol, member_node->Symbol.Symbol);
				return nullptr;
			}

			struct_metadata.Members.push_back(member_metadata);
		}

		u64 type_id = m_Metadata.GetType(structNode->Name.Symbol);
		u64 struct_id = m_Metadata.GetStructIDFromType(type_id);

		m_Metadata.RegisterStruct(struct_id, type_id, struct_metadata);

		m_Metadata.GetTypeFlags(type_id) |= TypeFlag::FLAG_STRUCT_TYPE;
	}

	IRInstruction* Compiler::EnumCodeGen(const EnumNode* enumNode, u64 type_id /*= (u64)-1*/)
	{
		//{
		//	SymbolType symbol_type = m_Metadata.GetSymbolType(enumNode->Name.Symbol);
		//
		//	if (symbol_type == SymbolType::Enum) {
		//		const EnumMetadata* previous = m_Metadata.GetEnum(enumNode->Name.Symbol);
		//
		//		MSG_LOC(enumNode);
		//		FMT_WARN("enum '{}' is already defined At: {}", enumNode->Name.Symbol, PrintTokenLocation(previous->Name));
		//
		//		return nullptr;
		//	}
		//
		//	if (symbol_type != SymbolType::None) {
		//		MSG_LOC(enumNode);
		//		FMT_WARN("enum '{}' name is already taken", enumNode->Name.Symbol);
		//
		//		return nullptr;
		//	}
		//}


		EnumMetadata* metadata = m_Metadata.GetEnum(enumNode->Name.Symbol);

		if (metadata == nullptr) {
			EnumMetadata new_enum;
			new_enum.Name = enumNode->Name;
			m_Metadata.RegisterEnum(GetEnumID(), type_id, new_enum);

			metadata = m_Metadata.GetEnum(enumNode->Name.Symbol);
		}

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

			if (metadata->GetMember(member_metadata.Name)) {
				MSG_LOC(enumNode);
				MSG_LOC(identifier);
				FMT_WARN("enum '{}' .member already declared before", enumNode->Name.Symbol);
				return nullptr;
			}

			metadata->InsertMember(member_metadata.Name, member_metadata);

			IOTA++;
		}

		return nullptr;
	}

	IRInstruction* Compiler::IfCodeGen(const IfNode* ifNode)
	{
		auto condition = GetExpressionByValue(ifNode->Condition);

		if (!condition)
			return nullptr;

		auto condition_type = m_Metadata.GetExprType(condition->SSA);

		// 			MSG_LOC(ifNode);
		// 			FMT_WARN(
		// 				"Expected if condition type to be a type convertable to 'bool' but instead its: {}", PrintType(condition_type));
		// 		}

		IRIf IF;
		IF.SSA = condition->SSA;

		IRLexBlock lexical_block;

		lexical_block.Begin = ifNode->Scope->OpenCurly;
		lexical_block.End = ifNode->Scope->CloseCurly;

		PushScope();
		m_Metadata.PushContext(ContextScopeType::FUNC);
		for (const Statement* stmt : ifNode->Scope->GetStatements())
		{
			auto first_inst = StatementCodeGen(stmt);

			auto ir_ssa_stack = PoPIRSSA();

			for (auto inst : ir_ssa_stack)
			{
				lexical_block.Instructions.push_back(inst);
			}

			if (!first_inst) {
				continue;
			}

			lexical_block.Instructions.push_back(first_inst);
		}
		m_Metadata.PopContext();
		PopScope();

		IF.Instructions.push_back(IR(lexical_block));

		return IR(IF);
	}

	IRInstruction* Compiler::WhileCodeGen(const WhileNode* whileNode)
	{
		PushScope();
		IRSSAValue* condition = GetExpressionByValue(whileNode->Condition);
		std::vector<IRSSA*> condition_ssas = PoPIRSSA();
		PopScope();

		if (!condition)
			return nullptr;

		IRWhile WHILE;
		WHILE.SSA = condition->SSA;

		WHILE.ConditionBlock = condition_ssas;

		IRLexBlock lexical_block;

		lexical_block.Begin = whileNode->Scope->OpenCurly;
		lexical_block.End = whileNode->Scope->CloseCurly;

		PushScope();
		m_Metadata.PushContext(ContextScopeType::FUNC);

		for (const Statement* stmt : whileNode->Scope->GetStatements())
		{
			auto inst = StatementCodeGen(stmt);

			auto ir_ssa_stack = PoPIRSSA();

			for (auto ssa_inst : ir_ssa_stack)
			{
				lexical_block.Instructions.push_back(ssa_inst);
			}

			if (!inst) {
				continue;
			}

			lexical_block.Instructions.push_back(inst);
		}
		m_Metadata.PopContext();
		PopScope();

		WHILE.Instructions.push_back(IR(lexical_block));

		return IR(WHILE);
	}

	IRInstruction* Compiler::RangeCodeGen(const RangeNode* rangeNode)
	{
		//IRSSAValue * begin = GetExpressionByValue(rangeNode->Begin);
		//IRSSAValue * end = GetExpressionByValue(rangeNode->End);

		IRSSAValue* begin = nullptr;
		IRSSAValue* end = nullptr;

		TypeStorage* begin_type = nullptr;
		TypeStorage* end_type = nullptr;

		BinaryDispatch(rangeNode->Begin, rangeNode->End, &begin_type, &end_type, &begin, &end);

		if (!begin || !end)
			return nullptr;

		TypeStorage* type = nullptr;

		{
			if (begin_type != end_type) {
				MSG_LOC(rangeNode);
				FMT_WARN("range types do not match '{}'..'{}'", PrintType(begin_type), PrintType(end_type));
				return nullptr;
			}

			if (!(m_Metadata.GetTypeFlags(begin_type->BaseID) & FLAG_NUMERIC_TYPE) || m_Metadata.GetTypeFlags(begin_type->BaseID) & FLAG_FLOATING_TYPE) {
				MSG_LOC(rangeNode);
				FMT_WARN("range only supports integer types invalid type is '{}'", PrintType(begin_type));
				return nullptr;
			}

			type = begin_type;
		}

		IRIterator* iterator = IR(IRIterator());

		//:
		iterator->IteratorIndex = CreateIRSSA(IR(IRAlloca(type)));
		CreateIRSSA(CreateStore(type, iterator->IteratorIndex->SSA, begin));

		iterator->IteratorIt = CreateIRSSA(IR(IRAlloca(type)));
		CreateIRSSA(CreateStore(type, iterator->IteratorIt->SSA, begin));

		//Cond:
		{
			PushScope();
			IRSSAValue* cmp_inst = CreateIRSSA(IR(IRLesser(CreateLoad(type, iterator->IteratorIndex->SSA), end)));
			iterator->ConditionSSA = cmp_inst->SSA;

			auto ssa_stack = PoPIRSSA();
			for (auto inst : ssa_stack) {
				iterator->ConditionBlock.push_back(inst);
			}
			PopScope();
		}

		//Incrementor:
		{
			PushScope();

			auto index_load = CreateIRSSA(CreateLoad(type, iterator->IteratorIndex->SSA));
			auto index_addition = CreateIRSSA(IR(IRADD(index_load, CreateConstantInteger(type->BaseID, 1), type->BaseID)));
			CreateIRSSA(CreateStore(type, iterator->IteratorIndex->SSA, index_addition));
			CreateIRSSA(CreateStore(type, iterator->IteratorIt->SSA, index_addition));

			auto ssa_stack = PoPIRSSA();
			for (auto inst : ssa_stack) {
				iterator->IncrementorBlock.push_back(inst);
			}

			PopScope();
		}

		iterator->ItTy = type;
		iterator->IndexTy = type;

		return iterator;
	}


	IRIterator* Compiler::DynArrayIteratorCodeGen(const Expression* expression, IRSSAValue* generated)
	{
		TSDynArray* dynamic_array_type = (TSDynArray*)m_Metadata.GetExprType(generated->SSA);
		TypeStorage* dynmaic_array_element_type = dynamic_array_type->ElementType;

		IRIterator* iterator = IR(IRIterator());

		auto index_type = TypeSystem::GetBasic(IR_u64);
		auto element_type = dynmaic_array_element_type;

		//:
		iterator->IteratorIndex = CreateIRSSA(IR(IRAlloca(index_type)));
		CreateIRSSA(CreateStore(
			index_type,
			iterator->IteratorIndex->SSA,
			CreateConstantInteger(index_type->BaseID, 0)));

		iterator->IteratorIt = CreateIRSSA(IR(IRAlloca(dynmaic_array_element_type)));

		//Cond:
		{
			PushScope();

			auto data_member_type = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
			IRSSAValue* data_member = CreateLoad(data_member_type, CreateMemberAccess("Array", "data", generated->SSA)->SSA);

			auto index_load = CreateIRSSA(CreateLoad(index_type, iterator->IteratorIndex->SSA));


			auto casted_data = CreatePointerCast(TypeSystem::IncreaseIndirection(element_type), data_member->SSA);

			auto data_pointer = CreateIRSSA(IR(IRArrayAccess(casted_data->SSA, index_load->SSA, element_type)));

			CreateIRSSA(CreateStore(element_type, iterator->IteratorIt->SSA, CreateLoad(element_type, data_pointer->SSA)));

			auto end = CreateLoad(index_type, CreateMemberAccess("Array", "count", generated->SSA)->SSA);
			IRSSAValue* cmp_inst = CreateIRSSA(IR(IRLesser(index_load, end)));

			iterator->ConditionSSA = cmp_inst->SSA;

			auto ssa_stack = PoPIRSSA();
			for (auto inst : ssa_stack) {
				iterator->ConditionBlock.push_back(inst);
			}
			PopScope();
		}

		//Incrementor:
		{
			PushScope();

			auto index_load = CreateIRSSA(CreateLoad(index_type, iterator->IteratorIndex->SSA));
			auto index_addition = CreateIRSSA(IR(IRADD(index_load, CreateConstantInteger(index_type->BaseID, 1), index_type->BaseID)));
			CreateIRSSA(CreateStore(index_type, iterator->IteratorIndex->SSA, index_addition));

			auto ssa_stack = PoPIRSSA();
			for (auto inst : ssa_stack) {
				iterator->IncrementorBlock.push_back(inst);
			}

			PopScope();
		}

		iterator->ItTy = element_type;
		iterator->IndexTy = index_type;

		return iterator;
	}

	IRIterator* Compiler::IteratorCodeGen(const Expression* expr)
	{
		if (expr->GetType() == NodeType::Range)
			return (IRIterator*)RangeCodeGen((RangeNode*)expr);

		IRSSAValue* expr_result = (IRSSAValue*)ExpressionCodeGen(expr);

		if (!expr_result) {
			return nullptr;
		}

		if (m_Metadata.GetExprType(expr_result->SSA)->Kind == TypeStorageKind::DynArray) {
			return DynArrayIteratorCodeGen(expr, expr_result);
		}
	}

	IRInstruction* Compiler::ForCodeGen(const ForNode* forNode)
	{
		IRIterator* iterator = IteratorCodeGen(forNode->Condition);

		if (!iterator)
			return nullptr;

		IRWhile* while_inst = IR(IRWhile());

		while_inst->ConditionBlock = iterator->ConditionBlock;
		while_inst->SSA = iterator->ConditionSSA;

		PushScope();
		m_Metadata.PushContext(ContextScopeType::FUNC);

		m_Metadata.RegisterVariable(m_Metadata.GetSSA(iterator->IteratorIndex->SSA), "it_index");
		m_Metadata.RegisterVariable(m_Metadata.GetSSA(iterator->IteratorIt->SSA), "it");

		VariableMetadata it_index_metadata;
		it_index_metadata.Tipe = iterator->IndexTy;
		it_index_metadata.Name = forNode->Condition->GetLocation();
		it_index_metadata.Name.Symbol = "it_index";
		m_Metadata.RegisterVariableMetadata(iterator->IteratorIndex->SSA, it_index_metadata);
		((IRAlloca*)m_Metadata.GetSSA(iterator->IteratorIndex->SSA)->Value)->VarMetadata = m_Metadata.GetVariableMetadata(iterator->IteratorIndex->SSA);

		VariableMetadata it_metadata;
		it_metadata.Tipe = iterator->ItTy;
		it_metadata.Name = forNode->Condition->GetLocation();
		it_metadata.Name.Symbol = "it";
		m_Metadata.RegisterVariableMetadata(iterator->IteratorIt->SSA, it_metadata);
		((IRAlloca*)m_Metadata.GetSSA(iterator->IteratorIt->SSA)->Value)->VarMetadata = m_Metadata.GetVariableMetadata(iterator->IteratorIt->SSA);

		for (const Statement* stmt : forNode->Scope->GetStatements())
		{
			auto inst = StatementCodeGen(stmt);
			auto ir_ssa_stack = PoPIRSSA();
			for (auto inst : ir_ssa_stack)
			{
				while_inst->Instructions.push_back(inst);
			}
			if (!inst) {
				continue;
			}
			while_inst->Instructions.push_back(inst);
		}
		m_Metadata.PopContext();
		PopScope();

		for (auto inst : iterator->IncrementorBlock) {
			while_inst->Instructions.push_back(inst);
		}

		return while_inst;
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
		case NodeType::TE_TypeName:
		case NodeType::TE_Pointer:
		case NodeType::TE_Func:
		case NodeType::TE_Array:
			return TypeExpressionCodeGen((TypeExpression*)expression);
			break;
		case NodeType::Range:
			return RangeCodeGen((RangeNode*)expression);
			break;
		case NodeType::NegateExpression:
			return NegateCodeGen((NegateExpr*)expression);
			break;
		}

		return nullptr;
	}

	IRInstruction* Compiler::IdentifierCodeGen(const Identifier* identifier)
	{
		if (identifier->Symbol.Symbol == "null") {
			return NullCodeGen();
		}

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

			IRGlobalAddress* glob_address = IR(IRGlobalAddress(glob_id));

			ssa->Value = glob_address;

			const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(glob_id);

			m_Metadata.RegExprType(ssa->ID, metadata->Tipe);

			return IR(IRSSAValue(ssa->ID));
		}
		else if (symbol_type == SymbolType::Type) {
			return TypeValueCodeGen(TypeSystem::GetBasic(identifier->Symbol.Symbol));
		}
		else if (symbol_type == SymbolType::Constant) {
			return ConstantValueCodeGen(m_Metadata.GetConstant(identifier->Symbol.Symbol));
		}

		return nullptr;
	}

	IRInstruction* Compiler::NumericLiteralCodeGen(const NumericLiteral* numericLiteral)
	{
		IRSSA* IRssa = CreateIRSSA();

		IRssa->Value = IR(IRCONSTValue());

		IRCONSTValue* Constant = (IRCONSTValue*)IRssa->Value;

		if (numericLiteral->type == NumericLiteral::Type::Float)
		{
			Constant->Type = GetLikelyConstantFloatType();
			memcpy(&Constant->Data, &numericLiteral->Val.Float, sizeof(double));
		}
		else if (numericLiteral->type == NumericLiteral::Type::Int)
		{
			u64 likely_integer = GetLikelyConstantIntegerType();
			Constant->Type = likely_integer;
			memcpy(&Constant->Data, &numericLiteral->Val.Int, sizeof(i64));
		}
		else {
			GS_CORE_ASSERT(0);
		}

		if (GetLikelyConstantIntegerType() == IR_void) {
			__debugbreak();
		}

		m_Metadata.RegExprType(IRssa->ID, TypeSystem::GetBasic(Constant->Type));

		return IR(IRSSAValue(IRssa->ID));
	}

	IRInstruction* Compiler::StringLiteralCodeGen(const StringLiteral* stringLiteral)
	{
		IRData* data = CreateIRData();

		for (char c : stringLiteral->Symbol.Symbol)
		{
			data->Data.push_back(c);
		}

		auto ir_ssa = CreateIRSSA();
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

		TypeStorage* left_type = nullptr;
		TypeStorage* right_type = nullptr;

		if (binaryExpr->Left->GetType() != NodeType::NumericLiteral) {
			A = GetExpressionByValue(binaryExpr->Left);

			if (!A) {
				return nullptr;
			}

			left_type = m_Metadata.GetExprType(A->SSA);

			SetLikelyConstantType(left_type->BaseID);
		}

		if (binaryExpr->Right->GetType() != NodeType::NumericLiteral) {
			B = GetExpressionByValue(binaryExpr->Right);
			if (!B) {
				return nullptr;
			}
			right_type = m_Metadata.GetExprType(B->SSA);

			SetLikelyConstantType(right_type->BaseID);
		}

		if (left_type == nullptr) {
			A = GetExpressionByValue(binaryExpr->Left);
			left_type = m_Metadata.GetExprType(A->SSA);
		}
		if (right_type == nullptr) {
			B = GetExpressionByValue(binaryExpr->Right);
			right_type = m_Metadata.GetExprType(B->SSA);
		}

		ResetLikelyConstantType();

		if (!A || !B)
		{
			//@Todo: log error here
			return nullptr;
		}

		IRSSA* IRssa = CreateIRSSA();

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
				case Operator::And:
				case Operator::Or:
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

					call.Function.Symbol = op_func_metadata->Symbol.Symbol;

					IRSSAValue* op_result = (IRSSAValue*)ExpressionCodeGen(AST(call));

					m_Metadata.RegExprType(op_result->SSA, op_func_metadata->ReturnType);

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
		case Operator::And:
		{
			IRssa->Value = IR(IRAnd(A, B));
			result_type = IR_bool;
		}
		break;
		case Operator::Or:
		{
			IRssa->Value = IR(IROr(A, B));
			result_type = IR_bool;
		}
		break;
		default:
			GS_CORE_ASSERT(0, "Unsuppoerted operator");
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

		TypeStorage* right_type = nullptr;
		TypeStorage* left_type = nullptr;

		IRInstruction* result = nullptr;

		if (left->GetType() == NodeType::Identifier)
		{
			Identifier* identifier_left = (Identifier*)binaryExpr->Left;

			SymbolType symbol_type = m_Metadata.GetSymbolType(identifier_left->Symbol.Symbol);

			if (symbol_type == SymbolType::None) {
				MSG_LOC(binaryExpr);
				FMT_WARN("undefined name: '{}'", identifier_left->Symbol.Symbol);
				return nullptr;
			}

			if (symbol_type == SymbolType::GlobVariable)
			{
				IRSSAValue* right_val = (IRSSAValue*)GetExpressionByValue(binaryExpr->Right);

				if (!right_val) {
					return nullptr;
				}


				u64 glob_id = m_Metadata.GetGlobalVariable(identifier_left->Symbol.Symbol);

				auto ssa = CreateIRSSA();
				ssa->Value = IR(IRGlobalAddress(glob_id));

				right_type = m_Metadata.GetExprType(right_val->SSA);
				left_type = m_Metadata.GetVariableMetadata(glob_id)->Tipe;

				IRStore* store = IR(IRStore());
				{
					store->Data = right_val;
					store->AddressSSA = ssa->ID;
					store->Type = left_type;
				}

				result = store;
			}

			if (symbol_type == SymbolType::Variable) {

				u64 var_ssa_id = GetVariableSSA(identifier_left->Symbol.Symbol);
				IRSSA* var_ssa = m_Metadata.GetSSA(var_ssa_id);

				const auto metadata = m_Metadata.GetVariableMetadata(var_ssa_id);

				SetLikelyConstantType(metadata->Tipe->BaseID);

				IRSSAValue* right_val = (IRSSAValue*)GetExpressionByValue(binaryExpr->Right);

				if (!right_val) {
					return nullptr;
				}

				right_type = m_Metadata.GetExprType(right_val->SSA);
				left_type = metadata->Tipe;

				IRStore* store = IR(IRStore());
				{
					store->Data = right_val;
					store->AddressSSA = var_ssa_id;
					store->Type = left_type;
				}

				ResetLikelyConstantType();

				result = store;
			}
		}
		if (left->GetType() == NodeType::MemberAccess)
		{
			IRSSAValue* member_access = (IRSSAValue*)ExpressionCodeGen(left);

			if (!member_access)
				return nullptr;

			left_type = m_Metadata.GetExprType(member_access->SSA);

			SetLikelyConstantType(left_type->BaseID);

			IRSSAValue* right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			if (!right_ssa) {
				return nullptr;
			}

			right_type = m_Metadata.GetExprType(right_ssa->SSA);

			if (!member_access || !right_ssa) {
				return nullptr;
			}

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = member_access->SSA;
				store->Type = left_type;
			}

			ResetLikelyConstantType();

			result = store;
		}
		if (left->GetType() == NodeType::ArrayAccess)
		{
			auto left_ssa = (IRSSAValue*)ExpressionCodeGen(left);
			auto right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			if (!right_ssa)
				return nullptr;

			if (!left_ssa)
				return nullptr;

			left_type = m_Metadata.GetExprType(left_ssa->SSA);
			right_type = m_Metadata.GetExprType(right_ssa->SSA);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = left_ssa->SSA;
				store->Type = right_type;
			}
			result = store;
		}
		if (left->GetType() == NodeType::DeReference)
		{
			auto left_ssa = (IRSSAValue*)ExpressionCodeGen(left);
			auto right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			if (!left_ssa || !right_ssa) {
				return nullptr;
			}

			auto left_type_code_gen = m_Metadata.GetExprType(left_ssa->SSA);

			left_type = TypeSystem::ReduceIndirection((TSPtr*)left_type_code_gen);
			right_type = m_Metadata.GetExprType(right_ssa->SSA);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = left_ssa->SSA;
				store->Type = left_type_code_gen;
			}
			result = store;
		}

		GS_CORE_ASSERT(left_type);
		GS_CORE_ASSERT(right_type);
		GS_CORE_ASSERT(result);

		if (!TypeSystem::StrictPromotion(right_type, left_type)) {
			MSG_LOC(binaryExpr);
			FMT_WARN("incompatible types in assignment: '{}' = '{}'", PrintType(left_type), PrintType(right_type));
			return nullptr;
		}

		return result;
	}

	IRInstruction* Compiler::FunctionCallCodeGen(const FunctionCall* call)
	{
		if (call->Function.Symbol == "type_info") {
			return TypeInfoCodeGen(call);
		}

		SymbolType callee_symbol_type = m_Metadata.GetSymbolType(call->Function.Symbol);

		if (callee_symbol_type == SymbolType::Variable || callee_symbol_type == SymbolType::GlobVariable) {
			return FuncRefCallCodeGen(call);
		}

		u64 IRF = m_Metadata.GetFunctionMetadata(call->Function.Symbol);
		FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(IRF);

		if (metadata == nullptr)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("trying to call a undefined function '{}'", call->Function.Symbol), MessageType::Warning });
			return nullptr;
		}

		if (metadata->PolyMorphic) {
			return CallPolyMorphicFunction(call);
		}

		IRFunctionCall ir_call;
		ir_call.FuncID = IRF;

		std::vector<IRSSAValue*> argumentValueRefs;
		std::vector<TypeStorage*> argumentTypes;

		// 		if (call->Function.Symbol == "print") {
		// 			__debugbreak();
		// 		}

		for (size_t i = 0; i < call->Arguments.size(); i++)
		{
			auto argument_expr = call->Arguments[i];

			if (!metadata->IsOverloaded()) {
				if (i < metadata->Arguments.size()) {
					SetLikelyConstantType(metadata->Arguments[i].Type->BaseID);
				}
			}

			IRSSAValue* argument_as_value_ref = (IRSSAValue*)ExpressionCodeGen(argument_expr);

			if (!metadata->IsOverloaded()) {
				ResetLikelyConstantType();
			}

			if (!argument_as_value_ref)
				return nullptr;

			argumentTypes.push_back(m_Metadata.GetExprType(argument_as_value_ref->SSA));
			argumentValueRefs.push_back(argument_as_value_ref);
		}

		if (metadata->IsOverloaded()) {

			TSFunc* overload_query = (TSFunc*)TypeSystem::GetFunction(argumentTypes, TypeSystem::GetVoid());
			TSFunc* compare_query = (TSFunc*)TypeSystem::GetFunction(argumentTypes, ((TSFunc*)metadata->Signature)->ReturnType);

			if (compare_query != metadata->Signature) {

				FunctionMetadata* overload_metadata = metadata->FindOverloadForCall(overload_query);

				TSFunc* first_signature = (TSFunc*)metadata->Signature;

				FunctionMetadata* promotion = nullptr;

				bool overloaded_call = true;

				if (!overload_metadata) {
					for (auto& [sig, func] : metadata->Overloads) {
						if (sig->Arguments.size() == overload_query->Arguments.size()) {
							bool promotable = true;
							for (size_t i = 0; i < sig->Arguments.size(); i++)
							{
								bool check_result = TypeSystem::StrictPromotion(overload_query->Arguments[i], sig->Arguments[i]);
								promotable = promotable && check_result;
							}
							if (promotable) {
								promotion = &func;
							}
						}
					}
				}

				if (compare_query->Arguments.size() == first_signature->Arguments.size()) {
					bool promotable = true;
					for (size_t i = 0; i < compare_query->Arguments.size(); i++)
					{
						bool check_result = TypeSystem::StrictPromotion(overload_query->Arguments[i], first_signature->Arguments[i]);
						promotable = promotable && check_result;
					}
					if (promotable) {
						promotion = metadata;
						overloaded_call = false;
					}
				}

				if (promotion) {
					overload_metadata = promotion;
				}

				if (!overload_metadata) {
					MSG_LOC(call);
					FMT_WARN("function call to '{}', found no overload that could take: '{}'", call->Function.Symbol, PrintType(overload_query));
					return nullptr;
				}

				metadata = overload_metadata;
				if (overloaded_call) {
					ir_call.Overload = metadata->Signature;
				}
			}
		}
		else if (metadata->Arguments.size() != 0)
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

		for (size_t i = 0; i < call->Arguments.size(); i++)
		{
			const ArgumentMetadata* decl_arg = metadata->GetArgument(i);

			IRInstruction* arg = nullptr;

			IRSSAValue* argument_code = nullptr;

			if (decl_arg != nullptr)
			{
				if (!decl_arg->Variadic) {
					if (decl_arg->Type->BaseID == IR_any) {
						argument_code = PassAsAny(call->Arguments[i], argumentValueRefs[i]);
					}
					else {
						argument_code = GetExpressionByValue(call->Arguments[i], argumentValueRefs[i]);
					}
				}
				else {

					std::vector<IRSSAValue*> pre_generated_variadic_arguments;

					for (size_t j = i; j < argumentValueRefs.size(); j++) {
						pre_generated_variadic_arguments.push_back(argumentValueRefs[j]);
					}

					argument_code = PassAsVariadicArray(call->Arguments, pre_generated_variadic_arguments, decl_arg);
				}

				if (argument_code == nullptr)
					return nullptr;

				TypeStorage* type = m_Metadata.GetExprType(argument_code->SSA);

				if (decl_arg->Type->BaseID != IR_any && !decl_arg->Variadic)
				{
					bool type_mismatch = !TypeSystem::StrictPromotion(decl_arg->Type, type);

					if (type_mismatch)
					{

						PushMessage(CompilerMessage{ PrintTokenLocation(call->Arguments[i]->GetLocation()), MessageType::Error });
						PushMessage(CompilerMessage{ "type mismatch in function call", MessageType::Warning });
						PushMessage(CompilerMessage{ fmt::format("needed a '{}' instead got '{}'",
																PrintType(decl_arg->Type),
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
				arg = GetExpressionByValue(call->Arguments[i], argumentValueRefs[i]);
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

		bool has_return = metadata->ReturnType->Kind == TypeStorageKind::Pointer;
		has_return |= metadata->ReturnType->BaseID != IR_void;

		if (has_return)
		{
			auto ir_ssa = CreateIRSSA();
			ir_ssa->Value = IR(ir_call);

			IRSSAValue* ir_ssa_val = IR(IRSSAValue(ir_ssa->ID));

			m_Metadata.RegExprType(ir_ssa_val->SSA, metadata->ReturnType);

			return ir_ssa_val;
		}
		else {
			auto void_ssa = CreateIRSSA();
			void_ssa->Value = IR(ir_call);
			return nullptr;
		}
	}

	IRInstruction* Compiler::CallPolyMorphicFunction(const FunctionCall* call)
	{
		u64 function_id = m_Metadata.GetFunctionMetadata(call->Function.Symbol);
		FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(function_id);

		GS_CORE_ASSERT(metadata);
		GS_CORE_ASSERT(metadata->PolyMorphic);

		std::vector<IRInstruction*> call_values;
		std::vector<TypeStorage*> call_types;

		if (call->Arguments.size() != metadata->ASTArguments.size()) {
			GS_CORE_ASSERT("argument count mismatch");
			return nullptr;
		}

		std::map<std::string, Expression*> replacements;

		for (size_t i = 0; i < call->Arguments.size(); i++)
		{
			ArgumentNode* call_parameter = metadata->ASTArguments[i];
			Expression* call_argument = call->Arguments[i];

			if (call_parameter->PolyMorphic) //PlaceHolder until we add proper constants system
			{
				auto argument_value = GetExpressionByValue(call_argument);

				if (!argument_value)
					return nullptr;

				auto argument_type = m_Metadata.GetExprType(argument_value->SSA);

				call_values.push_back(argument_value);
				call_types.push_back(argument_type);

				auto constant_arg_type = TypeExpressionGetType(call_parameter->Type);

				if (TypeSystem::GetBasic(IR_type) != constant_arg_type) {
					MSG_LOC(call);
					FMT_WARN("we only support 'Type' as a type for polymorphic parameter values currently");
					return nullptr;
				}

				replacements[call_parameter->Symbol.Symbol] = call_argument;
			}
			else {

				if (call_parameter->Type->GetType() != NodeType::TE_Dollar) {
					auto assumed_type = TypeExpressionGetType(call_parameter->Type);
					SetLikelyConstantType(assumed_type->BaseID);
				}

				auto argument_value = GetExpressionByValue(call_argument);

				if (call_parameter->Type->GetType() != NodeType::TE_Dollar) {
					ResetLikelyConstantType();
				}

				if (!argument_value)
					return nullptr;

				auto argument_type = m_Metadata.GetExprType(argument_value->SSA);

				call_values.push_back(argument_value);
				call_types.push_back(argument_type);

				if (call_parameter->Type->GetType() == NodeType::TE_Dollar) {
					TypeExpressionTypeName* polymorphic_selector_name = (TypeExpressionTypeName*)((TypeExpressionDollar*)call_parameter->Type)->TypeName;
					replacements[polymorphic_selector_name->Symbol.Symbol] = TypeGetTypeExpression(argument_type);
				}
			}
		}

		//LookUp

		auto cached = metadata->PolyMorphicInstantiations.find(PolymorphicOverload{ replacements });

		if (cached != metadata->PolyMorphicInstantiations.end()) {

			//Calling

			auto ir_func = cached->second;

			auto call_return_type = m_Metadata.GetFunctionMetadata(ir_func->ID)->ReturnType;

			if (call_return_type != TypeSystem::GetVoid()) {
				return CreateIRSSA(IR(IRFunctionCall(call_values, ir_func->ID)), call_return_type);
			}

			return IR(IRFunctionCall(call_values, ir_func->ID));
		}

		ASTCopier copier(metadata->Ast);
		auto ast_copy_as_function = (FunctionNode*)copier.Copy();
		GS_CORE_ASSERT(ast_copy_as_function->GetType() == NodeType::Function);

		for (Statement* parameter : ast_copy_as_function->GetArgList()->GetArguments()) {
			ArgumentNode* call_parameter = (ArgumentNode*)parameter;
			call_parameter->PolyMorphic = false;
		}

		ASTPolyMorpher poly_morpher(ast_copy_as_function, replacements);
		poly_morpher.Poly();

		std::string name_mangling = "__";
		for (auto& [name, expr] : replacements) {
			auto type = TypeExpressionGetType((TypeExpression*)expr);
			GS_CORE_ASSERT(type);
			name_mangling.append(PrintType(type));
		}

		ast_copy_as_function->Symbol.Symbol = ast_copy_as_function->Symbol.Symbol + name_mangling;

		HandleTopLevelFunction(ast_copy_as_function);

		u64 register_counter = m_SSAIDCounter;
		u64 calling_function_ctx_id = m_Metadata.CurrentContext()->ID;
		auto calling_function_return_type = GetExpectedReturnType();
		m_Metadata.PopContext();

		PushScope();
		IRFunction* ir_func = (IRFunction*)FunctionCodeGen(ast_copy_as_function);
		PopScope();

		SetExpectedReturnType(calling_function_return_type);

		m_SSAIDCounter = register_counter;
		m_Metadata.m_CurrentFunction--;

		m_Metadata.PushContext(calling_function_ctx_id);

		metadata->PolyMorphicInstantiations.emplace(PolymorphicOverload{ replacements }, ir_func);


		//Calling

		auto call_return_type = m_Metadata.GetFunctionMetadata(ir_func->ID)->ReturnType;

		if (call_return_type != TypeSystem::GetVoid()) {
			return CreateIRSSA(IR(IRFunctionCall(call_values, ir_func->ID)), call_return_type);
		}

		return IR(IRFunctionCall(call_values, ir_func->ID));
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

		TypeStorage* result_type = nullptr;

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

			result_type = struct_metadata->Members[member_id].Type;
		}


		IRMemberAccess ir_mem_access;

		{
			ir_mem_access.StructID = struct_id;
			ir_mem_access.ObjectSSA = object_ssa_id;
			ir_mem_access.MemberID = member_id;
			ir_mem_access.ReferenceAccess = reference_access;

			auto address_ssa = CreateIRSSA();

			address_ssa->Value = IR(ir_mem_access);

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


	IRInstruction* Compiler::NegateCodeGen(const NegateExpr* negateNode)
	{
		auto what_code = GetExpressionByValue(negateNode->What);
		if (!what_code)
			return nullptr;
		auto what_type = m_Metadata.GetExprType(what_code->SSA);
		return CreateIRSSA(IR(IRSUB(CreateConstant(what_type->BaseID, 0, 0.0), what_code, what_type->BaseID)), what_type);
	}

	IRInstruction* Compiler::FunctionRefCodegen(const Identifier* func)
	{
		u64 func_id = m_Metadata.GetFunctionMetadata(func->Symbol.Symbol);
		GS_CORE_ASSERT(func_id != NULL_ID, "Function must exist at this point");

		const FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(func_id);

		auto ssa = CreateIRSSA();
		ssa->Value = IR(IRFuncRef(func_id));

		m_Metadata.RegExprType(ssa->ID, metadata->Signature);

		return IR(IRSSAValue(ssa->ID));
	}

	IRInstruction* Compiler::FuncRefCallCodeGen(const FunctionCall* call)
	{
		//We only support function calls as variables for now (no expressions)

		Identifier* function_as_identifier = AST(Identifier());
		function_as_identifier->Symbol = call->Function;

		auto callee_ssa = GetExpressionByValue(function_as_identifier);
		if (!callee_ssa)
			return nullptr;
		auto callee_type = m_Metadata.GetExprType(callee_ssa->SSA);

		std::vector<u64> argument_SSAs;

		u64 index = 0;
		for (auto arg : call->Arguments) {
			SetLikelyConstantType(((TSFunc*)callee_type)->Arguments[index]->BaseID);
			auto argument_code = GetExpressionByValue(arg);
			ResetLikelyConstantType();

			if (!argument_code) {
				return nullptr;
			}

			argument_SSAs.push_back(argument_code->SSA);
			index++;
		}

		auto ir_call = IR(IRCallFuncRef(callee_ssa->SSA, argument_SSAs, callee_type));

		if (((TSFunc*)callee_type)->ReturnType != TypeSystem::GetBasic(IR_void)) {
			auto return_ssa = CreateIRSSA();
			return_ssa->Value = ir_call;
			m_Metadata.RegExprType(return_ssa->ID, ((TSFunc*)callee_type)->ReturnType);
			return IR(IRSSAValue(return_ssa->ID));
		}
		else {
			return ir_call;
		}
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
		auto object = (IRSSAValue*)ExpressionCodeGen(arrayAccess->Object);
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

		if (TypeSystem::IsArray(obj_expr_type)) {

			const StructMetadata* array_struct = m_Metadata.GetStructFromType(IR_array);

			IRMemberAccess* data_member_access = IR(IRMemberAccess());
			data_member_access->MemberID = array_struct->FindMember("data");
			data_member_access->ObjectSSA = object->SSA;
			data_member_access->StructID = m_Metadata.GetStructIDFromType(IR_array);
			data_member_access->ReferenceAccess = false;

			auto data_member_access_ssa = CreateIRSSA();
			data_member_access_ssa->Value = data_member_access;

			auto data_member_access_load_ssa =
				CreateLoad(TypeSystem::GetPtr(TypeSystem::GetBasic(IR_void), 1), data_member_access_ssa->ID);

			auto data_member_access_pointer_cast_ssa = CreateIRSSA();
			auto element_ty = TypeSystem::GetPtr(TypeSystem::GetArrayElementTy(obj_expr_type), 1);
			data_member_access_pointer_cast_ssa->Value = IR(IRPointerCast(element_ty, data_member_access_load_ssa->SSA));
			object = IR(IRSSAValue(data_member_access_pointer_cast_ssa->ID));

			obj_expr_type = element_ty;
		}
		else {
			object = GetExpressionByValue(arrayAccess->Object, object);
		}

		//@Todo add support for non pointer arrays

		IRSSA* array_access_ssa = CreateIRSSA();

		IRArrayAccess* ir_array_Access = IR(IRArrayAccess());

		ir_array_Access->ArrayAddress = object->SSA;
		ir_array_Access->ElementSSA = index->SSA;
		ir_array_Access->Type = TypeSystem::GetBasic(obj_expr_type->BaseID);

		array_access_ssa->Value = ir_array_Access;

		obj_expr_type = TypeSystem::ReduceIndirection((TSPtr*)obj_expr_type);

		m_Metadata.RegExprType(array_access_ssa->ID, obj_expr_type);

		return IR(IRSSAValue(array_access_ssa->ID));
	}

	IRInstruction* Compiler::TypeofCodeGen(const TypeOfNode* typeof)
	{
		u64 GlobalTypeInfoArrayIndex = -1;

		TypeStorage* type_of_type = nullptr;

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
				type_of_type = TypeSystem::GetBasic(type_ident->Symbol.Symbol);
			}

			if (symbol_type == SymbolType::Variable)
			{
				const auto metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariableSSA(type_ident->Symbol.Symbol));
				type_of_type = metadata->Tipe;
			}

			if (symbol_type == SymbolType::Enum)
			{
				const auto metadata = m_Metadata.GetEnum(type_ident->Symbol.Symbol);
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
			type_of_type = m_Metadata.GetExprType(code->SSA);
		}

		IRSSA* ssa = CreateIRSSA();

		IRTypeOf type_of;
		type_of.Type = type_of_type;

		ssa->Value = IR(type_of);

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
		auto castee_type = m_Metadata.GetExprType(expr_value->SSA);

		auto castee_size = m_Metadata.GetTypeSize(castee_type->BaseID);
		auto cast_size = m_Metadata.GetTypeSize(cast_type->BaseID);

		auto castee_flags = m_Metadata.GetTypeFlags(castee_type->BaseID);
		auto cast_flags = m_Metadata.GetTypeFlags(cast_type->BaseID);

		if (cast_type == castee_type) {
			m_Metadata.RegExprType(new_ssa->ID, cast_type);
			return expr_value;
		}

		IRCast* cast_ir_node = nullptr;


		if (TypeSystem::IsPointer(castee_type) && TypeSystem::IsPointer(cast_type)) {
			cast_ir_node = (IRCast*)IR(IRPointerCast(cast_type, expr_value->SSA));
		}
		else if (TypeSystem::IsPointer(castee_type) && !TypeSystem::IsPointer(cast_type)) {
			cast_ir_node = (IRCast*)IR(IRPtr2IntCast(cast_type, expr_value->SSA));
		}
		else if (!TypeSystem::IsPointer(castee_type) && TypeSystem::IsPointer(cast_type)) {
			cast_ir_node = (IRCast*)IR(IRInt2PtrCast(cast_type, expr_value->SSA));
		}
		else {

			if (castee_flags & FLAG_NUMERIC_TYPE && cast_flags & FLAG_NUMERIC_TYPE) {

				if (!(castee_flags & FLAG_FLOATING_TYPE) && !(cast_flags & FLAG_FLOATING_TYPE)) {

					if (castee_size < cast_size) {
						if (
							castee_flags & FLAG_UNSIGNED_TYPE &&
							cast_flags & FLAG_UNSIGNED_TYPE
							) {
							cast_ir_node = (IRCast*)IR(IRZExtCast(cast_type, expr_value->SSA));
						}
						else {
							cast_ir_node = (IRCast*)IR(IRSExtCast(cast_type, expr_value->SSA));
						}
					}
					else {
						cast_ir_node = (IRCast*)IR(IRIntTrunc(cast_type, expr_value->SSA));
					}
				}
				else {
					if (castee_flags & FLAG_FLOATING_TYPE && cast_flags & FLAG_FLOATING_TYPE) {
						if (castee_size < cast_size) {
							cast_ir_node = (IRCast*)IR(IRFPExt(cast_type, expr_value->SSA));
						}
						else {
							cast_ir_node = (IRCast*)IR(IRFPTrunc(cast_type, expr_value->SSA));
						}
					}
					else {
						if (!(castee_flags & FLAG_FLOATING_TYPE)) {
							cast_ir_node = (IRCast*)IR(IRInt2FP(cast_type, expr_value->SSA, !(cast_flags & FLAG_UNSIGNED_TYPE)));
						}
						else if (!(cast_flags & FLAG_FLOATING_TYPE)) {
							cast_ir_node = (IRCast*)IR(IRFP2Int(cast_type, expr_value->SSA, !(castee_flags & FLAG_UNSIGNED_TYPE)));
						}
					}
				}
			}
			else {
				MSG_LOC(cast);
				FMT_WARN("Invalid cast from type '{}' to type '{}'", PrintType(castee_type), PrintType(cast_type));
				return nullptr;
			}
		}

		m_Metadata.RegExprType(new_ssa->ID, cast_type);

		new_ssa->Value = cast_ir_node;
		return IR(IRSSAValue(new_ssa->ID), cast_ir_node);
	}

	IRInstruction* Compiler::NullCodeGen()
	{
		auto null_ptr_ssa = CreateIRSSA();
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
			FMT_WARN("'type_info' needed a 'Type' instead got a {}", PrintType(type_arg_type));
			return nullptr;
		}

		auto result_type = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_typeinfo), 1);

		IRSSA* type_info_ssa = CreateIRSSA();
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
			FMT_WARN("trying to dereference a non pointer value, the type of said value is '{}'", PrintType(exprType));
			return nullptr;
		}

		m_Metadata.RegExprType(expr_value->SSA, exprType);

		return expr_value;
	}

	IRSSAValue* Compiler::GetExpressionByValue(const Expression* expr, IRSSAValue* generated_code)
	{
		if (!generated_code) {
			generated_code = (IRSSAValue*)ExpressionCodeGen(expr);
		}

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

			auto ir_address = generated_code;

			if (ir_address == nullptr)
				return nullptr;

			auto expr_type = m_Metadata.GetExprType(ir_address->SSA);

			IRSSA* value_ssa = CreateIRSSA();

			IRLoad load;
			load.AddressSSA = ir_address->SSA;
			load.Type = expr_type;

			value_ssa->Value = IR(load);

			m_Metadata.RegExprType(value_ssa->ID, m_Metadata.GetExprType(ir_address->SSA));

			return IR(IRSSAValue(value_ssa->ID));
		}
		break;
		case NodeType::ArrayAccess:
		{
			auto ir_address = generated_code;

			if (!ir_address) {
				return nullptr;
			}

			auto expr_type = m_Metadata.GetExprType(ir_address->SSA);

			if (!TypeSystem::IsPointer(expr_type) && !TypeSystem::IsArray(expr_type))
			{
				IRSSA* value_ssa = CreateIRSSA();

				IRLoad load;

				load.AddressSSA = ir_address->SSA;
				load.Type = expr_type;

				value_ssa->Value = IR(load);

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
				return generated_code;
			}

			SymbolType symbol_type = m_Metadata.GetSymbolType(identifier->Symbol.Symbol);

			if (symbol_type == SymbolType::Function) {
				return generated_code;
			}

			if (symbol_type == SymbolType::Type)
			{
				return TypeValueCodeGen(TypeSystem::GetBasic(identifier->Symbol.Symbol));
			}
			else if (symbol_type == SymbolType::Variable)
			{
				auto ir_address = generated_code;

				if (!ir_address)
				{
					return nullptr;
				}

				const auto metadata = m_Metadata.GetVariableMetadata(ir_address->SSA);

				//const auto metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(identifier->Symbol.Symbol));

				IRLoad load;
				load.AddressSSA = ir_address->SSA;
				load.Type = metadata->Tipe;

				IRSSA* ssa = CreateIRSSA();
				IRSSA* var_ssa = m_Metadata.GetSSA(load.AddressSSA);
				ssa->Value = IR(load);

				m_Metadata.RegExprType(ssa->ID, metadata->Tipe);

				return IR(IRSSAValue(ssa->ID));
			}
			else if (symbol_type == SymbolType::Constant) {
				return generated_code;
			}
			else {

				auto ir_address = generated_code;

				if (!ir_address)
					return nullptr;

				auto expr_type = m_Metadata.GetExprType(ir_address->SSA);

				IRLoad load;
				load.AddressSSA = ir_address->SSA;

				IRSSA* ssa = CreateIRSSA();

				load.Type = expr_type;

				ssa->Value = IR(load);

				m_Metadata.RegExprType(ssa->ID, expr_type);

				return IR(IRSSAValue(ssa->ID));
			}
		}
		break;
		case NodeType::DeReference:
		{
			auto ir_address = generated_code;

			if (!ir_address)
				return nullptr;

			auto expr_type = m_Metadata.GetExprType(ir_address->SSA);
			auto new_type = TypeSystem::ReduceIndirection((TSPtr*)expr_type);

			u16 new_type_ind_count = (u64)TypeSystem::IndirectionCount(new_type);

			IRLoad* ir_load = IR(IRLoad());
			ir_load->Type = new_type;
			ir_load->AddressSSA = ir_address->SSA;

			auto load_ssa = CreateIRSSA();

			load_ssa->Value = ir_load;

			m_Metadata.RegExprType(load_ssa->ID, new_type);

			return IR(IRSSAValue(load_ssa->ID));
		}
		break;
		default:
			return generated_code;
			break;
		}
	}

	IRSSAValue* Compiler::PassAsAny(const Expression* expr, IRSSAValue* pre_generated /*= nullptr*/)
	{
		IRSSAValue* expr_result = (IRSSAValue*)GetExpressionByValue(expr, pre_generated);

		if (expr_result == nullptr)
			return nullptr;

		auto expr_type = m_Metadata.GetExprType(expr_result->SSA);

		if (expr_type->BaseID == IR_any) {
			return expr_result;
		}

		IRSSA* any_ssa = CreateIRSSA();
		any_ssa->Value = IR(IRAny(expr_result->SSA, expr_type));

		m_Metadata.RegExprType(any_ssa->ID, expr_type);

		auto ir_load_ssa = CreateIRSSA();

		IRLoad* ir_load = IR(IRLoad());
		ir_load->AddressSSA = any_ssa->ID;
		ir_load->Type = TypeSystem::GetBasic(IR_any);

		ir_load_ssa->Value = ir_load;

		m_Metadata.RegExprType(ir_load_ssa->ID, expr_type);

		return IR(IRSSAValue(ir_load_ssa->ID));
	}

	IRSSAValue* Compiler::PassAsVariadicArray(const std::vector<Expression*>& arguments, const std::vector<IRSSAValue*>& pre_generated_arguments, const ArgumentMetadata* decl_arg)
	{
		if (decl_arg->Type->BaseID == IR_any) {

			std::vector<IRAny> anys;
			anys.reserve(arguments.size());

			for (size_t i = 0; i < arguments.size(); i++) {

				Expression* arg = arguments[i];

				IRSSAValue* code = nullptr;

				if (i < pre_generated_arguments.size()) {
					code = GetExpressionByValue(arg, pre_generated_arguments[i]);
				}
				else {
					code = (IRSSAValue*)GetExpressionByValue(arg);
				}

				if (!code) {
					return nullptr;
				}

				auto arg_type = m_Metadata.GetExprType(code->SSA);
				code = CreateCopy(arg_type, code);
				arg_type = m_Metadata.GetExprType(code->SSA);

				anys.push_back(IRAny(code->SSA, arg_type));
			}

			auto any_array_ir_ssa = CreateIRSSA();
			any_array_ir_ssa->Value = IR(IRAnyArray(anys));

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

		type_expr_value_ssa->Value = IR(IRTypeValue(type));

		m_Metadata.RegExprType(type_expr_value_ssa->ID, expr_type);

		return IR(IRSSAValue(type_expr_value_ssa->ID));
	}


	IRSSAValue* Compiler::CreateLoad(TypeStorage* type, u64 address)
	{
		auto load_ssa = CreateIRSSA();
		load_ssa->Value = IR(IRLoad(address, type));
		m_Metadata.RegExprType(load_ssa->ID, type);
		return IR(IRSSAValue(load_ssa->ID));
	}

	IRSSAValue* Compiler::CreateStore(TypeStorage* type, u64 address, IRInstruction* data)
	{
		auto store_ssa = CreateIRSSA();
		store_ssa->Value = IR(IRStore(address, data, type));
		return IR(IRSSAValue(store_ssa->ID));
	}

	IRSSAValue* Compiler::CreateConstantInteger(u64 integer_base_type, i64 value)
	{
		IRCONSTValue* Constant = IR(IRCONSTValue());;

		Constant->Type = integer_base_type;
		memcpy(&Constant->Data, &value, sizeof(i64));

		return CreateIRSSA(Constant, TypeSystem::GetBasic(Constant->Type));
	}

	IRSSAValue* Compiler::CreateConstant(u64 base_type, i64 value_integer, double value_float)
	{
		IRCONSTValue* Constant = IR(IRCONSTValue());

		Constant->Type = base_type;

		if (m_Metadata.GetTypeFlags(base_type) & FLAG_FLOATING_TYPE) {
			memcpy(&Constant->Data, &value_float, sizeof(double));
		}
		else {
			memcpy(&Constant->Data, &value_integer, sizeof(i64));
		}

		return CreateIRSSA(Constant, TypeSystem::GetBasic(Constant->Type));
	}

	IRSSAValue* Compiler::CreateCopy(TypeStorage* type, IRSSAValue* loaded_value)
	{
		auto alloca = CreateIRSSA(IR(IRAlloca(type)), type);
		CreateStore(type, alloca->SSA, loaded_value);
		return alloca;
	}

	IRSSAValue* Compiler::CreateMemberAccess(const std::string& strct, const std::string& member, u64 address)
	{
		u64 struct_id = m_Metadata.GetStructIDFromType(m_Metadata.GetType(strct));

		const StructMetadata* metadata = m_Metadata.GetStructMetadata(struct_id);

		IRMemberAccess* member_access = IR(IRMemberAccess());
		member_access->StructID = struct_id;
		member_access->MemberID = metadata->FindMember(member);
		member_access->ObjectSSA = address;

		return CreateIRSSA(member_access);
	}

	IRSSAValue* Compiler::CreatePointerCast(TypeStorage* to_type, u64 address)
	{
		return CreateIRSSA(IR(IRPointerCast(to_type, address)));
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

	IRSSAValue* Compiler::CreateIRSSA(IRInstruction* value)
	{
		auto ssa = CreateIRSSA();
		ssa->Value = value;
		return IR(IRSSAValue(ssa->ID));
	}

	IRSSAValue* Compiler::CreateIRSSA(IRInstruction* value, TypeStorage* semantic_type)
	{
		auto ssa = CreateIRSSA();
		ssa->Value = value;
		m_Metadata.RegExprType(ssa->ID, semantic_type);
		return IR(IRSSAValue(ssa->ID));
	}

	IRData* Compiler::CreateIRData()
	{
		IRData* Data = IR(IRData());
		Data->ID = m_DATAIDCounter;

		m_DATAIDCounter++;

		PushIRData(Data);

		return Data;
	}

	TypeStorage* Compiler::TypeExpressionGetType(TypeExpression* type_expr)
	{
		return TypeSystem::TypeExpressionGetType(type_expr);
	}

	TypeExpression* Compiler::TypeGetTypeExpression(TypeStorage* type)
	{
		GS_CORE_ASSERT(type);

		TypeStorageKind kind = type->Kind;

		switch (kind) {
		case TypeStorageKind::Base: {
			TypeExpressionTypeName* type_name = AST(TypeExpressionTypeName());
			type_name->Symbol.Symbol = m_Metadata.GetType(type->BaseID);
			return type_name;
		}
								  break;
		case TypeStorageKind::Pointer: {

			TSPtr* as_pointer = (TSPtr*)type;

			TypeExpressionPointer* pointer = AST(TypeExpressionPointer());
			pointer->Indirection = TypeSystem::IndirectionCount(as_pointer);
			pointer->Pointee = TypeGetTypeExpression(as_pointer->Pointee);

			return pointer;
		}
									 break;
		case TypeStorageKind::DynArray: {

			TSDynArray* as_array = (TSDynArray*)type;
			TypeExpressionArray* array = AST(TypeExpressionArray());
			array->ElementType = TypeGetTypeExpression(as_array->ElementType);

			return array;
		}
									  break;
		case TypeStorageKind::Function: {

			TSFunc* as_func = (TSFunc*)type;
			TypeExpressionFunc* func = AST(TypeExpressionFunc());

			for (auto arg : as_func->Arguments) {
				func->Arguments.push_back(TypeGetTypeExpression(arg));
			}

			func->ReturnType = TypeGetTypeExpression(as_func->ReturnType);

			return func;
		}
									  break;
		}

		GS_CORE_ASSERT(0);
		return nullptr;
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

	void Compiler::BinaryDispatch(const Expression* left, const Expression* right, TypeStorage** left_type, TypeStorage** right_type, IRSSAValue** A, IRSSAValue** B)
	{
		if (left->GetType() != NodeType::NumericLiteral) {
			*A = GetExpressionByValue(left);

			if (!*A) {
				return;
			}

			*left_type = m_Metadata.GetExprType((*A)->SSA);

			SetLikelyConstantType((*left_type)->BaseID);
		}

		if (right->GetType() != NodeType::NumericLiteral) {
			*B = GetExpressionByValue(right);

			if (!*B) {
				return;
			}

			*right_type = m_Metadata.GetExprType((*B)->SSA);

			SetLikelyConstantType((*right_type)->BaseID);
		}

		if (*left_type == nullptr) {
			*A = GetExpressionByValue(left);
			if (!*A) {
				return;
			}
		}
		if (*right_type == nullptr) {
			*B = GetExpressionByValue(right);
			if (!*B) {
				return;
			}
		}

		*left_type = m_Metadata.GetExprType((*A)->SSA);
		*right_type = m_Metadata.GetExprType((*B)->SSA);

		ResetLikelyConstantType();
	}
}