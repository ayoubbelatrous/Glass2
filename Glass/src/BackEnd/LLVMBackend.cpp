#include "pch.h"

#include "BackEnd/LLVMBackend.h"

namespace Glass
{
	std::unique_ptr<llvm::LLVMContext> LLVMBackend::m_LLVMContext = std::make_unique<llvm::LLVMContext>();

	void LLVMBackend::GenerateObjFile()
	{
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

		if (!Error.empty()) {
			GS_CORE_ERROR("llvm error: Target Error {}", Error);
		}

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
	}

	void LLVMBackend::InitDebug()
	{
		m_DBuilder = new llvm::DIBuilder(*m_LLVMModule);

		m_DCU = m_DBuilder->createCompileUnit(
			llvm::dwarf::DW_LANG_C, m_DBuilder->createFile("Main.glass", "."),
			"Glass Compiler", false, "", 0);

		//m_LLVMModule->addModuleFlag(llvm::Module::Warning, "CodeView", 8);
	}

	void LLVMBackend::DumpDebugInfo()
	{
		m_DBuilder->finalize();
	}

	LLVMBackend::LLVMBackend(const MetaData* metadata, IRTranslationUnit* program)
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

		InsertLLVMType(IR_type, llvm::Type::getInt64Ty(*m_LLVMContext));

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

			InsertLLVMDebugType(IR_f32, m_DBuilder->createBasicType("f32", 32, llvm::dwarf::DW_ATE_float));
			InsertLLVMDebugType(IR_f64, m_DBuilder->createBasicType("f64", 64, llvm::dwarf::DW_ATE_float));

			InsertLLVMDebugType(IR_void, m_DBuilder->createUnspecifiedType("void"));

			InsertLLVMDebugType(IR_type, m_DBuilder->createBasicType("Type", 64, llvm::dwarf::DW_ATE_unsigned));

			InsertLLVMDebugType(IR_bool, m_DBuilder->createBasicType("bool", 8, llvm::dwarf::DW_ATE_unsigned));

			InsertLLVMDebugType(IR_int, m_DBuilder->createBasicType("int", 32, llvm::dwarf::DW_ATE_signed));
			InsertLLVMDebugType(IR_float, m_DBuilder->createBasicType("float", 32, llvm::dwarf::DW_ATE_float));
		}

		//Opaque_Type = llvm::Type::getInt8PtrTy(*m_LLVMContext);
	}

	void LLVMBackend::Compile()
	{
		// Add the current debug info version into the module.
		m_LLVMModule->addModuleFlag(llvm::Module::Warning, "Debug Info Version",
			llvm::DEBUG_METADATA_VERSION);

		EnumsCodegen();
		StructsCodeGen();
		ForeignCodeGen();

		GenerateTypeInfo();

		for (auto i : m_Program->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;
				SetLLVMFile(ir_file->File_Name, ir_file->Directory);

				for (auto tl_inst : ir_file->Instructions) {
					if (tl_inst->GetType() == IRNodeType::Function) {
						FunctionSignatureCodeGen((IRFunction*)tl_inst);
					}
				}
			}
		}

		for (auto i : m_Program->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;
				SetLLVMFile(ir_file->File_Name, ir_file->Directory);

				for (auto tl_inst : ir_file->Instructions) {
					CodeGen(tl_inst);
				}
			}
			else {
				CodeGen(i);
			}
		}

		llvm::verifyModule(*m_LLVMModule, &llvm::outs());

		GenerateObjFile();
	}

	void LLVMBackend::EnumsCodegen()
	{

		for (const auto& enum_pair : m_Metadata->m_Enums) {

			u64 id = enum_pair.first;

			const EnumMetadata& enum_metadata = enum_pair.second;

			//@Speed store enum type id in the enum metadata struct
			//@Compat this will fail when we add modules

			u64 type_id = m_Metadata->GetType(enum_metadata.Name.Symbol);

			InsertLLVMType(type_id, GetLLVMType(IR_u64));
			//@Debugging
			{
				InsertLLVMDebugType(type_id, m_DBuilder->createBasicType(enum_metadata.Name.Symbol, 64, llvm::dwarf::DW_ATE_unsigned));
			}
		}
	}

	void LLVMBackend::StructsCodeGen()
	{
		llvm::DataLayout llvm_DataLayout(m_LLVMModule);

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
				llvm_Members_Types.push_back(GetLLVMType(member_metadata.Type));
			}

			our_struct_type.LLVMType->setBody(llvm_Members_Types);
			our_struct_type.LLVMMembers = llvm_Members_Types;
		}

		for (const auto& func_pair : m_Metadata->m_StructMetadata) {

			u64 id = func_pair.first;
			const StructMetadata& struct_metadata = func_pair.second;

			LLVMStructType& our_struct_type = GetLLVMStructType(id);

			const llvm::StructLayout* llvm_StructLayout = llvm_DataLayout.getStructLayout(our_struct_type.LLVMType);

			//@Debugging
			{
				std::vector<llvm::Metadata*> llvm_Field_Types;
				u32 elem = 0;

				for (const MemberMetadata& member_metadata : struct_metadata.Members) {

					llvm::Type* llvm_MemberType = GetLLVMType(member_metadata.Type);

					u64 type_size = llvm_DataLayout.getTypeSizeInBits(llvm_MemberType);
					u32 offset_bits = (u32)llvm_StructLayout->getElementOffsetInBits(elem);
					u32 align_bits = llvm_DataLayout.getABITypeAlignment(llvm_MemberType) * 8;

					llvm::DIDerivedType* llvm_MemberDebugType = m_DBuilder->createMemberType(
						m_DCU,
						member_metadata.Name.Symbol,
						(llvm::DIFile*)mDContext,
						(int)member_metadata.Name.Line,
						type_size,
						align_bits,
						offset_bits,
						llvm::DINode::DIFlags::FlagZero,
						GetLLVMDebugType(member_metadata.Type)
					);

					llvm_Field_Types.push_back(llvm_MemberDebugType);
					elem++;
				}

				u64 struct_size = llvm_StructLayout->getSizeInBits();

				llvm::DICompositeType* struct_Debug_Type = m_DBuilder->createStructType(
					m_DCU,											// Scope
					struct_metadata.Name.Symbol,					// Name
					(llvm::DIFile*)mDContext,						// File
					(u32)struct_metadata.Name.Line,					// Line number
					struct_size,									// Size in bits
					32,												// Alignment in bits
					llvm::DINode::FlagZero,							// Flags
					nullptr,										// Derived from
					m_DBuilder->getOrCreateArray(llvm_Field_Types)	// Elements
				);

				InsertLLVMDebugType(our_struct_type.TypeID, struct_Debug_Type);
			}

			//@Todo add cmd option for this
			if (0)
			{
				GS_CORE_WARN("Struct: {}", struct_metadata.Name.Symbol);
				u64 i = 0;
				for (const MemberMetadata& member_metadata : struct_metadata.Members) {

					u64 elementOffset = llvm_StructLayout->getElementOffset((u32)i);

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
				Parameters.push_back(GetLLVMType(arg_metadata.Type));
			}

			llvm::FunctionType* Function_Type =
				llvm::FunctionType::get(GetLLVMType(metadata->ReturnType), Parameters, metadata->Variadic);

			llvm::Function* llvm_Func =
				llvm::Function::Create(
					Function_Type, llvm::Function::LinkageTypes::ExternalLinkage,
					metadata->Symbol.Symbol, m_LLVMModule);

			InsertLLVMFunction(GetFunctionHash(metadata->Symbol.Symbol, metadata->Signature->Hash), llvm_Func);
		}
	}

	void LLVMBackend::GenerateTypeInfo()
	{
		std::unordered_map<u64, TypeStorage*>& UniqueTypeInfoMap = TypeSystem::GetTypeMap();

		llvm::Type* llvm_TypeInfoElemMemberTy = GetLLVMType(IR_u64);

		llvm::Type* llvm_TypeInfoElemMemberBodyTy[4] =
		{
			llvm_TypeInfoElemMemberTy,
			llvm_TypeInfoElemMemberTy,
			llvm_TypeInfoElemMemberTy,
			llvm_TypeInfoElemMemberTy,
		};

		m_TypeInfoElemTy = llvm::StructType::create(*m_LLVMContext, "TypeInfoTableElem");
		m_TypeInfoElemTy->setBody(llvm_TypeInfoElemMemberBodyTy);

		std::unordered_map<u64, u64> Struct_Typeinfo_member_indices;
		llvm::GlobalVariable* llvm_GlobalMemberTypeInfo_Array = nullptr;

		{
			llvm::DataLayout llvm_DataLayout(m_LLVMModule);

			std::vector<llvm::Constant*> llvm_GlobalTypeInfoArray_MemberData;

			u64 total_Struct_type_info_elements = 0;

			for (const auto& [struct_id, struct_metadata] : m_Metadata->m_StructMetadata) {

				Struct_Typeinfo_member_indices[struct_id] = total_Struct_type_info_elements;

				const llvm::StructLayout* llvm_StructLayout =
					llvm_DataLayout.getStructLayout(GetLLVMStructType(struct_id).LLVMType);

				u64 element_idx = 0;

				for (const MemberMetadata& member : struct_metadata.Members) {

					llvm::Constant* ti_member_elem = nullptr;
					llvm::Constant* ti_elem_member_name = m_LLVMBuilder->CreateGlobalStringPtr(member.Name.Symbol, "", 0, m_LLVMModule);

					i32 ti_elem_offset =
						llvm_StructLayout->getElementOffset(element_idx);

					ti_member_elem = llvm::ConstantStruct::get(
						(llvm::StructType*)GetLLVMType(IR_typeinfo_member)
						, {
							ti_elem_member_name,
							llvm::ConstantInt::get(GetLLVMType(IR_type), TypeSystem::GetTypeInfoIndex(member.Type)),
							llvm::ConstantInt::get(GetLLVMType(IR_u32),ti_elem_offset),
						});

					llvm_GlobalTypeInfoArray_MemberData.push_back(ti_member_elem);
					total_Struct_type_info_elements++;
					element_idx++;
				}
			}

			auto llvm_TypeInfoMember_ArrayTy = llvm::ArrayType::get(
				GetLLVMType(IR_typeinfo_member), total_Struct_type_info_elements
			);

			llvm_GlobalMemberTypeInfo_Array =
				new llvm::GlobalVariable(
					*m_LLVMModule,
					llvm_TypeInfoMember_ArrayTy,
					true,
					llvm::GlobalValue::ExternalLinkage,
					llvm::ConstantArray::get(llvm_TypeInfoMember_ArrayTy, llvm_GlobalTypeInfoArray_MemberData),
					"TypeInfo_Members_Array"
				);
		}

		u64 total_type_info_elements = UniqueTypeInfoMap.size();

		llvm::ArrayType* llvm_TypeInfoArrayTy
			= llvm::ArrayType::get(m_TypeInfoElemTy, total_type_info_elements);

		std::vector<llvm::Constant*> llvm_GlobalTypeInfoArrayData;

		for (auto& [hash, typeinfo] : UniqueTypeInfoMap) {

			llvm::Constant* ti_elem = nullptr;

			u64 struct_id = m_Metadata->GetStructIDFromType(typeinfo->BaseID);

			bool skip = false;

			if (typeinfo->Kind == TypeStorageKind::Pointer) {
				if (((TSPtr*)typeinfo)->Pointee->Kind == TypeStorageKind::Function) {
					skip = true;
				}
			}

			if (typeinfo->Kind == TypeStorageKind::Poly) {
				skip = true;
			}

			if (typeinfo->Kind == TypeStorageKind::Function || skip) {
				ti_elem = llvm::ConstantStruct::get(m_TypeInfoElemTy
					, {
						llvm::ConstantInt::get(GetLLVMType(IR_u64),0),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),0),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),0),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),0),
					});
			}
			else if (struct_id == -1) {

				TypeInfoFlags type_info_flags = m_Metadata->GetTypeInfoFlags(typeinfo->BaseID);

				llvm::Constant* ti_elem_name = m_LLVMBuilder->CreateGlobalStringPtr(
					m_Metadata->GetType(typeinfo->BaseID), "", 0, m_LLVMModule);

				ti_elem = llvm::ConstantStruct::get(m_TypeInfoElemTy
					, {
						llvm::ConstantExpr::getPtrToInt(ti_elem_name, GetLLVMType(IR_u64)),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),(u64)type_info_flags),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),m_Metadata->GetTypeSize(typeinfo)),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),0),
					});
			}
			else if (struct_id != -1) {

				TypeInfoFlags type_info_flags = m_Metadata->GetTypeInfoFlags(typeinfo->BaseID);

				const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(struct_id);

				llvm::Constant* ti_elem_name =
					m_LLVMBuilder->CreateGlobalStringPtr(
						struct_metadata->Name.Symbol, "", 0, m_LLVMModule);

				auto type_table_as_type_info_member_ptr =
					llvm::ConstantExpr::getBitCast(
						llvm_GlobalMemberTypeInfo_Array,
						GetLLVMTypeFull(IR_typeinfo_member, 1));

				llvm::Constant* ti_elem_members_ptr =
					llvm::ConstantExpr::getGetElementPtr(
						GetLLVMType(IR_typeinfo_member),
						type_table_as_type_info_member_ptr,
						llvm::ConstantInt::get(GetLLVMType(IR_u64), Struct_Typeinfo_member_indices[struct_id])
					);

				ti_elem = llvm::ConstantStruct::get(m_TypeInfoElemTy
					, {
						llvm::ConstantExpr::getPtrToInt(ti_elem_name,GetLLVMType(IR_u64)),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),(u64)type_info_flags),
						llvm::ConstantExpr::getPtrToInt(ti_elem_members_ptr,GetLLVMType(IR_u64)),
						llvm::ConstantInt::get(GetLLVMType(IR_u64),struct_metadata->Members.size()),
					});
			}

			llvm_GlobalTypeInfoArrayData.push_back(ti_elem);
		}

		llvm::GlobalVariable* llvm_GlobalTypeInfoArray = nullptr;

		llvm_GlobalTypeInfoArray =
			new llvm::GlobalVariable(
				*m_LLVMModule,
				llvm_TypeInfoArrayTy,
				true,
				llvm::GlobalValue::ExternalLinkage,
				llvm::ConstantArray::get(llvm_TypeInfoArrayTy, llvm_GlobalTypeInfoArrayData),
				"TypeInfoArray"						// Name of the global variable
			);

		m_GlobalTypeInfoArray = llvm_GlobalTypeInfoArray;
	}

	llvm::Value* LLVMBackend::TypeOfCodeGen(const IRTypeOf* type_of)
	{
		llvm::Value* llvm_TypeInfoPtr = m_LLVMBuilder->CreateGEP(
			m_TypeInfoElemTy,
			m_GlobalTypeInfoArray,
			llvm::ConstantInt::get(GetLLVMType(IR_u64), TypeSystem::GetTypeInfoIndex(type_of->Type)),
			"ti_lookup");

		return m_LLVMBuilder->CreateBitCast(llvm_TypeInfoPtr, GetLLVMTypeFull(IR_typeinfo, 1));
	}


	llvm::Value* LLVMBackend::TypeInfoCodeGen(const IRTypeInfo* type_info)
	{
		auto index = GetName(type_info->ArgumentSSA);

		auto type_info_array = m_LLVMBuilder
			->CreateBitCast(m_GlobalTypeInfoArray, llvm::PointerType::get(m_TypeInfoElemTy, 0));

		llvm::Value* llvm_TypeInfoPtr = m_LLVMBuilder->CreateGEP(
			m_TypeInfoElemTy,
			type_info_array,
			index,
			"ti_lookup");

		return m_LLVMBuilder->CreateBitCast(llvm_TypeInfoPtr, GetLLVMTypeFull(IR_typeinfo, 1));
	}


	llvm::Value* LLVMBackend::TypeValueCodeGen(const IRTypeValue* type_value)
	{
		auto type_table_index = TypeSystem::GetTypeInfoIndex(type_value->Type);
		return llvm::ConstantInt::get(GetLLVMType(IR_type), type_table_index);
	}

	llvm::Value* LLVMBackend::FuncRefCodeGen(const IRFuncRef* func_ref)
	{
		auto metadata = m_Metadata->GetFunctionMetadata(func_ref->FunctionID);

		auto llvm_Func = GetLLVMFunction(GetFunctionHash(metadata->Symbol.Symbol, metadata->Signature->Hash));
		return llvm_Func.llvmFunction;
	}

	llvm::Value* LLVMBackend::CallFuncRefCodeGen(const IRCallFuncRef* func_ref)
	{
		auto llvm_FuncPtr = GetName(func_ref->PtrSSA);

		auto llvm_FuncType = GetLLVMType(func_ref->Signature);

		std::vector<llvm::Value*> llvm_Arguments;

		for (auto arg : func_ref->Arguments) {
			llvm_Arguments.push_back(GetName(arg));
		}

		return m_LLVMBuilder->CreateCall((llvm::FunctionType*)llvm_FuncType->getPointerElementType(), llvm_FuncPtr, llvm_Arguments);
	}


	llvm::Value* LLVMBackend::LexicalBlockCodeGen(const IRLexBlock* lexical_block)
	{
		auto llvm_lexical_block = m_DBuilder->createLexicalBlock(m_DLexicalBlocks.back(), (llvm::DIFile*)mDContext, (u32)lexical_block->Begin.Line, (u32)lexical_block->Begin.Begin);
		m_DLexicalBlocks.push_back(llvm_lexical_block);

		for (auto inst : lexical_block->Instructions) {
			CodeGen(inst);
		}

		SetDBGLocation(DBGSourceLoc((u32)lexical_block->End.Line, (u32)lexical_block->End.Begin));
		PopDBGLexicalBlock();

		return nullptr;
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

		case IRNodeType::GlobDecl: GlobalVariableCodeGen((IRGlobalDecl*)instruction); break;
		case IRNodeType::GlobAddress: return GlobalAddrCodeGen((IRGlobalAddress*)instruction);

		case IRNodeType::Load: return LoadCodeGen((IRLoad*)instruction);
		case IRNodeType::Store: return StoreCodeGen((IRStore*)instruction);

		case IRNodeType::MemberAccess: return MemberAccessCodeGen((IRMemberAccess*)instruction);
		case IRNodeType::ArrayAccess: return ArrayAccessCodeGen((IRArrayAccess*)instruction);

		case IRNodeType::Call: return CallCodeGen((IRFunctionCall*)instruction);

		case IRNodeType::SizeOf: return SizeOfCodeGen((IRSizeOF*)instruction);

		case IRNodeType::PointerCast:
		case IRNodeType::Int2PtrCast:
		case IRNodeType::Ptr2IntCast:
		case IRNodeType::SExtCast:
		case IRNodeType::ZExtCast:
		case IRNodeType::IntTrunc:
		case IRNodeType::Int2FP:
		case IRNodeType::FP2Int:
		case IRNodeType::FPExt:
		case IRNodeType::FPTrunc:
		{
			return CastCodeGen((IRCast*)instruction);
		}
		break;
		case IRNodeType::NullPtr: return NullPtrCodeGen((IRNullPtr*)instruction);

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
		case IRNodeType::BitOr:
		case IRNodeType::BitAnd:
		case IRNodeType::And:
		case IRNodeType::Or:
			return OpCodeGen((IRBinOp*)instruction);
			break;

		case IRNodeType::Any: return AnyCodeGen((IRAny*)instruction);
		case IRNodeType::AnyArray: return AnyArrayCodeGen((IRAnyArray*)instruction);

		case IRNodeType::TypeOf: return TypeOfCodeGen((IRTypeOf*)instruction);
		case IRNodeType::TypeInfo: return TypeInfoCodeGen((IRTypeInfo*)instruction);
		case IRNodeType::TypeValue: return TypeValueCodeGen((IRTypeValue*)instruction);


		case IRNodeType::FuncRef: return FuncRefCodeGen((IRFuncRef*)instruction);
		case IRNodeType::CallFuncRef: return CallFuncRefCodeGen((IRCallFuncRef*)instruction);

		case IRNodeType::LexicalBlock: return LexicalBlockCodeGen((IRLexBlock*)instruction);

		default:
			return 0;
		}
	}

	void LLVMBackend::FunctionSignatureCodeGen(const IRFunction* func)
	{
		const FunctionMetadata* func_metadata = m_Metadata->GetFunctionMetadata(func->ID);

		if (func_metadata->PolyMorphic) {
			for (auto& [overload, instance] : func_metadata->PolyMorphicInstantiations) {
				FunctionSignatureCodeGen(instance);
			}
			return;
		}

		GS_CORE_ASSERT(func_metadata, "");

		u64 function_id = GetFunctionHash(func_metadata->Symbol.Symbol, func_metadata->Signature->Hash);

		if (func->Overload) {
			func_metadata = &func_metadata->GetOverload((TSFunc*)func->Overload);
		}

		std::vector<llvm::Type*> Parameters;

		for (const ArgumentMetadata& arg_metadata : func_metadata->Arguments) {
			if (!arg_metadata.Variadic) {
				Parameters.push_back(GetLLVMType(arg_metadata.Type));
			}
			else {
				Parameters.push_back(GetLLVMType(IR_array));
			}
		}

		llvm::FunctionType* Function_Type =
			llvm::FunctionType::get(GetLLVMType(func_metadata->ReturnType), Parameters, false);

		std::string exported_name;
		if (func_metadata->Symbol.Symbol == "main") {
			exported_name = "main";
		}
		else {
			exported_name = MangleName(func_metadata->Symbol.Symbol, (TSFunc*)func_metadata->Signature);
		}

		llvm::Function* llvm_Func =
			llvm::Function::Create(Function_Type, llvm::Function::ExternalLinkage, exported_name, m_LLVMModule);

		if (!func->Overload) {
			InsertLLVMFunction(function_id, llvm_Func);
		}
		else {
			InsertLLVMFunctionOverload(function_id, (TSFunc*)func->Overload, llvm_Func);
		}
	}

	llvm::Value* LLVMBackend::FunctionCodeGen(const IRFunction* func)
	{
		const FunctionMetadata* func_metadata = m_Metadata->GetFunctionMetadata(func->ID);

		if (func_metadata->PolyMorphic) {
			for (auto& [overload, instance] : func_metadata->PolyMorphicInstantiations) {
				FunctionCodeGen(instance);
			}
			return nullptr;
		}

		GS_CORE_ASSERT(func_metadata, "");

		SetFunctionID(GetFunctionHash(func_metadata->Symbol.Symbol, func_metadata->Signature->Hash));

		if (func->Overload) {
			func_metadata = &func_metadata->GetOverload((TSFunc*)func->Overload);
		}

		std::vector<llvm::Type*> Parameters;

		for (const ArgumentMetadata& arg_metadata : func_metadata->Arguments) {
			if (!arg_metadata.Variadic) {
				Parameters.push_back(GetLLVMType(arg_metadata.Type));
			}
			else {
				Parameters.push_back(GetLLVMType(IR_array));
			}
		}

		llvm::FunctionType* Function_Type =
			llvm::FunctionType::get(GetLLVMType(func_metadata->ReturnType), Parameters, false);

		llvm::Function* llvm_Func = nullptr;

		if (func->Overload == nullptr) {
			llvm_Func = GetLLVMFunction(m_CurrentFunctionID).llvmFunction;
		}
		else {
			llvm_Func = GetLLVMFunction(m_CurrentFunctionID).Overloads[(TSFunc*)func->Overload];
		}

		//BODY CODE GEN///////////////////////////

		llvm::BasicBlock* BB = llvm::BasicBlock::Create(*m_LLVMContext, "entry", llvm_Func);
		m_LLVMBuilder->SetInsertPoint(BB);

		//@Debuggging
		FunctionDebugInfo(func_metadata, llvm_Func);
		m_LLVMBuilder->SetCurrentDebugLocation(llvm::DebugLoc());

		std::vector<llvm::Value*> argument_names;
		for (auto& arg : llvm_Func->args()) {
			argument_names.push_back((llvm::Value*)&arg);
		}

		InsertFunctionArgNames(m_CurrentFunctionID, argument_names);
		//InsertLLVMFunction(m_CurrentFunctionID, llvm_Func);

		//Argument Stack Storage
		u64 argument_id = 0;
		for (const ArgumentMetadata& arg_metadata : func_metadata->Arguments) {

			llvm::AllocaInst* argument_Alloca = nullptr;

			if (!arg_metadata.Variadic) {
				argument_Alloca = m_LLVMBuilder->CreateAlloca(GetLLVMType(arg_metadata.Type));
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
					GetLLVMDebugType(arg_metadata.Type),
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

		if (func_metadata->ReturnType->BaseID == IR_void) {
			m_LLVMBuilder->CreateRet(nullptr);
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


	llvm::Value* LLVMBackend::GlobalAddrCodeGen(const IRGlobalAddress* global_address)
	{
		return GetGlobalVariable(global_address->GlobID);
	}

	llvm::Value* LLVMBackend::ConstValueCodeGen(const IRCONSTValue* constant)
	{
		auto llvm_Type = GetLLVMType(constant->Type);

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
			return llvm::ConstantInt::get(llvm_Type, data, false);
		}
		else {
			return llvm::ConstantInt::getSigned(llvm_Type, data);
		}
	}

	llvm::Value* LLVMBackend::OpCodeGen(const IRBinOp* op)
	{
		IRNodeType type = op->GetType();

		//CodeGen(op->SSA_A);
		//CodeGen(op->SSA_B);

		llvm::Value* lhs = GetName(op->SSA_A->SSA);
		llvm::Value* rhs = GetName(op->SSA_B->SSA);
		// 
		// 		if (lhs->getType() != rhs->getType()) {
		// 			__debugbreak();
		// 		}

		llvm::Value* result = nullptr;

		switch (type)
		{
		case IRNodeType::ADD:
			if (m_Metadata->GetTypeFlags(op->Type) & TypeFlag::FLAG_FLOATING_TYPE) {
				result = m_LLVMBuilder->CreateFAdd(lhs, rhs);
			}
			else
			{
				result = m_LLVMBuilder->CreateAdd(lhs, rhs);
			}
			break;
		case IRNodeType::SUB:
			if (m_Metadata->GetTypeFlags(op->Type) & TypeFlag::FLAG_FLOATING_TYPE) {
				result = m_LLVMBuilder->CreateFSub(lhs, rhs);
			}
			else
			{
				result = m_LLVMBuilder->CreateSub(lhs, rhs);
			}
			break;
		case IRNodeType::MUL:
			if (m_Metadata->GetTypeFlags(op->Type) & TypeFlag::FLAG_FLOATING_TYPE) {
				result = m_LLVMBuilder->CreateFMul(lhs, rhs);
			}
			else
			{
				result = m_LLVMBuilder->CreateMul(lhs, rhs);
			}
			break;
		case IRNodeType::DIV:
			if (m_Metadata->GetTypeFlags(op->Type) & FLAG_FLOATING_TYPE) {
				result = m_LLVMBuilder->CreateFDiv(lhs, rhs);
			}
			else
			{
				result = m_LLVMBuilder->CreateSDiv(lhs, rhs);
			}
			break;

		case IRNodeType::Equal:
			result = m_LLVMBuilder->CreateICmpEQ(lhs, rhs, "comp");
			result = m_LLVMBuilder->CreateZExt(result, GetLLVMType(IR_bool));
			break;

		case IRNodeType::NotEqual:
			result = m_LLVMBuilder->CreateICmpNE(lhs, rhs, "comp");
			result = m_LLVMBuilder->CreateZExt(result, GetLLVMType(IR_bool));
			break;

		case IRNodeType::LesserThan:
			result = m_LLVMBuilder->CreateICmpULT(lhs, rhs);
			result = m_LLVMBuilder->CreateZExt(result, GetLLVMType(IR_bool));
			break;

		case IRNodeType::GreaterThan:
			result = m_LLVMBuilder->CreateICmpUGT(lhs, rhs);
			result = m_LLVMBuilder->CreateZExt(result, GetLLVMType(IR_bool));
			break;

		case IRNodeType::BitAnd:
			result = m_LLVMBuilder->CreateAnd(lhs, rhs);
			break;

		case IRNodeType::BitOr:
			result = m_LLVMBuilder->CreateOr(lhs, rhs);
			break;
		case IRNodeType::And:
		{
			lhs = m_LLVMBuilder->CreateICmpUGT(lhs, llvm::ConstantInt::get(lhs->getType(), 0));
			rhs = m_LLVMBuilder->CreateICmpUGT(rhs, llvm::ConstantInt::get(rhs->getType(), 0));

			result = m_LLVMBuilder->CreateAnd(lhs, rhs);
			result = m_LLVMBuilder->CreateZExt(result, GetLLVMType(IR_bool));
		}
		break;
		case IRNodeType::Or:
		{
			lhs = m_LLVMBuilder->CreateICmpUGT(lhs, llvm::ConstantInt::get(lhs->getType(), 0));
			rhs = m_LLVMBuilder->CreateICmpUGT(rhs, llvm::ConstantInt::get(rhs->getType(), 0));

			result = m_LLVMBuilder->CreateOr(lhs, rhs);
			result = m_LLVMBuilder->CreateZExt(result, GetLLVMType(IR_bool));
		}
		break;
		default:
			GS_CORE_ASSERT(0, "Unsupported Operator");
			break;
		}

		return result;
	}

	llvm::Value* LLVMBackend::AllocaCodeGen(const IRAlloca* aloca)
	{
		//make it work with function types

		llvm::AllocaInst* llvm_alloca =
			CreateEntryBlockAlloca(GetLLVMType(aloca->Type));

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


	void LLVMBackend::GlobalVariableCodeGen(const IRGlobalDecl* global_decl)
	{
		const VariableMetadata* global_metadata = m_Metadata->GetVariableMetadataRecursive(1, global_decl->GlobID);

		llvm::Constant* initilizer = nullptr;
		auto llvm_Type = GetLLVMType(global_decl->Type);

		std::string external_Name;

		if (!global_metadata->Foreign) {
			initilizer = llvm::ConstantAggregateZero::get(llvm_Type);
			if (global_decl->Initializer) {
				initilizer = (llvm::Constant*)CodeGen(global_decl->Initializer);
			}
		}
		else {
			external_Name = global_metadata->Name.Symbol;
		}

		auto llvm_GlobalVar =
			new llvm::GlobalVariable(
				*m_LLVMModule,
				llvm_Type,
				false,
				llvm::GlobalVariable::LinkageTypes::ExternalLinkage,
				initilizer,
				external_Name);

		InsertGlobalVariable(global_decl->GlobID, llvm_GlobalVar);
	}

	llvm::Value* LLVMBackend::LoadCodeGen(const IRLoad* load)
	{
		auto ld = m_LLVMBuilder->CreateLoad(GetLLVMType(load->Type), GetName(load->AddressSSA));
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
			llvm_Object = m_LLVMBuilder->CreateLoad(llvm_Struct_Type->getPointerTo(), llvm_Object);
		}
		// 		GS_CORE_WARN("Member Access Begin");
		// 		llvm_Object->print(llvm::outs());
		// 		GS_CORE_WARN("Member Access Second");
		// 		llvm_Struct_Type->print(llvm::outs());
		// 		GS_CORE_WARN("Member Access End");

		return m_LLVMBuilder->CreateStructGEP(llvm_Struct_Type, llvm_Object, (unsigned)member_access->MemberID);
	}

	llvm::Value* LLVMBackend::ArrayAccessCodeGen(const IRArrayAccess* array_access)
	{

		auto llvm_type = GetLLVMType(array_access->Type);
		auto llvm_pointer = GetName(array_access->ArrayAddress);
		auto llvm_index = GetName(array_access->ElementSSA);

		// 		llvm_type->print(llvm::errs(), true);
		// 		llvm::errs() << "\n";
		// 		llvm_pointer->print(llvm::errs(), true);
		// 		llvm::errs() << "\n";
		// 		llvm_index->print(llvm::errs(), true);
		// 		llvm::errs() << "\n";

		return m_LLVMBuilder->CreateGEP(
			llvm_type,
			llvm_pointer,
			llvm_index,
			"array_access"
		);
	}

	llvm::Value* LLVMBackend::CallCodeGen(const IRFunctionCall* call)
	{
		auto metadata = m_Metadata->GetFunctionMetadata(call->FuncID);
		u64 function_id = GetFunctionHash(metadata->Symbol.Symbol, metadata->Signature->Hash);

		if (call->Overload) {
			metadata = &metadata->GetOverload((TSFunc*)call->Overload);
		}

		llvm::Function* llvm_Func = nullptr;

		if (call->Overload) {
			llvm_Func = GetLLVMFunction(function_id).Overloads[(TSFunc*)metadata->Signature];
		}
		else
		{
			llvm_Func = GetLLVMFunction(function_id).llvmFunction;
		}

		GS_CORE_ASSERT(llvm_Func, "LLVM Function Must Exist At This Point");

		std::vector<llvm::Value*> llvm_Arguments;

		for (IRInstruction* arg : call->Arguments) {
			GS_CORE_ASSERT(arg->GetType() == IRNodeType::SSAValue, "LLVM Function Must Exist At This Point");

			IRSSAValue* as_ssa_value = (IRSSAValue*)arg;

			if (llvm_Func->isVarArg()) {

				auto llvm_arg_Val = GetName(as_ssa_value->SSA);

				if (llvm_arg_Val->getType() == GetLLVMType(IR_f32)) {
					llvm_Arguments.push_back(m_LLVMBuilder->CreateFPExt(llvm_arg_Val, GetLLVMType(IR_f64)));
				}
				else {
					llvm_Arguments.push_back(GetName(as_ssa_value->SSA));
				}
			}
			else
			{
				llvm_Arguments.push_back(GetName(as_ssa_value->SSA));
			}
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

	llvm::Value* LLVMBackend::CastCodeGen(const IRCast* ptr_cast)
	{
		llvm::Value* value = GetName(ptr_cast->SSA);
		llvm::Type* cast_type = GetLLVMType(ptr_cast->Type);

		switch (ptr_cast->GetType()) {
		case IRNodeType::PointerCast:
			return m_LLVMBuilder->CreateBitCast(value, cast_type);
		case IRNodeType::ZExtCast:
			return m_LLVMBuilder->CreateZExt(value, cast_type);
		case IRNodeType::SExtCast:
			return m_LLVMBuilder->CreateSExt(value, cast_type);
		case IRNodeType::Int2PtrCast:
			return m_LLVMBuilder->CreateIntToPtr(value, cast_type);
		case IRNodeType::Ptr2IntCast:
			return m_LLVMBuilder->CreatePtrToInt(value, cast_type);
		case IRNodeType::IntTrunc:
			return m_LLVMBuilder->CreateTrunc(value, cast_type);
		case IRNodeType::Int2FP:
		{
			auto as_int_2_float = (IRInt2FP*)ptr_cast;
			if (as_int_2_float->Signed)
				return m_LLVMBuilder->CreateSIToFP(value, cast_type);
			else
				return m_LLVMBuilder->CreateUIToFP(value, cast_type);
		}
		break;
		case IRNodeType::FP2Int:
		{
			auto as_float_2_int = (IRFP2Int*)ptr_cast;
			if (as_float_2_int->Signed)
				return m_LLVMBuilder->CreateFPToSI(value, cast_type);
			else
				return m_LLVMBuilder->CreateFPToUI(value, cast_type);
		}
		break;
		case IRNodeType::FPExt:
			return m_LLVMBuilder->CreateFPExt(value, cast_type);
		case IRNodeType::FPTrunc:
			return m_LLVMBuilder->CreateFPTrunc(value, cast_type);
		}

		GS_CORE_ASSERT(0, "Unknown Cast");
	}

	// 	llvm::Value* LLVMBackend::PointerCastCodeGen(const IRPointerCast* ptr_cast)
	// 	{
	// 		return m_LLVMBuilder->CreateBitCast(GetName(ptr_cast->PointerSSA), GetLLVMType(ptr_cast->Type));
	// 	}

	llvm::Value* LLVMBackend::NullPtrCodeGen(const IRNullPtr* null_ptr)
	{
		return llvm::ConstantPointerNull::get((llvm::PointerType*)GetLLVMTypeFull(null_ptr->TypeID, null_ptr->Indirection));
	}

	llvm::Value* LLVMBackend::IfCodeGen(const IRIf* _if)
	{
		llvm::Function* function = m_LLVMBuilder->GetInsertBlock()->getParent();

		llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*m_LLVMContext, "then", function);
		llvm::BasicBlock* contBlock = llvm::BasicBlock::Create(*m_LLVMContext, "cont", function);

		llvm::Value* condition = GetName(_if->SSA);

		condition =
			m_LLVMBuilder->CreateICmp(llvm::CmpInst::ICMP_UGT, condition,
				llvm::ConstantInt::get(condition->getType(), 0));

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

		condition =
			m_LLVMBuilder->CreateICmp(llvm::CmpInst::ICMP_UGT, condition,
				llvm::ConstantInt::get(condition->getType(), 0));

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

		auto type_table_index = TypeSystem::GetTypeInfoIndex(any->Type);

		m_LLVMBuilder->CreateStore(any_Data, llvm_any_data_ptr);
		m_LLVMBuilder->CreateStore(llvm::ConstantInt::get(GetLLVMType(IR_i64), type_table_index), llvm_any_type_ptr);

		return llvm_Struct;
	}

	llvm::Value* LLVMBackend::AnyArrayCodeGen(const IRAnyArray* any_array)
	{
		llvm::Type* llvm_AnyStructTy = GetLLVMStructType(m_Metadata->GetStructIDFromType(IR_any)).LLVMType;
		GS_CORE_ASSERT(llvm_AnyStructTy);

		llvm::Type* llvm_ArrayStructTy = GetLLVMStructType(m_Metadata->GetStructIDFromType(IR_array)).LLVMType;
		GS_CORE_ASSERT(llvm_ArrayStructTy);

		llvm::Value* llvm_AnyArray = CreateEntryBlockAlloca(
			llvm_AnyStructTy,
			llvm::ConstantInt::get(GetLLVMType(IR_u64),
				any_array->Arguments.size()));

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

	llvm::AllocaInst* LLVMBackend::CreateEntryBlockAlloca(llvm::Type* type, llvm::Constant* arraySize /*= nullptr*/)
	{
		llvm::Function* function = m_LLVMBuilder->GetInsertBlock()->getParent();

		llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
			function->getEntryBlock().begin());
		auto alloca = TmpB.CreateAlloca(type, arraySize);
		return alloca;
	}

	std::string LLVMBackend::MangleName(const std::string& name, TSFunc* signature)
	{
		return fmt::format("{}_{}", name, (void*)signature->Hash);
	}

	llvm::Value* LLVMBackend::ReturnCodeGen(const IRReturn* ret) {

		CodeGen(ret->Value);

		return m_LLVMBuilder->CreateRet(GetName(((IRSSAValue*)ret->Value)->SSA));
	}
}