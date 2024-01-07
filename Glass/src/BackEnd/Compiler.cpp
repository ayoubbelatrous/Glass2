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

	Compiler::Compiler(std::vector<CompilerFile*> files)
	{
		for (auto file : files) {
			InsertFile(std::filesystem::absolute(file->GetPath()).string(), file);
		}
	}

	void Compiler::InitTypeSystem()
	{
		TypeSystem::Init(m_Metadata);

		m_Metadata.RegisterType(IR_void, "void", 0, 0);

		m_Metadata.RegisterType(IR_float, "float", 4, 4);
		m_Metadata.RegisterType(IR_int, "int", 4, 4);

		m_Metadata.RegisterType(IR_i8, "i8", 1, 1);
		m_Metadata.RegisterType(IR_i16, "i16", 2, 2);
		m_Metadata.RegisterType(IR_i32, "i32", 4, 4);
		m_Metadata.RegisterType(IR_i64, "i64", 8, 8);

		m_Metadata.RegisterType(IR_u8, "u8", 1, 1);
		m_Metadata.RegisterType(IR_u16, "u16", 2, 2);
		m_Metadata.RegisterType(IR_u32, "u32", 4, 4);
		m_Metadata.RegisterType(IR_u64, "u64", 8, 8);

		m_Metadata.RegisterType(IR_f32, "f32", 4, 4);
		m_Metadata.RegisterType(IR_f64, "f64", 8, 8);

		m_Metadata.RegisterType(IR_bool, "bool", 1, 1);

		m_Metadata.RegisterType(IR_type, "Type", 8, 8);


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

			struct_metadata.SizeComplete = true;

			m_Metadata.ComputeStructSizeAlignOffsets(&struct_metadata);

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
				if (stmt_Struct->Name.Symbol == "TypeInfo_Enum") {
					generate_base_struct(stmt_Struct, IR_typeinfo_enum, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Enum_Member") {
					generate_base_struct(stmt_Struct, IR_typeinfo_enum_member, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Member") {
					generate_base_struct(stmt_Struct, IR_typeinfo_member, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Struct") {
					generate_base_struct(stmt_Struct, IR_typeinfo_struct, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Pointer") {
					generate_base_struct(stmt_Struct, IR_typeinfo_pointer, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Function") {
					generate_base_struct(stmt_Struct, IR_typeinfo_function, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Dyn_Array") {
					generate_base_struct(stmt_Struct, IR_typeinfo_dyn_array, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "TypeInfo_Func_Param") {
					generate_base_struct(stmt_Struct, IR_typeinfo_func_param, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "GlobalDef_Function") {
					generate_base_struct(stmt_Struct, IR_globaldef_function, GetStructID());
				}
				if (stmt_Struct->Name.Symbol == "string") {
					generate_base_struct(stmt_Struct, IR_string, GetStructID());
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
		auto first_pass_instructions = FirstPass();

		IRTranslationUnit* tu = IR(IRTranslationUnit());

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

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
		}

		auto ir_data = PoPIRData();

		std::vector<IRInstruction*> instructions;

		for (auto entry : first_pass_instructions) {
			if (entry != nullptr)
				instructions.push_back(entry);
		}

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
		auto files_copy = m_Files;

		for (auto [file_id, file] : files_copy)
		{
			m_CurrentFile = file_id;

			ModuleFile* module_file = file->GetAST();

			for (Statement* stmt : module_file->GetStatements()) {
				if (stmt->GetType() == NodeType::Load) {
					LoadCodeGen((LoadNode*)stmt);
				}
			}
		}
	}

	void Compiler::LoadFileLoads(u64 file_id)
	{
		m_CurrentFile = file_id;
		auto file = m_Files.at(m_CurrentFile);

		ModuleFile* module_file = file->GetAST();

		for (Statement* stmt : module_file->GetStatements()) {
			if (stmt->GetType() == NodeType::Load) {
				LoadCodeGen((LoadNode*)stmt);
			}
		}
	}

	void Compiler::LoadCodeGen(LoadNode* loadNode)
	{
		std::string fileName = ((StringLiteral*)loadNode->FileName)->Symbol.Symbol;

		auto current_file_path = m_Files[m_CurrentFile]->GetPath();
		current_file_path.remove_filename();

		fs_path relative_path = current_file_path / fileName;
		fs_path absolute_path = std::filesystem::absolute(relative_path);

		if (FileLoaded(absolute_path.string())) {
			return;
		}

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

		u64 inserted_file_id = InsertFile(absolute_path.string(), file);

		LoadFileLoads(inserted_file_id);
	}

	void Compiler::LibraryPass()
	{
		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

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
		}
	}

	std::vector<IRInstruction*> Compiler::FirstPass()
	{
		std::vector<IRInstruction*> instructions;

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

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

		}

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

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
		}

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

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
		}

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();
				if (tl_type == NodeType::Foreign) {
					ForeignCodeGen((ForeignNode*)stmt);
				}
			}
		}

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();
				if (tl_type == NodeType::Operator) {
					OperatorCodeGen((OperatorNode*)stmt);
				}
			}
		}

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

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
		}

		for (auto [file_id, file] : m_Files)
		{
			m_CurrentFile = file_id;

			ModuleFile* module_file = file->GetAST();

			for (const Statement* stmt : module_file->GetStatements())
			{
				NodeType tl_type = stmt->GetType();

				switch (tl_type)
				{
				case NodeType::Variable:
					instructions.push_back(GlobalVariableCodeGen((VariableNode*)stmt));
					break;
				}
			}
		}

		SizingLoop();

		return instructions;
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
				structure.SizeComplete = true;

				m_Metadata.ComputeStructSizeAlignOffsets(&structure);

				m_Metadata.m_TypeSizes[structure.TypeID] = structure.Size;
				m_Metadata.m_TypeAlignments[structure.TypeID] = structure.Alignment;
			}
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

			std::vector<TypeExpression*> results;
			TypeExpressionGetPolyMorphicTypeNames(parameter->Type, results);
			poly_morphic |= results.size() != 0;

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

			if (return_type == nullptr) {
				MSG_LOC(fnNode);
				MSG_LOC(fnNode->ReturnType);
				FMT_WARN("function '{}' has undefined return type", fnNode->Symbol.Symbol);
				return;
			}

			for (const Statement* a : fnNode->GetArgList()->GetArguments())
			{
				const ArgumentNode* parameter = (ArgumentNode*)a;

				ArgumentMetadata argument;
				argument.Name = parameter->Symbol.Symbol;
				argument.Type = TypeExpressionGetType(parameter->Type);
				argument.Variadic = parameter->Variadic;

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

		metadata.FileID = m_CurrentFile;

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

		m_Metadata.RegisterType(type_id, strct->Name.Symbol, 0, 0);

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
		case NodeType::AutoCast:
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

		case NodeType::Else:
			return ElseCodeGen((ElseNode*)statement);
			break;

		case NodeType::Scope:
			return ScopeCodeGen((ScopeNode*)statement);
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

		const Library* library = m_Metadata.GetLibrary(frn->library_name->Symbol.Symbol);

		if (!library) {
			PushMessage(CompilerMessage{ PrintTokenLocation(frn->library_name->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("unknown library in foreign import"), MessageType::Warning });
			return nullptr;
		}

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
			metadata.Foreign_Library = library->Name.Symbol;

			m_Metadata.RegisterFunction(ID, metadata);

			return nullptr;
		}
		else if (tipe == NodeType::Variable) {
			return GlobalVariableCodeGen((VariableNode*)frn->statement, true);
		}

		PushMessage(CompilerMessage{ PrintTokenLocation(frn->statement->GetLocation()), MessageType::Error });
		PushMessage(CompilerMessage{ fmt::format("unsupported foreign entity"), MessageType::Warning });

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
		ResetRegisterIDCounter();

		IRFunction* IRF = IR(IRFunction());

		IRF->ID = m_Metadata.GetFunctionMetadata(fnNode->Symbol.Symbol);
		auto metadata = m_Metadata.GetFunctionMetadata(IRF->ID);


		if (!metadata)
			return nullptr;

		m_Metadata.m_CurrentFunction = IRF->ID;

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

		PoPIRRegisters();

		//////////////////////////////////////////////////////////////////////////
		//		@PUSH_SCOPE
		m_Metadata.PushContext(ContextScopeType::FUNC);
		//////////////////////////////////////////////////////////////////////////

		u64 argument_index = 0;
		for (auto a : fnNode->GetArgList()->GetArguments())
		{
			ArgumentNode* arg = (ArgumentNode*)a;
			TypeStorage* argument_type = TypeExpressionGetType(arg->Type);
			TypeStorage* argument_variable_type = nullptr;

			if (!argument_type)
			{
				PushMessage(CompilerMessage{ PrintTokenLocation(arg->Type->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("argument '{}' is of undefined type '{}', at '{}' function definition",
														arg->Symbol.Symbol, PrintType(argument_type), fnNode->DefinitionTk.Symbol),
											MessageType::Warning });
				return nullptr;
			}

			if (!arg->Variadic) {
				argument_variable_type = argument_type;
			}
			else {
				argument_variable_type = TypeSystem::GetDynArray(argument_type);
			}

			VariableMetadata var_metadata = {
				arg->Symbol,
				argument_variable_type,
			};

			IRRegister* arg_register = CreateIRRegister();
			arg_register->Value = IR(IRArgumentAllocation(argument_index, argument_variable_type));

			RegisterVariable(arg_register, arg->Symbol.Symbol);
			m_Metadata.RegisterVariableMetadata(arg_register->ID, var_metadata);

			ArgumentMetadata arg_metadata;
			arg_metadata.Name = arg->Symbol.Symbol;
			arg_metadata.Type = argument_type;
			arg_metadata.Variadic = arg->Variadic;
			arg_metadata.AllocationLocation = IR(IRRegisterValue(arg_register->ID));
			args_metadata.push_back(arg_metadata);

			argument_index++;
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

		auto ir_registers = PoPIRRegisters();

		for (auto ir_register : ir_registers)
		{
			IRF->Instructions.push_back(ir_register);
		}

		for (const Statement* stmt : fnNode->GetStatements())
		{
			IRInstruction* code = StatementCodeGen(stmt);

			auto ir_registers = PoPIRRegisters();

			for (auto ir_register : ir_registers)
			{
				IRF->Instructions.push_back(ir_register);
			}

			if (!code)
				continue;

			if (code->GetType() != IRNodeType::RegisterValue) {
				//We Discard function scope level r values

				if (code != nullptr) {
					IRF->Instructions.push_back(code);
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//		@POP_SCOPE
		m_Metadata.PopContext();
		//////////////////////////////////////////////////////////////////////////

		return (IRInstruction*)IRF;
	}

	IRInstruction* Compiler::VariableCodeGen(const VariableNode* variableNode)
	{

		if (ContextGlobal()) {
			//return GlobalVariableCodeGen(variableNode);
			return nullptr;
		}
		else {
			if (variableNode->Constant) {
				return ConstantCodeGen(variableNode);
			}
		}

		SymbolType symbol_type = m_Metadata.GetSymbolType(variableNode->Symbol.Symbol);

		if (symbol_type != SymbolType::None) {
			MSG_LOC(variableNode);
			FMT_WARN("variable '{}' name already taken, pick another one!", variableNode->Symbol.Symbol);
			return nullptr;
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

		IRRegisterValue* value = nullptr;

		TypeStorage* VariableType = nullptr;

		if (variableNode->Type != nullptr) {

			VariableType = TypeExpressionGetType(variableNode->Type);

			if (VariableType) {
				SetLikelyConstantType(VariableType);
				m_AutoCastTargetType = VariableType;
			}
		}

		if (variableNode->Assignment != nullptr)
		{
			value = GetExpressionByValue(variableNode->Assignment);
			if (!value) {
				return nullptr;
			}
		}

		ResetLikelyConstantType();
		m_AutoCastTargetType = nullptr;

		TypeStorage* assignment_type = nullptr;

		if (variableNode->Assignment) {
			assignment_type = m_Metadata.GetExprType(value->RegisterID);
		}

		if (variableNode->Type == nullptr) {
			VariableType = assignment_type;
		}

		if (!VariableType) {
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->Type->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable is of unknown type"), MessageType::Warning });
			return nullptr;
		}

		if (!TypeSystem::GetTypeSize(VariableType)) {
			MSG_LOC(variableNode);
			FMT_WARN("variable cannot be assigned a type with size 0: {}", PrintType(VariableType));
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

		IRRegister* variable_address_register = CreateIRRegister();
		variable_address_register->Value = alloca;

		RegisterVariable(variable_address_register, variableNode->Symbol.Symbol);

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			VariableType,
			false,
			false,
			variable_address_register };

		m_Metadata.RegisterVariableMetadata(variable_address_register->ID, var_metadata);
		m_Metadata.RegExprType(variable_address_register->ID, VariableType);

		//@Debugging
		alloca->VarMetadata = m_Metadata.GetVariableMetadata(variable_address_register->ID);

		if (value)
		{
			IRStore* assignment_store = IR(IRStore());

			assignment_store->AddressRegister = variable_address_register->ID;
			assignment_store->Data = value;
			assignment_store->Type = VariableType;

			return assignment_store;
		}
		else {

			auto type_size = TypeSystem::GetTypeSize(VariableType);

			if (type_size <= 8) {

				IRRegisterValue* zero_initializer = nullptr;

				if (TypeSystem::IsPointer(VariableType)) {
					zero_initializer = Create_Null(VariableType);
				}
				else {
					zero_initializer = CreateConstant(VariableType->BaseID, 0, 0.0);
				}

				CreateStore(VariableType, variable_address_register->ID, zero_initializer);
			}
			else {
				Create_Intrinsic_Memset(VariableType, variable_address_register->ID, 0);
			}
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
		TypeStorage* constant_type = TypeExpressionGetType(variableNode->Type);

		SetLikelyConstantType(constant_type);
		m_AutoCastTargetType = constant_type;

		IRInstruction* assignment = GetExpressionByValue(variableNode->Assignment);
		UN_WRAP(assignment);

		{
			GS_CORE_ASSERT(assignment->GetType() == IRNodeType::RegisterValue);
			IRRegister* assignment_register = GetRegister(((IRRegisterValue*)assignment)->RegisterID);

			if (assignment_register->Value->GetType() != IRNodeType::ConstValue) {
				MSG_LOC(variableNode->Assignment);
				FMT_WARN("expected constant '{}' to be assigned to a constant value", variableNode->Symbol.Symbol);
				FMT_WARN("NOTE: we currently only support numeric literals as constant assignments", variableNode->Symbol.Symbol);
			}

			assignment = assignment_register->Value;
		}

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
		return CreateIRRegister(constant->Value, constant->Type);
	}

	IRInstruction* Compiler::GlobalVariableCodeGen(const VariableNode* variableNode, bool foreign /*= false*/)
	{
		if (variableNode->Constant) {
			return ConstantCodeGen(variableNode);
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

			global->Initializer->Constant_Type = Type;
		}

		return global;
	}

	IRInstruction* Compiler::ReturnCodeGen(const ReturnNode* returnNode)
	{
		IRReturn ret;

		IRRegisterValue* expr = nullptr;
		TypeStorage* expr_type = nullptr;

		if (returnNode->Expr) {
			SetLikelyConstantType(GetExpectedReturnType());

			expr = (IRRegisterValue*)GetExpressionByValue(returnNode->Expr);
			UN_WRAP(expr);

			expr_type = m_Metadata.GetExprType(expr->RegisterID);

			GS_CORE_ASSERT(m_ExpectedReturnType);

			if (!TypeSystem::StrictPromotion(expr_type, m_ExpectedReturnType)) {
				MSG_LOC(returnNode->Expr);
				FMT_WARN("return type doesn't match function type: {} => {}", PrintType(expr_type), PrintType(m_ExpectedReturnType));
			}

			ResetLikelyConstantType();
		}

		if (expr_type && m_ExpectedReturnType == TypeSystem::GetVoid()) {
			MSG_LOC(returnNode->Expr);
			FMT_WARN("functions with void return type cannot return a value");
		}

		ret.Value = expr;
		ret.Type = expr_type;

		if (!ret.Type) {
			ret.Type = TypeSystem::GetVoid();
		}

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
		UN_WRAP(condition);

		auto condition_ir_register = m_Metadata.GetRegister(condition->RegisterID);
		UN_WRAP(condition_ir_register);
		auto condition_ir_node_type = condition_ir_register->Value->GetType();

		switch (condition_ir_node_type) {
		case IRNodeType::GreaterThan:
		case IRNodeType::LesserThan:
		case IRNodeType::Equal:
		case IRNodeType::NotEqual: {
			condition_ir_register->IsCondition = true;
		}
								 break;
		}

		if (!condition)
			return nullptr;

		auto condition_type = m_Metadata.GetExprType(condition->RegisterID);

		IRIf IF;
		IF.ConditionRegister = condition->RegisterID;
		IF.ConditionType = condition_type;

		IRLexBlock lexical_block;

		lexical_block.Begin = ifNode->Scope->OpenCurly;
		lexical_block.End = ifNode->Scope->CloseCurly;

		PushScope();
		m_Metadata.PushContext(ContextScopeType::FUNC);
		for (const Statement* stmt : ifNode->Scope->GetStatements())
		{
			auto first_inst = StatementCodeGen(stmt);

			auto register_stack = PoPIRRegisters();

			for (auto inst : register_stack)
			{
				if (inst->GetType() != IRNodeType::RegisterValue) {
					if (inst != nullptr) {
						lexical_block.Instructions.push_back(inst);
					}
				}
			}

			if (!first_inst) {
				continue;
			}

			if (first_inst->GetType() != IRNodeType::RegisterValue) {
				if (first_inst != nullptr) {
					lexical_block.Instructions.push_back(first_inst);
				}
			}
		}

		auto register_stack = PoPIRRegisters();

		for (auto inst : register_stack)
		{
			if (inst->GetType() != IRNodeType::RegisterValue) {
				if (inst != nullptr) {
					lexical_block.Instructions.push_back(inst);
				}
			}
		}

		m_Metadata.PopContext();
		PopScope();

		IF.Instructions.push_back(IR(lexical_block));

		if (ifNode->Else) {
			IF.ElseBlock = StatementCodeGen(ifNode->Else);
		}

		return IR(IF);
	}

	IRInstruction* Compiler::ElseCodeGen(ElseNode* elseNode)
	{
		m_Metadata.PushContext(ContextScopeType::FUNC);

		IRLexBlock lexical_block;

		std::vector<IRInstruction*> Instructions;

		PushScope();

		auto code = StatementCodeGen(elseNode->statement);

		auto Registers = PoPIRRegisters();

		for (auto ir_register : Registers)
		{
			lexical_block.Instructions.push_back(ir_register);
		}

		lexical_block.Instructions.push_back(code);

		PopScope();
		m_Metadata.PopContext();

		return IR(lexical_block);

		return nullptr;
	}

	IRInstruction* Compiler::WhileCodeGen(const WhileNode* whileNode)
	{
		PushScope();
		IRRegisterValue* condition = GetExpressionByValue(whileNode->Condition);
		std::vector<IRRegister*> condition_registers = PoPIRRegisters();
		PopScope();

		if (!condition)
			return nullptr;

		auto condition_ir_register = m_Metadata.GetRegister(condition->RegisterID);
		auto condition_ir_node_type = condition_ir_register->Value->GetType();

		switch (condition_ir_node_type) {
		case IRNodeType::GreaterThan:
		case IRNodeType::LesserThan:
		case IRNodeType::Equal:
		case IRNodeType::NotEqual: {
			condition_ir_register->IsCondition = true;
		}
								 break;
		}

		if (!condition)
			return nullptr;

		auto condition_type = m_Metadata.GetExprType(condition->RegisterID);

		IRWhile WHILE;
		WHILE.ConditionRegisterID = condition->RegisterID;
		WHILE.ConditionType = condition_type;

		WHILE.ConditionBlock = condition_registers;

		IRLexBlock lexical_block;

		lexical_block.Begin = whileNode->Scope->OpenCurly;
		lexical_block.End = whileNode->Scope->CloseCurly;

		PushScope();
		m_Metadata.PushContext(ContextScopeType::FUNC);

		for (const Statement* stmt : whileNode->Scope->GetStatements())
		{
			auto inst = StatementCodeGen(stmt);

			auto register_stack = PoPIRRegisters();

			for (auto ir_register : register_stack)
			{
				if (ir_register->GetType() != IRNodeType::RegisterValue) {
					if (ir_register != nullptr) {
						lexical_block.Instructions.push_back(ir_register);
					}
				}
			}

			if (!inst) {
				continue;
			}

			if (inst->GetType() != IRNodeType::RegisterValue) {
				if (inst != nullptr) {
					lexical_block.Instructions.push_back(inst);
				}
			}
		}

		m_Metadata.PopContext();
		PopScope();

		WHILE.Instructions.push_back(IR(lexical_block));

		return IR(WHILE);
	}

	IRInstruction* Compiler::RangeCodeGen(const RangeNode* rangeNode)
	{
		IRRegisterValue* begin = nullptr;
		IRRegisterValue* end = nullptr;

		TypeStorage* begin_type = nullptr;
		TypeStorage* end_type = nullptr;

		IRIterator* iterator = IR(IRIterator());

		std::vector<IRRegister*> end_code;

		BinaryDispatch(rangeNode->Begin, rangeNode->End, &begin_type, &end_type, &begin, &end, nullptr, &end_code);

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

		//:
		iterator->IteratorIndex = CreateIRRegister(IR(IRAlloca(type)));
		CreateStore(type, iterator->IteratorIndex->RegisterID, begin);

		iterator->IteratorIt = CreateIRRegister(IR(IRAlloca(type)));
		CreateStore(type, iterator->IteratorIt->RegisterID, begin);

		//Cond:
		{
			PushScope();

			for (auto end_inst : end_code) {
				iterator->ConditionBlock.push_back(end_inst);
			}

			IRRegisterValue* cmp_inst = CreateIRRegister(IR(IRLesser(CreateLoad(type, iterator->IteratorIndex->RegisterID), end, type)));

			IRRegister* cmp_register = m_Metadata.GetRegister(cmp_inst->RegisterID);
			cmp_register->IsCondition = true;

			iterator->ConditionRegisterID = cmp_inst->RegisterID;

			auto register_stack = PoPIRRegisters();

			for (auto inst : register_stack) {
				iterator->ConditionBlock.push_back(inst);
			}

			PopScope();
		}

		//Incrementor:
		{
			PushScope();

			auto index_load = CreateLoad(type, iterator->IteratorIndex->RegisterID);
			auto index_addition = CreateIRRegister(IR(IRADD(index_load, CreateConstantInteger(type->BaseID, 1), type)));
			CreateStore(type, iterator->IteratorIndex->RegisterID, index_addition);
			CreateStore(type, iterator->IteratorIt->RegisterID, index_addition);

			auto register_stack = PoPIRRegisters();
			for (auto inst : register_stack) {
				iterator->IncrementorBlock.push_back(inst);
			}

			PopScope();
		}

		iterator->ItTy = type;
		iterator->IndexTy = type;

		return iterator;
	}


	IRIterator* Compiler::DynArrayIteratorCodeGen(const Expression* expression, IRRegisterValue* generated)
	{
		TSDynArray* dynamic_array_type = (TSDynArray*)m_Metadata.GetExprType(generated->RegisterID);
		TypeStorage* dynmaic_array_element_type = dynamic_array_type->ElementType;

		IRIterator* iterator = IR(IRIterator());

		auto index_type = TypeSystem::GetBasic(IR_u64);
		auto element_type = dynmaic_array_element_type;

		//:
		iterator->IteratorIndex = CreateIRRegister(IR(IRAlloca(index_type)));
		CreateStore(
			index_type,
			iterator->IteratorIndex->RegisterID,
			CreateConstantInteger(index_type->BaseID, 0));

		iterator->IteratorIt = CreateIRRegister(IR(IRAlloca(dynmaic_array_element_type)));

		//Cond:
		{
			PushScope();

			auto generated_register = GetRegister(generated->RegisterID);

			auto end = CreateLoad(index_type, CreateMemberAccess("Array", "count", generated->RegisterID)->RegisterID);

			auto index_load = CreateLoad(index_type, iterator->IteratorIndex->RegisterID);
			IRRegisterValue* cmp_inst = CreateIRRegister(IR(IRLesser(index_load, end, index_type)));

			IRRegister* cmp_register = m_Metadata.GetRegister(cmp_inst->RegisterID);
			cmp_register->IsCondition = true;

			iterator->ConditionRegisterID = cmp_inst->RegisterID;

			auto register_stack = PoPIRRegisters();
			for (auto inst : register_stack) {
				iterator->ConditionBlock.push_back(inst);
			}
			PopScope();
		}

		//Start:
		{
			PushScope();

			auto index_load = CreateLoad(index_type, iterator->IteratorIndex->RegisterID);

			auto data_member_type = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
			IRRegisterValue* data_member = CreateLoad(data_member_type, CreateMemberAccess("Array", "data", generated->RegisterID)->RegisterID);
			auto casted_data = CreatePointerCast(TypeSystem::IncreaseIndirection(element_type), data_member->RegisterID);
			auto data_pointer = CreateIRRegister(IR(IRArrayAccess(casted_data->RegisterID, index_load->RegisterID, element_type, index_type)));
			CreateStore(element_type, iterator->IteratorIt->RegisterID, CreateLoad(element_type, data_pointer->RegisterID));

			auto register_stack = PoPIRRegisters();
			for (auto inst : register_stack) {
				iterator->StartBlock.push_back(inst);
			}

			PopScope();
		}

		//Incrementor:
		{
			PushScope();

			auto index_load = CreateLoad(index_type, iterator->IteratorIndex->RegisterID);
			auto index_addition = CreateIRRegister(IR(IRADD(index_load, CreateConstantInteger(index_type->BaseID, 1), index_type)));
			CreateStore(index_type, iterator->IteratorIndex->RegisterID, index_addition);

			auto register_stack = PoPIRRegisters();
			for (auto inst : register_stack) {
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

		IRRegisterValue* expr_result = (IRRegisterValue*)ExpressionCodeGen(expr);

		if (!expr_result) {
			return nullptr;
		}

		if (m_Metadata.GetExprType(expr_result->RegisterID)->Kind == TypeStorageKind::DynArray) {
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
		while_inst->ConditionRegisterID = iterator->ConditionRegisterID;

		PushScope();
		m_Metadata.PushContext(ContextScopeType::FUNC);

		std::string iterator_name = "it";

		if (forNode->Named_Iterator)
		{
			if (forNode->Named_Iterator->GetType() != NodeType::Identifier)
			{
				MSG_LOC(forNode->Named_Iterator);
				FMT_WARN("for loop expected named iterator to be a identifier");
				return nullptr;
			}

			auto as_ident = (Identifier*)forNode->Named_Iterator;

			SymbolType symbol_type = m_Metadata.GetSymbolType(as_ident->Symbol.Symbol);

			if (symbol_type != SymbolType::None)
			{
				MSG_LOC(as_ident);
				FMT_WARN("variable '{}' name already taken, pick another one!", as_ident->Symbol.Symbol);
				return nullptr;
			}

			const VariableMetadata* variable_metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(as_ident->Symbol.Symbol));

			if (variable_metadata != nullptr)
			{
				MSG_LOC(as_ident);
				FMT_WARN(fmt::format("variable '{}' is already defined", as_ident->Symbol.Symbol));
				return nullptr;
			}

			iterator_name = as_ident->Symbol.Symbol;
		}

		m_Metadata.RegisterVariable(m_Metadata.GetRegister(iterator->IteratorIndex->RegisterID), "it_index");
		m_Metadata.RegisterVariable(m_Metadata.GetRegister(iterator->IteratorIt->RegisterID), iterator_name);

		VariableMetadata it_index_metadata;
		it_index_metadata.Tipe = iterator->IndexTy;
		it_index_metadata.Name = forNode->Condition->GetLocation();
		it_index_metadata.Name.Symbol = "it_index";
		m_Metadata.RegisterVariableMetadata(iterator->IteratorIndex->RegisterID, it_index_metadata);
		((IRAlloca*)m_Metadata.GetRegister(iterator->IteratorIndex->RegisterID)->Value)->VarMetadata = m_Metadata.GetVariableMetadata(iterator->IteratorIndex->RegisterID);

		VariableMetadata it_metadata;
		it_metadata.Tipe = iterator->ItTy;
		it_metadata.Name = forNode->Condition->GetLocation();
		it_metadata.Name.Symbol = iterator_name;
		m_Metadata.RegisterVariableMetadata(iterator->IteratorIt->RegisterID, it_metadata);
		((IRAlloca*)m_Metadata.GetRegister(iterator->IteratorIt->RegisterID)->Value)->VarMetadata = m_Metadata.GetVariableMetadata(iterator->IteratorIt->RegisterID);

		for (auto inst : iterator->StartBlock) {
			while_inst->Instructions.push_back(inst);
		}

		for (const Statement* stmt : forNode->Scope->GetStatements())
		{
			auto inst = StatementCodeGen(stmt);
			auto register_stack = PoPIRRegisters();
			for (auto inst : register_stack)
			{
				if (inst->GetType() != IRNodeType::RegisterValue) {
					while_inst->Instructions.push_back(inst);
				}
			}
			if (!inst) {
				continue;
			}

			if (inst->GetType() != IRNodeType::RegisterValue) {
				while_inst->Instructions.push_back(inst);
			}
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
			return CastNodeCodeGen((CastNode*)expression);
			break;
		case NodeType::AutoCast:
			return AutoCastCodeGen((AutoCastNode*)expression);
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

			u64 ID = GetVariableRegister(identifier->Symbol.Symbol);
			IRRegister* ir_register = GetRegister(ID);

			IRRegisterValue register_val;

			register_val.RegisterID = GetVariableRegister(identifier->Symbol.Symbol);

			m_Metadata.RegExprType(register_val.RegisterID, metadata->Tipe);

			return (IRInstruction*)IR(register_val);
		}
		else if (symbol_type == SymbolType::GlobVariable) {

			u64 glob_id = m_Metadata.GetGlobalVariable(identifier->Symbol.Symbol);

			auto ir_register = CreateIRRegister();

			IRGlobalAddress* glob_address = IR(IRGlobalAddress(glob_id));

			ir_register->Value = glob_address;

			const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(glob_id);

			m_Metadata.RegExprType(ir_register->ID, metadata->Tipe);

			return IR(IRRegisterValue(ir_register->ID));
		}
		else if (symbol_type == SymbolType::Type || symbol_type == SymbolType::Enum) {
			return TypeValueCodeGen(TypeSystem::GetBasic(identifier->Symbol.Symbol));
		}
		else if (symbol_type == SymbolType::Constant) {
			return ConstantValueCodeGen(m_Metadata.GetConstant(identifier->Symbol.Symbol));
		}

		return nullptr;
	}

	IRInstruction* Compiler::NumericLiteralCodeGen(const NumericLiteral* numericLiteral)
	{
		IRRegister* ir_register = CreateIRRegister();

		ir_register->Value = IR(IRCONSTValue());

		IRCONSTValue* Constant = (IRCONSTValue*)ir_register->Value;

		auto likely_constant_type = GetLikelyConstantType();

		if (likely_constant_type == nullptr) {
			if (numericLiteral->type == NumericLiteral::Type::Float)
			{
				likely_constant_type = GetLikelyConstantFloatType();
				memcpy(&Constant->Data, &numericLiteral->Val.Float, sizeof(double));
			}
			else if (numericLiteral->type == NumericLiteral::Type::Int)
			{
				likely_constant_type = GetLikelyConstantIntegerType();
				memcpy(&Constant->Data, &numericLiteral->Val.Int, sizeof(double));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			auto type_flags = TypeSystem::GetTypeFlags(likely_constant_type);

			if (type_flags & TypeFlag::FLAG_FLOATING_TYPE) {

				if (numericLiteral->type == NumericLiteral::Type::Float)
				{
					memcpy(&Constant->Data, &numericLiteral->Val.Float, sizeof(double));
				}
				else if (numericLiteral->type == NumericLiteral::Type::Int)
				{
					double as_double = (double)numericLiteral->Val.Int;
					memcpy(&Constant->Data, &as_double, sizeof(double));
				}
			}
			else {
				if (numericLiteral->type == NumericLiteral::Type::Float)
				{
					likely_constant_type = GetLikelyConstantFloatType();
					memcpy(&Constant->Data, &numericLiteral->Val.Float, sizeof(double));
				}
				else if (numericLiteral->type == NumericLiteral::Type::Int)
				{
					memcpy(&Constant->Data, &numericLiteral->Val.Int, sizeof(double));
				}
			}
		}

		Constant->Constant_Type = likely_constant_type;

		m_Metadata.RegExprType(ir_register->ID, Constant->Constant_Type);

		return IR(IRRegisterValue(ir_register->ID));
	}

	IRInstruction* Compiler::StringLiteralCodeGen(const StringLiteral* stringLiteral)
	{
		IRData* data = CreateIRData();

		for (char c : stringLiteral->Symbol.Symbol)
		{
			data->Data.push_back(c);
		}
		auto data_ir_register = CreateIRRegister();
		data_ir_register->Value = IR(IRDataValue(0, data->ID));

		auto c_string_ty = TypeSystem::GetPtr(TypeSystem::GetU8(), 1);

		if (m_AutoCastTargetType == c_string_ty)
		{
			m_Metadata.RegExprType(data_ir_register->ID, c_string_ty);
			return IR(IRRegisterValue(data_ir_register->ID));
		}
		else {

			auto count_ir_register = CreateConstantInteger(IR_u64, data->Data.size());

			auto string_initializer = Create_String_Initializer(data_ir_register->ID, count_ir_register->RegisterID);

			m_Metadata.RegExprType(string_initializer->RegisterID, TypeSystem::GetString());

			return string_initializer;
		}
	}

	IRInstruction* Compiler::BinaryExpressionCodeGen(const BinaryExpression* binaryExpr)
	{
		auto OPerator = binaryExpr->OPerator;

		if (OPerator == Operator::AddAssign || OPerator == Operator::SubAssign
			|| OPerator == Operator::MulAssign || OPerator == Operator::DivAssign
			|| OPerator == Operator::BitAndAssign || OPerator == Operator::BitOrAssign) {
			return OpAssignmentCodeGen(binaryExpr);
		}

		if (OPerator == Operator::Assign)
		{
			return AssignmentCodeGen(binaryExpr);
		}

		IRRegisterValue* A = nullptr;
		IRRegisterValue* B = nullptr;

		TypeStorage* left_type = nullptr;
		TypeStorage* right_type = nullptr;

		if (binaryExpr->Left->GetType() != NodeType::NumericLiteral) {
			A = GetExpressionByValue(binaryExpr->Left);

			if (!A) {
				return nullptr;
			}

			left_type = m_Metadata.GetExprType(A->RegisterID);

			m_AutoCastTargetType = left_type;
			SetLikelyConstantType(left_type);
		}

		if (binaryExpr->Right->GetType() != NodeType::NumericLiteral) {
			B = GetExpressionByValue(binaryExpr->Right);
			if (!B) {
				return nullptr;
			}
			right_type = m_Metadata.GetExprType(B->RegisterID);

			m_AutoCastTargetType = right_type;
			SetLikelyConstantType(right_type);
		}

		if (left_type == nullptr) {
			A = GetExpressionByValue(binaryExpr->Left);
			left_type = m_Metadata.GetExprType(A->RegisterID);
		}
		if (right_type == nullptr) {
			B = GetExpressionByValue(binaryExpr->Right);
			right_type = m_Metadata.GetExprType(B->RegisterID);
		}

		ResetLikelyConstantType();
		m_AutoCastTargetType = nullptr;

		if (!A || !B)
		{
			//@Todo: log error here
			return nullptr;
		}

		IRRegister* ir_register = CreateIRRegister();

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
				case Operator::Modulo:
					return "Modulo Divisions";
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
					const auto op_func_metadata = m_Metadata.GetFunctionMetadata(op_func_id);

					Expression* lhs = binaryExpr->Left;
					Expression* rhs = binaryExpr->Right;

					IRFunctionCall* call = IR(IRFunctionCall());

					call->FuncID = op_func_id;
					call->Arguments.push_back(A);
					call->Arguments.push_back(B);

					GS_CORE_ASSERT(left_type);
					GS_CORE_ASSERT(right_type);

					call->ArgumentTypes.push_back(left_type);
					call->ArgumentTypes.push_back(right_type);

					IRRegisterValue* op_result = CreateIRRegister(call, op_func_metadata->ReturnType);

					m_Metadata.RegExprType(op_result->RegisterID, op_func_metadata->ReturnType);

					return op_result;
				}
			}
		}

		TypeStorage* result_type = left_type;
		TypeStorage* op_type = left_type;

		if (!TypeSystem::StrictPromotion(right_type, left_type)) {
			PushMessage(CompilerMessage{ PrintTokenLocation((binaryExpr->OperatorToken)), MessageType::Error });
			FMT_WARN("incompatible types in {1}: '{0}' {1} '{2}'", PrintType(left_type), binaryExpr->OperatorToken.Symbol, PrintType(right_type));
			return nullptr;
		}

		switch (binaryExpr->OPerator)
		{
		case Operator::Add:
		{
			IRADD IROp;

			IROp.RegisterA = A;
			IROp.RegisterB = B;

			ir_register->Value = IR(IROp);
		}
		break;
		case Operator::Subtract:
		{
			IRSUB IROp;

			IROp.RegisterA = A;
			IROp.RegisterB = B;

			ir_register->Value = IR(IROp);
		}
		break;
		case Operator::Multiply:
		{
			IRMUL IROp;

			IROp.RegisterA = A;
			IROp.RegisterB = B;

			ir_register->Value = IR(IROp);
		}
		break;
		case Operator::Modulo:
		{
			IRSREM IROp;

			IROp.RegisterA = A;
			IROp.RegisterB = B;

			ir_register->Value = IR(IROp);
		}
		break;
		case Operator::Divide:
		{
			IRDIV IROp;

			IROp.RegisterA = A;
			IROp.RegisterB = B;

			ir_register->Value = IR(IROp);
		}
		break;
		case Operator::Equal:
		{
			ir_register->Value = IR(IREQ(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		case Operator::NotEqual:
		{
			ir_register->Value = IR(IRNOTEQ(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		case Operator::GreaterThan:
		{
			ir_register->Value = IR(IRGreater(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		case Operator::LesserThan:
		{
			ir_register->Value = IR(IRLesser(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		case Operator::GreaterThanEq:
		{
			ir_register->Value = IR(IRGreater(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		case Operator::LesserThanEq:
		{
			ir_register->Value = IR(IRLesser(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		case Operator::BitAnd:
		{
			ir_register->Value = IR(IRBitAnd(A, B));
		}
		break;
		case Operator::BitOr:
		{
			ir_register->Value = IR(IRBitOr(A, B));
		}
		break;
		case Operator::And:
		{
			ir_register->Value = IR(IRAnd(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		case Operator::Or:
		{
			ir_register->Value = IR(IROr(A, B));
			result_type = TypeSystem::GetBool();
		}
		break;
		default:
			GS_CORE_ASSERT(0, "Unsuppoerted operator");
			return nullptr;
			break;
		}

		((IRBinOp*)ir_register->Value)->Type = op_type;

		IRRegisterValue* register_value = IR(IRRegisterValue());

		register_value->RegisterID = ir_register->ID;

		m_Metadata.RegExprType(register_value->RegisterID, result_type);

		return register_value;
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
				u64 glob_id = m_Metadata.GetGlobalVariable(identifier_left->Symbol.Symbol);

				left_type = m_Metadata.GetVariableMetadata(glob_id)->Tipe;
				m_AutoCastTargetType = left_type;
				IRRegisterValue* right_val = (IRRegisterValue*)GetExpressionByValue(binaryExpr->Right);
				m_AutoCastTargetType = nullptr;
				if (!right_val) {
					return nullptr;
				}
				auto ir_register = CreateIRRegister();
				ir_register->Value = IR(IRGlobalAddress(glob_id));

				right_type = m_Metadata.GetExprType(right_val->RegisterID);

				IRStore* store = IR(IRStore());
				{
					store->Data = right_val;
					store->AddressRegister = ir_register->ID;
					store->Type = left_type;
				}

				result = store;
			}
			else if (symbol_type == SymbolType::Variable) {

				u64 var_register_id = GetVariableRegister(identifier_left->Symbol.Symbol);
				IRRegister* var_rgister = m_Metadata.GetRegister(var_register_id);

				const auto metadata = m_Metadata.GetVariableMetadata(var_register_id);

				SetLikelyConstantType(metadata->Tipe);

				m_AutoCastTargetType = metadata->Tipe;
				IRRegisterValue* right_val = (IRRegisterValue*)GetExpressionByValue(binaryExpr->Right);
				m_AutoCastTargetType = nullptr;

				if (!right_val) {
					return nullptr;
				}

				right_type = m_Metadata.GetExprType(right_val->RegisterID);
				left_type = metadata->Tipe;

				IRStore* store = IR(IRStore());
				{
					store->Data = right_val;
					store->AddressRegister = var_register_id;
					store->Type = left_type;
				}

				ResetLikelyConstantType();

				result = store;
			}
			else {
				MSG_LOC(identifier_left);
				FMT_WARN("left side: '{}' is un-assignable", identifier_left->Symbol.Symbol);
				return nullptr;
			}
		}
		if (left->GetType() == NodeType::MemberAccess)
		{
			PushScope();
			IRRegisterValue* member_access = (IRRegisterValue*)ExpressionCodeGen(left);
			auto left_registers = PoPIRRegisters();
			PopScope();

			if (!member_access)
				return nullptr;

			left_type = m_Metadata.GetExprType(member_access->RegisterID);

			SetLikelyConstantType(left_type);
			m_AutoCastTargetType = left_type;

			IRRegisterValue* right_register = (IRRegisterValue*)GetExpressionByValue(right);
			m_AutoCastTargetType = nullptr;

			if (!right_register) {
				return nullptr;
			}

			if (right_register->GetType() != IRNodeType::RegisterValue) {
				MSG_LOC(right);
				FMT_WARN("trying to assign to void");
				return nullptr;
			}

			right_type = m_Metadata.GetExprType(right_register->RegisterID);

			if (!member_access || !right_register) {
				return nullptr;
			}

			for (auto lft : left_registers) {
				PushIRRegister(lft);
			}

			IRStore* store = IR(IRStore());
			{
				store->Data = right_register;
				store->AddressRegister = member_access->RegisterID;
				store->Type = left_type;
			}

			ResetLikelyConstantType();
			m_AutoCastTargetType = nullptr;

			result = store;
		}
		if (left->GetType() == NodeType::ArrayAccess)
		{
			auto left_register = (IRRegisterValue*)ExpressionCodeGen(left);

			if (!left_register)
				return nullptr;

			left_type = m_Metadata.GetExprType(left_register->RegisterID);

			SetLikelyConstantType(left_type);
			m_AutoCastTargetType = left_type;

			auto right_register = (IRRegisterValue*)GetExpressionByValue(right);

			if (!right_register)
				return nullptr;

			right_type = m_Metadata.GetExprType(right_register->RegisterID);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_register;
				store->AddressRegister = left_register->RegisterID;
				store->Type = right_type;
			}

			result = store;

			ResetLikelyConstantType();
			m_AutoCastTargetType = nullptr;
		}
		if (left->GetType() == NodeType::DeReference)
		{
			auto left_register = (IRRegisterValue*)ExpressionCodeGen(left);

			if (!left_register) {
				return nullptr;
			}


			left_type = TypeSystem::ReduceIndirection((TSPtr*)m_Metadata.GetExprType(left_register->RegisterID));

			SetLikelyConstantType(left_type);
			m_AutoCastTargetType = left_type;

			auto right_register = (IRRegisterValue*)GetExpressionByValue(right);

			if (!right_register) {
				return nullptr;
			}

			right_type = m_Metadata.GetExprType(right_register->RegisterID);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_register;
				store->AddressRegister = left_register->RegisterID;
				store->Type = left_type;
			}
			result = store;
		}

		GS_CORE_ASSERT(left_type);
		GS_CORE_ASSERT(right_type);
		GS_CORE_ASSERT(result);

		if (!TypeSystem::StrictPromotion(right_type, left_type)) {
			MSG_LOC(binaryExpr);
			FMT_WARN("incompatible types in assignment: '{}' {} '{}'", PrintType(left_type), binaryExpr->OperatorToken.Symbol, PrintType(right_type));
			return nullptr;
		}

		return CreateIRRegister(result);
	}

	IRInstruction* Compiler::OpAssignmentCodeGen(const BinaryExpression* binaryExpr)
	{
		Operator OPerator;

		switch (binaryExpr->OPerator)
		{

		case Operator::AddAssign:
			OPerator = Operator::Add;
			break;
		case Operator::SubAssign:
			OPerator = Operator::Subtract;
			break;
		case Operator::MulAssign:
			OPerator = Operator::Multiply;
			break;
		case Operator::DivAssign:
			OPerator = Operator::Divide;
			break;

		case Operator::BitAndAssign:
			OPerator = Operator::BitAnd;
			break;

		case Operator::BitOrAssign:
			OPerator = Operator::BitOr;
			break;
		default:
			break;
		}

		BinaryExpression operation_binary_expression;
		operation_binary_expression.Left = binaryExpr->Left;
		operation_binary_expression.Right = binaryExpr->Right;
		operation_binary_expression.OPerator = OPerator;
		operation_binary_expression.OperatorToken = binaryExpr->OperatorToken;
		auto operation = IR(operation_binary_expression);

		BinaryExpression assignment_binary_expression;
		assignment_binary_expression.Left = binaryExpr->Left;
		assignment_binary_expression.Right = operation;
		assignment_binary_expression.OPerator = Operator::Assign;
		assignment_binary_expression.OperatorToken = binaryExpr->OperatorToken;
		auto assignment = IR(assignment_binary_expression);

		return AssignmentCodeGen(assignment);
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

		std::vector<IRRegisterValue*> argumentValueRefs;
		std::vector<TypeStorage*> argumentTypes;

		for (size_t i = 0; i < call->Arguments.size(); i++)
		{
			auto argument_expr = call->Arguments[i];

			if (!metadata->IsOverloaded()) {
				if (i < metadata->Arguments.size()) {
					SetLikelyConstantType(metadata->Arguments[i].Type);
					m_AutoCastTargetType = metadata->Arguments[i].Type;
				}
			}

			IRRegisterValue* argument_as_value_ref = (IRRegisterValue*)ExpressionCodeGen(argument_expr);

			if (!metadata->IsOverloaded()) {
				ResetLikelyConstantType();
				m_AutoCastTargetType = nullptr;
			}

			if (!argument_as_value_ref)
				return nullptr;

			argumentTypes.push_back(m_Metadata.GetExprType(argument_as_value_ref->RegisterID));
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
			TypeStorage* arg_type = nullptr;

			IRRegisterValue* argument_code = nullptr;

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

					std::vector<IRRegisterValue*> pre_generated_variadic_arguments;

					for (size_t j = i; j < argumentValueRefs.size(); j++) {
						pre_generated_variadic_arguments.push_back(argumentValueRefs[j]);
					}

					argument_code = PassAsVariadicArray(call->Arguments, pre_generated_variadic_arguments, decl_arg);
				}

				if (argument_code == nullptr)
					return nullptr;

				TypeStorage* type = m_Metadata.GetExprType(argument_code->RegisterID);

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
				arg_type = type;
			}
			else
			{
				arg = GetExpressionByValue(call->Arguments[i], argumentValueRefs[i]);
				arg_type = m_Metadata.GetExprType(((IRRegisterValue*)arg)->RegisterID);
			}

			ir_call.Arguments.push_back(arg);
			ir_call.ArgumentTypes.push_back(arg_type);

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
			auto ir_register = CreateIRRegister();
			ir_register->Value = IR(ir_call);

			IRRegisterValue* ir_register_val = IR(IRRegisterValue(ir_register->ID));

			m_Metadata.RegExprType(ir_register_val->RegisterID, metadata->ReturnType);

			return ir_register_val;
		}
		else {
			auto void_register = CreateIRRegister();
			void_register->Value = IR(ir_call);
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

			if (call_parameter->PolyMorphic)
			{
				auto argument_value = GetExpressionByValue(call_argument);

				if (!argument_value)
					return nullptr;

				auto argument_type = m_Metadata.GetExprType(argument_value->RegisterID);

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

				bool has_dollar_typename = false;

				std::vector<TypeExpression*> dollar_type_names;
				TypeExpressionGetPolyMorphicTypeNames(call_parameter->Type, dollar_type_names);

				has_dollar_typename = dollar_type_names.size() != 0;

				if (!has_dollar_typename) {
					auto assumed_type = TypeExpressionGetType(call_parameter->Type);

					if (assumed_type) {
						SetLikelyConstantType(assumed_type);
						m_AutoCastTargetType = assumed_type;
					}
					else {
						m_AutoCastTargetType = nullptr;
					}
				}

				auto argument_value = GetExpressionByValue(call_argument);

				if (!has_dollar_typename) {
					ResetLikelyConstantType();
					m_AutoCastTargetType = nullptr;
				}

				if (!argument_value)
					return nullptr;

				auto argument_type = m_Metadata.GetExprType(argument_value->RegisterID);

				call_values.push_back(argument_value);
				call_types.push_back(argument_type);

				if (has_dollar_typename) { // TODO: pattern match for more than one types ($T,$S) Struct($K,$V)

					TypeExpression* decomposed = call_parameter->Type;
					TypeStorage* matched_type = argument_type;

					while (true)
					{
						if (decomposed->GetType() == NodeType::TE_Pointer) {
							TypeExpressionPointer* as_pointer = (TypeExpressionPointer*)decomposed;

							if (TypeSystem::IndirectionCount(matched_type) < as_pointer->Indirection) {
								MSG_LOC(call_argument);
								FMT_WARN("failed to match polymorphic argument: different levels of indirection ({}) <- {}", as_pointer->Indirection, TypeSystem::IndirectionCount(matched_type));
								MSG_LOC(as_pointer->Pointee);
								return nullptr;
							}

							matched_type = TypeSystem::ReduceIndirection((TSPtr*)matched_type, as_pointer->Indirection);
							decomposed = as_pointer->Pointee;
						}
						else if (decomposed->GetType() == NodeType::TE_Dollar) {
							break;
						}
						else if (decomposed->GetType() == NodeType::TE_TypeName) {
							break;
						}
						else if (decomposed->GetType() == NodeType::Identifier) {
							break;
						}
						else if (decomposed->GetType() == NodeType::TE_Func) {
							GS_CORE_ASSERT(nullptr);
							return nullptr;
						}
						else if (decomposed->GetType() == NodeType::TE_Array) {
							TypeExpressionArray* as_array = (TypeExpressionArray*)decomposed;

							if (!TypeSystem::IsArray(matched_type)) {
								MSG_LOC(call_argument);
								FMT_WARN("failed to match polymorphic argument: needed a dynamic array of this {} instead", PrintType(matched_type));
								MSG_LOC(as_array->ElementType);
								return nullptr;
							}

							decomposed = as_array->ElementType;
							matched_type = TypeSystem::GetArrayElementTy(matched_type);
						}
						else {
							GS_CORE_ASSERT(nullptr);
							return nullptr;
						}
					}

					TypeExpressionTypeName* polymorphic_selector_name = (TypeExpressionTypeName*)dollar_type_names[0];
					replacements[polymorphic_selector_name->Symbol.Symbol] = TypeGetTypeExpression(matched_type);
				}
			}
		}

		//LookUp

		auto cached = metadata->FindPolymorphicOverload(PolymorphicOverload{ replacements });

		if (cached != nullptr) {

			//Calling

			auto ir_func = cached;

			auto call_return_type = m_Metadata.GetFunctionMetadata(ir_func->ID)->ReturnType;

			if (call_return_type != TypeSystem::GetVoid()) {
				return CreateIRRegister(IR(IRFunctionCall(call_values, call_types, ir_func->ID)), call_return_type);
			}

			return IR(IRFunctionCall(call_values, call_types, ir_func->ID));
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
			name_mangling.append(TypeSystem::PrintTypeNoSpecialCharacters(type));
		}

		ast_copy_as_function->Symbol.Symbol = ast_copy_as_function->Symbol.Symbol + name_mangling;

		HandleTopLevelFunction(ast_copy_as_function);

		u64 register_counter = m_RegisterIDCounter;
		u64 calling_function_ctx_id = m_Metadata.CurrentContext()->ID;
		auto calling_function_return_type = GetExpectedReturnType();

		auto current_function_id = m_Metadata.m_CurrentFunction;

		m_Metadata.PopContext();

		u64 current_file = m_CurrentFile;

		m_CurrentFile = metadata->FileID;

		PushScope();
		IRFunction* ir_func = (IRFunction*)FunctionCodeGen(ast_copy_as_function);
		PopScope();

		m_CurrentFile = current_file;

		SetExpectedReturnType(calling_function_return_type);

		m_RegisterIDCounter = register_counter;
		m_Metadata.m_CurrentFunction = current_function_id;

		m_Metadata.PushContext(calling_function_ctx_id);

		metadata->PolyMorphicInstantiations.emplace(PolymorphicOverload{ replacements }, ir_func);


		//Calling

		auto call_return_type = m_Metadata.GetFunctionMetadata(ir_func->ID)->ReturnType;

		if (call_return_type != TypeSystem::GetVoid()) {
			return CreateIRRegister(IR(IRFunctionCall(call_values, call_types, ir_func->ID)), call_return_type);
		}

		return IR(IRFunctionCall(call_values, call_types, ir_func->ID));
	}

	IRInstruction* Compiler::MemberAccessCodeGen(const MemberAccess* memberAccess)
	{
		if (!memberAccess->Object) {
			MSG_LOC(memberAccess->Member);
			FMT_WARN("inferred member accesses are not implemented!");
			return nullptr;
		}

		if (memberAccess->Object->GetType() == NodeType::Identifier) {

			SymbolType symbol_type = m_Metadata.GetSymbolType(((Identifier*)memberAccess->Object)->Symbol.Symbol);

			if (symbol_type == SymbolType::Enum) {
				return (IRRegisterValue*)EnumMemberAccessCodeGen(memberAccess);
			}
		}

		u64 struct_id = 0;
		u64 member_id = 0;
		u64 object_register_id = 0;
		bool reference_access = false;

		TypeStorage* result_type = nullptr;

		IRRegisterValue* obj_register_value = (IRRegisterValue*)ExpressionCodeGen(memberAccess->Object);

		if (!obj_register_value)
			return nullptr;

		const StructMetadata* struct_metadata = nullptr;

		{
			object_register_id = obj_register_value->RegisterID;

			TypeStorage* obj_expr_type = m_Metadata.GetExprType(obj_register_value->RegisterID);

			if (TypeSystem::IndirectionCount(obj_expr_type) > 1 && !TypeSystem::IsArray(obj_expr_type)) {
				MSG_LOC(memberAccess->Object);
				FMT_WARN("The type '{}' is not a struct and does not support members", PrintType(obj_expr_type));
			}

			bool l_value = false;

			if (memberAccess->Object->GetType() == NodeType::Identifier || memberAccess->Object->GetType() == NodeType::MemberAccess) {
				l_value = true;
			}

			if (l_value) {
				if (TypeSystem::IndirectionCount(obj_expr_type) == 1) {
					reference_access = true;
				}
			}

			struct_id = m_Metadata.GetStructIDFromType(obj_expr_type->BaseID);

			if (TypeSystem::IsArray(obj_expr_type)) {
				struct_id = m_Metadata.GetStructIDFromType(IR_array);
			}

			if (TypeSystem::IsPointer(obj_expr_type)) {
				if (TypeSystem::IsArray(((TSPtr*)obj_expr_type)->Pointee)) {
					struct_id = m_Metadata.GetStructIDFromType(IR_array);
				}
			}

			if (struct_id == NULL_ID) {
				MSG_LOC(memberAccess->Object);
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
			ir_mem_access.ObjectRegister = object_register_id;
			ir_mem_access.MemberID = member_id;
			ir_mem_access.ReferenceAccess = reference_access;

			auto address_register = CreateIRRegister();

			address_register->Value = IR(ir_mem_access);

			m_Metadata.RegExprType(address_register->ID, result_type);

			return IR(IRRegisterValue(address_register->ID));
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

		SetLikelyConstantType(TypeSystem::GetU64());
		IRRegisterValue* result = (IRRegisterValue*)NumericLiteralCodeGen(AST(node));
		ResetLikelyConstantType();

		m_Metadata.RegExprType(result->RegisterID, TypeSystem::GetBasic(metadata->Name.Symbol));

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
				IRRegisterValue* expr_value = (IRRegisterValue*)ExpressionCodeGen(size_of->Expr);
				TypeStorage* expr_type = m_Metadata.GetExprType(expr_value->RegisterID);
				size = m_Metadata.GetTypeSize(expr_type);
			}
		}
		else {

			switch (size_of->Expr->GetType())
			{
			case NodeType::TE_TypeName:
			case NodeType::TE_Pointer:
			case NodeType::TE_Array:
			case NodeType::TE_Func: {

				auto as_type = TypeExpressionGetType((TypeExpression*)size_of->Expr);

				if (as_type) {
					size = m_Metadata.GetTypeSize(as_type);
				}
				else {
					MSG_LOC(size_of->Expr);
					FMT_WARN("uknown type '{}' inside sizeof");
				}
			}
								  break;
			default:
				IRRegisterValue* expr_value = (IRRegisterValue*)ExpressionCodeGen(size_of->Expr);
				TypeStorage* expr_type = m_Metadata.GetExprType(expr_value->RegisterID);
				size = m_Metadata.GetTypeSize(expr_type);
				break;
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
		auto what_type = m_Metadata.GetExprType(what_code->RegisterID);
		return CreateIRRegister(IR(IRSUB(CreateConstant(what_type->BaseID, 0, 0.0), what_code, what_type)), what_type);
	}

	IRInstruction* Compiler::FunctionRefCodegen(const Identifier* func)
	{
		u64 func_id = m_Metadata.GetFunctionMetadata(func->Symbol.Symbol);
		GS_CORE_ASSERT(func_id != NULL_ID, "Function must exist at this point");

		const FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(func_id);

		auto ir_register = CreateIRRegister();
		ir_register->Value = IR(IRFuncRef(func_id));

		m_Metadata.RegExprType(ir_register->ID, metadata->Signature);

		return IR(IRRegisterValue(ir_register->ID));
	}

	IRInstruction* Compiler::FuncRefCallCodeGen(const FunctionCall* call)
	{
		//We only support function calls as variables for now (no expressions)

		Identifier* function_as_identifier = AST(Identifier());
		function_as_identifier->Symbol = call->Function;

		auto callee_register = GetExpressionByValue(function_as_identifier);
		if (!callee_register)
			return nullptr;
		auto callee_type = m_Metadata.GetExprType(callee_register->RegisterID);

		std::vector<u64> argument_Registers;

		u64 index = 0;
		for (auto arg : call->Arguments) {
			SetLikelyConstantType(((TSFunc*)callee_type)->Arguments[index]);
			auto argument_code = GetExpressionByValue(arg);
			ResetLikelyConstantType();

			if (!argument_code) {
				return nullptr;
			}

			argument_Registers.push_back(argument_code->RegisterID);
			index++;
		}

		auto ir_call = IR(IRCallFuncRef(callee_register->RegisterID, argument_Registers, callee_type));

		if (((TSFunc*)callee_type)->ReturnType != TypeSystem::GetBasic(IR_void)) {
			auto return_register = CreateIRRegister();
			return_register->Value = ir_call;
			m_Metadata.RegExprType(return_register->ID, ((TSFunc*)callee_type)->ReturnType);
			return IR(IRRegisterValue(return_register->ID));
		}
		else {
			return ir_call;
		}
	}

	IRInstruction* Compiler::ScopeCodeGen(const ScopeNode* scope)
	{
		m_Metadata.PushContext(ContextScopeType::FUNC);

		PushScope();

		IRLexBlock lexical_block;

		lexical_block.Begin = scope->OpenCurly;
		lexical_block.End = scope->CloseCurly;

		std::vector<IRInstruction*> Instructions;

		for (auto stmt : scope->GetStatements()) {

			auto code = StatementCodeGen(stmt);

			auto Registers = PoPIRRegisters();

			for (auto ir_register : Registers)
			{
				lexical_block.Instructions.push_back(ir_register);
			}

			if (!code)
				continue;

			if (code->GetType() != IRNodeType::RegisterValue) {
				if (code != nullptr) {
					lexical_block.Instructions.push_back(code);
				}
			}
		}

		PopScope();
		m_Metadata.PopContext();

		return IR(lexical_block);
	}

	IRInstruction* Compiler::ArrayAccessCodeGen(const ArrayAccess* arrayAccess)
	{
		auto object = (IRRegisterValue*)ExpressionCodeGen(arrayAccess->Object);
		auto index = (IRRegisterValue*)GetExpressionByValue(arrayAccess->Index);

		if (!object || !index) {
			return nullptr;
		}

		auto obj_expr_type = m_Metadata.GetExprType(object->RegisterID);
		auto index_expr_type = m_Metadata.GetExprType(index->RegisterID);

		if (!TypeSystem::IsPointer(obj_expr_type) && !TypeSystem::IsArray(obj_expr_type) && obj_expr_type != TypeSystem::GetString())
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(arrayAccess->Object->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ "type of expression must be a pointer type in order to be accessed by the [] operator", MessageType::Warning });
			return nullptr;
		}

		if (TypeSystem::IsArray(obj_expr_type)) {

			const StructMetadata* array_struct = m_Metadata.GetStructFromType(IR_array);

			IRMemberAccess* data_member_access = IR(IRMemberAccess());
			data_member_access->MemberID = array_struct->FindMember("data");
			data_member_access->ObjectRegister = object->RegisterID;
			data_member_access->StructID = m_Metadata.GetStructIDFromType(IR_array);
			data_member_access->ReferenceAccess = false;

			auto data_member_access_register = CreateIRRegister();
			data_member_access_register->Value = data_member_access;

			auto data_member_access_load_register =
				CreateLoad(TypeSystem::GetPtr(TypeSystem::GetBasic(IR_void), 1), data_member_access_register->ID);

			auto data_member_access_pointer_cast_register = CreateIRRegister();
			auto element_ty = TypeSystem::GetPtr(TypeSystem::GetArrayElementTy(obj_expr_type), 1);
			data_member_access_pointer_cast_register->Value = IR(IRPointerCast(element_ty, data_member_access_load_register->RegisterID));
			object = IR(IRRegisterValue(data_member_access_pointer_cast_register->ID));

			obj_expr_type = element_ty;
		}
		else if (obj_expr_type == TypeSystem::GetString()) {

			const StructMetadata* string_struct = m_Metadata.GetStructFromType(IR_string);

			IRMemberAccess* data_member_access = IR(IRMemberAccess());
			data_member_access->MemberID = string_struct->FindMember("data");
			data_member_access->ObjectRegister = object->RegisterID;
			data_member_access->StructID = m_Metadata.GetStructIDFromType(IR_string);
			data_member_access->ReferenceAccess = false;

			auto data_member_access_register = CreateIRRegister();
			data_member_access_register->Value = data_member_access;

			auto u8_ptr_ty = TypeSystem::GetPtr(TypeSystem::GetU8(), 1);

			auto data_member_access_load_register =
				CreateLoad(u8_ptr_ty, data_member_access_register->ID);

			auto element_ty = u8_ptr_ty;

			object = IR(IRRegisterValue(data_member_access_load_register->RegisterID));

			obj_expr_type = element_ty;
		}
		else {
			object = GetExpressionByValue(arrayAccess->Object, object);
		}

		IRRegister* array_access_register = CreateIRRegister();

		IRArrayAccess* ir_array_Access = IR(IRArrayAccess());

		obj_expr_type = TypeSystem::ReduceIndirection((TSPtr*)obj_expr_type);
		m_Metadata.RegExprType(array_access_register->ID, obj_expr_type);

		ir_array_Access->ArrayAddress = object->RegisterID;
		ir_array_Access->ElementIndexRegister = index->RegisterID;
		ir_array_Access->Type = obj_expr_type;
		ir_array_Access->Index_Type = index_expr_type;

		array_access_register->Value = ir_array_Access;


		return IR(IRRegisterValue(array_access_register->ID));
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
				type_of_type = TypeSystem::GetBasic(IR_type);
			}

			if (symbol_type == SymbolType::Variable)
			{
				const auto metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariableRegister(type_ident->Symbol.Symbol));
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
			IRRegisterValue* code = (IRRegisterValue*)ExpressionCodeGen(typeof->What);
			type_of_type = m_Metadata.GetExprType(code->RegisterID);
		}

		IRRegister* ir_register = CreateIRRegister();
		ir_register->Value = IR(IRTypeValue(type_of_type));

		m_Metadata.RegExprType(ir_register->ID, TypeSystem::GetType());

		return IR(IRRegisterValue(ir_register->ID));
	}

	IRInstruction* Compiler::AutoCastCodeGen(const AutoCastNode* autoCastNode)
	{
		auto auto_cast_type = m_AutoCastTargetType;

		auto cast_expr_code = GetExpressionByValue(autoCastNode->Expr);

		if (!cast_expr_code) {
			return nullptr;
		}

		if (!auto_cast_type) {
			MSG_LOC(autoCastNode);
			FMT_WARN("cannot infer auto cast type!");
			return nullptr;
		}

		return CastCodeGen(auto_cast_type, cast_expr_code, autoCastNode);
	}

	IRInstruction* Compiler::CastNodeCodeGen(const CastNode* castNode)
	{
		auto cast_expr_code = GetExpressionByValue(castNode->Expr);
		auto cast_type = TypeExpressionGetType(castNode->Type);

		if (!cast_type) {
			MSG_LOC(castNode);
			FMT_WARN("unknown cast type");
			return nullptr;
		}

		if (!cast_expr_code || !cast_type)
			return nullptr;

		return CastCodeGen(cast_type, cast_expr_code, castNode);
	}

	IRInstruction* Compiler::CastCodeGen(TypeStorage* cast_type, IRRegisterValue* code, const Expression* ast_node)
	{
		GS_CORE_ASSERT(cast_type);
		GS_CORE_ASSERT(code);
		GS_CORE_ASSERT(ast_node);

		auto expr_value = code;

		if (!expr_value)
			return nullptr;

		auto new_register = CreateIRRegister();

		auto castee_type = m_Metadata.GetExprType(expr_value->RegisterID);

		auto castee_size = m_Metadata.GetTypeSize(castee_type->BaseID);
		auto cast_size = m_Metadata.GetTypeSize(cast_type->BaseID);

		auto castee_flags = m_Metadata.GetTypeFlags(castee_type->BaseID);
		auto cast_flags = m_Metadata.GetTypeFlags(cast_type->BaseID);

		IRCast* cast_ir_node = nullptr;


		if (TypeSystem::IsPointer(castee_type) && TypeSystem::IsPointer(cast_type)) {
			cast_ir_node = (IRCast*)IR(IRPointerCast(cast_type, expr_value->RegisterID));
		}
		else if (TypeSystem::IsPointer(castee_type) && !TypeSystem::IsPointer(cast_type)) {
			cast_ir_node = (IRCast*)IR(IRPtr2IntCast(cast_type, expr_value->RegisterID));
		}
		else if (!TypeSystem::IsPointer(castee_type) && TypeSystem::IsPointer(cast_type)) {
			cast_ir_node = (IRCast*)IR(IRInt2PtrCast(cast_type, expr_value->RegisterID));
		}
		else {

			if (castee_flags & FLAG_NUMERIC_TYPE && cast_flags & FLAG_NUMERIC_TYPE) {

				if (!(castee_flags & FLAG_FLOATING_TYPE) && !(cast_flags & FLAG_FLOATING_TYPE)) {

					if (castee_size < cast_size) {
						if (
							castee_flags & FLAG_UNSIGNED_TYPE &&
							cast_flags & FLAG_UNSIGNED_TYPE
							) {
							cast_ir_node = (IRCast*)IR(IRZExtCast(cast_type, expr_value->RegisterID));
						}
						else {
							cast_ir_node = (IRCast*)IR(IRSExtCast(cast_type, expr_value->RegisterID));
						}
					}
					else {
						cast_ir_node = (IRCast*)IR(IRIntTrunc(cast_type, expr_value->RegisterID));
					}
				}
				else {
					if (castee_flags & FLAG_FLOATING_TYPE && cast_flags & FLAG_FLOATING_TYPE) {
						if (castee_size < cast_size) {
							cast_ir_node = (IRCast*)IR(IRFPExt(cast_type, expr_value->RegisterID));
						}
						else {
							cast_ir_node = (IRCast*)IR(IRFPTrunc(cast_type, expr_value->RegisterID));
						}
					}
					else {
						if (!(castee_flags & FLAG_FLOATING_TYPE)) {
							cast_ir_node = (IRCast*)IR(IRInt2FP(cast_type, expr_value->RegisterID, !(cast_flags & FLAG_UNSIGNED_TYPE)));
						}
						else if (!(cast_flags & FLAG_FLOATING_TYPE)) {
							cast_ir_node = (IRCast*)IR(IRFP2Int(cast_type, expr_value->RegisterID, !(castee_flags & FLAG_UNSIGNED_TYPE)));
						}
					}
				}
			}
			else {
				MSG_LOC(ast_node);
				FMT_WARN("Invalid cast from type '{}' to type '{}'", PrintType(castee_type), PrintType(cast_type));
				return nullptr;
			}
		}

		m_Metadata.RegExprType(new_register->ID, cast_type);

		cast_ir_node->From = castee_type;

		new_register->Value = cast_ir_node;
		return IR(IRRegisterValue(new_register->ID), cast_ir_node);
	}

	IRInstruction* Compiler::NullCodeGen()
	{
		auto type = TypeSystem::GetVoidPtr();

		if (m_AutoCastTargetType != nullptr) {
			type = m_AutoCastTargetType;
		}

		auto null_ptr_register_val = Create_Null(type);

		m_Metadata.RegExprType(null_ptr_register_val->RegisterID, type);

		return null_ptr_register_val;
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

		auto argument = type_info_call->Arguments[0];

		IRRegisterValue* type_arg_value = GetExpressionByValue(argument);

		if (!type_arg_value) {
			return nullptr;
		}

		TypeStorage* type_arg_type = m_Metadata.GetExprType(type_arg_value->RegisterID);

		if (type_arg_type != TypeSystem::GetBasic(IR_type)) {
			MSG_LOC(type_info_call);
			FMT_WARN("'type_info' type argument type mismatch");
			FMT_WARN("'type_info' needed a 'Type' instead got a {}", PrintType(type_arg_type));
			return nullptr;
		}

		auto result_type = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_typeinfo), 1);

		IRRegister* type_info_register = CreateIRRegister();
		type_info_register->Value = IR(IRTypeInfo(type_arg_value->RegisterID));

		m_Metadata.RegExprType(type_info_register->ID, result_type);
		return IR(IRRegisterValue(type_info_register->ID));
	}

	IRInstruction* Compiler::RefCodeGen(const RefNode* refNode)
	{
		IRRegisterValue* referee = (IRRegisterValue*)ExpressionCodeGen(refNode->What);

		if (!referee) {
			return nullptr;
		}

		auto referee_type = m_Metadata.GetExprType(referee->RegisterID);

		TSPtr* result_type = (TSPtr*)TypeSystem::IncreaseIndirection(referee_type);

		IRRegister* rvalue_ptr_storage = CreateIRRegister();
		rvalue_ptr_storage->Value = referee;

		m_Metadata.RegExprType(rvalue_ptr_storage->ID, result_type);

		return IR(IRRegisterValue(rvalue_ptr_storage->ID));
	}

	IRInstruction* Compiler::DeRefCodeGen(const DeRefNode* deRefNode)
	{
		IRRegisterValue* expr_value = (IRRegisterValue*)GetExpressionByValue(deRefNode->What);

		if (!expr_value)
			return nullptr;

		TypeStorage* exprType = m_Metadata.GetExprType(expr_value->RegisterID);

		u16 indirection_count = TypeSystem::IndirectionCount(exprType);

		if (indirection_count == 0) {
			MSG_LOC(deRefNode);
			FMT_WARN("trying to dereference a non pointer value, the type of said value is '{}'", PrintType(exprType));
			return nullptr;
		}

		m_Metadata.RegExprType(expr_value->RegisterID, exprType);

		return expr_value;
	}

	IRRegisterValue* Compiler::GetExpressionByValue(const Expression* expr, IRRegisterValue* generated_code)
	{
		if (!generated_code) {
			generated_code = (IRRegisterValue*)ExpressionCodeGen(expr);
		}

		switch (expr->GetType())
		{
		case NodeType::MemberAccess:
		{
			MemberAccess* member_access = (MemberAccess*)expr;

			if (!member_access->Object) {
				MSG_LOC(member_access->Member);
				FMT_WARN("inferred member accesses are not implemented!");
				return nullptr;
			}

			if (member_access->Object->GetType() == NodeType::Identifier) {

				SymbolType symbol_type = m_Metadata.GetSymbolType(((Identifier*)member_access->Object)->Symbol.Symbol);

				if (symbol_type == SymbolType::Enum) {
					return (IRRegisterValue*)EnumMemberAccessCodeGen(member_access);
				}
			}

			auto ir_address = generated_code;

			if (ir_address == nullptr)
				return nullptr;

			auto expr_type = m_Metadata.GetExprType(ir_address->RegisterID);

			IRRegister* value_register = CreateIRRegister();

			IRLoad load;
			load.AddressRegister = ir_address->RegisterID;
			load.Type = expr_type;

			value_register->Value = IR(load);

			m_Metadata.RegExprType(value_register->ID, m_Metadata.GetExprType(ir_address->RegisterID));

			return IR(IRRegisterValue(value_register->ID));
		}
		break;
		case NodeType::ArrayAccess:
		{
			auto ir_address = generated_code;

			if (!ir_address) {
				return nullptr;
			}

			auto expr_type = m_Metadata.GetExprType(ir_address->RegisterID);
			//expr_type = TypeSystem::ReduceIndirection((TSPtr*)expr_type);

			IRRegister* value_register = CreateIRRegister();

			IRLoad load;

			load.AddressRegister = ir_address->RegisterID;
			load.Type = expr_type;

			value_register->Value = IR(load);

			m_Metadata.RegExprType(value_register->ID, expr_type);

			return IR(IRRegisterValue(value_register->ID));
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

			if (symbol_type == SymbolType::Type || symbol_type == SymbolType::Enum)
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

				const auto metadata = m_Metadata.GetVariableMetadata(ir_address->RegisterID);

				//const auto metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(identifier->Symbol.Symbol));

				IRLoad load;
				load.AddressRegister = ir_address->RegisterID;
				load.Type = metadata->Tipe;

				IRRegister* ir_register = CreateIRRegister();
				IRRegister* ir_register_val = m_Metadata.GetRegister(load.AddressRegister);
				ir_register->Value = IR(load);

				m_Metadata.RegExprType(ir_register->ID, metadata->Tipe);

				return IR(IRRegisterValue(ir_register->ID));
			}
			else if (symbol_type == SymbolType::Constant) {
				return generated_code;
			}
			else {

				auto ir_address = generated_code;

				if (!ir_address)
					return nullptr;

				auto expr_type = m_Metadata.GetExprType(ir_address->RegisterID);

				IRLoad load;
				load.AddressRegister = ir_address->RegisterID;

				IRRegister* ir_register = CreateIRRegister();

				load.Type = expr_type;

				ir_register->Value = IR(load);

				m_Metadata.RegExprType(ir_register->ID, expr_type);

				return IR(IRRegisterValue(ir_register->ID));
			}
		}
		break;
		case NodeType::DeReference:
		{
			auto ir_address = generated_code;

			if (!ir_address)
				return nullptr;

			auto expr_type = m_Metadata.GetExprType(ir_address->RegisterID);
			auto new_type = TypeSystem::ReduceIndirection((TSPtr*)expr_type);

			u16 new_type_ind_count = (u64)TypeSystem::IndirectionCount(new_type);

			IRLoad* ir_load = IR(IRLoad());
			ir_load->Type = new_type;
			ir_load->AddressRegister = ir_address->RegisterID;

			auto load_register = CreateIRRegister();

			load_register->Value = ir_load;

			m_Metadata.RegExprType(load_register->ID, new_type);

			return IR(IRRegisterValue(load_register->ID));
		}
		break;
		default:
			return generated_code;
			break;
		}
	}

	IRRegisterValue* Compiler::PassAsAny(const Expression* expr, IRRegisterValue* pre_generated /*= nullptr*/)
	{
		IRRegisterValue* expr_result = (IRRegisterValue*)GetExpressionByValue(expr, pre_generated);

		if (expr_result == nullptr)
			return nullptr;

		auto expr_type = m_Metadata.GetExprType(expr_result->RegisterID);

		if (expr_type->BaseID == IR_any) {
			return expr_result;
		}

		IRRegister* any_register = CreateIRRegister();
		any_register->Value = IR(IRAny(expr_result->RegisterID, expr_type));

		m_Metadata.RegExprType(any_register->ID, expr_type);

		auto ir_load_register = CreateIRRegister();

		IRLoad* ir_load = IR(IRLoad());
		ir_load->AddressRegister = any_register->ID;
		ir_load->Type = TypeSystem::GetBasic(IR_any);

		ir_load_register->Value = ir_load;

		m_Metadata.RegExprType(ir_load_register->ID, expr_type);

		return IR(IRRegisterValue(ir_load_register->ID));
	}

	IRRegisterValue* Compiler::PassAsVariadicArray(const std::vector<Expression*>& arguments, const std::vector<IRRegisterValue*>& pre_generated_arguments, const ArgumentMetadata* decl_arg)
	{
		if (decl_arg->Type->BaseID == IR_any) {

			std::vector<IRAny> anys;
			anys.reserve(arguments.size());

			for (size_t i = 0; i < arguments.size(); i++) {

				Expression* arg = arguments[i];

				IRRegisterValue* code = nullptr;

				if (i < pre_generated_arguments.size()) {
					code = GetExpressionByValue(arg, pre_generated_arguments[i]);
				}
				else {
					code = (IRRegisterValue*)GetExpressionByValue(arg);
				}

				if (!code) {
					return nullptr;
				}

				auto arg_type = m_Metadata.GetExprType(code->RegisterID);
				code = CreateCopy(arg_type, code);
				arg_type = m_Metadata.GetExprType(code->RegisterID);

				anys.push_back(IRAny(code->RegisterID, arg_type));
			}

			auto any_array_ir_register = CreateIRRegister();
			any_array_ir_register->Value = IR(IRAnyArray(anys));

			m_Metadata.RegExprType(any_array_ir_register->ID, TypeSystem::GetBasic(IR_array));

			return IR(IRRegisterValue(any_array_ir_register->ID));
		}

		GS_CORE_ASSERT(0, "Non any var args are yet to be implemented");

		return nullptr;
	}


	IRRegisterValue* Compiler::TypeExpressionCodeGen(TypeExpression* type_expr)
	{
		auto type = TypeExpressionGetType(type_expr);

		if (!type) {
			MSG_LOC(type_expr);
			FMT_WARN("unknown type");
			return nullptr;
		}

		return TypeValueCodeGen(type);
	}

	IRRegisterValue* Compiler::TypeValueCodeGen(TypeStorage* type)
	{
		auto expr_type = TypeSystem::GetBasic(IR_type);

		auto type_expr_value_register = CreateIRRegister();

		type_expr_value_register->Value = IR(IRTypeValue(type));

		m_Metadata.RegExprType(type_expr_value_register->ID, expr_type);

		return IR(IRRegisterValue(type_expr_value_register->ID));
	}


	IRRegisterValue* Compiler::CreateLoad(TypeStorage* type, u64 address)
	{
		auto load_register = CreateIRRegister();
		load_register->Value = IR(IRLoad(address, type));
		m_Metadata.RegExprType(load_register->ID, type);
		return IR(IRRegisterValue(load_register->ID));
	}

	IRRegisterValue* Compiler::CreateStore(TypeStorage* type, u64 address, IRInstruction* data)
	{
		auto store_register = CreateIRRegister();
		store_register->Value = IR(IRStore(address, data, type));
		return IR(IRRegisterValue(store_register->ID));
	}

	IRRegisterValue* Compiler::Create_Null(TypeStorage* type)
	{
		auto ir_register = CreateIRRegister();
		ir_register->Value = IR(IRNullPtr(type));
		return IR(IRRegisterValue(ir_register->ID));
	}

	IRRegisterValue* Compiler::Create_String_Initializer(u64 data_register_id, u64 count_register_id)
	{
		auto ir_register = CreateIRRegister();
		ir_register->Value = IR(IRStringInitializer(data_register_id, count_register_id));
		return IR(IRRegisterValue(ir_register->ID));
	}

	void Compiler::Create_Intrinsic_Memset(TypeStorage* type, u64 pointer_ir_register, u64 value)
	{
		auto ir_register = CreateIRRegister();
		auto intrinsic_memset = IR(IRIntrinsicMemSet(type, pointer_ir_register, value));
		ir_register->Value = intrinsic_memset;
	}

	IRRegisterValue* Compiler::CreateConstantInteger(u64 integer_base_type, i64 value)
	{
		IRCONSTValue* Constant = IR(IRCONSTValue());;

		Constant->Constant_Type = TypeSystem::GetBasic(integer_base_type);
		memcpy(&Constant->Data, &value, sizeof(i64));

		return CreateIRRegister(Constant, Constant->Constant_Type);
	}

	IRRegisterValue* Compiler::CreateConstant(u64 base_type, i64 value_integer, double value_float)
	{
		IRCONSTValue* Constant = IR(IRCONSTValue());

		Constant->Constant_Type = TypeSystem::GetBasic(base_type);

		if (m_Metadata.GetTypeFlags(base_type) & FLAG_FLOATING_TYPE) {
			memcpy(&Constant->Data, &value_float, sizeof(double));
		}
		else {
			memcpy(&Constant->Data, &value_integer, sizeof(i64));
		}

		return CreateIRRegister(Constant, Constant->Constant_Type);
	}

	IRRegisterValue* Compiler::CreateCopy(TypeStorage* type, IRRegisterValue* loaded_value)
	{
		auto alloca = CreateIRRegister(IR(IRAlloca(type)), type);
		CreateStore(type, alloca->RegisterID, loaded_value);
		return alloca;
	}

	IRRegisterValue* Compiler::CreateMemberAccess(const std::string& strct, const std::string& member, u64 address)
	{
		u64 struct_id = m_Metadata.GetStructIDFromType(m_Metadata.GetType(strct));

		const StructMetadata* metadata = m_Metadata.GetStructMetadata(struct_id);

		IRMemberAccess* member_access = IR(IRMemberAccess());
		member_access->StructID = struct_id;
		member_access->MemberID = metadata->FindMember(member);
		member_access->ObjectRegister = address;

		return CreateIRRegister(member_access);
	}

	IRRegisterValue* Compiler::CreatePointerCast(TypeStorage* to_type, u64 address)
	{
		return CreateIRRegister(IR(IRPointerCast(to_type, address)));
	}

	IRFunction* Compiler::CreateIRFunction(const FunctionNode* functionNode)
	{
		IRFunction* IRF = IR(IRFunction());
		IRF->ID = GetFunctionID();

		return IRF;
	}

	IRRegister* Compiler::CreateIRRegister()
	{
		IRRegister* Register = IR(IRRegister());
		Register->ID = m_RegisterIDCounter;

		m_RegisterIDCounter++;

		PushIRRegister(Register);

		Register->SetDBGLoc(m_CurrentDBGLoc);

		return Register;
	}

	IRRegisterValue* Compiler::CreateIRRegister(IRInstruction* value)
	{
		auto ir_register = CreateIRRegister();
		ir_register->Value = value;
		return IR(IRRegisterValue(ir_register->ID));
	}

	IRRegisterValue* Compiler::CreateIRRegister(IRInstruction* value, TypeStorage* semantic_type)
	{
		auto ir_register = CreateIRRegister();
		ir_register->Value = value;
		m_Metadata.RegExprType(ir_register->ID, semantic_type);
		return IR(IRRegisterValue(ir_register->ID));
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

	void Compiler::TypeExpressionGetPolyMorphicTypeNames(TypeExpression* type_expr, std::vector<TypeExpression*>& results)
	{
		if (type_expr->GetType() == NodeType::TE_Pointer) {
			TypeExpressionPointer* as_pointer = (TypeExpressionPointer*)type_expr;
			TypeExpressionGetPolyMorphicTypeNames(as_pointer->Pointee, results);
		}

		if (type_expr->GetType() == NodeType::TE_Dollar) {
			TypeExpressionDollar* as_dollar = (TypeExpressionDollar*)type_expr;
			results.push_back(as_dollar->TypeName);
		}

		if (type_expr->GetType() == NodeType::TE_TypeName) {
		}

		if (type_expr->GetType() == NodeType::Identifier) {
			GS_CORE_ASSERT(nullptr);
		}

		if (type_expr->GetType() == NodeType::TE_Func) {
			GS_CORE_ASSERT(nullptr);
		}

		if (type_expr->GetType() == NodeType::TE_Array) {
			TypeExpressionArray* as_array = (TypeExpressionArray*)type_expr;
			TypeExpressionGetPolyMorphicTypeNames(as_array->ElementType, results);
		}
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

	void Compiler::BinaryDispatch(const Expression* left, const Expression* right, TypeStorage** left_type, TypeStorage** right_type, IRRegisterValue** A, IRRegisterValue** B, std::vector<IRRegister*>* a_code /*= nullptr*/, std::vector<IRRegister*>* b_code /*= nullptr*/)
	{
		auto previous_register = PoPIRRegisters();
		GS_CORE_ASSERT(previous_register.size() == 0);

		if (left->GetType() != NodeType::NumericLiteral) {
			if (a_code) {
				PushScope();

				*A = GetExpressionByValue(left);

				auto a_register_stack = PoPIRRegisters();

				for (auto a_register : a_register_stack) {
					a_code->push_back(a_register);
				}

				PopScope();
			}
			else {
				*A = GetExpressionByValue(left);
			}

			if (!*A) {
				return;
			}

			*left_type = m_Metadata.GetExprType((*A)->RegisterID);

			SetLikelyConstantType((*left_type));
		}

		if (right->GetType() != NodeType::NumericLiteral) {

			if (b_code) {
				PushScope();

				*B = GetExpressionByValue(right);

				auto b_register_stack = PoPIRRegisters();

				for (auto b_register : b_register_stack) {
					b_code->push_back(b_register);
				}

				PopScope();
			}
			else {
				*B = GetExpressionByValue(right);
			}

			if (!*B) {
				return;
			}

			*right_type = m_Metadata.GetExprType((*B)->RegisterID);

			SetLikelyConstantType((*right_type));
		}

		if (*left_type == nullptr) {
			if (a_code) {
				PushScope();

				*A = GetExpressionByValue(left);

				auto a_register_stack = PoPIRRegisters();

				for (auto a_register : a_register_stack) {
					a_code->push_back(a_register);
				}

				PopScope();
			}
			else {
				*A = GetExpressionByValue(left);
			}
			if (!*A) {
				return;
			}
		}
		if (*right_type == nullptr) {
			if (b_code) {
				PushScope();

				*B = GetExpressionByValue(right);

				auto b_register_stack = PoPIRRegisters();

				for (auto b_register : b_register_stack) {
					b_code->push_back(b_register);
				}

				PopScope();
			}
			else {
				*B = GetExpressionByValue(right);
			}
			if (!*B) {
				return;
			}
		}

		*left_type = m_Metadata.GetExprType((*A)->RegisterID);
		*right_type = m_Metadata.GetExprType((*B)->RegisterID);

		ResetLikelyConstantType();
	}
}