#include "pch.h"

#include "BackEnd/Compiler.h"
#include "Application.h"

namespace Glass
{
	Compiler::Compiler(std::vector<CompilerFile*> files) :m_Files(files)
	{
		m_Metadata.RegisterType((u64)IRType::IR_void, "void");
		m_Metadata.RegisterType((u64)IRType::IR_i8, "i8");
		m_Metadata.RegisterType((u64)IRType::IR_i16, "i16");
		m_Metadata.RegisterType((u64)IRType::IR_i32, "i32");
		m_Metadata.RegisterType((u64)IRType::IR_i64, "i64");

		m_Metadata.RegisterType((u64)IRType::IR_u8, "u8");
		m_Metadata.RegisterType((u64)IRType::IR_u16, "u16");
		m_Metadata.RegisterType((u64)IRType::IR_u32, "u32");
		m_Metadata.RegisterType((u64)IRType::IR_u64, "u64");

		m_Metadata.RegisterType((u64)IRType::IR_bool, "bool");

		// 
		// 		{
		// 			std::vector<ArgumentMetadata> args;
		// 
		// 			ArgumentMetadata fmt_arg;
		// 
		// 			fmt_arg.Name = "format";
		// 			fmt_arg.Tipe.TT = TypeType::Pointer;
		// 			fmt_arg.Tipe.ID = (u64)IRType::IR_u8;
		// 
		// 			args.push_back(fmt_arg);
		// 
		// 			m_Metadata.RegisterFunction(99, "printf", (u64)IRType::IR_void, args, true);
		// 		}
	}

	IRTranslationUnit* Compiler::CodeGen()
	{
		IRTranslationUnit* tu = Application::AllocateIRNode(IRTranslationUnit());
		for (CompilerFile* file : m_Files) {
			ModuleFile* module_file = file->GetAST();
			for (const Statement* stmt : module_file->GetStatements()) {
				auto stmt_code = StatementCodeGen(stmt);
				if (stmt_code != nullptr) {
					tu->Instructions.push_back(stmt_code);
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
		default:
			return nullptr;
			break;
		}
	}

	IRInstruction* Compiler::ForeignCodeGen(const ForeignNode* frn)
	{
		FunctionNode* fn_decl = (FunctionNode*)frn->statement;

		NodeType tipe = frn->statement->GetType();

		if (tipe != NodeType::Function) {
			PushMessage(CompilerMessage{ PrintTokenLocation(frn->statement->GetLocation()),MessageType::Error });
			PushMessage(CompilerMessage{ fmt::format("#foreign directive only supports functions at the moment"),MessageType::Warning });
		}

		u64 ID = GetTypeID();

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

		return nullptr;
	}

	IRInstruction* Compiler::FunctionCodeGen(const FunctionNode* functionNode)
	{
		ResetSSAIDCounter();

		IRFunction* IRF = CreateIRFunction(functionNode);

		Type return_type;
		return_type.ID = (u64)IRType::IR_void;

		if (functionNode->ReturnType) {
			return_type.ID = m_Metadata.GetType(functionNode->ReturnType->Symbol.Symbol);
		}

		return_type.TT = functionNode->ReturnType->Pointer ? TypeType::Pointer : TypeType::Value;

		m_Metadata.RegisterFunction(IRF->ID, functionNode->Symbol.Symbol, return_type);

		std::vector<ArgumentMetadata> args_metadata;

		for (const auto a : functionNode->GetArgList()->GetArguments()) {
			IRSSA* arg_ssa = CreateIRSSA();

			VariableNode* var = (VariableNode*)a;

			RegisterVariable(arg_ssa, var->Symbol.Symbol);

			arg_ssa->Type = m_Metadata.GetType(var->Type->Symbol.Symbol);
			arg_ssa->Pointer = var->Type->Pointer;

			VariableMetadata var_metadata = {
			var->Symbol,
			Type
			{
				arg_ssa->Type,
				var->Type->Array,
				var->Type->Pointer ? TypeType::Pointer : TypeType::Value,
			},
			true
			};

			m_Metadata.RegisterVariableMetadata(arg_ssa->ID, var_metadata);


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

			if (dynamic_cast<IRWhile*>(code) != nullptr) {
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
			value = ExpressionCodeGen(variableNode->Assignment);
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
			address_of.SSA = Application::AllocateIRNode(StorageSSAVal);
		}

		IRssa->Value = Application::AllocateIRNode(address_of);
		IRssa->PointsTo = StorageSSA;

		RegisterVariable(IRssa, variableNode->Symbol.Symbol);

		VariableMetadata var_metadata = {
			variableNode->Symbol,
			Type
			{
				 m_Metadata.GetType(variableNode->Type->Symbol.Symbol),
				variableNode->Type->Array,
				variableNode->Type->Pointer ? TypeType::Pointer : TypeType::Value,
			}
		};

		m_Metadata.RegisterVariableMetadata(StorageSSA->ID, var_metadata);

		if (value != nullptr) {
			IRStore store;
			store.AddressSSA = IRssa->ID;
			store.Data = value;
			store.Type = StorageSSA->Type;

			return Application::AllocateIRNode(store);
		}
		else {
			return nullptr;
		}
	}

	IRInstruction* Compiler::ReturnCodeGen(const ReturnNode* returnNode)
	{
		IRReturn ret;

		ret.Type = (u64)IRType::IR_i32;

		ret.Value = GetExpressionByValue(returnNode->Expr);

		return Application::AllocateIRNode(ret);
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

		u64 type_id = GetStructID();
		u64 struct_id = GetTypeID();

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

			ir_struct.Members.push_back(Application::AllocateIRNode(ir_member));

			member_id_counter++;
		}

		return Application::AllocateIRNode(ir_struct);
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

		return Application::AllocateIRNode(IF);
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


		return Application::AllocateIRNode(WHILE);
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

		u64 ID = 0;
		ID = GetVariableSSA(identifier->Symbol.Symbol);

		IRSSA* ssa = GetSSA(ID);

		bool isArg = false;

		if (ssa->PointsTo == nullptr) {
			const auto& metadata = m_Metadata.GetVariableMetadata(ID);
			isArg = metadata->IsArg;
		}

		if (isArg == false) {

			IRLoad load;
			load.SSAddress = GetVariableSSA(identifier->Symbol.Symbol);

			ssa = CreateIRSSA();

			IRSSA* var_ssa = m_Metadata.GetSSA(load.SSAddress);

			load.Type = var_ssa->PointsTo->Type;

			ssa->Type = var_ssa->PointsTo->Type;
			ssa->Value = Application::AllocateIRNode(load);
			ssa->PointsTo = var_ssa->PointsTo;
			ssa->PointsToType = var_ssa->PointsTo->Type;

			IRSSAValue ssa_val;

			ssa_val.SSA = ssa->ID;

			return (IRInstruction*)Application::AllocateIRNode(ssa_val);
		}
		else {
			IRSSAValue ssa_val;
			ssa_val.SSA = ID;
			return (IRInstruction*)Application::AllocateIRNode(ssa_val);
		}
	}

	IRInstruction* Compiler::NumericLiteralCodeGen(const NumericLiteral* numericLiteral)
	{
		IRSSA* IRssa = CreateIRSSA();

		IRssa->Value = Application::AllocateIRNode(IRCONSTValue());

		IRCONSTValue* Value = (IRCONSTValue*)IRssa->Value;

		IRssa->Type = (u64)IRType::IR_i32;
		Value->Type = IRssa->Type;

		memcpy(&Value->Data, &numericLiteral->Value, sizeof(int));

		IRSSAValue* ssa_value = Application::AllocateIRNode(IRSSAValue());

		ssa_value->SSA = IRssa->ID;

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
		ir_ssa->Value = Application::AllocateIRNode(IRDataValue(0, data->ID));

		IRSSAValue ssa_val;

		ssa_val.SSA = ir_ssa->ID;

		return Application::AllocateIRNode(ssa_val);
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

			IRssa->Value = Application::AllocateIRNode(IROp);
		}
		break;
		case Operator::Subtract:
		{
			IRSUB IROp;

			IROp.SSA_A = A;
			IROp.SSA_B = B;

			IRssa->Value = Application::AllocateIRNode(IROp);
		}
		break;
		case Operator::Multiply:
		{
			IRMUL IROp;

			IROp.SSA_A = A;
			IROp.SSA_B = B;

			IRssa->Value = Application::AllocateIRNode(IROp);
		}
		break;
		case Operator::Divide:
		{
			IRDIV IROp;

			IROp.SSA_A = A;
			IROp.SSA_B = B;

			IRssa->Value = Application::AllocateIRNode(IROp);
		}
		break;
		break;
		default:
			return nullptr;
			break;
		}

		IRSSAValue* ssa_value = Application::AllocateIRNode(IRSSAValue());

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

			IRStore* store = Application::AllocateIRNode(IRStore());
			{
				store->Data = right_val;
				store->AddressSSA = var_ssa_id;
				store->Type = var_ssa->PointsTo->Type;
			}

			return store;
		}
		if (left->GetType() == NodeType::MemberAccess) {
			IRSSAValue* member_access = (IRSSAValue*)ExpressionCodeGen(left);
			IRSSAValue* right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			IRStore* store = Application::AllocateIRNode(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = member_access->SSA;
				store->Type = m_Metadata.GetSSA(member_access->SSA)->PointsToType;
			}

			return store;
		}
		if (left->GetType() == NodeType::ArrayAccess) {
			auto left_ssa = (IRSSAValue*)ArrayAccessCodeGen((ArrayAccess*)left);
			auto right_ssa = (IRSSAValue*)GetExpressionByValue(right);

			IRStore* store = Application::AllocateIRNode(IRStore());
			{
				store->Data = right_ssa;
				store->AddressSSA = left_ssa->SSA;
				store->Type = m_Metadata.GetSSA(right_ssa->SSA)->Type;
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
		const FunctionMetadata* metadata = m_Metadata.GetFunctionMetadata(IRF);

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

		IRFunctionCall ir_call;

		ir_call.FuncID = IRF;

		for (size_t i = 0; i < call->Arguments.size(); i++)
		{
			const ArgumentMetadata* decl_arg = metadata->GetArgument(i);

			IRInstruction* arg = nullptr;

			if (decl_arg != nullptr) {

				auto expr = ExpressionCodeGen(call->Arguments[i]);

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

				Compiler::Type type;

				if (arg_ssa->PointsTo != nullptr) {
					type = m_Metadata.GetVariableMetadata(arg_ssa->PointsTo->ID)->Tipe;
				}
				else {
					type.ID = arg_ssa->Type;
				}

				if (decl_arg->Tipe.ID != type.ID) {

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
					IRAsAddress ir_as_address;
					ir_as_address.SSA = arg_SSAID;

					auto address_ssa = CreateIRSSA();
					address_ssa->Type = decl_arg->Tipe.ID;
					address_ssa->Pointer = true;
					address_ssa->Value = Application::AllocateIRNode(ir_as_address);

					arg = Application::AllocateIRNode(IRSSAValue());
					((IRSSAValue*)arg)->SSA = address_ssa->ID;
				}
				else {
					arg = Application::AllocateIRNode(IRSSAValue());
					((IRSSAValue*)arg)->SSA = arg_SSAID;
				}
			}
			else {
				arg = GetExpressionByValue(call->Arguments[i]);
			}

			ir_call.Arguments.push_back(arg);
		}

		if (metadata->ReturnType.ID != (u64)IRType::IR_void)
		{
			auto ir_ssa = CreateIRSSA();

			ir_ssa->Type = metadata->ReturnType.ID;
			ir_ssa->Value = Application::AllocateIRNode(ir_call);
			ir_ssa->Pointer = metadata->ReturnType.TT == TypeType::Pointer;

			IRSSAValue* ir_sss_val = Application::AllocateIRNode(IRSSAValue());

			ir_sss_val->SSA = ir_ssa->ID;

			return ir_sss_val;
		}

		return Application::AllocateIRNode(ir_call);
	}

	IRInstruction* Compiler::MemberAccessCodeGen(const MemberAccess* memberAccess)
	{
		u64 struct_id = 0;
		u64 member_id = 0;
		u64 object_ssa_id = 0;

		const MemberMetadata* member_metadata = nullptr;
		const StructMetadata* struct_metadata = nullptr;

		{
			switch (memberAccess->Object->GetType()) {
			case NodeType::Identifier:
			{
				Identifier* object = (Identifier*)memberAccess->Object;
				u64 var_ssa = m_Metadata.GetVariableSSA(object->Symbol.Symbol);
				struct_id = m_Metadata.GetStructIDFromType(m_Metadata.GetSSA(var_ssa)->PointsTo->Type);
				struct_metadata = m_Metadata.GetStructMetadata(struct_id);
				object_ssa_id = var_ssa;
			}
			break;
			case NodeType::MemberAccess:
			{
				auto object_code = (IRSSAValue*)ExpressionCodeGen(memberAccess->Object);
				struct_id = m_Metadata.GetStructIDFromType(m_Metadata.GetSSA(object_code->SSA)->PointsToType);
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
			}
			break;
			case NodeType::MemberAccess:
			{
				MemberAccess* member = (MemberAccess*)memberAccess->Member;
				member_id = struct_metadata->FindMember(((Identifier*)member->Object)->Symbol.Symbol);
				member_metadata = &struct_metadata->Members[member_id];
			}
			break;
			}
		}

		IRMemberAccess ir_mem_access;

		{
			ir_mem_access.StructID = struct_id;
			ir_mem_access.ObjectSSA = object_ssa_id;
			ir_mem_access.MemberID = member_id;

			auto address_ssa = CreateIRSSA();

			address_ssa->Value = Application::AllocateIRNode(ir_mem_access);
			address_ssa->Type = IR_u64;
			address_ssa->PointsToType = member_metadata->Tipe.ID;

			IRSSAValue address_ssa_val;
			address_ssa_val.SSA = address_ssa->ID;

			return Application::AllocateIRNode(address_ssa_val);
		}
	}

	IRInstruction* Compiler::ArrayAccessCodeGen(const ArrayAccess* arrayAccess)
	{
		auto object = (IRSSAValue*)ExpressionCodeGen(arrayAccess->Object);
		auto index = (IRSSAValue*)ExpressionCodeGen(arrayAccess->Index);

		auto obj_ssa = m_Metadata.GetSSA(object->SSA);
		auto index_ssa = m_Metadata.GetSSA(index->SSA);

		u64 type = 0;
		u64 obj_address_ssa = 0;

		if (obj_ssa->PointsTo) {

			Identifier* identifier = (Identifier*)arrayAccess->Object;

			type = obj_ssa->PointsTo->Type;
			obj_address_ssa = m_Metadata.GetVariableSSA(identifier->Symbol.Symbol);

			if (obj_ssa->PointsTo->Pointer == false) {
				PushMessage(CompilerMessage{ PrintTokenLocation(arrayAccess->Object->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ "type of expression must be a pointer type in order to be accessed by the [] operator",MessageType::Warning });
			}
		}
		else {

			type = obj_ssa->Type;

			if (obj_ssa->Pointer == false) {
				PushMessage(CompilerMessage{ PrintTokenLocation(arrayAccess->Object->GetLocation()), MessageType::Error });
				PushMessage(CompilerMessage{ "type of expression must be a pointer type in order to be accessed by the [] operator",MessageType::Warning });
			}
		}

		auto sizeof_ssa = CreateIRSSA();

		sizeof_ssa->Value = Application::AllocateAstNode(IRSizeOF(type));
		sizeof_ssa->Type = IR_u64;

		auto ptr_mul_ssa = CreateIRSSA();

		ptr_mul_ssa->Value = Application::AllocateAstNode(IRADD(index, Application::AllocateIRNode(IRSSAValue(sizeof_ssa->ID))));
		ptr_mul_ssa->Type = IR_u64;

		auto ptr_add_ssa = CreateIRSSA();

		ptr_add_ssa->Value = Application::AllocateAstNode(IRADD(Application::AllocateAstNode(IRSSAValue(ptr_mul_ssa->ID)), Application::AllocateAstNode(IRSSAValue(obj_address_ssa))));
		ptr_add_ssa->Type = IR_u64;

		auto address_ssa = CreateIRSSA();

		address_ssa->Value = Application::AllocateAstNode(IRSSAValue(ptr_add_ssa->ID));
		address_ssa->Type = IR_u64;

		return Application::AllocateAstNode(IRSSAValue(address_ssa->ID));
	}

	IRSSAValue* Compiler::GetExpressionByValue(const Expression* expr)
	{
		switch (expr->GetType())
		{
		case NodeType::MemberAccess:
		{
			auto ir_address = (IRSSAValue*)MemberAccessCodeGen((MemberAccess*)expr);

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

			load.SSAddress = ir_address->SSA;
			load.Type = IR_u64;

			value_ssa->Value = Application::AllocateIRNode(load);
			value_ssa->Type = load.Type;
			value_ssa->Pointer = false;

			IRSSAValue* ssa_value = Application::AllocateIRNode(IRSSAValue());

			ssa_value->SSA = value_ssa->ID;

			return ssa_value;
		}
		break;
		case NodeType::ArrayAccess:
		{
			auto ir_address = (IRSSAValue*)ArrayAccessCodeGen((ArrayAccess*)expr);
			IRSSA* value_ssa = CreateIRSSA();

			IRLoad load;

			load.SSAddress = ir_address->SSA;
			load.Type = IR_u64;

			value_ssa->Value = Application::AllocateIRNode(load);
			value_ssa->Type = load.Type;
			value_ssa->Pointer = false;

			IRSSAValue* ssa_value = Application::AllocateIRNode(IRSSAValue());

			ssa_value->SSA = value_ssa->ID;

			return ssa_value;
		}
		break;
		default:
			return (IRSSAValue*)ExpressionCodeGen(expr);
			break;
		}
	}

	IRFunction* Compiler::CreateIRFunction(const FunctionNode* functionNode)
	{
		IRFunction* IRF = Application::AllocateIRNode(IRFunction());
		IRF->ID = m_FunctionIDCounter;

		m_FunctionIDCounter++;

		return IRF;
	}

	IRSSA* Compiler::CreateIRSSA()
	{
		IRSSA* SSA = Application::AllocateIRNode(IRSSA());
		SSA->ID = m_SSAIDCounter;

		m_SSAIDCounter++;

		PushIRSSA(SSA);

		return SSA;
	}

	IRData* Compiler::CreateIRData()
	{
		IRData* Data = Application::AllocateIRNode(IRData());
		Data->ID = m_DATAIDCounter;

		m_DATAIDCounter++;

		PushIRData(Data);

		return Data;
	}
}