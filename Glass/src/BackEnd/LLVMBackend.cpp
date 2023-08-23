#include "pch.h"

#include "BackEnd/LLVMBackend.h"


namespace Glass
{
	std::unique_ptr<llvm::LLVMContext> LLVMBackend::m_LLVMContext = std::make_unique<llvm::LLVMContext>();

	LLVMBackend::LLVMBackend(const Compiler::MetaData* metadata, IRTranslationUnit* program)
		:m_Metadata(metadata), m_Program(program)
	{
		m_LLVMModule = new llvm::Module("Glass", *m_LLVMContext);
		m_LLVMBuilder = std::make_unique<llvm::IRBuilder<>>(*m_LLVMContext);

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
	}

	void LLVMBackend::Compile()
	{
		EnumsCodegen();
		StructsCodeGen();
		ForeignCodeGen();

		//		printf();
// 		{
// 			std::vector<llvm::Type*> Parameters;
// 			Parameters.push_back(llvm::Type::getInt8PtrTy(*m_LLVMContext));
// 
// 			llvm::FunctionType* Function_Type =
// 				llvm::FunctionType::get(GetLLVMType(IR_int), Parameters, true);
// 
// 
// 			llvm::Function* llvm_Func =
// 				llvm::Function::Create(Function_Type, llvm::Function::InternalLinkage, "printfsdfsdf", m_LLVMModule);
// 		}

		for (auto inst : m_Program->Instructions) {
			CodeGen(inst);
		}

		m_LLVMModule->print(llvm::outs(), nullptr, true, true);

		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();

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

		auto Filename = "output.o";
		std::error_code EC;
		llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

		llvm::legacy::PassManager pass;
		auto FileType = llvm::CGFT_ObjectFile;

		if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
			return;
		}

		pass.run(*m_LLVMModule);
		dest.flush();

		std::string linker_command = "clang output.o -o a.exe";

		int link_status = system(linker_command.c_str());

		if (link_status) {
			GS_CORE_WARN("{}", linker_command);
			GS_CORE_ERROR("clang failed");
		}
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

			llvm_Func->setCallingConv(llvm::CallingConv::C);

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
			return OpCodeGen((IRBinOp*)instruction);
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
			Parameters.push_back(GetLLVMTypeFull(arg_metadata.Tipe));
		}

		llvm::FunctionType* Function_Type =
			llvm::FunctionType::get(GetLLVMTypeFull(func_metadata->ReturnType), Parameters, false);


		llvm::Function* llvm_Func =
			llvm::Function::Create(Function_Type, llvm::Function::ExternalLinkage, func_metadata->Name, m_LLVMModule);

		std::vector<llvm::Value*> argument_names;

		//BODY CODE GEN///////////////////////////

		llvm::BasicBlock* BB = llvm::BasicBlock::Create(*m_LLVMContext, "entry", llvm_Func);
		m_LLVMBuilder->SetInsertPoint(BB);

		for (auto& arg : llvm_Func->args()) {
			argument_names.push_back((llvm::Value*)&arg);
		}

		InsertFunctionArgNames(m_CurrentFunctionID, argument_names);
		InsertLLVMFunction(m_CurrentFunctionID, llvm_Func);

		//Argument Stack Storage
		u64 argument_id = 0;
		for (const ArgumentMetadata& arg_metadata : func_metadata->Arguments) {
			InsertName(arg_metadata.SSAID, m_LLVMBuilder->CreateAlloca(GetLLVMTypeFull(arg_metadata.Tipe)));
			m_LLVMBuilder->CreateStore(GetFunctionArgumentName(m_CurrentFunctionID, argument_id), GetName(arg_metadata.SSAID));
			argument_id++;
		}

		for (auto inst : func->Instructions) {
			CodeGen(inst);
		}

		llvm::verifyFunction(*llvm_Func, &llvm::errs());

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

		CodeGen(op->SSA_A);
		CodeGen(op->SSA_A);

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
		}

		return result;
	}

	llvm::Value* LLVMBackend::AllocaCodeGen(const IRAlloca* alloca)
	{
		llvm::AllocaInst* llvm_alloca =
			m_LLVMBuilder->CreateAlloca(GetLLVMTypeFull(alloca->Type, alloca->Pointer), nullptr);

		return llvm_alloca;
	}

	llvm::Value* LLVMBackend::LoadCodeGen(const IRLoad* load)
	{
		return m_LLVMBuilder->CreateLoad(GetLLVMTypeFull(load->Type, load->ReferencePointer), GetName(load->SSAddress));
	}

	llvm::Value* LLVMBackend::StoreCodeGen(const IRStore* store)
	{
		u64 data_ssa = 0;
		data_ssa = ((IRSSAValue*)store->Data)->SSA;
		return m_LLVMBuilder->CreateStore(GetName(data_ssa), GetName(store->AddressSSA));
	}

	llvm::Value* LLVMBackend::MemberAccessCodeGen(const IRMemberAccess* member_access)
	{
		LLVMStructType& struct_type = GetLLVMStructType(member_access->StructID);
		llvm::Type* llvm_Struct_Type = struct_type.LLVMType;
		GS_CORE_ASSERT(llvm_Struct_Type, "llvm_Struct_Type can't be null at this point");

		llvm::Value* llvm_Object = GetName(member_access->ObjectSSA);
		GS_CORE_ASSERT(llvm_Object, "llvm_Object can't be null at this point");

		GS_CORE_ASSERT(member_access->MemberID < struct_type.LLVMMembers.size(), "Member Idx out of range");

		return m_LLVMBuilder->CreateStructGEP(llvm_Struct_Type, llvm_Object, (unsigned)member_access->MemberID);
	}

	llvm::Value* LLVMBackend::ArrayAccessCodeGen(const IRArrayAccess* array_access)
	{
		return m_LLVMBuilder->CreateGEP(GetLLVMType(array_access->Type), GetName(array_access->ArrayAddress), GetName(array_access->ElementSSA));
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


		llvm::Value* condition =
			m_LLVMBuilder->CreateICmp(llvm::CmpInst::ICMP_UGT, GetName(_if->SSA),
				llvm::ConstantInt::get(GetLLVMType(IR_bool), 0));

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

		// Insert a branch to the loop condition block.
		m_LLVMBuilder->CreateBr(loopCondBlock);
		m_LLVMBuilder->SetInsertPoint(loopCondBlock);

		for (auto inst : _while->ConditionBlock) {
			CodeGen(inst);
		}

		llvm::Value* condition =
			m_LLVMBuilder->CreateICmp(llvm::CmpInst::ICMP_UGT, GetName(_while->SSA),
				llvm::ConstantInt::get(GetLLVMType(IR_bool), 0));

		m_LLVMBuilder->CreateCondBr(condition, loopBodyBlock, afterLoopBlock);

		m_LLVMBuilder->SetInsertPoint(loopBodyBlock);

		for (auto inst : _while->Instructions) {
			CodeGen(inst);
		}

		m_LLVMBuilder->CreateBr(loopCondBlock);

		m_LLVMBuilder->SetInsertPoint(afterLoopBlock);

		return nullptr;
	}

	llvm::AllocaInst* LLVMBackend::CreateEntryBlockAlloca(llvm::Function* function, llvm::StringRef var_name)
	{
		llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
			function->getEntryBlock().begin());
		return TmpB.CreateAlloca(llvm::Type::getDoubleTy(*m_LLVMContext), nullptr, var_name);
	}

	llvm::Value* LLVMBackend::ReturnCodeGen(const IRReturn* ret) {

		CodeGen(ret->Value);

		return m_LLVMBuilder->CreateRet(GetName(((IRSSAValue*)ret->Value)->SSA));
	}
}