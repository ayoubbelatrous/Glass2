#include "pch.h"

#include "Application.h"

#include "BackEnd/Compiler.h"
#include "FrontEnd/AstCopier.h"
#include "FrontEnd/AstPolymorpher.h"
#include "Interpeter/Interpeter.h"

namespace Glass
{
	Compiler::Compiler(std::vector<CompilerFile*> files) :m_Files(files)
	{
		m_Metadata.RegisterType((u64)IRType::IR_void, "void");

		m_Metadata.RegisterType((u64)IRType::IR_float, "float");
		m_Metadata.RegisterType((u64)IRType::IR_int, "int");

		m_Metadata.RegisterType((u64)IRType::IR_i8, "i8");
		m_Metadata.RegisterType((u64)IRType::IR_i16, "i16");
		m_Metadata.RegisterType((u64)IRType::IR_i32, "i32");
		m_Metadata.RegisterType((u64)IRType::IR_i64, "i64");

		m_Metadata.RegisterType((u64)IRType::IR_u8, "u8");
		m_Metadata.RegisterType((u64)IRType::IR_u16, "u16");
		m_Metadata.RegisterType((u64)IRType::IR_u32, "u32");
		m_Metadata.RegisterType((u64)IRType::IR_u64, "u64");

		m_Metadata.RegisterType((u64)IRType::IR_f32, "f32");
		m_Metadata.RegisterType((u64)IRType::IR_f64, "f64");

		m_Metadata.RegisterType((u64)IRType::IR_bool, "bool");

		//m_Metadata.RegisterType((u64)IRType::IR_typeinfo, "type_info");

		StructMetadata type_info_Metadata;
		type_info_Metadata.Name.Symbol = "type_info";

		//id
		{
			MemberMetadata id_member_metadata;
			id_member_metadata.Name.Symbol = "id";
			id_member_metadata.Tipe.ID = IR_u64;
			id_member_metadata.Tipe.TT = TypeType::Value;

			type_info_Metadata.Members.push_back(id_member_metadata);
		}

		//name
		{
			MemberMetadata name_member_metadata;
			name_member_metadata.Name.Symbol = "name";
			name_member_metadata.Tipe.ID = IR_u8;
			name_member_metadata.Tipe.TT = TypeType::Pointer;

			type_info_Metadata.Members.push_back(name_member_metadata);
		}

		m_Metadata.RegisterStruct(GetStructID(), IR_typeinfo, type_info_Metadata);

		m_TypeIDCounter = IR_typeinfo;
	}

	IRTranslationUnit* Compiler::CodeGen()
	{
		IRTranslationUnit* tu = IR(IRTranslationUnit());
		for (CompilerFile* file : m_Files) {
			ModuleFile* module_file = file->GetAST();
			for (const Statement* stmt : module_file->GetStatements()) {
				auto stmt_code = StatementCodeGen(stmt);
				if (stmt_code != nullptr) {
					tu->Instructions.push_back(stmt_code);
				}
			}
		}

		for (auto& func : m_Metadata.m_Functions) {
			FunctionMetadata& metadata = func.second;
			if (metadata.PolyMorphic) {
				for (auto [irf, function_node] : metadata.PendingPolymorphInstantiations) {
					PushScope();
					IRFunction* ir_function = (IRFunction*)FunctionCodeGen(function_node);
					PopScope();
					irf->Instructions = ir_function->Instructions;
					irf->Arguments = ir_function->Arguments;

					m_Metadata.m_Functions[irf->ID] = *m_Metadata.GetFunctionMetadata(ir_function->ID);
				}
			}
		}

		auto ir_data = PoPIRData();

		std::vector<IRInstruction*> instructions;

		for (auto entry : ir_data) {
			instructions.push_back(entry);
		}

		for (auto entry : tu->Instructions) {
			instructions.push_back(entry);
		}

		tu->Instructions = instructions;

		for (auto& func : m_Metadata.m_Functions) {
			FunctionMetadata& metadata = func.second;
			if (metadata.PolyMorphic) {

				for (auto& [overload, irf] : metadata.PolyMorhOverLoads) {
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
		case NodeType::Identifier:
		case NodeType::NumericLiteral:
		case NodeType::BinaryExpression:
		case NodeType::StringLiteral:
		case NodeType::Call:
		case NodeType::MemberAccess:
		case NodeType::ArrayAccess:
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

			if (tipe != NodeType::Function) {
				PushMessage(CompilerMessage{ PrintTokenLocation(frn->statement->GetLocation()),MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("#foreign directive only supports functions at the moment"),MessageType::Warning });
			}

			u64 ID = GetFunctionID();

			Type return_type;
			return_type.ID = m_Metadata.GetType(fn_decl->ReturnType->Symbol.Symbol);
			return_type.TT = fn_decl->ReturnType->Pointer ? TypeType::Pointer : TypeType::Value;

			std::vector<ArgumentMetadata> args;

			for (const Statement* a : fn_decl->GetArgList()->GetArguments())
			{
				const VariableNode* decl_arg = (VariableNode*)a;

				ArgumentMetadata fmt_arg;

				fmt_arg.Name = decl_arg->Symbol.Symbol;
				fmt_arg.Tipe.TT = decl_arg->Type->Pointer ? TypeType::Pointer : TypeType::Value;
				fmt_arg.Tipe.ID = m_Metadata.GetType(decl_arg->Type->Symbol.Symbol);
				fmt_arg.Tipe.Array = decl_arg->Type->Array;

				args.push_back(fmt_arg);
			}

			m_Metadata.RegisterFunction(ID, fn_decl->Symbol.Symbol, return_type, args, fn_decl->Variadic);

		}
		else if (tipe == NodeType::StructNode) {
			StructNode* strct_decl = (StructNode*)frn->statement;
			m_Metadata.RegisterType(GetTypeID(), strct_decl->Name.Symbol);
		}
		return nullptr;
	}

	IRInstruction* Compiler::FunctionCodeGen(FunctionNode* functionNode)
	{
		ResetSSAIDCounter();

		u64 poly_func_id = GetFunctionID();

		Type return_type;
		return_type.ID = (u64)IRType::IR_void;

		if (functionNode->ReturnType) {
			return_type.ID = m_Metadata.GetType(functionNode->ReturnType->Symbol.Symbol);
		}

		return_type.TT = functionNode->ReturnType->Pointer ? TypeType::Pointer : TypeType::Value;

		m_Metadata.RegisterFunction(poly_func_id, functionNode->Symbol.Symbol, return_type);

		bool poly_morphic = false;

		{
			const auto& arg_list = functionNode->GetArgList()->GetArguments();

			for (const auto arg : arg_list) {
				auto var = (VariableNode*)arg;
				if (var->Type->PolyMorphic) {
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

			for (auto arg : arg_list) {
				auto var = (VariableNode*)arg;

				ArgumentMetadata arg_metadata;

				arg_metadata.Name = var->Symbol.Symbol;

				if (var->Type->PolyMorphic) {
					arg_metadata.PolyMorphic = true;
					arg_metadata.PolyMorhID = func_metadata->GetPolyMorphID(var->Type->Symbol.Symbol);
				}
				else {
					arg_metadata.Tipe.ID = m_Metadata.GetType(var->Type->Symbol.Symbol);
				}

				arg_metadata.Tipe.TT = var->Type->Pointer ? TypeType::Pointer : TypeType::Value;

				args_metadata.push_back(arg_metadata);

				func_metadata->Arguments = args_metadata;
			}

			func_metadata->FunctionAST = functionNode;

			return nullptr;
		}

		std::vector<ArgumentMetadata> args_metadata;

		IRFunction* IRF = CreateIRFunction(functionNode);

		PoPIRSSA();

		for (auto a : functionNode->GetArgList()->GetArguments()) {

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

			if (arg_type == (u64)-1) {

				PushMessage(CompilerMessage{ PrintTokenLocation(var->Type->GetLocation()),MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("argument '{}' is of undefined type '{}', at '{}' function definition",
					var->Symbol.Symbol,var->Type->Symbol.Symbol,functionNode->DefinitionTk.Symbol
				),MessageType::Warning });

				return nullptr;
			}

			VariableMetadata var_metadata = {
			var->Symbol,
			Type
			{
				arg_type,
				var->Type->Array,
				var->Type->Pointer ? TypeType::Pointer : TypeType::Value,
			},
			};

			arg_ssa->Type = var_metadata.Tipe.ID;
			arg_ssa->Pointer = var->Type->Pointer;

			m_Metadata.RegisterVariableMetadata(arg_address_ssa->ID, var_metadata);

			IRF->Arguments.push_back(arg_ssa);

			ArgumentMetadata arg_metadata;

			arg_metadata.Name = var->Symbol.Symbol;
			arg_metadata.Tipe.TT = arg_ssa->Pointer ? TypeType::Pointer : TypeType::Value;
			arg_metadata.Tipe.ID = arg_ssa->Type;

			args_metadata.push_back(arg_metadata);
		}

		m_Metadata.RegisterFunction(IRF->ID, functionNode->Symbol.Symbol, return_type, args_metadata);

		for (const Statement* stmt : functionNode->GetStatements()) {

			// 			if (stmt->GetType() == NodeType::If) {
			// 				auto post_ssas = PoPIRSSA();
			// 
			// 				for (auto ssa : post_ssas) {
			// 					IRF->Instructions.push_back(ssa);
			// 				}
			// 			}

			IRInstruction* code = StatementCodeGen(stmt);

			auto SSAs = PoPIRSSA();

			for (auto ssa : SSAs) {
				IRF->Instructions.push_back(ssa);
			}

			if (dynamic_cast<IRSSA*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRReturn*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRStore*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRLoad*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRFunctionCall*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRMemberAccess*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRIf*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRWhile*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}

			if (dynamic_cast<IRBreak*>(code) != nullptr) {
				IRF->Instructions.push_back(code);
			}
		}


		m_Metadata.m_CurrentFunction++;

		return (IRInstruction*)IRF;
	}

	IRInstruction* Compiler::VariableCodeGen(const VariableNode* variableNode)
	{
		const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(variableNode->Symbol.Symbol));

		if (metadata != nullptr)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(variableNode->GetLocation()),MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable '{}' is already defined",variableNode->Symbol.Symbol),MessageType::Warning });
			PushMessage(CompilerMessage{ fmt::format("Defined At!",variableNode->Symbol.Symbol),MessageType::Info });
			PushMessage(CompilerMessage{ "\t" + PrintTokenLocation(metadata->Name),MessageType::Info });
			return nullptr;
		}

		IRInstruction* value = nullptr;

		if (variableNode->Assignment != nullptr) {
			value = GetExpressionByValue(variableNode->Assignment);
		}

		IRSSA* StorageSSA = CreateIRSSA();

		StorageSSA->Type = m_Metadata.GetType(variableNode->Type->Symbol.Symbol);

		if (variableNode->Type->Array || variableNode->Type->Pointer) {
			StorageSSA->Pointer = true;
		}

		StorageSSA->Value = nullptr;

		IRSSA* IRssa = CreateIRSSA();
		IRssa->Type = (u64)IRType::IR_u64;

		IRSSAValue StorageSSAVal;
		IRAddressOf address_of;

		{
			StorageSSAVal.SSA = StorageSSA->ID;
			address_of.SSA = IR(StorageSSAVal);
		}

		IRssa->Value = IR(address_of);

		IRssa->PointerReference = StorageSSA->Pointer;
		IRssa->ReferenceType = StorageSSA->Type;
		IRssa->SSAType = RegType::VarAddress;

		RegisterVariable(IRssa, variableNode->Symbol.Symbol);

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			Type
			{
				 m_Metadata.GetType(variableNode->Type->Symbol.Symbol),
				variableNode->Type->Array,
				variableNode->Type->Pointer ? TypeType::Pointer : TypeType::Value,
			},
			false
			,
			StorageSSA,
			IRssa
		};

		m_Metadata.RegisterVariableMetadata(IRssa->ID, var_metadata);

		m_Metadata.RegExprType(IRssa->ID, var_metadata.Tipe);

		if (value != nullptr) {
			IRStore store;
			store.AddressSSA = IRssa->ID;
			store.Data = value;
			store.Type = StorageSSA->Type;
			store.Pointer = StorageSSA->Pointer;

			return IR(store);
		}
		else {
			return nullptr;
		}
	}

	IRInstruction* Compiler::ReturnCodeGen(const ReturnNode* returnNode)
	{
		IRReturn ret;

		ret.Type = (u64)IRType::IR_i32;

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

		for (VariableNode* member : struct_members) {

			MemberMetadata member_metadata;
			member_metadata.Name = member->Symbol;
			member_metadata.Tipe.ID = m_Metadata.GetType(member->Type->Symbol.Symbol);
			member_metadata.Tipe.TT = member->Type->Pointer ? TypeType::Pointer : TypeType::Value;
			member_metadata.Tipe.Array = member->Type->Array;

			struct_metadata.Members.push_back(member_metadata);
		}

		u64 type_id = GetTypeID();
		u64 struct_id = GetStructID();

		m_Metadata.RegisterStruct(struct_id, type_id, struct_metadata);

		IRStruct ir_struct;

		ir_struct.TypeID = type_id;
		ir_struct.ID = struct_id;

		u64 member_id_counter = 0;

		for (MemberMetadata& member : struct_metadata.Members) {

			IRStructMember ir_member;

			ir_member.ID = member_id_counter;
			ir_member.TypeID = member.Tipe.ID;
			ir_member.Pointer = member.Tipe.TT == TypeType::Pointer;

			ir_struct.Members.push_back(IR(ir_member));

			member_id_counter++;
		}

		return IR(ir_struct);
	}

	IRInstruction* Compiler::IfCodeGen(const IfNode* ifNode)
	{
		IRSSAValue* condition = GetExpressionByValue(ifNode->Condition);

		IRIf IF;
		IF.SSA = condition->SSA;

		std::vector<IRInstruction*> instructions;

		PushScope();

		for (const Statement* stmt : ifNode->Scope->GetStatements()) {
			instructions.push_back(StatementCodeGen(stmt));
		}


		auto ir_ssa_stack = PoPIRSSA();

		for (auto inst : ir_ssa_stack) {
			IF.Instructions.push_back(inst);
		}

		for (auto inst : instructions) {
			IF.Instructions.push_back(inst);
		}

		PopScope();

		return IR(IF);
	}

	IRInstruction* Compiler::WhileCodeGen(const WhileNode* whileNode)
	{
		IRSSAValue* condition = GetExpressionByValue(whileNode->Condition);

		IRWhile WHILE;
		WHILE.SSA = condition->SSA;

		std::vector<IRInstruction*> instructions;

		PushScope();

		for (const Statement* stmt : whileNode->Scope->GetStatements()) {
			instructions.push_back(StatementCodeGen(stmt));
		}


		auto ir_ssa_stack = PoPIRSSA();

		for (auto inst : ir_ssa_stack) {
			WHILE.Instructions.push_back(inst);
		}

		for (auto inst : instructions) {
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
		case NodeType::TypeOf:
			return TypeofCodeGen((TypeOfNode*)expression);
			break;
		}

		return nullptr;
	}

	IRInstruction* Compiler::IdentifierCodeGen(const Identifier* identifier)
	{
		const VariableMetadata* metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(identifier->Symbol.Symbol));

		if (metadata == nullptr)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(identifier->GetLocation()),MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("variable '{}' is not defined",identifier->Symbol.Symbol),MessageType::Warning });
			return nullptr;
		}

		u64 ID = GetVariableSSA(identifier->Symbol.Symbol);

		IRSSA* ssa = GetSSA(ID);

		IRSSAValue ssa_val;

		ssa_val.SSA = GetVariableSSA(identifier->Symbol.Symbol);

		m_Metadata.RegExprType(ssa_val.SSA, metadata->Tipe);

		return (IRInstruction*)IR(ssa_val);
	}

	IRInstruction* Compiler::NumericLiteralCodeGen(const NumericLiteral* numericLiteral)
	{
		IRSSA* IRssa = CreateIRSSA();

		IRssa->Value = IR(IRCONSTValue());

		IRCONSTValue* Value = (IRCONSTValue*)IRssa->Value;

		if (numericLiteral->type == NumericLiteral::Type::Float) {
			IRssa->Type = (u64)IRType::IR_float;
			memcpy(&Value->Data, &numericLiteral->Val.Float, sizeof(float));
		}
		if (numericLiteral->type == NumericLiteral::Type::Int) {
			IRssa->Type = (u64)IRType::IR_int;
			memcpy(&Value->Data, &numericLiteral->Val.Int, sizeof(i32));
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

		for (char c : stringLiteral->Symbol.Symbol) {
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
			type.TT = TypeType::Pointer;
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

		IRSSA* IRssa = CreateIRSSA();

		IRssa->Type = m_Metadata.GetType("i32");

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
		default:
			return nullptr;
			break;
		}

		IRSSAValue* ssa_value = IR(IRSSAValue());

		ssa_value->SSA = IRssa->ID;

		return ssa_value;
	}

	IRInstruction* Compiler::AssignmentCodeGen(const BinaryExpression* binaryExpr)
	{
		auto left = binaryExpr->Left;
		auto right = binaryExpr->Right;

		if (left->GetType() == NodeType::Identifier) {
			IRSSAValue* right_val = (IRSSAValue*)ExpressionCodeGen(binaryExpr->Right);

			u64 var_ssa_id = GetVariableSSA(((Identifier*)left)->Symbol.Symbol);
			IRSSA* var_ssa = m_Metadata.GetSSA(var_ssa_id);

			const auto metadata = m_Metadata.GetVariableMetadata(var_ssa_id);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_val;
				store->AddressSSA = var_ssa_id;
				store->Type = metadata->DataSSA->Type;
				store->Pointer = metadata->DataSSA->Pointer;
			}

			return store;
		}
		if (left->GetType() == NodeType::MemberAccess) {
			IRSSAValue* member_access = (IRSSAValue*)ExpressionCodeGen(left);
			IRSSAValue* right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = member_access->SSA;
				store->Type = m_Metadata.GetSSA(member_access->SSA)->ReferenceType;
				store->Pointer = m_Metadata.GetExprType(member_access->SSA).TT == TypeType::Pointer;
			}

			return store;
		}
		if (left->GetType() == NodeType::ArrayAccess) {
			auto left_ssa = (IRSSAValue*)GetExpressionByValue(left);
			auto right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			IRStore* store = IR(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = left_ssa->SSA;
				store->Type = m_Metadata.GetExprType(right_ssa->SSA).ID;
			}
			return store;
		}
		else {
		}
		return nullptr;
	}

	IRInstruction* Compiler::FunctionCallCodeGen(const FunctionCall* call)
	{
		u64 IRF = m_Metadata.GetFunctionMetadata(call->Function.Symbol);
		FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(IRF);

		if (metadata == nullptr)
		{
			PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()),MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("trying to call a undefined function '{}'",call->Function.Symbol),MessageType::Warning });
			return nullptr;
		}
		else {
			if ((call->Arguments.size() > metadata->Arguments.size()) && !metadata->Variadic) {
				PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()),MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("too many arguments for '{}()' function call ",call->Function.Symbol),MessageType::Warning });
				return nullptr;
			}
			if ((call->Arguments.size() < metadata->Arguments.size())) {
				PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()),MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("too few arguments for '{}()' function call ",call->Function.Symbol),MessageType::Warning });
				return nullptr;
			}
		}

		std::vector<IRSSAValue*> argument_expr_results;

		if (metadata->PolyMorphic) {
			// 			PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()),MessageType::Error });
			// 			PushMessage(CompilerMessage{ fmt::format("calls to polymorphic '{}' function is unsupported",call->Function.Symbol),MessageType::Warning });
			// 			return nullptr;

			PolyMorphOverloads overloads;

			auto& args = call->Arguments;

			for (size_t i = 0; i < args.size(); i++)
			{
				auto expr_code = GetExpressionByValue(args[i]);

				if (!expr_code) {
					return nullptr;
				}

				argument_expr_results.push_back(expr_code);

				if (!metadata->Arguments[i].PolyMorphic) {
					continue;
				}

				const Glass::Type& expr_type = m_Metadata.GetExprType(expr_code->SSA);

				PolyMorphicType poly_type;
				poly_type.ID = metadata->GetPolyMorphID(metadata->Arguments[i].Name);

				overloads.TypeArguments.push_back({ poly_type, expr_type });
			}
			const IRFunction* overload = GetPolyMorphOverLoad(IRF, overloads);

			IRF = overload->ID;

			if (overload == nullptr) {
				PushMessage(CompilerMessage{ PrintTokenLocation(call->GetLocation()),MessageType::Error });
				PushMessage(CompilerMessage{ fmt::format("failed to instantiate polymorphic function '{}'",call->Function.Symbol),MessageType::Warning });
				return nullptr;
			}
		}

		IRFunctionCall ir_call;

		ir_call.FuncID = IRF;

		if (metadata->PolyMorphic) {
			for (size_t i = 0; i < call->Arguments.size(); i++)
			{
				ir_call.Arguments.push_back(IR(IRSSAValue(argument_expr_results[i]->SSA)));
			}
		}
		else {
			for (size_t i = 0; i < call->Arguments.size(); i++)
			{
				const ArgumentMetadata* decl_arg = metadata->GetArgument(i);

				IRInstruction* arg = nullptr;

				if (decl_arg != nullptr) {

					IRSSAValue* expr = nullptr;

					expr = GetExpressionByValue(call->Arguments[i]);

					if (expr == nullptr)
						return nullptr;

					IRNodeType Type = expr->GetType();

					u64 arg_SSAID = 0;

					switch (Type)
					{
					case IRNodeType::SSAValue:
						arg_SSAID = ((IRSSAValue*)expr)->SSA;
						break;
					}

					IRSSA* arg_ssa = m_Metadata.GetSSA(arg_SSAID);

					Glass::Type type;

					if (arg_ssa->SSAType == RegType::VarAddress) {
						type = m_Metadata.GetVariableMetadata(arg_ssa->ID)->Tipe;
					}
					else {
						type.ID = arg_ssa->Type;
					}

					bool type_mismatch = !CheckTypeConversion(decl_arg->Tipe.ID, type.ID);

					if (type_mismatch) {

						PushMessage(CompilerMessage{ PrintTokenLocation(call->Arguments[i]->GetLocation()),MessageType::Error });
						PushMessage(CompilerMessage{ "type mismatch in function call",MessageType::Warning });
						PushMessage(CompilerMessage{ fmt::format("needed a '{}' instead got '{}'",
							PrintType(decl_arg->Tipe),
							PrintType(type)),

							MessageType::Info });
						PushMessage(CompilerMessage{ fmt::format("In place of function argument '{}'", decl_arg->Name),
						MessageType::Info });
					}

					if (decl_arg->Tipe.TT == TypeType::Pointer) {
						arg = GetExpressionByValue(call->Arguments[i]);
					}
					else {
						arg = IR(IRSSAValue(arg_SSAID));
					}
				}
				else {
					arg = GetExpressionByValue(call->Arguments[i]);
				}

				ir_call.Arguments.push_back(arg);
			}

		}

		if (metadata->ReturnType.ID != (u64)IRType::IR_void)
		{
			auto ir_ssa = CreateIRSSA();

			ir_ssa->Type = metadata->ReturnType.ID;
			ir_ssa->Value = IR(ir_call);
			ir_ssa->Pointer = metadata->ReturnType.TT == TypeType::Pointer;

			IRSSAValue* ir_sss_val = IR(IRSSAValue());

			ir_sss_val->SSA = ir_ssa->ID;

			return ir_sss_val;
		}

		return IR(ir_call);
	}

	IRInstruction* Compiler::MemberAccessCodeGen(const MemberAccess* memberAccess)
	{
		u64 struct_id = 0;
		u64 member_id = 0;
		u64 object_ssa_id = 0;
		bool reference_access = false;

		const MemberMetadata* member_metadata = nullptr;
		const StructMetadata* struct_metadata = nullptr;

		Glass::Type type;

		{
			switch (memberAccess->Object->GetType()) {
			case NodeType::Identifier:
			{
				Identifier* object = (Identifier*)memberAccess->Object;

				{
					SymbolType symbol_type = m_Metadata.GetSymbolType(object->Symbol.Symbol);
					if (symbol_type == SymbolType::None) {
						PushMessage(CompilerMessage{ PrintTokenLocation(object->GetLocation()), MessageType::Error });
						PushMessage(CompilerMessage{ fmt::format("unknown symbol '{}'",object->ToString()),MessageType::Warning });
						return nullptr;
					}

					if (symbol_type == SymbolType::Type) {
						PushMessage(CompilerMessage{ PrintTokenLocation(object->GetLocation()), MessageType::Error });
						PushMessage(CompilerMessage{ fmt::format("symbol '{}' is a type, cannot have members",object->ToString()),MessageType::Warning });
						return nullptr;
					}

					if (symbol_type == SymbolType::Function) {
						PushMessage(CompilerMessage{ PrintTokenLocation(object->GetLocation()), MessageType::Error });
						PushMessage(CompilerMessage{ fmt::format("symbol '{}' is a function, cannot have members",object->ToString()),MessageType::Warning });
						return nullptr;
					}
				}

				u64 var_ssa = m_Metadata.GetVariableSSA(object->Symbol.Symbol);

				const auto var_metadata = m_Metadata.GetVariableMetadata(var_ssa);

				struct_id = m_Metadata.GetStructIDFromType(var_metadata->Tipe.ID);
				struct_metadata = m_Metadata.GetStructMetadata(struct_id);
				object_ssa_id = var_ssa;

				if (var_metadata->Tipe.TT == TypeType::Pointer) {
					reference_access = true;
				}
			}
			break;
			case NodeType::MemberAccess:
			{
				auto object_code = (IRSSAValue*)ExpressionCodeGen(memberAccess->Object);
				struct_id = m_Metadata.GetStructIDFromType(GetSSA(object_code->SSA)->ReferenceType);
				struct_metadata = m_Metadata.GetStructMetadata(struct_id);

				object_ssa_id = object_code->SSA;
			}
			break;
			}
		}

		{
			switch (memberAccess->Member->GetType()) {
			case NodeType::Identifier:
			{
				Identifier* member = (Identifier*)memberAccess->Member;
				member_id = struct_metadata->FindMember(member->Symbol.Symbol);
				member_metadata = &struct_metadata->Members[member_id];

				type = member_metadata->Tipe;
			}
			break;
			case NodeType::MemberAccess:
			{
				MemberAccess* member = (MemberAccess*)memberAccess->Member;
				member_id = struct_metadata->FindMember(((Identifier*)member->Object)->Symbol.Symbol);
				member_metadata = &struct_metadata->Members[member_id];

				type = member_metadata->Tipe;
			}
			break;
			}
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
			address_ssa->ReferenceType = member_metadata->Tipe.ID;

			IRSSAValue address_ssa_val;
			address_ssa_val.SSA = address_ssa->ID;

			m_Metadata.RegExprType(address_ssa->ID, type);

			return IR(address_ssa_val);
		}
	}

	IRInstruction* Compiler::ArrayAccessCodeGen(const ArrayAccess* arrayAccess)
	{
		auto object = (IRSSAValue*)GetExpressionByValue(arrayAccess->Object);
		auto index = (IRSSAValue*)GetExpressionByValue(arrayAccess->Index);

		auto obj_ssa = m_Metadata.GetSSA(object->SSA);
		auto index_ssa = m_Metadata.GetSSA(index->SSA);

		u64 obj_address_ssa = 0;
		u64 type = 0;

		obj_address_ssa = obj_ssa->ID;

		auto& obj_expr_type = m_Metadata.GetExprType(obj_address_ssa);

		type = obj_expr_type.ID;

		// 		obj_address_ssa = object->SSA;
		// 
		// 		// 		if (obj_ssa->ReferenceType) {
		// 		// 
		// 		// 			Identifier* identifier = (Identifier*)arrayAccess->Object;
		// 		// 
		// 		// 			type = obj_ssa->ReferenceType;
		// 		// 			obj_address_ssa = m_Metadata.GetVariableSSA(identifier->Symbol.Symbol);
		// 		// 		}
		//type = obj_ssa->Type;

		if (obj_expr_type.TT != TypeType::Pointer) {
			PushMessage(CompilerMessage{ PrintTokenLocation(arrayAccess->Object->GetLocation()), MessageType::Error });
			PushMessage(CompilerMessage{ "type of expression must be a pointer type in order to be accessed by the [] operator",MessageType::Warning });
		}

		auto sizeof_ssa = CreateIRSSA();

		sizeof_ssa->Value = IR(IRSizeOF(type));
		sizeof_ssa->Type = IR_u64;

		auto ptr_mul_ssa = CreateIRSSA();

		ptr_mul_ssa->Value = IR(IRMUL(index, IR(IRSSAValue(sizeof_ssa->ID))));
		ptr_mul_ssa->Type = IR_u64;

		auto ptr_as_value = CreateIRSSA();

		ptr_as_value->Value = IR(IRAddressAsValue(obj_address_ssa));
		ptr_as_value->Type = IR_u64;

		auto ptr_add_ssa = CreateIRSSA();

		ptr_add_ssa->Value = IR(IRADD(IR(IRSSAValue(ptr_mul_ssa->ID)), IR(IRSSAValue(ptr_as_value->ID))));
		ptr_add_ssa->Type = IR_u64;

		auto address_ssa = CreateIRSSA();

		address_ssa->Value = IR(IRSSAValue(ptr_add_ssa->ID));
		address_ssa->Type = IR_u64;

		m_Metadata.RegExprType(address_ssa->ID, obj_expr_type);

		return IR(IRSSAValue(address_ssa->ID));
	}

	IRInstruction* Compiler::TypeofCodeGen(const TypeOfNode* typeof)
	{
		u64 type_id = 0;
		u64 variable_id = 0;
		bool pointer = false;

		if (typeof->What->GetType() == NodeType::Identifier) {

			Identifier* type_ident = (Identifier*)typeof->What;

			SymbolType symbol_type = m_Metadata.GetSymbolType(type_ident->Symbol.Symbol);

			if (symbol_type == SymbolType::None) {
				PushMessage(CompilerMessage{ PrintTokenLocation(type_ident->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ "typeof() undefined expression inside '()' parentheses",MessageType::Warning });
			}

			if (symbol_type == SymbolType::Type) {
				type_id = m_Metadata.GetType(type_ident->Symbol.Symbol);
			}

			if (symbol_type == SymbolType::Variable) {
				const auto metadata = m_Metadata.GetVariableMetadata(m_Metadata.GetVariableSSA(type_ident->Symbol.Symbol));
				type_id = metadata->Tipe.ID;
				pointer = metadata->Tipe.TT == TypeType::Pointer;
				variable_id = metadata->DataSSA->ID;
			}

			if (symbol_type == SymbolType::Function) {
				PushMessage(CompilerMessage{ PrintTokenLocation(type_ident->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ "typeof() doesn't support functions for now",MessageType::Warning });
			}

		}
		else {
			IRSSAValue* code = (IRSSAValue*)ExpressionCodeGen(typeof->What);
			type_id = m_Metadata.GetExprType(code->SSA).ID;
		}

		IRSSA* ssa = CreateIRSSA();

		IRTypeOf type_of;
		type_of.VariableID = variable_id;
		type_of.Type.id = type_id;
		type_of.Type.name = m_Metadata.GetType(type_id);
		type_of.Type.pointer = pointer;

		ssa->Value = IR(type_of);

		ssa->Type = IR_typeinfo;
		ssa->Pointer = true;

		Type type;
		type.ID = IR_typeinfo;
		type.TT = TypeType::Pointer;

		m_Metadata.RegExprType(ssa->ID, type);

		return IR(IRSSAValue(ssa->ID));
	}

	IRInstruction* Compiler::RefCodeGen(const RefNode* refNode)
	{
		IRSSAValue* expr_value = (IRSSAValue*)ExpressionCodeGen(refNode->What);
		IRSSA* expr_ssa = m_Metadata.GetSSA(expr_value->SSA);

		IRAsAddress* ssa_value = IR(IRAsAddress());
		ssa_value->SSA = expr_value->SSA;

		IRSSA* ir_ssa = CreateIRSSA();
		ir_ssa->Type = expr_ssa->ReferenceType;
		ir_ssa->Pointer = true;
		ir_ssa->Value = ssa_value;

		return IR(IRSSAValue(ir_ssa->ID));
	}

	IRInstruction* Compiler::DeRefCodeGen(const RefNode* deRefNode)
	{
		return nullptr;
	}

	IRSSAValue* Compiler::GetExpressionByValue(const Expression* expr)
	{
		switch (expr->GetType())
		{
		case NodeType::MemberAccess:
		{
			auto ir_address = (IRSSAValue*)MemberAccessCodeGen((MemberAccess*)expr);

			if (ir_address == nullptr)
				return nullptr;

			// 			auto mem_expr = (MemberAccess*)expr;
			// 
			// 			Identifier* object = (Identifier*)mem_expr->Object;
			// 			Identifier* member = (Identifier*)mem_expr->Member;
			// 
			// 			u64 member_type = 0;
			// 			bool member_ptr = false;
			// 
			// 			u64 object_type = m_Metadata.GetVariableMetadata(m_Metadata.GetVariable(object->Symbol.Symbol))->Tipe.ID;
			// 
			// 			const StructMetadata* struct_metadata = m_Metadata.GetStructFromType(object_type);
			// 
			// 			for (size_t i = 0; i < struct_metadata->Members.size(); i++)
			// 			{
			// 				if (struct_metadata->Members[i].Name.Symbol == member->Symbol.Symbol) {
			// 					member_type = struct_metadata->Members[i].Tipe.ID;
			// 					member_ptr = struct_metadata->Members[i].Tipe.TT == TypeType::Pointer;
			// 				}
			// 			}
			// 
			// 			IRSSA* ssa = CreateIRSSA();
			// 
			// 			ssa->Type = member_type;
			// 			ssa->Value = ir_mem_access;
			// 
			// 			if (member_ptr) {
			// 				ssa->Pointer = true;
			// 				ssa->Type = member_type;
			// 			}
			// 
			// 			IRSSAValue ssa_value;
			// 
			// 			ssa_value.SSA = ssa->ID;

			IRSSA* value_ssa = CreateIRSSA();

			IRLoad load;

			const Glass::Type& expr_type = m_Metadata.GetExprType(ir_address->SSA);

			load.SSAddress = ir_address->SSA;
			load.Type = expr_type.ID;

			if (expr_type.TT == TypeType::Pointer || expr_type.Array) {
				load.ReferencePointer = true;
			}

			value_ssa->Value = IR(load);
			value_ssa->Type = load.Type;
			value_ssa->Pointer =
				m_Metadata.GetExprType(ir_address->SSA).TT == TypeType::Pointer || m_Metadata.GetExprType(ir_address->SSA).Array;

			IRSSAValue* ssa_value = IR(IRSSAValue());

			ssa_value->SSA = value_ssa->ID;

			return ssa_value;
		}
		break;
		case NodeType::ArrayAccess:
		{
			auto ir_address = (IRSSAValue*)ArrayAccessCodeGen((ArrayAccess*)expr);

			auto& expr_type = m_Metadata.GetExprType(ir_address->SSA);

			IRSSA* value_ssa = CreateIRSSA();

			IRLoad load;

			load.SSAddress = ir_address->SSA;
			load.Type = expr_type.ID;

			value_ssa->Value = IR(load);
			value_ssa->Type = load.Type;
			value_ssa->Pointer = false;

			m_Metadata.RegExprType(value_ssa->ID, expr_type);

			return  IR(IRSSAValue(value_ssa->ID));
		}
		break;
		case NodeType::Identifier:
		{
			auto ir_address = (IRSSAValue*)IdentifierCodeGen((Identifier*)expr);

			if (!ir_address) {
				return nullptr;
			}

			const auto metadata = m_Metadata.GetVariableMetadata(ir_address->SSA);

			IRLoad load;
			load.SSAddress = ir_address->SSA;
			load.ReferencePointer = metadata->Tipe.TT == TypeType::Pointer;

			IRSSA* ssa = CreateIRSSA();

			IRSSA* var_ssa = m_Metadata.GetSSA(load.SSAddress);

			load.Type = metadata->Tipe.ID;

			ssa->Type = metadata->Tipe.ID;
			ssa->Value = IR(load);
			ssa->Pointer = metadata->Tipe.TT == TypeType::Pointer;

			ssa->Reference = true;
			ssa->ReferenceType = metadata->Tipe.ID;
			ssa->PointerReference = metadata->Tipe.TT == TypeType::Pointer;

			m_Metadata.RegExprType(ssa->ID, metadata->Tipe);

			return IR(IRSSAValue(ssa->ID));
		}
		break;
		default:
			return (IRSSAValue*)ExpressionCodeGen(expr);
			break;
		}
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

		for (Statement* a : args) {
			VariableNode* arg = (VariableNode*)a;

			arg->Type->PolyMorphic = false;
		}

		IRFunction* ir_func = CreateIRFunction(new_function);

		metadata->PendingPolymorphInstantiations.push_back({ ir_func ,new_function });

		return ir_func;
	}
}