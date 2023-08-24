#include "pch.h"

#include "BackEnd/LLVMBackend.h"


namespace Glass
{
	std::unique_ptr<llvm::LLVMContext> LLVMBackend::m_LLVMContext = std::make_unique<llvm::LLVMContext>();

	void LLVMBackend::InitDebug()
	{
		m_DBuilder = new llvm::DIBuilder(*m_LLVMModule);

		m_DCU = m_DBuilder->createCompileUnit(
			llvm::dwarf::DW_LANG_C, m_DBuilder->createFile("HelloWorld.glass", "./Examples/"),
			"Glass Compiler", false, "", 0);
	}

	void LLVMBackend::DumpDebugInfo()
	{
		m_DBuilder->finalize();
	}

	LLVMBackend::LLVMBackend(const Compiler::MetaData* metadata, IRTranslationUnit* program)
		:m_Metadata(metadata), m_Program(program)
	{
		m_LLVMModule = new llvm::Module("Glass", *m_LLVMContext);
		m_LLVMBuilder = std::make_unique<llvm::IRBuilder<>>(*m_LLVMContext);

		InitDebug();

		InsertLLVMType(IR_int, llvm::Type::getInt32Ty(*m_LLVMContext));

		InsertLLVMType(IR_i8, llvm::Type::getInt8Ty(*m_LLVMContext));
		InsertLLVMType(IR_i16, llvm::Type::getInt16Ty(*m_LLVMContext));
		InsertLLVMType(IR_i32, llvm::Type::getInt32Ty(*m_LLVMContext));
		InsertLLVMType(IR_i64, llvm::Type::getInt64Ty(*m_LLVMContext));

		InsertLLVMType(IR_u8, llvm::Type::getInt8Ty(*m_LLVMContext));
		InsertLLVMType(IR_u16, llvm::Type::getInt16Ty(*m_LLVMContext));
		InsertLLVMType(IR_u32, llvm::Type::getInt32Ty(*m_LLVMContext));
		InsertLLVMType(IR_u64, llvm::Type::getInt64Ty(*m_LLVMContext));

		InsertLLVMType(IR_float, llvm::Type::getFloatTy(*m_LLVMContext));

		InsertLLVMType(IR_f32, llvm::Type::getFloatTy(*m_LLVMContext));
		InsertLLVMType(IR_f64, llvm::Type::getDoubleTy(*m_LLVMContext));

		InsertLLVMType(IR_bool, llvm::Type::getInt8Ty(*m_LLVMContext));

		InsertLLVMType(IR_void, llvm::Type::getVoidTy(*m_LLVMContext));

		//@Debugging
		{
			InsertLLVMDebugType(IR_u8, m_DBuilder->createBasicType("u8", 8, llvm::dwarf::DW_ATE_unsigned));
			InsertLLVMDebugType(IR_u16, m_DBuilder->createBasicType("u16", 16, llvm::dwarf::DW_ATE_unsigned));
			InsertLLVMDebugType(IR_u32, m_DBuilder->createBasicType("u32", 32, llvm::dwarf::DW_ATE_unsigned));
			InsertLLVMDebugType(IR_u64, m_DBuilder->createBasicType("u64", 64, llvm::dwarf::DW_ATE_unsigned));

			InsertLLVMDebugType(IR_i8, m_DBuilder->createBasicType("i8", 8, llvm::dwarf::DW_ATE_signed));
			InsertLLVMDebugType(IR_i16, m_DBuilder->createBasicType("i16", 16, llvm::dwarf::DW_ATE_signed));
			InsertLLVMDebugType(IR_i32, m_DBuilder->createBasicType("i32", 32, llvm::dwarf::DW_ATE_signed));
			InsertLLVMDebugType(IR_i64, m_DBuilder->createBasicType("i64", 64, llvm::dwarf::DW_ATE_signed));

			InsertLLVMDebugType(IR_void, m_DBuilder->createUnspecifiedType("void"));

			//Array type
			{
				std::vector<llvm::Metadata*> fieldTypes;

				Glass::Type count_field_Type;
				count_field_Type.ID = IR_u64;

				Glass::Type data_field_Type;
				data_field_Type.ID = IR_void;
				data_field_Type.Pointer = 1;

				fieldTypes.push_back(GetLLVMDebugType(count_field_Type));
				fieldTypes.push_back(GetLLVMDebugType(data_field_Type));

				llvm::DICompositeType* array_Debug_Type = m_DBuilder->createStructType(
					m_DCU,                    // Scope
					"Array",					// Name
					nullptr,                    // File
					0,                          // Line number
					128,					    // Size in bits
					32,                          // Alignment in bits
					llvm::DINode::FlagZero,     // Flags
					nullptr,                    // Derived from
					m_DBuilder->getOrCreateArray(fieldTypes) // Elements
				);

				InsertLLVMDebugType(IR_array, array_Debug_Type);
			}
			//Any type
			{
				std::vector<llvm::Metadata*> fieldTypes;

				Glass::Type type_field_Type;
				type_field_Type.ID = IR_u64;

				Glass::Type data_field_Type;
				data_field_Type.ID = IR_void;
				data_field_Type.Pointer = 1;

				fieldTypes.push_back(GetLLVMDebugType(type_field_Type));
				fieldTypes.push_back(GetLLVMDebugType(data_field_Type));

				llvm::DICompositeType* any_Debug_Type = m_DBuilder->createStructType(
					m_DCU,							// Scope
					"Any",							// Name
					nullptr,						// File
					0,								// Line number
					128,							// Size in bits
					32,								// Alignment in bits
					llvm::DINode::FlagZero,			// Flags
					nullptr,						// Derived from
					m_DBuilder->getOrCreateArray(fieldTypes) // Elements
				);

				InsertLLVMDebugType(IR_any, any_Debug_Type);
			}
		}

		Opaque_Type = llvm::StructType::create(*m_LLVMContext, "ptr");
	}

	void LLVMBackend::Compile()
	{
		// Add the current debug info version into the module.
		m_LLVMModule->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
			llvm::DEBUG_METADATA_VERSION);

		EnumsCodegen();
		StructsCodeGen();
		ForeignCodeGen();

		for (auto inst : m_Program->Instructions) {
			CodeGen(inst);
		}

		// Create a file stream for output
		std::error_code ELC;
		llvm::raw_fd_ostream outputFile("output.ll", ELC, llvm::sys::fs::OF_None);

		m_LLVMModule->print(outputFile, nullptr);

		//m_LLVMModule->print(llvm::outs(), nullptr, true, true);

		llvm::InitializeAllTargetInfos();
		llvm::InitializeAllTargets();
		llvm::InitializeAllTargetMCs();
		llvm::InitializeAllAsmParsers();
		llvm::InitializeAllAsmPrinters();

		DumpDebugInfo();

		auto TargetTriple = llvm::sys::getDefaultTargetTriple();
		m_LLVMModule->setTargetTriple(TargetTriple);

		std::string Error;
		auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

		auto CPU = "generic";
		auto Features = "";

		llvm::TargetOptions opt;
		auto RM = llvm::Optional<llvm::Reloc::Model>();
		auto TheTargetMachine =
			Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

		m_LLVMModule->setDataLayout(TheTargetMachine->createDataLayout());

		auto Filename = "output.obj";
		std::error_code EC;
		llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

		llvm::legacy::PassManager pass;
		auto FileType = llvm::CGFT_ObjectFile;

		if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
			return;
		}

		pass.run(*m_LLVMModule);
		dest.flush();

		delete m_LLVMModule;
	}

	void LLVMBackend::EnumsCodegen()
	{

		for (const auto& enum_pair : m_Metadata->m_Enums) {

			u64 id = enum_pair.first;

			const EnumMetadata& enum_metadata = enum_pair.second;

			//@Speed store enum type id in the enum metadata struct
			//@Compat this will fail when we add modules

			InsertLLVMType(m_Metadata->GetType(enum_metadata.Name.Symbol), GetLLVMType(IR_u64));
		}
	}

	void LLVMBackend::StructsCodeGen()
	{
		for (const auto& struct_pair : m_Metadata->m_StructMetadata) {

			u64 id = struct_pair.first;
			const StructMetadata& struct_metadata = struct_pair.second;

			llvm::StructType* llvm_StructType = llvm::StructType::create(*m_LLVMContext, struct_metadata.Name.Symbol);
			GS_CORE_ASSERT(llvm_StructType, "llvm struct type can't be null at this point");

			LLVMStructType struct_type;

			struct_type.LLVMType = llvm_StructType;
			struct_type.StructID = id;
			struct_type.TypeID = struct_metadata.TypeID;

			InsertLLVMStructType(struct_type.StructID, struct_type);
		}

		llvm::DataLayout dataLayout(m_LLVMModule);

		for (const auto& func_pair : m_Metadata->m_StructMetadata) {

			u64 id = func_pair.first;
			const StructMetadata& struct_metadata = func_pair.second;

			LLVMStructType& our_struct_type = GetLLVMStructType(id);

			std::vector<llvm::Type*> llvm_Members_Types;

			llvm_Members_Types.reserve(struct_metadata.Members.size());

			for (const MemberMetadata& member_metadata : struct_metadata.Members) {
				llvm_Members_Types.push_back(GetLLVMTypeFull(member_metadata.Tipe));
			}

			our_struct_type.LLVMType->setBody(llvm_Members_Types);
			our_struct_type.LLVMMembers = llvm_Members_Types;

			//@Todo add cmd option for this
			if (0)
			{
				GS_CORE_WARN("Struct: {}", struct_metadata.Name.Symbol);
				const llvm::StructLayout* structLayout = dataLayout.getStructLayout(our_struct_type.LLVMType);
				u64 i = 0;
				for (const MemberMetadata& member_metadata : struct_metadata.Members) {
					u64 elementOffset = structLayout->getElementOffset(i);

					GS_CORE_WARN("Member Offset: {}", elementOffset);

					i = i + 1;
				}
			}
		}

	}

	void LLVMBackend::ForeignCodeGen()
	{
		std::vector<u64> foreign_functions;

		for (const auto& func_pair : m_Metadata->m_Functions) {

			u64 id = func_pair.first;
			const FunctionMetadata& function_metadata = func_pair.second;

			if (function_metadata.Foreign) {
				foreign_functions.push_back(id);
			}
		}


		for (u64 foreign_func_id : foreign_functions) {
			const FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(foreign_func_id);

			std::vector<llvm::Type*> Parameters;

			for (const ArgumentMetadata& arg_metadata : metadata->Arguments) {
				Parameters.push_back(GetLLVMTypeFull(arg_metadata.Tipe));
			}

			llvm::FunctionType* Function_Type =
				llvm::FunctionType::get(GetLLVMTypeFull(metadata->ReturnType), Parameters, metadata->Variadic);

			llvm::Function* llvm_Func =
				llvm::Function::Create(
					Function_Type, llvm::Function::LinkageTypes::ExternalLinkage,
					metadata->Name, m_LLVMModule);

			//llvm_Func->setCallingConv(llvm::CallingConv::C);

			InsertLLVMFunction(foreign_func_id, llvm_Func);
		}
	}

	llvm::Value* LLVMBackend::CodeGen(const IRInstruction* instruction)
	{
		IRNodeType Type = instruction->GetType();

		switch (Type) {
		case IRNodeType::Function:
		{
			llvm::Function* func = (llvm::Function*)FunctionCodeGen((IRFunction*)instruction);
			SetFunctionID(0);
			return 0;
			break;
		}
		case IRNodeType::ConstValue: return ConstValueCodeGen((IRCONSTValue*)instruction);
		case IRNodeType::Data: return DataCodeGen((IRData*)instruction);
		case IRNodeType::DataValue: return DataValueCodeGen((IRDataValue*)instruction);

		case IRNodeType::Return: return ReturnCodeGen((IRReturn*)instruction);
		case IRNodeType::SSA: return SSACodeGen((IRSSA*)instruction);
		case IRNodeType::SSAValue: return SSAValueCodeGen((IRSSAValue*)instruction);
		case IRNodeType::Alloca: return AllocaCodeGen((IRAlloca*)instruction);

		case IRNodeType::Load: return LoadCodeGen((IRLoad*)instruction);
		case IRNodeType::Store: return StoreCodeGen((IRStore*)instruction);

		case IRNodeType::MemberAccess: return MemberAccessCodeGen((IRMemberAccess*)instruction);
		case IRNodeType::ArrayAccess: return ArrayAccessCodeGen((IRArrayAccess*)instruction);

		case IRNodeType::Call: return CallCodeGen((IRFunctionCall*)instruction);

		case IRNodeType::SizeOf: return SizeOfCodeGen((IRSizeOF*)instruction);

		case IRNodeType::PointerCast: return PointerCastCodeGen((IRPointerCast*)instruction);

		case IRNodeType::If: return IfCodeGen((IRIf*)instruction);
		case IRNodeType::While: return WhileCodeGen((IRWhile*)instruction);

		case IRNodeType::ADD:
		case IRNodeType::SUB:
		case IRNodeType::MUL:
		case IRNodeType::DIV:
		case IRNodeType::Equal:
		case IRNodeType::NotEqual:
		case IRNodeType::LesserThan:
		case IRNodeType::GreaterThan:
			return OpCodeGen((IRBinOp*)instruction);
			break;

		case IRNodeType::Any: return AnyCodeGen((IRAny*)instruction);
		case IRNodeType::AnyArray: return AnyArrayCodeGen((IRAnyArray*)instruction);
		default:
			return 0;
		}
	}

	llvm::Value* LLVMBackend::FunctionCodeGen(const IRFunction* func)
	{
		SetFunctionID(func->ID);

		const FunctionMetadata* func_metadata = m_Metadata->GetFunctionMetadata(func->ID);

		std::vector<llvm::Type*> Parameters;

		for (const ArgumentMetadata& arg_metadata : func_metadata->Arguments) {
			if (!arg_metadata.Variadic) {
				Parameters.push_back(GetLLVMTypeFull(arg_metadata.Tipe));
			}
			else {
				Parameters.push_back(GetLLVMType(IR_array));
			}
		}

		llvm::FunctionType* Function_Type =
			llvm::FunctionType::get(GetLLVMTypeFull(func_metadata->ReturnType), Parameters, false);

		llvm::Function* llvm_Func =
			llvm::Function::Create(Function_Type, llvm::Function::ExternalLinkage, func_metadata->Name, m_LLVMModule);

		//BODY CODE GEN///////////////////////////

		llvm::BasicBlock* BB = llvm::BasicBlock::Create(*m_LLVMContext, "entry", llvm_Func);
		m_LLVMBuilder->SetInsertPoint(BB);

		//@Debuggging
		FunctionDebugInfo(func->ID, llvm_Func);
		m_LLVMBuilder->SetCurrentDebugLocation(llvm::DebugLoc());

		std::vector<llvm::Value*> argument_names;
		for (auto& arg : llvm_Func->args()) {
			argument_names.push_back((llvm::Value*)&arg);
		}

		InsertFunctionArgNames(m_CurrentFunctionID, argument_names);
		InsertLLVMFunction(m_CurrentFunctionID, llvm_Func);

		//Argument Stack Storage
		u64 argument_id = 0;
		for (const ArgumentMetadata& arg_metadata : func_metadata->Arguments) {

			llvm::AllocaInst* argument_Alloca = nullptr;

			if (!arg_metadata.Variadic) {

				argument_Alloca = m_LLVMBuilder->CreateAlloca(GetLLVMTypeFull(arg_metadata.Tipe));

			}
			else {
				argument_Alloca = m_LLVMBuilder->CreateAlloca(GetLLVMType(IR_array));
			}

			InsertName(arg_metadata.SSAID, argument_Alloca);

			//@Debugging
			{
				u32 line_number = (u32)func_metadata->Symbol.Line;

				llvm::DIType* llvm_DType = nullptr;

				llvm::DILocalVariable* D = m_DBuilder->createParameterVariable(
					m_DLexicalBlocks.back(),
					arg_metadata.Name,
					(u32)argument_id,
					(llvm::DIFile*)mDContext,
					line_number,
					GetLLVMDebugType(arg_metadata.Tipe),
					true);

				m_DBuilder->insertDeclare(argument_Alloca, D, m_DBuilder->createExpression(),
					llvm::DILocation::get(m_DLexicalBlocks.back()->getContext(), line_number, 0, m_DLexicalBlocks.back()),
					m_LLVMBuilder->GetInsertBlock());
			}

			m_LLVMBuilder->CreateStore(GetFunctionArgumentName(m_CurrentFunctionID, argument_id), GetName(arg_metadata.SSAID));
			argument_id++;
		}

		for (auto inst : func->Instructions) {
			CodeGen(inst);
		}

		FinalizeFunctionDebugInfo(llvm_Func);

		llvm::verifyFunction(*llvm_Func, &llvm::errs());

		PopDBGLexicalBlock();

		return llvm_Func;
	}

	llvm::Value* LLVMBackend::SSACodeGen(const IRSSA* ssa) {
		if (ssa->Value) {

			if (ssa->Value->GetType() == IRNodeType::AddressOf) {

				IRAddressOf* address_of = (IRAddressOf*)ssa->Value;

				if (address_of->SSA->GetType() == IRNodeType::ARGValue) {
					IRARGValue* arg_value = (IRARGValue*)address_of->SSA;

					return 0;
				}
			}

			auto value = CodeGen(ssa->Value);

			//@Debugging
			SetDBGLocation(ssa->GetDBGLoc());

			if (value) {
				InsertName(ssa->ID, value);
			}
		}

		return 0;
	}

	llvm::Value* LLVMBackend::SSAValueCodeGen(const IRSSAValue* ssa_value) {
		return GetName(ssa_value->SSA);
	}

	llvm::Value* LLVMBackend::ConstValueCodeGen(const IRCONSTValue* constant)
	{
		if (m_Metadata->GetTypeFlags(constant->Type) & FLAG_FLOATING_TYPE) {

			double data = 0;
			memcpy(&data, &constant->Data, sizeof(double));

			llvm::Type* floatType = nullptr;

			if (constant->Type == IR_float || constant->Type == IR_f32) {
				floatType = llvm::Type::getFloatTy(*m_LLVMContext);
			}
			else if (constant->Type == IR_f64) {
				floatType = llvm::Type::getDoubleTy(*m_LLVMContext);
			}

			return llvm::ConstantFP::get(floatType, data);
		}

		i64 data = 0;

		memcpy(&data, &constant->Data, sizeof(i64));

		bool is_unsigned = m_Metadata->GetTypeFlags(constant->Type) & FLAG_UNSIGNED_TYPE;

		if (is_unsigned) {
			return llvm::ConstantInt::get(GetLLVMType(constant->Type), data, false);
		}
		else {
			return llvm::ConstantInt::getSigned(GetLLVMType(constant->Type), data);
		}
	}

	llvm::Value* LLVMBackend::OpCodeGen(const IRBinOp* op)
	{
		IRNodeType type = op->GetType();

		//CodeGen(op->SSA_A);
		//CodeGen(op->SSA_B);

		llvm::Value* lhs = GetName(op->SSA_A->SSA);
		llvm::Value* rhs = GetName(op->SSA_B->SSA);

		llvm::Value* result = nullptr;

		switch (type)
		{
		case IRNodeType::ADD:
			result = m_LLVMBuilder->CreateAdd(lhs, rhs, "addition");
			break;
		case IRNodeType::SUB:
			result = m_LLVMBuilder->CreateSub(lhs, rhs, "subtract");
			break;
		case IRNodeType::MUL:
			result = m_LLVMBuilder->CreateMul(lhs, rhs, "multiply");
			break;
		case IRNodeType::DIV:
			result = m_LLVMBuilder->CreateSDiv(lhs, rhs, "signed div");
			break;

		case IRNodeType::Equal:
			result = m_LLVMBuilder->CreateICmpEQ(lhs, rhs, "comp");
			break;

		case IRNodeType::NotEqual:
			result = m_LLVMBuilder->CreateICmpNE(lhs, rhs, "comp");
			break;

		case IRNodeType::LesserThan:
			result = m_LLVMBuilder->CreateICmpULT(lhs, rhs);
			break;

		case IRNodeType::GreaterThan:
			result = m_LLVMBuilder->CreateICmpUGT(lhs, rhs);
			break;
		}

		return result;
	}

	llvm::Value* LLVMBackend::AllocaCodeGen(const IRAlloca* aloca)
	{
		llvm::AllocaInst* llvm_alloca =
			CreateEntryBlockAlloca(GetLLVMTypeFull(aloca->Type, aloca->Pointer));

		//@Debugging
		if (aloca->VarMetadata)
		{
			const VariableMetadata* var_metadata = aloca->VarMetadata;

			u32 line_number = (u32)var_metadata->Name.Line;

			llvm::DILocalVariable* D = m_DBuilder->createAutoVariable(
				m_DLexicalBlocks.back(),
				var_metadata->Name.Symbol,
				(llvm::DIFile*)mDContext,
				line_number,
				GetLLVMDebugType(var_metadata->Tipe),
				true);

			m_DBuilder->insertDeclare(llvm_alloca, D, m_DBuilder->createExpression(),
				llvm::DILocation::get(m_DLexicalBlocks.back()->getContext(), line_number, 0, m_DLexicalBlocks.back()),
				m_LLVMBuilder->GetInsertBlock());
		}

		return llvm_alloca;
	}

	llvm::Value* LLVMBackend::LoadCodeGen(const IRLoad* load)
	{
		auto ld = m_LLVMBuilder->CreateLoad(GetLLVMTypeFull(load->Type, load->ReferencePointer), GetName(load->SSAddress));
		return ld;
	}

	llvm::Value* LLVMBackend::StoreCodeGen(const IRStore* store)
	{
		u64 data_ssa = 0;
		data_ssa = ((IRSSAValue*)store->Data)->SSA;
		auto st = m_LLVMBuilder->CreateStore(GetName(data_ssa), GetName(store->AddressSSA));
		return st;
	}

	llvm::Value* LLVMBackend::MemberAccessCodeGen(const IRMemberAccess* member_access)
	{
		LLVMStructType& struct_type = GetLLVMStructType(member_access->StructID);
		llvm::Type* llvm_Struct_Type = struct_type.LLVMType;
		GS_CORE_ASSERT(llvm_Struct_Type, "llvm_Struct_Type can't be null at this point");

		llvm::Value* llvm_Object = GetName(member_access->ObjectSSA);
		GS_CORE_ASSERT(llvm_Object, "llvm_Object can't be null at this point");

		GS_CORE_ASSERT(member_access->MemberID < struct_type.LLVMMembers.size(), "Member Idx out of range");

		if (member_access->ReferenceAccess) {
			llvm_Object = m_LLVMBuilder->CreateLoad(llvm_Object);
		}

		return m_LLVMBuilder->CreateStructGEP(llvm_Struct_Type, llvm_Object, (unsigned)member_access->MemberID);
	}

	llvm::Value* LLVMBackend::ArrayAccessCodeGen(const IRArrayAccess* array_access)
	{
		return m_LLVMBuilder->CreateGEP(
			GetLLVMType(array_access->Type),
			GetName(array_access->ArrayAddress),
			GetName(array_access->ElementSSA),
			"array_access"
		);
	}

	llvm::Value* LLVMBackend::CallCodeGen(const IRFunctionCall* call)
	{
		llvm::Function* llvm_Func = GetLLVMFunction(call->FuncID);
		GS_CORE_ASSERT(llvm_Func, "LLVM Function Must Exist At This Point");

		std::vector<llvm::Value*> llvm_Arguments;

		for (IRInstruction* arg : call->Arguments) {
			GS_CORE_ASSERT(arg->GetType() == IRNodeType::SSAValue, "LLVM Function Must Exist At This Point");

			IRSSAValue* as_ssa_value = (IRSSAValue*)arg;

			llvm_Arguments.push_back(GetName(as_ssa_value->SSA));
		}

		return m_LLVMBuilder->CreateCall(llvm_Func, llvm_Arguments);
	}

	llvm::Value* LLVMBackend::DataValueCodeGen(const IRDataValue* data_value)
	{
		return GetLLVMData(data_value->DataID);
	}

	llvm::Value* LLVMBackend::DataCodeGen(const IRData* data)
	{
		std::string string_data;

		string_data.reserve(data->Data.size());

		u64 i = 0;
		while (i < data->Data.size()) {
			char c = data->Data[i];

			if (c == '\\') {
				if (data->Data[i + 1] == 'n') {

					string_data.push_back('\x0A');

					i += 2;
					continue;
				}
			}

			string_data.push_back(c);

			i++;
		}

		InsertLLVMData(data->ID, m_LLVMBuilder->CreateGlobalStringPtr(string_data, "", 0, m_LLVMModule));

		return nullptr;
	}

	llvm::Value* LLVMBackend::SizeOfCodeGen(const IRSizeOF* size_of)
	{
		// we are using our calculated size for types
		// we do not account for padding
		//I dont know how to handle sizeof in llvm depending on platform and arch the size will certainly change
		return llvm::ConstantInt::get(GetLLVMType(IR_i64), m_Metadata->GetTypeSize(size_of->Type));
	}

	llvm::Value* LLVMBackend::PointerCastCodeGen(const IRPointerCast* ptr_cast)
	{
		return m_LLVMBuilder->CreateBitCast(GetName(ptr_cast->PointerSSA), GetLLVMTypeFull(ptr_cast->Type, ptr_cast->Pointer));
	}

	llvm::Value* LLVMBackend::IfCodeGen(const IRIf* _if)
	{
		llvm::Function* function = m_LLVMBuilder->GetInsertBlock()->getParent();

		llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*m_LLVMContext, "then", function);
		llvm::BasicBlock* contBlock = llvm::BasicBlock::Create(*m_LLVMContext, "cont", function);

		// 		llvm::Value* condition =
		// 			m_LLVMBuilder->CreateICmp(llvm::CmpInst::ICMP_UGT, GetName(_if->SSA),
		// 				llvm::ConstantInt::get(GetLLVMType(IR_bool), 0));

		llvm::Value* condition = GetName(_if->SSA);

		m_LLVMBuilder->CreateCondBr(condition, thenBlock, contBlock);
		m_LLVMBuilder->SetInsertPoint(thenBlock);

		for (auto inst : _if->Instructions) {
			CodeGen(inst);
		}

		m_LLVMBuilder->CreateBr(contBlock);
		m_LLVMBuilder->SetInsertPoint(contBlock);

		return nullptr;
	}

	llvm::Value* LLVMBackend::WhileCodeGen(const IRWhile* _while)
	{
		llvm::Function* function = m_LLVMBuilder->GetInsertBlock()->getParent();

		llvm::BasicBlock* loopCondBlock = llvm::BasicBlock::Create(*m_LLVMContext, "loop.cond", function);
		llvm::BasicBlock* loopBodyBlock = llvm::BasicBlock::Create(*m_LLVMContext, "loop.body", function);
		llvm::BasicBlock* afterLoopBlock = llvm::BasicBlock::Create(*m_LLVMContext, "after.loop", function);

		m_LLVMBuilder->CreateBr(loopCondBlock);
		m_LLVMBuilder->SetInsertPoint(loopCondBlock);

		for (auto inst : _while->ConditionBlock) {
			CodeGen(inst);
		}

		llvm::Value* condition = GetName(_while->SSA);

		m_LLVMBuilder->CreateCondBr(condition, loopBodyBlock, afterLoopBlock);

		m_LLVMBuilder->SetInsertPoint(loopBodyBlock);

		for (auto inst : _while->Instructions) {
			CodeGen(inst);
		}

		m_LLVMBuilder->CreateBr(loopCondBlock);

		m_LLVMBuilder->SetInsertPoint(afterLoopBlock);

		return nullptr;
	}

	llvm::Value* LLVMBackend::AnyCodeGen(const IRAny* any)
	{
		llvm::Type* llvm_AnyStructTy = GetLLVMStructType(m_Metadata->GetStructIDFromType(IR_any)).LLVMType;
		GS_CORE_ASSERT(llvm_AnyStructTy);

		llvm::Value* any_Data = m_LLVMBuilder->CreateBitCast(GetName(any->DataSSA), GetLLVMTypeFull(IR_void, 1));

		llvm::Value* llvm_Struct = CreateEntryBlockAlloca(llvm_AnyStructTy);

		llvm::Value* llvm_any_type_ptr = m_LLVMBuilder->CreateStructGEP(
			llvm_AnyStructTy,
			llvm_Struct,
			0);

		llvm::Value* llvm_any_data_ptr = m_LLVMBuilder->CreateStructGEP(
			llvm_AnyStructTy,
			llvm_Struct,
			1);

		m_LLVMBuilder->CreateStore(any_Data, llvm_any_data_ptr);
		m_LLVMBuilder->CreateStore(llvm::ConstantInt::get(GetLLVMType(IR_i64), any->TypeID), llvm_any_type_ptr);

		return llvm_Struct;
	}

	llvm::Value* LLVMBackend::AnyArrayCodeGen(const IRAnyArray* any_array)
	{
		llvm::Type* llvm_AnyStructTy = GetLLVMStructType(m_Metadata->GetStructIDFromType(IR_any)).LLVMType;
		GS_CORE_ASSERT(llvm_AnyStructTy);

		llvm::Type* llvm_ArrayStructTy = GetLLVMStructType(m_Metadata->GetStructIDFromType(IR_array)).LLVMType;
		GS_CORE_ASSERT(llvm_ArrayStructTy);

		llvm::Value* llvm_AnyArray = m_LLVMBuilder->CreateAlloca(
			llvm_AnyStructTy,
			llvm::ConstantInt::get(GetLLVMType(IR_u64),
				any_array->Arguments.size()),
			"any_varargs");

		u64 i = 0;
		for (auto& arg : any_array->Arguments) {

			llvm::Value* llvm_AnyArrayElemPtr =
				m_LLVMBuilder->CreateGEP(
					llvm_AnyStructTy,
					llvm_AnyArray,
					llvm::ConstantInt::get(GetLLVMType(IR_u64), i)
					, "varargs_getelem");

			llvm::Value* elem = m_LLVMBuilder->CreateLoad(llvm_AnyStructTy, AnyCodeGen(&arg));

			llvm::Value* llvm_ArrayElemPtr =
				m_LLVMBuilder->CreateBitCast(llvm_AnyArrayElemPtr, GetLLVMTypeFull(IR_any, 1));

			m_LLVMBuilder->CreateStore(elem, llvm_ArrayElemPtr);

			i++;
		}

		llvm::Value* llvm_ArrayStruct = CreateEntryBlockAlloca(llvm_ArrayStructTy);


		llvm::Value* llvm_array_count_ptr = m_LLVMBuilder->CreateStructGEP(
			llvm_ArrayStructTy,
			llvm_ArrayStruct,
			0);

		llvm::Value* llvm_array_data_ptr = m_LLVMBuilder->CreateStructGEP(
			llvm_ArrayStructTy,
			llvm_ArrayStruct,
			1);

		llvm::Value* llvm_AnyArrayFirstElemPtr =
			m_LLVMBuilder->CreateGEP(
				llvm_AnyStructTy,
				llvm_AnyArray,
				llvm::ConstantInt::get(GetLLVMType(IR_u64), 0)
				, "varargs first");

		llvm::Value* llvm_ArrayStructData =
			m_LLVMBuilder->CreateBitCast(llvm_AnyArrayFirstElemPtr, GetLLVMTypeFull(IR_void, 1));

		// 		llvm::Value* llvm_ArrayStructData =
		// 			m_LLVMBuilder->CreateBitCast(llvm_AnyArray, GetLLVMTypeFull(IR_void, 1));
		// 		//it gave me a pointer to an element to the whole array

		m_LLVMBuilder->CreateStore(llvm_ArrayStructData, llvm_array_data_ptr);
		m_LLVMBuilder->CreateStore(llvm::ConstantInt::get(GetLLVMType(IR_i64), any_array->Arguments.size()), llvm_array_count_ptr);

		return m_LLVMBuilder->CreateLoad(llvm_ArrayStructTy, llvm_ArrayStruct);
	}

	llvm::AllocaInst* LLVMBackend::CreateEntryBlockAlloca(llvm::Type* type)
	{
		llvm::Function* function = m_LLVMBuilder->GetInsertBlock()->getParent();

		llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
			function->getEntryBlock().begin());
		auto alloca = TmpB.CreateAlloca(type);
		return alloca;
	}

	llvm::Value* LLVMBackend::ReturnCodeGen(const IRReturn* ret) {

		CodeGen(ret->Value);

		return m_LLVMBuilder->CreateRet(GetName(((IRSSAValue*)ret->Value)->SSA));
	}
}