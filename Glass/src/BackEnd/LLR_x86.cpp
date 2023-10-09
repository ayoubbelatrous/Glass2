#include "pch.h"

#include "BackEnd/LLR_x86.h"
#include "BackEnd/TypeSystem.h"

#include "math.h"

namespace Glass
{
	static const std::map<IRNodeType, X86_ASM> elbat_cigol_detrevni = {
		// inverted because we skip the true case if condition is false not otherway around
		{IRNodeType::LesserThan, X86_JMPGE},
		{IRNodeType::GreaterThan, X86_JMPLE},
		{IRNodeType::NotEqual, X86_JMPE},
		{IRNodeType::Equal, X86_JMPNE},
	};

	static const std::map<IRNodeType, X86_ASM> op_to_set_inst = {
		{IRNodeType::GreaterThan, X86_SETG},
		{IRNodeType::LesserThan, X86_SETL},
		{IRNodeType::Equal, X86_SETE},
		{IRNodeType::NotEqual, X86_SETNE},
	};

	static const std::map<RegisterUsage, std::vector<X86_Register>> registers = {
		{REG_I8, {
					 SIL,
					 DIL,
					 BL,
					 R10b,
					 R11b,
					 R12b,
					 R13b,
					 R14b,
					 R15b,
					 R8b,
					 R9b,
					 AL,
					 CL,
					 DL,
				 }},
		{REG_I16, {
					  SI,
					  DI,
					  BX,
					  R10w,
					  R11w,
					  R12w,
					  R13w,
					  R14w,
					  R15w,
					  R8w,
					  R9w,
					  AX,
					  CX,
					  DX,
				  }},
		{REG_I32, {
					  ESI,
					  EDI,
					  EBX,
					  R10d,
					  R11d,
					  R12d,
					  R13d,
					  R14d,
					  R15d,
					  R8d,
					  R9d,
					  EAX,
					  ECX,
					  EDX,
				  }},
		{REG_I64, {
					  RSI,
					  RDI,
					  RBX,
					  R10,
					  R11,
					  R12,
					  R13,
					  R14,
					  R15,
					  R8,
					  R9,
					  RAX,
					  RCX,
					  RDX,
				  }},

		{REG_F32, {XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15, XMM0, XMM1, XMM2, XMM3, XMM4}},
		{REG_F64, {XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15, XMM0, XMM1, XMM2, XMM3, XMM4}},
	};

	static std::map<X86_Register, RegisterUsage> register_sizes;
	static std::map<std::pair<X86_REG_Overlap, RegisterUsage>, X86_Register> register_trunc_map;

	static const std::map<X86_Register, X86_REG_Overlap> register_overlap = {
		{RAX, X86_AX},
		{EAX, X86_AX},
		{AX, X86_AX},
		{AL, X86_AX},

		{RBX, X86_BX},
		{EBX, X86_BX},
		{BX, X86_BX},
		{BL, X86_BX},

		{RCX, X86_CX},
		{ECX, X86_CX},
		{CX, X86_CX},
		{CL, X86_CX},

		{RDX, X86_DX},
		{EDX, X86_DX},
		{DX, X86_DX},
		{DL, X86_DX},

		{RSI, X86_SI},
		{ESI, X86_SI},
		{SI, X86_SI},
		{SIL, X86_SI},

		{RDI, X86_DI},
		{EDI, X86_DI},
		{DI, X86_SI},
		{DIL, X86_SI},

		{R8, X86_R8},
		{R9, X86_R9},
		{R10, X86_R10},
		{R11, X86_R11},
		{R11, X86_R12},
		{R12, X86_R12},
		{R13, X86_R13},
		{R15, X86_R14},
		{R14, X86_R15},

		{R8d, X86_R8},
		{R9d, X86_R9},
		{R10d, X86_R10},
		{R11d, X86_R11},
		{R11d, X86_R12},
		{R12d, X86_R12},
		{R13d, X86_R13},
		{R15d, X86_R14},
		{R14d, X86_R15},

		{R8w, X86_R8},
		{R9w, X86_R9},
		{R10w, X86_R10},
		{R11w, X86_R11},
		{R11w, X86_R12},
		{R12w, X86_R12},
		{R13w, X86_R13},
		{R15w, X86_R14},
		{R14w, X86_R15},

		{R8b, X86_R8},
		{R9b, X86_R9},
		{R10b, X86_R10},
		{R11b, X86_R11},
		{R11b, X86_R12},
		{R12b, X86_R12},
		{R13b, X86_R13},
		{R15b, X86_R14},
		{R14b, X86_R15},

		{XMM0, X86_XMM0},
		{XMM1, X86_XMM1},
		{XMM2, X86_XMM2},
		{XMM3, X86_XMM3},
		{XMM4, X86_XMM4},
		{XMM5, X86_XMM5},
		{XMM6, X86_XMM6},
		{XMM7, X86_XMM7},

		{XMM8, X86_XMM8},
		{XMM9, X86_XMM9},
		{XMM10, X86_XMM10},
		{XMM11, X86_XMM11},
		{XMM12, X86_XMM12},
		{XMM13, X86_XMM13},
		{XMM14, X86_XMM14},
		{XMM15, X86_XMM15} };

	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		: m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
		Init();
	}

	void X86_BackEnd::Init()
	{
		for (const auto& [size, regs] : registers)
		{
			for (auto reg : regs)
			{
				register_sizes[reg] = size;
			}
		}

		for (const auto& [reg, overlap] : register_overlap)
		{
			register_trunc_map[{overlap, register_sizes.at(reg)}] = reg;
		}
	}

	std::vector<X86_Inst*> X86_BackEnd::Assemble()
	{
		std::vector<X86_Inst*> build_stream;

		for (auto& [Id, func] : m_Metadata->m_Functions)
		{
			std::string mangled_name;

			if (func.Foreign)
			{
				mangled_name = func.Symbol.Symbol;
				m_Externals.push_back(mangled_name);
			}
			else
			{
				if (func.PolyMorphic == false)
				{
					mangled_name = MangleName(func.Symbol.Symbol, (TSFunc*)func.Signature);
				}
			}

			X86_Inst* label = ASMA(X86_Inst());
			label->type = X86_LABEL_REF;
			label->as.label.name = ASMA(mangled_name)->c_str();

			for (auto& [signature, overload] : func.Overloads)
			{
				mangled_name = MangleName(func.Symbol.Symbol, signature);

				X86_Inst* label = ASMA(X86_Inst());
				label->type = X86_LABEL_REF;
				label->as.label.name = ASMA(mangled_name)->c_str();

				m_Data.IR_FunctionOverloadsLabels[Id][signature] = label;
			}

			m_Data.IR_FunctionLabels[Id] = ASMA(*label);
		}

		MemCpy = ASMA(X86_Inst());
		MemCpy->type = X86_LABEL_REF;
		MemCpy->as.label.name = "memcpy";

		AssemblyPolyMorphicFunctions(build_stream);

		for (auto i : m_TranslationUnit->Instructions)
		{
			if (i->GetType() == IRNodeType::File)
			{

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions)
				{
					Assemble(tl_inst, build_stream);
				}
			}
			else
			{
				Assemble(i, build_stream);
			}
		}

		std::string assem = Print(build_stream);

		std::ofstream asm_out("output.s");

		asm_out << "format MS64 COFF\n";
		asm_out << "public main\n";

		for (auto& ext : m_Externals)
		{
			asm_out << fmt::format("extrn '{0}' as {0}\n", ext);
		}

		asm_out << "section '.code' code readable executable ;align 16\n";
		asm_out << assem;

		std::string data_section;
		for (auto& [Id, str] : m_DataStrings)
		{

			std::string_view view = std::string_view((const char*)str.data(), str.size());

			data_section += fmt::format("str{} db ", Id);

			data_section += "'";

			u64 i = 0;
			while (i < view.size())
			{
				char c = view[i];

				if (c == '\\')
				{

					data_section += "'";

					if (view[i + 1] == 'n')
					{
						data_section += ", 0ah, ";
						i += 2;
						data_section += "'";
						continue;
					}
				}

				data_section += c;

				i++;
			}

			data_section += "'";
			data_section += ", 0\n";
		}

		std::string type_info_section;
		AssembleTypeInfoTable(type_info_section);

		asm_out << type_info_section;

		for (auto& [Id, str] : m_TypeInfoStrings)
		{

			std::string_view view = std::string_view((const char*)str.data(), str.size());

			data_section += fmt::format("t_str{} db ", Id);

			data_section += "'";

			u64 i = 0;
			while (i < view.size())
			{
				char c = view[i];

				if (c == '\\')
				{

					data_section += "'";

					if (view[i + 1] == 'n')
					{
						data_section += ", 0ah, ";
						i += 2;
						data_section += "'";
						continue;
					}
				}

				data_section += c;

				i++;
			}

			data_section += "'";
			data_section += ", 0\n";
		}

		std::string bss_section;
		for (auto& [glob_id, size] : GlobalUnInitializedVariables) {
			bss_section += "glb" + std::to_string(glob_id - 512000) + " rb " + std::to_string(size) + '\n';
		}

		asm_out << "section '.rdata' data readable align 16\n";
		asm_out << data_section;

		asm_out << "section '.bss' data readable writable align 16\n";
		asm_out << bss_section;

		return build_stream;
	}

	void X86_BackEnd::Assemble(IRInstruction* inst, std::vector<X86_Inst*>& stream)
	{
		IRNodeType node_type = inst->GetType();
		switch (node_type)
		{
		case IRNodeType::Function:
		{
			AssembleFunction((IRFunction*)inst, stream);
		}
		break;
		case IRNodeType::Argument:
		{
			AssembleArgument((IRArgumentAllocation*)inst, stream);
		}
		break;
		case IRNodeType::Alloca:
		{
			AssembleAlloca((IRAlloca*)inst, stream);
		}
		break;
		case IRNodeType::Store:
		{
			AssembleStore((IRStore*)inst, stream);
		}
		break;
		case IRNodeType::Load:
		{
			AssembleLoad((IRLoad*)inst, stream);
		}
		break;
		case IRNodeType::MemberAccess:
		{
			AssembleMemberAccess((IRMemberAccess*)inst, stream);
		}
		break;
		case IRNodeType::ArrayAccess:
		{
			AssembleArrayAccess((IRArrayAccess*)inst, stream);
		}
		break;
		case IRNodeType::Call:
		{
			AssembleCall((IRFunctionCall*)inst, stream);
		}
		break;
		case IRNodeType::Return:
		{
			AssembleReturn((IRReturn*)inst, stream);
		}
		break;
		case IRNodeType::ConstValue:
		{
			AssembleConstValue((IRCONSTValue*)inst, stream);
		}
		break;
		case IRNodeType::Register:
		{
			AssembleIRRegister((IRRegister*)inst, stream);
		}
		break;
		case IRNodeType::RegisterValue:
		{
			AssembleIRRegisterValue((IRRegisterValue*)inst, stream);
		}
		break;
		case IRNodeType::Data:
		{
			AssembleData((IRData*)inst, stream);
		}
		break;
		case IRNodeType::DataValue:
		{
			AssembleDataValue((IRDataValue*)inst, stream);
		}
		break;
		case IRNodeType::TypeOf:
		{
			AssembleTypeOf((IRTypeOf*)inst, stream);
		}
		break;
		case IRNodeType::TypeValue:
		{
			AssembleTypeValue((IRTypeValue*)inst, stream);
		}
		break;
		case IRNodeType::TypeInfo:
		{
			AssembleTypeInfo((IRTypeInfo*)inst, stream);
		}
		break;
		case IRNodeType::NullPtr:
		{
			SetRegisterValue(Make_Constant(0));
		}
		break;
		case IRNodeType::GlobDecl:
		{
			AssembleGlobalDecl((IRGlobalDecl*)inst, stream);
		}
		break;
		case IRNodeType::GlobAddress:
		{
			AssembleGlobalAddress((IRGlobalAddress*)inst, stream);
		}
		break;

		case IRNodeType::ADD:
		case IRNodeType::SUB:
		case IRNodeType::MUL:
		case IRNodeType::DIV:
		case IRNodeType::BitAnd:
		case IRNodeType::BitOr:
			AssembleBinOp((IRBinOp*)inst, stream);
			break;
		case IRNodeType::GreaterThan:
		case IRNodeType::LesserThan:
		case IRNodeType::Equal:
		case IRNodeType::NotEqual:
			AssembleLogicalOp((IRBinOp*)inst, stream);
			break;
		case IRNodeType::If:
			AssembleIf((IRIf*)inst, stream);
			break;
		case IRNodeType::While:
			AssembleWhile((IRWhile*)inst, stream);
			break;
		case IRNodeType::Any:
			AssembleAny((IRAny*)inst, stream);
			break;
		case IRNodeType::AnyArray:
			AssembleAnyArray((IRAnyArray*)inst, stream);
			break;
		case IRNodeType::LexicalBlock:
			AssembleLexicalBlock((IRLexBlock*)inst, stream);
			break;
		case IRNodeType::PointerCast:
		{
			SetRegisterValue(m_Data.IR_RegisterValues.at((((IRPointerCast*)inst)->PointerRegister)));
		}
		break;
		case IRNodeType::IntTrunc:
		{
			AssembleIntTruncCast((IRIntTrunc*)inst, stream);
		}
		break;
		case IRNodeType::Int2FP:
		{
			AssembleInt2FP((IRInt2FP*)inst, stream);
		}
		break;
		case IRNodeType::Ptr2IntCast: {
			auto ptr_2_int = (IRPtr2IntCast*)inst;

			if (TypeSystem::GetTypeSize(ptr_2_int->Type) < TypeSystem::GetTypeSize(TypeSystem::GetVoidPtr())) {
				AssembleIntTruncCast((IRIntTrunc*)inst, stream);
			}
		}
									break;

		default:
			GS_CORE_ASSERT(0, "Unknown IR Instruction");
			break;
		}
	}

	void X86_BackEnd::AssembleTypeInfoTable(std::string& stream)
	{

		std::vector<TypeStorage*>& UniqueTypeInfoMap = TypeSystem::GetTypeMap();

		stream += "section '.TIDB' data readable align 16\n";

		stream += "TIDB ";

		for (auto type : UniqueTypeInfoMap)
		{

			TypeStorageKind type_kind = type->Kind;

			switch (type_kind)
			{
			case TypeStorageKind::Base:
			case TypeStorageKind::Pointer:
			{

				TypeInfoFlags type_info_flags = m_Metadata->GetTypeInfoFlags(type->BaseID);

				const std::string& type_info_name = m_Metadata->GetType(type->BaseID);
				std::vector<char> type_info_name_string_vector;

				for (auto c : type_info_name)
				{
					type_info_name_string_vector.push_back(c);
				}

				u64 type_info_name_str_id = GetStringID();
				m_TypeInfoStrings.push_back({ type_info_name_str_id, type_info_name_string_vector });

				stream += "dq ";
				stream += fmt::format("t_str{}, {}, {}, 0", type_info_name_str_id, type_info_flags, TypeSystem::GetTypeSize(type));
				stream += "\n";
			}
			break;
			default:
				stream += "dq 0,0,0,0\n";
				break;
			}
		}
	}

	void X86_BackEnd::AssembleData(IRData* inst, std::vector<X86_Inst*>& stream)
	{
		m_DataStrings.push_back({ inst->ID, inst->Data });
	}

	void X86_BackEnd::AssembleTypeOf(IRTypeOf* inst, std::vector<X86_Inst*>& stream)
	{
#define TYPE_INFO_ELEMENT_SIZE 32

		X86_Inst* index_int_table = Make_Constant(TypeSystem::GetTypeInfoIndex(inst->Type) * TYPE_INFO_ELEMENT_SIZE);

		X86_Inst* t_tabel_begin = ASMA(X86_Inst());
		t_tabel_begin->type = X86_LABEL_REF;
		t_tabel_begin->as.label.name = "TIDB";

		X86_Inst* lookup = ASMA(X86_Inst());
		lookup->type = X86_CONSTANT_OFFSET;
		lookup->as.constant_offset.from = t_tabel_begin;
		lookup->as.constant_offset.offset = index_int_table;
		lookup->as.constant_offset.offset_type = X86_CONSTANT_ADD;

		auto t_pointer_register = Allocate_Register(RegisterUsage::REG_I64, GetRegisterID(), stream);

		SetRegisterValue(t_pointer_register);

		return Make_LEA(lookup, t_pointer_register, stream);
	}

	void X86_BackEnd::AssembleTypeValue(IRTypeValue* inst, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* index_int_table = Make_Constant(TypeSystem::GetTypeInfoIndex(inst->Type));
		SetRegisterValue(index_int_table);
	}

	void X86_BackEnd::AssembleTypeInfo(IRTypeInfo* inst, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* t_tabel_begin = ASMA(X86_Inst());
		t_tabel_begin->type = X86_LABEL_REF;
		t_tabel_begin->as.label.name = "TIDB";

		auto argument = GetIRRegister(inst->ArgumentRegister);

		if (argument->type == X86_CONSTANT)
		{
			X86_Inst* lookup = ASMA(X86_Inst());
			lookup->type = X86_CONSTANT_OFFSET;
			lookup->as.constant_offset.from = t_tabel_begin;
			lookup->as.constant_offset.offset = Make_Constant(*((i64*)argument->as.constant.bytes) * TYPE_INFO_ELEMENT_SIZE);
			lookup->as.constant_offset.offset_type = X86_CONSTANT_ADD;

			auto t_pointer_register = Allocate_Register(RegisterUsage::REG_I64, GetRegisterID(), stream);
			SetRegisterValue(t_pointer_register);
			Make_LEA(lookup, t_pointer_register, stream);
		}
		else
		{
			auto element_adress_register = Allocate_Register(REG_I64, GetRegisterID(), stream);

			Make_Move(argument, element_adress_register, stream, TypeSystem::GetBasic(IR_type));

			X86_Inst* multiply = ASMA(X86_Inst());
			multiply->type = X86_IMUL;
			multiply->as.bin_op.destination = element_adress_register;
			multiply->as.bin_op.value = Make_Constant(TYPE_INFO_ELEMENT_SIZE);
			stream.push_back(multiply);

			auto table_address_register = Allocate_Register(REG_I64, GetRegisterID(), stream);
			Make_LEA(Make_DeRef(t_tabel_begin), table_address_register, stream);

			X86_Inst* add = ASMA(X86_Inst());
			add->type = X86_IADD;
			add->as.bin_op.destination = element_adress_register;
			add->as.bin_op.value = table_address_register;
			stream.push_back(add);

			Free_Register(table_address_register->as.reg_alloc.register_allocation_id);

			SetRegisterValue(element_adress_register);
		}
	}

	void X86_BackEnd::AssembleDataValue(IRDataValue* inst, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* string_ref_inst = ASMA(X86_Inst());
		string_ref_inst->type = X86_DATA_STR_REF;
		string_ref_inst->as.data_str_ref.string_id = inst->DataID;

		SetRegisterValue(string_ref_inst);
	}

	void X86_BackEnd::AssembleFunction(IRFunction* func, std::vector<X86_Inst*>& stream)
	{

		FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(func->ID);

		if (func->Overload) {
			metadata = &metadata->GetOverload((TSFunc*)func->Overload);
		}

		GS_CORE_ASSERT(metadata);

		m_Metadata->m_CurrentFunction = func->ID;

		m_CurrentFunction = metadata;

		if (metadata->PolyMorphic)
			return;

		Reset_CallStackSize();
		m_Data.CurrentFunction_InputCallStackOffset = 0;
		m_Data.CurrentFunction_InputCallStackPointer = 0;

		const char* label_name;
		const char* return_label_name;

		if (metadata->Symbol.Symbol == "main")
		{
			label_name = "main";
			return_label_name = "main_return";
		}
		else
		{
			label_name = ASMA(MangleName(metadata->Symbol.Symbol, (TSFunc*)metadata->Signature))->c_str();
			return_label_name = ASMA(std::string(label_name) + "_return")->c_str();
		}

		std::vector<X86_Inst*> body_stream;

		bool has_return_value = TypeSystem::GetVoid() != metadata->ReturnType;
		auto retunr_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);

		X86_Inst* return_label = ASMA(X86_Inst());

		if (has_return_value)
		{

			if (retunr_type_size > 8)
			{
				CurrentFunctionReturnAllocation = GetReturnLocation(metadata->ReturnType, stream);
			}
			else
			{
				CurrentReturnTarget = AllocateStack(metadata->ReturnType);
			}

			return_label->type = X86_LABEL;
			return_label->as.label.name = return_label_name;
		}

		X86_Inst* return_label_ref = ASMA(X86_Inst());
		return_label_ref->type = X86_LABEL_REF;
		return_label_ref->as.label.name = return_label_name;

		ReturnJmpTarget = return_label_ref;

		m_Data.CurrentFunction_InputCallStackOffset += 8; // size of rbp

		CurrentFunctionArgumentAllocations = AllocateArgumentLocations((TSFunc*)metadata->Signature, stream, GetArgument_Location_Dir::ARG_DIR_IN);

		u64 index = 0;
		for (auto i : func->Instructions)
		{
			Assemble(i, body_stream);
			index++;
		}

		CurrentFunctionArgumentAllocations = {};
		CurrentFunctionReturnAllocation = nullptr;

		X86_Inst* label = ASMA(X86_Inst());
		label->type = X86_LABEL;
		label->as.label.name = label_name;
		stream.push_back(label);

		X86_Inst stack_base_ptr_save;
		stack_base_ptr_save.type = X86_PUSH;
		stack_base_ptr_save.as.push.source = Make_Register(RBP);
		stream.push_back(ASMA(stack_base_ptr_save));

		X86_Inst s_ptr_move = {};
		s_ptr_move.type = X86_MOV;
		s_ptr_move.as.move.source = Make_Register(RSP);
		s_ptr_move.as.move.destination = Make_Register(RBP);
		stream.push_back(ASMA(s_ptr_move));

		const u64 stack_alignment = 16;

		u64 aligned_frame_size = m_Data.CurrentFunction_StackFrameSize;
		aligned_frame_size += m_Data.CurrentFunction_CallStackSize ? m_Data.CurrentFunction_CallStackSize : 32;

		aligned_frame_size += (stack_alignment - (aligned_frame_size % stack_alignment)) % stack_alignment;

		// if (aligned_frame_size) {
		X86_Inst prologue;
		prologue.type = X86_ISUB;
		prologue.as.bin_op.destination = Make_Register(RSP);
		prologue.as.bin_op.value = Make_Constant((i64)aligned_frame_size);
		stream.push_back(ASMA(prologue));
		//}

		for (auto body_inst : body_stream)
		{
			stream.push_back(body_inst);
		}

		if (has_return_value)
		{
			stream.push_back(return_label);

			if (retunr_type_size > 8)
			{
			}
			else
			{
				X86_Inst* return_move = ASMA(X86_Inst());
				return_move->type = X86_MOV;
				return_move->as.move.destination = GetReturnLocation(metadata->ReturnType, stream);
				return_move->as.move.source = CurrentReturnTarget;
				stream.push_back(return_move);
			}
		}

		// if (aligned_frame_size) {
		X86_Inst epilogue;
		epilogue.type = X86_IADD;
		epilogue.as.bin_op.destination = Make_Register(RSP);
		epilogue.as.bin_op.value = Make_Constant((i64)aligned_frame_size);
		stream.push_back(ASMA(epilogue));
		//}

		X86_Inst stack_base_ptr_pull;
		stack_base_ptr_pull.type = X86_POP;
		stack_base_ptr_pull.as.push.source = Make_Register(RBP);
		stream.push_back(ASMA(stack_base_ptr_pull));

		X86_Inst ret;
		ret.type = X86_RET;

		stream.push_back(ASMA(ret));

		m_Data.CurrentFunction_StackFrameSize = 0;
		CalledVariadicFunction = false;
		Free_All_Register();
		CurrentReturnTarget = nullptr;
		ReturnJmpTarget = nullptr;
		FunctionCounter++;
		LabelCounter++;
	}

	void X86_BackEnd::AssembleConstValue(IRCONSTValue* inst, std::vector<X86_Inst*>& stream)
	{
		if (m_Metadata->GetTypeFlags(inst->Type) & FLAG_FLOATING_TYPE)
		{
			double data = 0;
			memcpy(&data, &inst->Data, sizeof(double));
			SetRegisterValue(Make_Constant_Float(data));
		}
		else
		{
			i64 data = 0;
			memcpy(&data, &inst->Data, sizeof(i64));
			SetRegisterValue(Make_Constant(data));
		}
	}

	void X86_BackEnd::AssemblyPolyMorphicFunctions(std::vector<X86_Inst*>& stream)
	{
		for (const auto& [id, func] : m_Metadata->m_Functions)
		{
			if (func.PolyMorphic)
			{
				for (const auto& [overload_id, instance] : func.PolyMorphicInstantiations)
				{
					AssembleFunction(instance, stream);
				}
			}
		}
	}

	void X86_BackEnd::AssembleAlloca(IRAlloca* inst, std::vector<X86_Inst*>& stream)
	{
		u64 allocation_size = TypeSystem::GetTypeSize(inst->Type);

		if (allocation_size < 4)
		{
			allocation_size = 4;
		}

		m_Data.CurrentFunction_StackFrameSize += allocation_size;

		X86_Inst stack_frame_offset = {};
		stack_frame_offset.type = X86_CONSTANT_OFFSET;
		stack_frame_offset.as.constant_offset.from = Make_Register(RBP);
		stack_frame_offset.as.constant_offset.offset = Make_Constant((i64)m_Data.CurrentFunction_StackFrameSize);
		stack_frame_offset.as.constant_offset.size = InWords(inst->Type);

		if (inst->VarMetadata)
		{
			stack_frame_offset.comment = inst->VarMetadata->Name.Symbol.c_str();
		}
		else
		{
			stack_frame_offset.comment = "unknown variable name alloca";
		}

		SetRegisterValue(ASMA(stack_frame_offset));
	}

	void X86_BackEnd::AssembleGlobalDecl(IRGlobalDecl* inst, std::vector<X86_Inst*>& stream)
	{
		GlobalUnInitializedVariables.push_back({ inst->GlobID, TypeSystem::GetTypeSize(inst->Type) });
	}

	void X86_BackEnd::AssembleGlobalAddress(IRGlobalAddress* inst, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* glob = ASMA(X86_Inst());
		glob->type = X86_GLOB_DE_REF;
		glob->as.global.global_id = inst->GlobID;
		glob->as.global.size = InWords(m_Metadata->GetVariableMetadata(inst->GlobID)->Tipe);

		SetRegisterValue(glob);
	}

	void X86_BackEnd::AssembleStore(IRStore* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(inst->Data->GetType() == IRNodeType::RegisterValue);

		GS_CORE_ASSERT(inst->Type);
		u64 stored_size = TypeSystem::GetTypeSize(inst->Type);

		if (!stored_size)
		{
			SetRegisterValue(ASMA(X86_Inst()));
			return;
		}

		IRRegisterValue* data_as_register_ref = ((IRRegisterValue*)inst->Data);

		if (stored_size > 8)
		{

			if (IsRegisterValue)
			{
				SetRegisterValue(ASMA(X86_Inst()));
			}

			auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
			auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

			auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_OUT);
			auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream);

			auto src = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_OUT);
			auto source = GetIRRegister(((IRRegisterValue*)inst->Data)->RegisterID);

			if (source->type == X86_CONSTANT_OFFSET)
			{

				X86_Inst* lea = ASMA(X86_Inst());
				lea->type = X86_LEA;
				lea->as.lea.source = source;
				lea->as.lea.destination = src;
				stream.push_back(lea);
			}
			else
			{
				Make_Move(source, src, stream, void_ptr_Ty);
			}

			auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_OUT);
			auto destination = GetIRRegister(inst->AddressRegister);

			if (destination->type == X86_CONSTANT_OFFSET)
			{

				X86_Inst* lea = ASMA(X86_Inst());
				lea->type = X86_LEA;
				lea->as.lea.source = destination;
				lea->as.lea.destination = dest;
				stream.push_back(lea);
			}
			else
			{
				Make_Move(destination, dest, stream, void_ptr_Ty);
			}

			Make_Move(Make_Constant(stored_size), sz, stream, void_ptr_Ty);

			X86_Inst* call = ASMA(X86_Inst());
			call->type = X86_CALL;
			call->as.call.What = MemCpy;
			stream.push_back(call);

			Free_Register(dest->as.reg_alloc.register_allocation_id);
			Free_Register(src->as.reg_alloc.register_allocation_id);
			Free_Register(sz->as.reg_alloc.register_allocation_id);
			Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);
		}
		else
		{

			if (IsRegisterValue)
			{
				SetRegisterValue(ASMA(X86_Inst()));
			}

			auto location = GetIRRegister(inst->AddressRegister);
			auto data_location = GetIRRegister(data_as_register_ref->RegisterID);

			u64 intermediate_register_id = -1;

			if (data_location->type == X86_DATA_STR_REF)
			{
				intermediate_register_id = GetRegisterID();
				auto intermediate_register = Allocate_Register(REG_I64, intermediate_register_id, stream);
				data_location = Make_Move(data_location, intermediate_register, stream, inst->Type);
			}

			if (location->type == X86_CONSTANT_OFFSET || location->type == X86_GLOB_DE_REF)
			{
				auto data_ir_register = m_Metadata->GetRegister(data_as_register_ref->RegisterID);
				//auto data_ir_register_type = data_ir_register->Value->GetType();;

				X86_ASM move_type = X86_MOV;

				if (data_location->type != X86_CONSTANT && data_location->type != X86_CONSTANT_FLOAT)
				{
					move_type = MovByType(inst->Type);
				}

				X86_Inst move = {};
				move.type = move_type;
				move.as.move.source = data_location;
				move.as.move.destination = location;
				move.comment = "store";
				stream.push_back(ASMA(move));
				//}
			}
			else
			{

				X86_Inst dereference = {};
				dereference.type = X86_DE_REF;
				dereference.as.de_ref.what = location;
				dereference.as.de_ref.size = InWords(stored_size);

				X86_Inst move = {};
				move.type = MovByType(inst->Type);
				move.as.move.source = data_location;
				move.as.move.destination = ASMA(dereference);
				move.comment = "store";
				stream.push_back(ASMA(move));
			}

			if (intermediate_register_id != -1)
			{
				Free_Register(intermediate_register_id);
			}
		}
	}

	void X86_BackEnd::AssembleLoad(IRLoad* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(inst->Type);
		u64 load_size = TypeSystem::GetTypeSize(inst->Type);

		if (!load_size)
		{
			SetRegisterValue(ASMA(X86_Inst()));
			return;
		}

		auto value_ptr = GetIRRegister(inst->AddressRegister);
		auto loaded_value_type = inst->Type;

		if (load_size > 8)
		{

			// we are taking advantage of the fact that register cannot be modified
			// so we just need to take the ptr no need for a useless copy

			SetRegisterValue(value_ptr);
		}
		else
		{

			auto loaded_value_location = Allocate_Register(RegisterUsageByType(inst->Type), GetRegisterID(), stream);
			SetRegisterValue(loaded_value_location);

			if (value_ptr->type == X86_CONSTANT_OFFSET || value_ptr->type == X86_GLOB_DE_REF)
			{
				X86_Inst move = {};
				move.type = MovByType(inst->Type);
				move.as.move.source = value_ptr;
				move.as.move.destination = loaded_value_location;
				move.comment = "load";
				stream.push_back(ASMA(move));
			}
			else
			{

				X86_Inst dereference = {};
				dereference.type = X86_DE_REF;
				dereference.as.de_ref.what = value_ptr;
				dereference.as.de_ref.size = InWords(loaded_value_type);

				X86_Inst move = {};
				move.type = MovByType(inst->Type);
				move.as.move.source = ASMA(dereference);
				move.as.move.destination = loaded_value_location;
				move.comment = "load";
				stream.push_back(ASMA(move));
			}
		}
	}

	void X86_BackEnd::AssembleMemberAccess(IRMemberAccess* member_access, std::vector<X86_Inst*>& stream)
	{
		const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(member_access->StructID);
		GS_CORE_ASSERT(struct_metadata);
		GS_CORE_ASSERT(member_access->MemberID < struct_metadata->Members.size());

		const MemberMetadata& member = struct_metadata->Members[member_access->MemberID];

		X86_Inst* struct_location = GetIRRegister(member_access->ObjectRegister, false);

		RegisterFreeList free_list = {};

		if (member_access->ReferenceAccess)
		{

			if (struct_location->type != X86_REG_ALLOC && struct_location->type != X86_REG_NAME)
			{
				auto tmp_ptr_reg_id = GetRegisterID();
				free_list.Add(tmp_ptr_reg_id);
				auto tmp_ptr_reg = Allocate_Register(RegisterUsage::REG_I64, tmp_ptr_reg_id, stream, false);
				Make_Move(struct_location, tmp_ptr_reg, stream, 8);
				struct_location = tmp_ptr_reg;
			}

			X86_Inst* member_location = ASMA(X86_Inst());
			member_location->type = X86_CONSTANT_OFFSET;
			member_location->as.constant_offset.from = struct_location;
			member_location->as.constant_offset.offset = Make_Constant(member.Offset);
			member_location->as.constant_offset.offset_type = X86_CONSTANT_ADD;
			member_location->as.constant_offset.size = InWords(member.Type);

			member_location->comment = ASMA("." + member.Name.Symbol)->c_str();

			SetRegisterValue(member_location);
		}
		else
		{
			if (struct_location->type == X86_GLOB_DE_REF)
			{
				X86_Inst* glob_location = ASMA(X86_Inst());
				glob_location->type = X86_LABEL_REF;
				glob_location->as.label.name = ASMA(fmt::format("glb{}", struct_location->as.global.global_id - 512000))->c_str();

				X86_Inst* member_location = ASMA(X86_Inst());
				member_location->type = X86_CONSTANT_OFFSET;
				member_location->as.constant_offset.offset = Make_Constant(member.Offset);
				member_location->as.constant_offset.from = glob_location;
				member_location->as.constant_offset.size = InWords(member.Type);
				member_location->as.constant_offset.offset_type = X86_CONSTANT_ADD;

				member_location->comment = ASMA("." + member.Name.Symbol)->c_str();

				SetRegisterValue(member_location);
			}
			else if (struct_location->type == X86_CONSTANT_OFFSET)
			{

				X86_Inst* member_location = ASMA(X86_Inst(*struct_location));
				member_location->as.constant_offset.offset = ASMA(*member_location->as.constant_offset.offset);
				member_location->as.constant_offset.from = ASMA(*member_location->as.constant_offset.from);
				member_location->as.constant_offset.size = InWords(member.Type);

				i64& offset = *(i64*)(&member_location->as.constant_offset.offset->as.constant.bytes);
				offset -= member.Offset;

				member_location->comment = ASMA("." + member.Name.Symbol)->c_str();

				SetRegisterValue(member_location);
			}
			else if (struct_location->type == X86_REG_ALLOC || struct_location->type == X86_REG_NAME)
			{

				X86_Inst* member_location = ASMA(X86_Inst());
				member_location->type = X86_CONSTANT_OFFSET;
				member_location->as.constant_offset.from = struct_location;
				member_location->as.constant_offset.offset = Make_Constant(member.Offset);
				member_location->as.constant_offset.offset_type = X86_CONSTANT_ADD;
				//member_location->as.constant_offset.size = InWords(member.Type);

				member_location->comment = ASMA("." + member.Name.Symbol)->c_str();

				SetRegisterValue(member_location);
			}
			else
			{
				GS_CORE_ASSERT(0);
			}
		}

		if (struct_location->type == X86_REG_ALLOC)
		{
			free_list.Add(struct_location->as.reg_alloc.register_allocation_id);
		}

		SetRegisterFreeList(CurrentRegister->ID, free_list);
	}

	void X86_BackEnd::AssembleArrayAccess(IRArrayAccess* array_access, std::vector<X86_Inst*>& stream)
	{
		u64 element_size = TypeSystem::GetTypeSize(array_access->Type);
		auto element_size_constant = Make_Constant((i64)element_size);

		if (element_size <= 8)
		{
			auto index_rgister = GetIRRegister(array_access->ElementIndexRegister, false);
			auto address_rgister = GetIRRegister(array_access->ArrayAddress, false);

			auto index_type_size = TypeSystem::GetTypeSize(m_Data.IR_RegisterTypes.at(array_access->ElementIndexRegister));

			GS_CORE_ASSERT(index_type_size <= 8);

			if (index_type_size != 8 && index_rgister->type != X86_CONSTANT)
			{
				Free_Register(index_rgister->as.reg_alloc.register_allocation_id);
				index_rgister = Register_Sext(index_rgister, 8, stream, false);
			}

			X86_Inst* constant_mul = ASMA(X86_Inst());
			constant_mul->type = X86_ADDR_MUL;
			constant_mul->as.const_binop.a = element_size_constant;
			constant_mul->as.const_binop.b = index_rgister;

			X86_Inst* constant_offset = ASMA(X86_Inst());
			constant_offset->type = X86_CONSTANT_OFFSET;
			constant_offset->as.constant_offset.offset_type = X86_CONSTANT_ADD;
			constant_offset->as.constant_offset.from = address_rgister;
			constant_offset->as.constant_offset.offset = constant_mul;
			constant_offset->as.constant_offset.size = InWords(element_size);

			SetRegisterValue(constant_offset);

			RegisterFreeList reg_free_list;
			reg_free_list.count = 1;

			reg_free_list.free_list[0] = address_rgister->as.reg_alloc.register_allocation_id;

			if (index_rgister->type == X86_REG_ALLOC)
			{
				reg_free_list.free_list[1] = index_rgister->as.reg_alloc.register_allocation_id;
				reg_free_list.count++;
			}

			SetRegisterFreeList(CurrentRegister->ID, reg_free_list);
		}
		else
		{

			auto element_adress_register = Allocate_Register(REG_I64, GetRegisterID(), stream);

			auto index_rgister = GetIRRegister(array_access->ElementIndexRegister);
			Make_Move(index_rgister, element_adress_register, stream, array_access->Type);

			X86_Inst* multiply = ASMA(X86_Inst());
			multiply->type = X86_IMUL;
			multiply->as.bin_op.destination = element_adress_register;
			multiply->as.bin_op.value = element_size_constant;
			stream.push_back(multiply);

			auto address_register = GetIRRegister(array_access->ArrayAddress);

			X86_Inst* add = ASMA(X86_Inst());
			add->type = X86_IADD;
			add->as.bin_op.destination = element_adress_register;
			add->as.bin_op.value = address_register;
			stream.push_back(add);

			SetRegisterValue(element_adress_register);
		}
	}

	void X86_BackEnd::AssembleArgument(IRArgumentAllocation* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(m_CurrentFunction);

		auto argument_type = m_CurrentFunction->Arguments[inst->ArgumentIndex].Type;
		GS_CORE_ASSERT(argument_type);

		auto& argument_input_location = CurrentFunctionArgumentAllocations.Locations[inst->ArgumentIndex];

		auto argument_size = TypeSystem::GetTypeSize(argument_type);

		if (argument_input_location.LocationType != ARG_LOC_REGISTER)
		{

			if (argument_input_location.LocationType == ARG_LOC_REGISTER_PTR || argument_input_location.LocationType == ARG_LOC_PTR_IN_STACK)
			{

				auto argument_store_location = AllocateStack(argument_type);
				SetRegisterValue(argument_store_location);

				auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
				auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

				auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream);
				auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_IN);
				auto source = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_IN);
				auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_IN);

				Make_Move(argument_input_location.Location, source, stream, void_ptr_Ty);

				X86_Inst* lea = ASMA(X86_Inst());
				lea->type = X86_LEA;
				lea->as.lea.source = argument_store_location;
				lea->as.lea.destination = dest;
				stream.push_back(lea);

				Make_Move(Make_Constant(argument_size), sz, stream, void_ptr_Ty);

				X86_Inst* call = ASMA(X86_Inst());
				call->type = X86_CALL;
				call->as.call.What = MemCpy;
				stream.push_back(call);

				Free_Register(dest->as.reg_alloc.register_allocation_id);
				Free_Register(source->as.reg_alloc.register_allocation_id);
				Free_Register(sz->as.reg_alloc.register_allocation_id);
				Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);

				if (argument_input_location.LocationType == ARG_LOC_REGISTER_PTR)
				{
					Free_Register(argument_input_location.Location->as.reg_alloc.register_allocation_id);
				}
			}
			else if (argument_input_location.LocationType == ARG_LOC_STACK)
			{
				auto argument_store_location = AllocateStack(argument_type);
				SetRegisterValue(argument_store_location);
				Make_Move(argument_input_location.Location, argument_store_location, stream, argument_type, "store argument to stack");
			}
		}
		else
		{

			auto argument_store_location = AllocateStack(argument_type);
			SetRegisterValue(argument_store_location);
			Make_Move(argument_input_location.Location, argument_store_location, stream, argument_type, "store argument to stack");

			Free_Register(argument_input_location.Location->as.reg_alloc.register_allocation_id);
		}
	}

	void X86_BackEnd::AssembleCall(IRFunctionCall* inst, std::vector<X86_Inst*>& stream)
	{
		const FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(inst->FuncID);
		GS_CORE_ASSERT(metadata);

		X86_Inst* label_ref = m_Data.IR_FunctionLabels.at(inst->FuncID);

		if (inst->Overload) {
			metadata = &metadata->GetOverload((TSFunc*)inst->Overload);
			label_ref = m_Data.IR_FunctionOverloadsLabels.at(inst->FuncID).at(inst->Overload);
		}

		u64 return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);

		if (metadata->Variadic)
		{
			CalledVariadicFunction = true;
		}

		Reset_CallStackPointer();

		auto return_location = GetReturnLocation(metadata->ReturnType, stream);

		X86_Inst* register_value = return_location;

		if (return_type_size > 8)
		{

			X86_Inst lea = {};
			lea.type = X86_LEA;
			lea.as.lea.source = AllocateStack(metadata->ReturnType);
			lea.as.lea.destination = return_location;
			lea.comment = "return data via pointer";
			stream.push_back(ASMA(lea));

			register_value = lea.as.lea.source;

			Free_Register(return_location->as.reg_alloc.register_allocation_id);
		}

		TSFunc* signature = nullptr;

		if (metadata->Variadic)
		{
			std::vector<TypeStorage*> signature_argument_types;
			for (size_t i = 0; i < inst->Arguments.size(); i++)
			{
				signature_argument_types.push_back(m_Data.IR_RegisterTypes.at(((IRRegisterValue*)inst->Arguments[i])->RegisterID));
			}
			signature = (TSFunc*)TypeSystem::GetFunction(signature_argument_types, TypeSystem::GetVoid());
		}
		else
		{
			signature = (TSFunc*)metadata->Signature;
		}

		auto argument_allocation = AllocateArgumentLocations(signature, stream, GetArgument_Location_Dir::ARG_DIR_OUT, metadata->Variadic);

		for (size_t i = 0; i < inst->Arguments.size(); i++)
		{
			auto arg = (IRRegisterValue*)inst->Arguments[i];
			GS_CORE_ASSERT(arg->GetType() == IRNodeType::RegisterValue);

			auto arg_type = GetIRRegisterType(arg->RegisterID);
			u64 arg_type_size = TypeSystem::GetTypeSize(arg_type);
			auto arg_type_flags = TypeSystem::GetTypeFlags(arg_type);

			auto arg_source = GetIRRegister(arg->RegisterID);
			GS_CORE_ASSERT(arg_source);

			auto& allocation = argument_allocation.Locations[i];

			if (allocation.LocationType == ARG_LOC_REGISTER)
			{

				if (arg_type_flags & FLAG_FLOATING_TYPE)
				{

					GS_CORE_ASSERT(allocation.SecondaryLocation);

					arg_source = Make_Move(arg_source, allocation.Location, stream, arg_type, "intermediate move");

					if (arg_type_size == 4 && metadata->Variadic)
					{
						X86_Inst* fp_ext = ASMA(X86_Inst());
						fp_ext->type = X86_CVTSS2SD;
						fp_ext->as.bin_op.value = arg_source;
						fp_ext->as.bin_op.destination = allocation.Location;

						stream.push_back(fp_ext);
					}

					auto needed_argument_type = arg_type;

					if (arg_type_size == 4 && metadata->Variadic)
					{
						needed_argument_type = TypeSystem::GetBasic(IR_f64);
					}

					auto needed_argument_type_size = TypeSystem::GetTypeSize(needed_argument_type);

					X86_Inst* move_to_secondary = ASMA(X86_Inst());
					move_to_secondary->type = X86_MOVQ;
					move_to_secondary->as.bin_op.value = allocation.Location;
					move_to_secondary->as.bin_op.destination = allocation.SecondaryLocation;

					if (needed_argument_type_size < 8)
					{
						move_to_secondary->type = X86_MOVD;
					}

					move_to_secondary->comment = "copy";

					stream.push_back(move_to_secondary);

					Free_Register(allocation.SecondaryLocation->as.reg_alloc.register_allocation_id);
				}
				else
				{
					Make_Move(arg_source, allocation.Location, stream, arg_type, "argument move");
				}

				Free_Register(allocation.Location->as.reg_alloc.register_allocation_id);
			}
			else if (allocation.LocationType == ARG_LOC_STACK)
			{
				if (arg_type_flags & FLAG_FLOATING_TYPE && arg_type_size == 4 && metadata->Variadic)
				{

					auto intermediate_register_id = GetRegisterID();
					auto intermediate_register = Allocate_Register(REG_F64, intermediate_register_id, stream);

					if (arg_source->type != X86_REG_ALLOC)
					{
						arg_source = Make_Move(arg_source, intermediate_register, stream, arg_type, "intermediate move");
					}

					X86_Inst* fp_ext = ASMA(X86_Inst());
					fp_ext->type = X86_CVTSS2SD;
					fp_ext->as.bin_op.value = arg_source;
					fp_ext->as.bin_op.destination = intermediate_register;

					stream.push_back(fp_ext);

					auto needed_argument_type = arg_type;

					if (arg_type_size == 4 && metadata->Variadic)
					{
						needed_argument_type = TypeSystem::GetBasic(IR_f64);
					}

					auto needed_argument_type_size = TypeSystem::GetTypeSize(needed_argument_type);

					Make_Move(intermediate_register, allocation.Location, stream, needed_argument_type, "intermediate move");

					Free_Register(intermediate_register_id);
				}
				else
				{
					Make_Move(arg_source, allocation.Location, stream, arg_type, "argument move");
				}
			}
			else if (allocation.LocationType == ARG_LOC_PTR_IN_STACK)
			{

				u64 intermediate_register_id = GetRegisterID();
				auto intermediate_register = Allocate_Register(REG_I64, intermediate_register_id, stream);

				X86_Inst lea = {};
				lea.type = X86_LEA;
				lea.as.lea.source = arg_source;
				lea.as.lea.destination = intermediate_register;
				stream.push_back(ASMA(lea));

				Make_Move(intermediate_register, allocation.Location, stream, arg_type, "intermediate move");
				Free_Register(intermediate_register_id);
			}
			else if (allocation.LocationType == ARG_LOC_REGISTER_PTR)
			{
				X86_Inst lea = {};
				lea.type = X86_LEA;
				lea.as.lea.source = arg_source;
				lea.as.lea.destination = allocation.Location;
				stream.push_back(ASMA(lea));

				Free_Register(allocation.Location->as.reg_alloc.register_allocation_id);
			}
		}

		X86_Inst call = {};
		call.type = X86_CALL;
		call.as.call.What = label_ref;

		stream.push_back(ASMA(call));

		if (IsRegisterValue)
		{
			SetRegisterValue(register_value);
		}
		else
		{
			Free_Register(return_location->as.reg_alloc.register_allocation_id);
		}
	}

	void X86_BackEnd::AssembleReturn(IRReturn* inst, std::vector<X86_Inst*>& stream)
	{

		u64 return_type_size = TypeSystem::GetTypeSize(inst->Type);

		GS_CORE_ASSERT(inst->Value);
		GS_CORE_ASSERT(inst->Value->GetType() == IRNodeType::RegisterValue);

		if (return_type_size > 8)
		{

			GS_CORE_ASSERT(CurrentFunctionReturnAllocation);

			auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
			auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

			auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_OUT);
			auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream);

			auto src = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_OUT);
			auto source = GetIRRegister(((IRRegisterValue*)inst->Value)->RegisterID);

			if (source->type == X86_CONSTANT_OFFSET)
			{

				X86_Inst* lea = ASMA(X86_Inst());
				lea->type = X86_LEA;
				lea->as.lea.source = source;
				lea->as.lea.destination = src;
				stream.push_back(lea);
			}
			else
			{
				Make_Move(source, src, stream, void_ptr_Ty);
			}

			auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_OUT);
			auto destination = CurrentFunctionReturnAllocation;

			Make_Move(destination, dest, stream, void_ptr_Ty);

			Make_Move(Make_Constant(return_type_size), sz, stream, void_ptr_Ty);

			X86_Inst* call = ASMA(X86_Inst());
			call->type = X86_CALL;
			call->as.call.What = MemCpy;
			stream.push_back(call);

			Free_Register(dest->as.reg_alloc.register_allocation_id);
			Free_Register(src->as.reg_alloc.register_allocation_id);
			Free_Register(sz->as.reg_alloc.register_allocation_id);
			Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);
		}
		else
		{
			Make_Move(GetIRRegister(((IRRegisterValue*)inst->Value)->RegisterID), CurrentReturnTarget, stream, 4);
		}

		X86_Inst jmp = {};
		jmp.type = X86_JMP;
		jmp.as.jmp.Where = ReturnJmpTarget;
		stream.push_back(ASMA(jmp));
	}

	void X86_BackEnd::AssembleBinOp(IRBinOp* inst, std::vector<X86_Inst*>& stream)
	{
		IRNodeType node_type = inst->GetType();

		static std::map<IRNodeType, X86_ASM> op_lookup = {
			{IRNodeType::ADD, X86_IADD},
			{IRNodeType::SUB, X86_ISUB},

			{IRNodeType::MUL, X86_IMUL},
			{IRNodeType::DIV, X86_IDIV},

			{IRNodeType::BitAnd, X86_AND},
			{IRNodeType::BitOr, X86_OR},
		};

		auto temprary_move_a = GetIRRegister(inst->RegisterA->RegisterID);
		auto dest_inst = Allocate_Register(RegisterUsageByType(inst->Type), GetRegisterID(), stream);

		SetRegisterValue(dest_inst);

		bool move_same = AreEqual(temprary_move_a, dest_inst) && UselessMoveElimination;

		if (!move_same)
		{
			X86_Inst move = {};
			move.type = X86_MOV;
			move.as.move.source = temprary_move_a;
			move.as.move.destination = dest_inst;
			move.comment = "move first argument argument to add";
			stream.push_back(ASMA(move));
		}

		X86_Inst add;
		add.type = op_lookup.at(node_type);
		add.as.bin_op.destination = dest_inst;
		add.as.bin_op.value = GetIRRegister(inst->RegisterB->RegisterID);
		stream.push_back(ASMA(add));

		return;
	}

	void X86_BackEnd::AssembleLogicalOp(IRBinOp* inst, std::vector<X86_Inst*>& stream)
	{
		bool is_condition = false;

		if (IsRegisterValue)
		{
			is_condition = CurrentRegister->IsCondition;
		}

		GS_CORE_ASSERT(inst->Type);
		auto compare_type = inst->Type;
		GS_CORE_ASSERT(compare_type);
		auto compare_size = TypeSystem::GetTypeSize(compare_type);
		GS_CORE_ASSERT(compare_size);

		auto compared_register = GetRegisterID();

		auto register_a = GetIRRegister(inst->RegisterA->RegisterID);
		auto destination_register = Allocate_Register(RegisterUsageByType(compare_type), compared_register, stream);

		Make_Move(register_a, destination_register, stream, compare_size, "compare move");

		X86_Inst* compare = ASMA(X86_Inst());
		compare->type = X86_CMP;
		compare->as.cmp.a = destination_register;
		compare->as.cmp.b = GetIRRegister(inst->RegisterB->RegisterID);
		Free_Register(destination_register->as.reg_alloc.register_allocation_id);
		stream.push_back(compare);

		if (is_condition)
		{
			SetRegisterValue(ASMA(X86_Inst()));
			// this means that this a if or while condition no need to store anything setl will become jl
		}
		else
		{

			auto b8_register = Allocate_Register(REG_I8, GetRegisterID(), stream);

			X86_Inst* conditional_set = ASMA(X86_Inst());
			conditional_set->type = op_to_set_inst.at(inst->GetType());
			conditional_set->as.cond_set.destination = b8_register;
			stream.push_back(conditional_set);

			SetRegisterValue(b8_register);
		}
	}

	void X86_BackEnd::AssembleIf(IRIf* ir_if, std::vector<X86_Inst*>& stream)
	{
		auto cont_label_name = GetContLabelName();

		X86_Inst* continue_label_ref = ASMA(X86_Inst());
		continue_label_ref->type = X86_LABEL_REF;
		continue_label_ref->as.label.name = cont_label_name;

		auto condition_register = GetIRRegister(ir_if->ConditionRegister);
		auto condition_ir_register = m_Metadata->GetRegister(ir_if->ConditionRegister);

		auto condition_register_value_ir_type = condition_ir_register->Value->GetType();

		if (!condition_ir_register->IsCondition)
		{
			X86_Inst* compare = ASMA(X86_Inst());
			compare->type = X86_CMP;
			compare->as.cmp.a = condition_register;
			compare->as.cmp.b = Make_Constant(0);
			stream.push_back(compare);

			X86_Inst* cond_jump = ASMA(X86_Inst());
			cond_jump->type = X86_JMPLE;
			cond_jump->as.jmp.Where = continue_label_ref;
			stream.push_back(cond_jump);
		}
		else
		{
			X86_Inst* cond_jump = ASMA(X86_Inst());
			cond_jump->type = elbat_cigol_detrevni.at(condition_register_value_ir_type);
			cond_jump->as.jmp.Where = continue_label_ref;
			stream.push_back(cond_jump);
		}

		for (auto inst : ir_if->Instructions)
		{
			Assemble(inst, stream);
		}

		X86_Inst* continue_label = ASMA(X86_Inst());
		continue_label->type = X86_LABEL;
		continue_label->as.label.name = cont_label_name;
		stream.push_back(continue_label);
	}

	void X86_BackEnd::AssembleWhile(IRWhile* ir_while, std::vector<X86_Inst*>& stream)
	{
		auto condition_label_name = GetLoopLabelName();
		auto cont_label_name = GetContLabelName();

		// after loop
		X86_Inst* continue_label_ref = ASMA(X86_Inst());
		continue_label_ref->type = X86_LABEL_REF;
		continue_label_ref->as.label.name = cont_label_name;

		// loop start
		X86_Inst* condition_label = ASMA(X86_Inst());
		condition_label->type = X86_LABEL;
		condition_label->as.label.name = condition_label_name;
		stream.push_back(condition_label);

		X86_Inst* condition_label_ref = ASMA(X86_Inst());
		condition_label_ref->type = X86_LABEL_REF;
		condition_label_ref->as.label.name = condition_label_name;

		for (auto inst : ir_while->ConditionBlock)
		{
			Assemble(inst, stream);
		}

		auto condition_ir_register = m_Metadata->GetRegister(ir_while->ConditionRegisterID);
		auto condition_register_value_ir_type = condition_ir_register->Value->GetType();

		if (!condition_ir_register->IsCondition)
		{

			auto condition_register = GetIRRegister(ir_while->ConditionRegisterID);

			if (condition_register->type != X86_REG_ALLOC || condition_register->type != X86_REG_NAME)
			{
				auto intermeddiate_temporary_reg_id = GetRegisterID();
				auto intermeddiate_temporary_reg = Allocate_Register(REG_I8, intermeddiate_temporary_reg_id, stream);

				Make_Move(condition_register, intermeddiate_temporary_reg, stream, TypeSystem::GetBasic(IR_u8));
				Free_Register(intermeddiate_temporary_reg_id);
				condition_register = intermeddiate_temporary_reg;
			}

			X86_Inst* compare = ASMA(X86_Inst());
			compare->type = X86_CMP;
			compare->as.cmp.a = condition_register;
			compare->as.cmp.b = Make_Constant(0);
			stream.push_back(compare);

			X86_Inst* cond_jump = ASMA(X86_Inst());
			cond_jump->type = X86_JMPLE;
			cond_jump->as.jmp.Where = continue_label_ref;
			stream.push_back(cond_jump);
		}
		else
		{
			X86_Inst* cond_jump = ASMA(X86_Inst());
			cond_jump->type = elbat_cigol_detrevni.at(condition_register_value_ir_type);
			cond_jump->as.jmp.Where = continue_label_ref;
			stream.push_back(cond_jump);
		}

		for (auto inst : ir_while->Instructions)
		{
			Assemble(inst, stream);
		}

		X86_Inst* jump = ASMA(X86_Inst());
		jump->type = X86_JMP;
		jump->as.jmp.Where = condition_label_ref;
		stream.push_back(jump);

		X86_Inst* continue_label = ASMA(X86_Inst());
		continue_label->type = X86_LABEL;
		continue_label->as.label.name = cont_label_name;
		stream.push_back(continue_label);
	}

	void X86_BackEnd::AssembleAny(IRAny* ir_any, std::vector<X86_Inst*>& stream)
	{
		const StructMetadata* any_struct = m_Metadata->GetStructFromType(IR_any);
		GS_CORE_ASSERT(any_struct);
		X86_Inst* any_alloca = AllocateStack(ir_any->Type);
		GS_CORE_ASSERT(any_alloca);

		SetRegisterValue(any_alloca);

		{
			X86_Inst* data_member = ASMA(*any_alloca);

			data_member->as.constant_offset.offset = ASMA(*data_member->as.constant_offset.offset);
			data_member->as.constant_offset.from = ASMA(*data_member->as.constant_offset.from);
			data_member->as.constant_offset.size = InWords(8);

			i64& data_member_offset = *(i64*)(&data_member->as.constant_offset.offset->as.constant.bytes);
			data_member_offset -= any_struct->Members[any_struct->FindMember("data")].Offset;

			Make_Move(GetIRRegister(ir_any->DataRegister), data_member, stream, 8);
		}

		{
			X86_Inst* type_member = ASMA(*any_alloca);

			type_member->as.constant_offset.offset = ASMA(*any_alloca->as.constant_offset.offset);
			type_member->as.constant_offset.from = ASMA(*any_alloca->as.constant_offset.from);
			type_member->as.constant_offset.size = InWords(8);

			i64& type_member_offset = *(i64*)(&any_alloca->as.constant_offset.offset->as.constant.bytes);
			type_member_offset -= any_struct->Members[any_struct->FindMember("type")].Offset;

			auto type_table_index = TypeSystem::GetTypeInfoIndex(ir_any->Type);

			Make_Move(Make_Constant((i64)type_table_index), type_member, stream, 8);
		}
	}

	void X86_BackEnd::AssembleAnyArray(IRAnyArray* ir_any_array, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* any_array = AllocateStack(TypeSystem::GetArray());
		X86_Inst* any_array_data = AllocateStack(TypeSystem::GetTypeSize(TypeSystem::GetAny()) * ir_any_array->Arguments.size());

		auto array_struct_info = m_Metadata->GetStructFromType(TypeSystem::GetArray()->BaseID);

		auto tmp_reg_id = GetRegisterID();
		auto tmp_reg = Allocate_Register(RegisterUsage::REG_I64, tmp_reg_id, stream);

		{
			auto& data_member_info = array_struct_info->Members[array_struct_info->FindMember("data")];
			auto& count_member_info = array_struct_info->Members[array_struct_info->FindMember("count")];

			X86_Inst* count_member = ASMA(*any_array);
			count_member->as.constant_offset.offset = ASMA(*any_array->as.constant_offset.offset);
			count_member->as.constant_offset.from = ASMA(*any_array->as.constant_offset.from);
			count_member->as.constant_offset.size = InWords(8);
			i64& count_member_offset = *(i64*)(&count_member->as.constant_offset.offset->as.constant.bytes);
			count_member_offset -= count_member_info.Offset;

			X86_Inst* data_member = ASMA(*any_array);
			data_member->as.constant_offset.offset = ASMA(*any_array->as.constant_offset.offset);
			data_member->as.constant_offset.from = ASMA(*any_array->as.constant_offset.from);
			data_member->as.constant_offset.size = InWords(8);
			i64& data_member_offset = *(i64*)(&data_member->as.constant_offset.offset->as.constant.bytes);
			data_member_offset -= data_member_info.Offset;

			Make_LEA(any_array_data, tmp_reg, stream);
			Make_Move(tmp_reg, data_member, stream, data_member_info.Type, "var_args array data set");

			Make_Move(Make_Constant(ir_any_array->Arguments.size()), count_member, stream, count_member_info.Type, "var_args array count set");
		}

		u64 offset = 0;

		auto any_struct_info = m_Metadata->GetStructFromType(TypeSystem::GetAny()->BaseID);

		auto& data_member_info = any_struct_info->Members[any_struct_info->FindMember("data")];
		auto& type_member_info = any_struct_info->Members[any_struct_info->FindMember("type")];

		for (IRAny& any_arg : ir_any_array->Arguments) {

			X86_Inst* type_member = ASMA(*any_array);
			type_member->as.constant_offset.offset = ASMA(*any_array_data->as.constant_offset.offset);
			type_member->as.constant_offset.from = ASMA(*any_array_data->as.constant_offset.from);
			type_member->as.constant_offset.size = InWords(8);
			i64& type_member_offset = *(i64*)(&type_member->as.constant_offset.offset->as.constant.bytes);
			type_member_offset -= offset + type_member_info.Offset;

			X86_Inst* data_member = ASMA(*any_array);
			data_member->as.constant_offset.offset = ASMA(*any_array_data->as.constant_offset.offset);
			data_member->as.constant_offset.from = ASMA(*any_array_data->as.constant_offset.from);
			data_member->as.constant_offset.size = InWords(8);
			i64& data_member_offset = *(i64*)(&data_member->as.constant_offset.offset->as.constant.bytes);
			data_member_offset -= offset + data_member_info.Offset;

			Make_LEA(GetIRRegister(any_arg.DataRegister), tmp_reg, stream);
			Make_Move(tmp_reg, data_member, stream, data_member_info.Type, "var_args argument data set");

			Make_Move(Make_Constant(TypeSystem::GetTypeInfoIndex(any_arg.Type)), type_member, stream, type_member_info.Type, "var_args argument type set");

			offset += 16;
		}

		Free_Register(tmp_reg_id);

		SetRegisterValue(any_array);
	}

	void X86_BackEnd::AssembleLexicalBlock(IRLexBlock* lexical_block, std::vector<X86_Inst*>& stream)
	{
		for (auto inst : lexical_block->Instructions)
		{
			Assemble(inst, stream);
		}
	}

	void X86_BackEnd::AssembleIntTruncCast(IRIntTrunc* ir_int_trunc, std::vector<X86_Inst*>& stream)
	{
		auto integer_register = GetIRRegister(ir_int_trunc->IntegerRegister, false);

		if (integer_register->type == X86_REG_ALLOC || integer_register->type == X86_REG_NAME)
		{
			auto reg = register_trunc_map.at({ register_overlap.at(integer_register->as.reg_alloc.Register), RegisterUsageByType(ir_int_trunc->Type) });
			X86_Inst* truncated_reg = ASMA(*integer_register);
			truncated_reg->as.reg_alloc.Register = reg;
			SetRegisterValue(truncated_reg);
		}
		else
		{

			if (integer_register->type == X86_CONSTANT) {
				SetRegisterValue(integer_register);
			}
			else
			{
				auto castee_type = m_Data.IR_RegisterTypes.at(ir_int_trunc->IntegerRegister);
				auto reg = register_trunc_map.at({ register_overlap.at(integer_register->as.reg_alloc.Register), RegisterUsageByType(castee_type) });

				auto temprary_reg_id = GetRegisterID();
				auto temprary_reg = Allocate_Register(RegisterUsageByType(castee_type), temprary_reg_id, stream);

				Make_Move(integer_register, temprary_reg, stream, castee_type);
				Free_Register(temprary_reg_id);

				X86_Inst* truncated_reg = ASMA(*temprary_reg);
				truncated_reg->as.reg_alloc.Register = reg;

				SetRegisterValue(truncated_reg);
			}
		}
	}

	void X86_BackEnd::AssembleInt2FP(IRInt2FP* ir_int_2_fp, std::vector<X86_Inst*>& stream)
	{
		u64 fp_size = TypeSystem::GetTypeSize(ir_int_2_fp->Type);

		X86_ASM cvt_type = (X86_ASM)0;

		if (fp_size > 4)
		{
			cvt_type = X86_CVTSI2SD;
		}
		else
		{
			cvt_type = X86_CVTSI2SS;
		}

		auto integer = GetIRRegister(ir_int_2_fp->IntegerRegister);
		auto cvt_register = Allocate_Register(RegisterUsage::REG_F64, GetRegisterID(), stream);

		X86_Inst* cvt = ASMA(X86_Inst());
		cvt->type = cvt_type;
		cvt->as.bin_op.destination = cvt_register;
		cvt->as.bin_op.value = integer;

		stream.push_back(cvt);

		SetRegisterValue(cvt_register);
	}

	void X86_BackEnd::AssembleIRRegister(IRRegister* inst, std::vector<X86_Inst*>& stream)
	{
		GS_CORE_ASSERT(inst->Value);

		IsRegisterValue = true;
		CurrentRegister = inst;

		GS_CORE_ASSERT(!RegisterValue);

		std::vector<X86_Inst*> register_stream;
		Assemble(inst->Value, register_stream);

		GS_CORE_ASSERT(RegisterValue);

		m_Data.IR_RegisterValues[inst->ID] = RegisterValue;
		m_Data.IR_RegisterTypes[inst->ID] = GetIRNodeType(inst->Value);

		for (size_t i = 0; i < register_stream.size(); i++)
		{
			stream.push_back(register_stream[i]);
		}

		RegisterValue = nullptr;
		IsRegisterValue = false;
	}

	void X86_BackEnd::AssembleIRRegisterValue(IRRegisterValue* register_value, std::vector<X86_Inst*>& stream)
	{
		IRRegister* Register = m_Metadata->GetRegister(register_value->RegisterID);

		if (Register->Value->GetType() == IRNodeType::Alloca || Register->Value->GetType() == IRNodeType::Argument || Register->Value->GetType() == IRNodeType::MemberAccess || Register->Value->GetType() == IRNodeType::ArrayAccess)
		{

			auto address_register = Allocate_Register(REG_I64, GetRegisterID(), stream);

			Make_LEA(GetIRRegister(register_value->RegisterID), address_register, stream);
			SetRegisterValue(address_register);

			Free_Register(address_register->as.reg_alloc.register_allocation_id);
		}
		else
		{
			if (IsRegisterValue)
				SetRegisterValue(GetIRRegister(register_value->RegisterID));
		}
	}

	TypeStorage* X86_BackEnd::GetIRNodeType(IRInstruction* inst)
	{
		TypeStorage* type = nullptr;
		IRNodeType node_type = inst->GetType();

		switch (node_type)
		{
		case IRNodeType::ConstValue:
		{
			IRCONSTValue* as_const_value = (IRCONSTValue*)inst;
			type = TypeSystem::GetBasic(as_const_value->Type);
		}
		break;
		case IRNodeType::DataValue:
		{
			type = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u8), 1);
		}
		break;
		case IRNodeType::Alloca:
		{
			IRAlloca* as_alloca = (IRAlloca*)inst;
			type = as_alloca->Type;
		}
		break;
		case IRNodeType::Load:
		{
			IRLoad* as_load = (IRLoad*)inst;
			type = as_load->Type;
		}
		break;
		case IRNodeType::MemberAccess:
		{
			auto member_access = (IRMemberAccess*)inst;

			const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(member_access->StructID);
			GS_CORE_ASSERT(struct_metadata);
			GS_CORE_ASSERT(member_access->MemberID < struct_metadata->Members.size());

			const MemberMetadata& member = struct_metadata->Members[member_access->MemberID];
			type = TypeSystem::GetPtr(member.Type, 1);
		}
		break;
		case IRNodeType::ArrayAccess:
		{
			auto array_access = (IRArrayAccess*)inst;
			type = TypeSystem::GetPtr(array_access->Type, 1);
		}
		break;
		case IRNodeType::Store:
		{
			IRStore* as_store = (IRStore*)inst;
			type = as_store->Type;
		}
		break;
		case IRNodeType::Call:
		{
			IRFunctionCall* as_call = (IRFunctionCall*)inst;
			type = m_Metadata->GetFunctionMetadata(as_call->FuncID)->ReturnType;
		}
		break;
		case IRNodeType::ADD:
		case IRNodeType::SUB:
		case IRNodeType::MUL:
		case IRNodeType::DIV:
		{
			IRBinOp* as_binop = (IRBinOp*)inst;
			type = as_binop->Type;
		}
		break;
		case IRNodeType::Argument:
		{
			GS_CORE_ASSERT(m_CurrentFunction);

			IRArgumentAllocation* argument = (IRArgumentAllocation*)inst;
			type = m_CurrentFunction->Arguments[argument->ArgumentIndex].Type;
		}
		break;
		case IRNodeType::RegisterValue:
		{
			type = m_Data.IR_RegisterTypes.at(((IRRegisterValue*)inst)->RegisterID);
		}
		break;
		case IRNodeType::GlobAddress:
		{
			type = m_Metadata->GetVariableMetadata(((IRGlobalAddress*)inst)->GlobID)->Tipe;
		}
		break;
		case IRNodeType::NullPtr:
		{
			type = TypeSystem::GetPtr(TypeSystem::GetBasic(((IRNullPtr*)inst)->TypeID), 1);
		}
		break;
		case IRNodeType::PointerCast:
		{
			type = ((IRPointerCast*)inst)->Type;
		}
		break;
		case IRNodeType::IntTrunc:
		case IRNodeType::Int2PtrCast:
		case IRNodeType::Ptr2IntCast:
		{
			type = ((IRIntTrunc*)inst)->Type;
		}
		break;
		case IRNodeType::Int2FP:
		{
			type = ((IRInt2FP*)inst)->Type;
		}
		break;
		case IRNodeType::GreaterThan:
		case IRNodeType::LesserThan:
		case IRNodeType::Equal:
		case IRNodeType::NotEqual:
		case IRNodeType::BitAnd:
		case IRNodeType::BitOr:
		{
			type = ((IRBinOp*)inst)->Type;
		}
		break;

		case IRNodeType::TypeValue:
		{
			return TypeSystem::GetBasic(IR_type);
		}
		break;
		case IRNodeType::TypeInfo:
		case IRNodeType::TypeOf:
		{
			return TypeSystem::GetPtr(TypeSystem::GetBasic(IR_typeinfo), 1);
		}
		break;
		case IRNodeType::Any:
		{
			return TypeSystem::GetBasic(IR_any);
		}
		case IRNodeType::AnyArray:
		{
			return TypeSystem::GetDynArray(TypeSystem::GetAny());
		}
		break;

		default:
			GS_CORE_ASSERT(0);
			break;
		}

		return type;
	}

	std::string X86_BackEnd::Print(const std::vector<X86_Inst*>& assm)
	{
		std::string stream;
		std::string comments;

		i64 indentation_distance = 50;

		for (size_t i = 0; i < assm.size(); i++)
		{
			X86_Inst inst = *assm[i];

			if (inst.type != X86_LABEL && inst.type != X86_SECTION && inst.type != X86_NAMED_OFFSET)
			{
				stream += "\t";
			}

			i64 line_start = (i64)stream.size();
			Print(inst, stream, comments);
			i64 line_end = (i64)stream.size();

			if (comments.size() > 0)
			{
				i64 line_len = 0;
				for (i64 i = line_start; i < line_end; i++)
				{
					line_len += 1;
					if (stream[i] == '\t')
					{
						line_len += 3;
					}
				}

				if (line_len < indentation_distance)
				{
					for (i64 i = 0; i < indentation_distance - line_len; i++)
					{
						stream.push_back(' ');
					}
				}
			}

			stream += comments;

			comments.clear();

			stream += '\n';
		}

		return stream;
	}

	void X86_BackEnd::Print(X86_Inst inst, std::string& stream, std::string& comments)
	{
		static std::vector<const char*> words = { "", "byte ", "word ", "dword ", "qword " };

		switch (inst.type)
		{
		case X86_LABEL_REF:
			stream += inst.as.label.name;
			break;
		case X86_LABEL:
			stream += inst.as.label.name;
			GS_CORE_ASSERT(inst.as.label.name);
			stream += ":";
			break;

		case X86_CALL:
			stream += "call\t";
			Print(*inst.as.call.What, stream, comments);
			break;

		case X86_JMP:
			stream += "jmp\t";
			Print(*inst.as.call.What, stream, comments);
			break;

		case X86_PUSH:
		case X86_PUSHQ:
			stream += "push";
			stream += "\t\t";
			Print(*inst.as.push.source, stream, comments);
			break;

		case X86_POP:
		case X86_POPQ:
			stream += "pop";
			stream += "\t\t";
			Print(*inst.as.push.source, stream, comments);
			break;

		case X86_MOVD:
			stream += "movd";
			stream += "\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_MOVQ:
			stream += "movq";
			stream += "\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_MOV:
			stream += "mov";
			stream += "\t\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_MOVZX:
			stream += "movzx";
			stream += "\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_MOVSXD:
			stream += "movsxd";
			stream += "\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_MOVSS:
			stream += "movss";
			stream += "\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_MOVSD:
			stream += "movsd";
			stream += "\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;
		case X86_CVTSS2SD:
			stream += "cvtss2sd";
			stream += "\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;
		case X86_CVTSI2SS:
			stream += "cvtsi2ss";
			stream += "  ";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;
		case X86_CVTSI2SD:
			stream += "cvtsi2sd";
			stream += "  ";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;
		case X86_LEA:
			stream += "lea";
			stream += "\t\t";
			Print(*inst.as.move.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.move.source, stream, comments);
			break;

		case X86_REG_NAME:
			stream += RegisterToString(inst.as.reg_name.Register);
			break;
		case X86_REG_ALLOC:
			stream += RegisterToString(inst.as.reg_alloc.Register);
			break;

		case X86_RET:
			stream += "ret";
			stream += '\n';
			stream += '\n';
			break;

		case X86_ISUB:
			stream += "sub";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;
		case X86_IADD:
			stream += "add";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;
		case X86_IMUL:
			stream += "imul";
			stream += "\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;

		case X86_IDIV:
			stream += "div";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;

		case X86_AND:
			stream += "and";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;

		case X86_OR:
			stream += "or";
			stream += "\t\t";
			Print(*inst.as.bin_op.destination, stream, comments);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, comments);
			break;

		case X86_CONSTANT:
			stream += std::to_string(*(i64*)&inst.as.constant.bytes);
			break;

		case X86_CONSTANT_FLOAT:
			stream += std::to_string(*(double*)&inst.as.constant.bytes);
			break;

		case X86_ADDR_MUL:
			Print(*inst.as.const_binop.a, stream, comments);
			stream += "*";
			Print(*inst.as.const_binop.b, stream, comments);
			break;

		case X86_CONSTANT_OFFSET:
		{
			stream += words.at(inst.as.constant_offset.size);
			stream += "[";
			Print(*inst.as.constant_offset.from, stream, comments);
			stream += inst.as.constant_offset.offset_type ? " + " : " - ";
			Print(*inst.as.constant_offset.offset, stream, comments);
			stream += "]";
			break;
		}
		case X86_GLOB_DE_REF:
		{
			stream += words.at(inst.as.de_ref.size);
			stream += "[";
			stream += "glb" + std::to_string(inst.as.global.global_id - 512000);
			stream += "]";
			break;
		}
		case X86_DE_REF:
		{
			stream += words.at(inst.as.de_ref.size);
			stream += "[";
			Print(*inst.as.de_ref.what, stream, comments);
			stream += "]";
			break;
		}

		case X86_DATA_STR_REF:
		{

			// stream += '[';
			stream += "str";
			stream += std::to_string(inst.as.data_str_ref.string_id);
			//			stream += ']';

			break;
		}

		case X86_CMP:
			stream += "cmp";
			stream += "\t\t";
			Print(*inst.as.cmp.a, stream, comments);
			stream += ", ";
			Print(*inst.as.cmp.b, stream, comments);
			break;

		case X86_SETG:
		case X86_SETGE:
		case X86_SETL:
		case X86_SETLE:
		case X86_SETE:
		case X86_SETNE:
		{

			static const std::map<X86_ASM, const char*> set_inst_names = {
				{X86_SETG, "setg"},
				{X86_SETGE, "setge"},
				{X86_SETL, "setl"},
				{X86_SETLE, "setle"},
				{X86_SETE, "sete"},
				{X86_SETNE, "setne"},
			};

			stream += set_inst_names.at(inst.type);
			stream += "\t";

			Print(*inst.as.cond_set.destination, stream, comments);
		}
		break;
		case X86_JMPG:
		case X86_JMPGE:
		case X86_JMPL:
		case X86_JMPLE:
		case X86_JMPE:
		case X86_JMPNE:
		{

			static const std::map<X86_ASM, const char*> cjmp_inst_names = {
				{X86_JMPG, "jg"},
				{X86_JMPGE, "jge"},

				{X86_JMPL, "jl"},
				{X86_JMPLE, "jle"},

				{X86_JMPE, "je"},
				{X86_JMPNE, "jne"},
			};

			stream += cjmp_inst_names.at(inst.type);
			stream += "\t";

			Print(*inst.as.jmp.Where, stream, comments);
		}
		break;
		case X86_SECTION:
		{

			static const std::map<Section_Type, std::string> section_type_names = {
				{SEC_Code, "code"},
				{SEC_Data, "data"},
			};

			stream += "section ";

			stream += fmt::format("'.{}' ", inst.as.section.name);
			stream += fmt::format("{} ", section_type_names.at(inst.as.section.type));

			if (inst.as.section.flags & SEC_Readable)
			{
				stream += "readable ";
			}

			if (inst.as.section.flags & SEC_Writable)
			{
				stream += "writable ";
			}
		}
		break;
		case X86_NAMED_OFFSET:
		{
			stream += inst.as.named_offset.name;
			Print(*inst.as.named_offset.value, stream, comments);
		}
		break;
		case X86_QUAD_DATA_ARRAY:
		{
			stream += " dq ";

			u64* data = (u64*)inst.as.data_array.data;
			u64 data_size = inst.as.data_array.byte_size;

			for (u64 i = 0; i < data_size / 8; i++)
			{
				stream += ',';
				stream += std::to_string(data[i]);
			}
		}
		break;
		case X86_INST_ARRAY:
		{

			X86_Inst** data = inst.as.inst_array.data;
			u64 count = inst.as.inst_array.count;

			for (u64 i = 0; i < count; i++)
			{
				Print(*data[i], stream, comments);
			}
		}
		break;
		default:
			stream += "un-implemented print inst";
			break;
		}

		if (inst.comment)
		{
			comments += ";";
			comments += inst.comment;
		}
	}

	std::string X86_BackEnd::MangleName(const std::string& name, TSFunc* signature)
	{
		return fmt::format("{}_{}", name, (void*)signature->Hash);
	}

	std::string X86_BackEnd::RegisterToString(X86_Register reg)
	{
		static std::vector<std::string> names = {
			"rsp",
			"rbp",
			"rax",
			"rcx",
			"rdx",
			"rbx",
			"rsi",
			"rdi",
			"esp",
			"ebp",
			"eax",
			"ecx",
			"edx",
			"ebx",
			"esi",
			"edi",
			"sp",
			"bp",
			"ax",
			"cx",
			"dx",
			"bx",
			"si",
			"di",
			"spl",
			"bpl",
			"ah",
			"al",
			"ch",
			"cl",
			"dh",
			"dl",
			"bh",
			"bl",
			"sil",
			"dil",

			"xmm0",
			"xmm1",
			"xmm2",
			"xmm3",
			"xmm4",
			"xmm5",
			"xmm6",
			"xmm7",
			"xmm8",
			"xmm9",
			"xmm10",
			"xmm11",
			"xmm12",
			"xmm13",
			"xmm14",
			"xmm15",

			"r8",
			"r9",
			"r10",
			"r11",
			"r12",
			"r13",
			"r14",
			"r15",
			"r8d",
			"r9d",
			"r10d",
			"r11d",
			"r12d",
			"r13d",
			"r14d",
			"r15d",
			"r8w",
			"r9w",
			"r10w",
			"r11w",
			"r12w",
			"r13w",
			"r14w",
			"r15w",
			"r8b",
			"r9b",
			"r10b",
			"r11b",
			"r12b",
			"r13b",
			"r14b",
			"r15b",
		};

		return names[reg];
	}

	void X86_BackEnd::Make_MemCpy(u64 source_register_id, u64 destination_register_id, std::vector<X86_Inst*>& stream, TypeStorage* type)
	{
		GS_CORE_ASSERT(type);
		auto cpymem_size = TypeSystem::GetTypeSize(type);

		auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
		auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

		auto src = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_OUT);
		auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_OUT);
		auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_OUT);
		auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream); // so that it will not override rax spill it if in use

		Make_Move(GetIRRegister(destination_register_id), dest, stream, void_ptr_Ty);
		Make_Move(GetIRRegister(source_register_id), src, stream, void_ptr_Ty);
		Make_Move(Make_Constant(cpymem_size), sz, stream, void_ptr_Ty);

		X86_Inst* call = ASMA(X86_Inst());
		call->type = X86_CALL;
		call->as.call.What = MemCpy;
		stream.push_back(call);

		Free_Register(dest->as.reg_alloc.register_allocation_id);
		Free_Register(src->as.reg_alloc.register_allocation_id);
		Free_Register(sz->as.reg_alloc.register_allocation_id);
		Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);
	}

	void X86_BackEnd::Make_LocalStack_MemCpy(X86_Inst* source_stack_offset, X86_Inst* destination_stack_offset, std::vector<X86_Inst*>& stream, TypeStorage* type)
	{
		GS_CORE_ASSERT(type);
		auto cpymem_size = TypeSystem::GetTypeSize(type);

		auto void_ptr_Ty = TypeSystem::GetPtr(TypeSystem::GetVoid(), 1);
		auto size_Ty = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u64), 1);

		auto dest = GetArgumentLocation(void_ptr_Ty, 0, stream, ARG_DIR_OUT);
		auto src = GetArgumentLocation(void_ptr_Ty, 1, stream, ARG_DIR_OUT);
		auto sz = GetArgumentLocation(size_Ty, 2, stream, ARG_DIR_OUT);
		auto memcpy_ret = GetReturnLocation(void_ptr_Ty, stream); // so that it will not override rax, will spill it if in use

		Make_LEA(destination_stack_offset, dest, stream);
		Make_LEA(source_stack_offset, src, stream);
		Make_Move(Make_Constant(cpymem_size), sz, stream, size_Ty);

		X86_Inst* call = ASMA(X86_Inst());
		call->type = X86_CALL;
		call->as.call.What = MemCpy;
		stream.push_back(call);

		Free_Register(dest->as.reg_alloc.register_allocation_id);
		Free_Register(src->as.reg_alloc.register_allocation_id);
		Free_Register(sz->as.reg_alloc.register_allocation_id);
		Free_Register(memcpy_ret->as.reg_alloc.register_allocation_id);
	}

	X86_Inst* X86_BackEnd::Make_Move(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& intermediate_stream, TypeStorage* type, const char* comment)
	{
		auto type_size = TypeSystem::GetTypeSize(type);

		if (source->type == X86_CONSTANT_OFFSET && destination->type == X86_CONSTANT_OFFSET)
		{ //	needs intermediate register

			auto dest_register = Allocate_Register(RegisterUsageByType(type), GetRegisterID(), intermediate_stream);

			X86_ASM move_type = X86_MOV;

			if (source->type != X86_CONSTANT && source->type != X86_CONSTANT_FLOAT)
			{
				move_type = MovByType(type);
			}

			X86_Inst intermediate = {};
			intermediate.type = move_type;
			intermediate.as.move.source = source;
			intermediate.as.move.destination = dest_register;
			intermediate.comment = "intermediate move to register";
			source = dest_register;
			intermediate_stream.push_back(ASMA(intermediate));

			Free_Register(dest_register->as.reg_alloc.register_allocation_id);
		}

		if (source->type == X86_CONSTANT_FLOAT && (destination->type == X86_REG_ALLOC || destination->type == X86_REG_NAME))
		{ //	needs intermediate register

			if (IsXMM(destination->as.reg_alloc.Register))
			{

				auto dest_register = Allocate_Register(RegisterUsageByTypeNoXMM(type), GetRegisterID(), intermediate_stream);

				X86_ASM move_type = X86_MOV;

				X86_Inst intermediate = {};
				intermediate.type = move_type;
				intermediate.as.move.source = source;
				intermediate.as.move.destination = dest_register;
				intermediate.comment = "intermediate move to register";
				source = dest_register;
				intermediate_stream.push_back(ASMA(intermediate));

				Free_Register(dest_register->as.reg_alloc.register_allocation_id);
			}
		}

		bool useless_move = AreEqual(source, destination);

		if (!useless_move)
		{

			X86_ASM move_type = X86_MOV;

			if (source->type != X86_CONSTANT && source->type != X86_CONSTANT_FLOAT)
			{
				move_type = MovByType(type);
			}

			if ((source->type == X86_REG_ALLOC || source->type == X86_REG_NAME) && (destination->type == X86_REG_ALLOC || destination->type == X86_REG_NAME))
			{ //	needs intermediate register
				if (IsXMM(destination->as.reg_alloc.Register) && !IsXMM(source->as.reg_alloc.Register))
				{
					if (type_size < 8)
					{
						move_type = X86_MOVD;
					}
					else
					{
						move_type = X86_MOVQ;
					}
				}
			}

			X86_Inst* move = ASMA(X86_Inst());
			move->type = move_type;
			move->as.move.source = source;
			move->as.move.destination = destination;
			move->comment = comment;

			intermediate_stream.push_back(move);
		}

		return destination;
	}

	X86_Inst* X86_BackEnd::Make_Move(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& intermediate_stream, u64 size, const char* comment)
	{

		if (source->type == X86_CONSTANT_OFFSET && destination->type == X86_CONSTANT_OFFSET)
		{ //	needs intermediate register

			auto dest_register = Allocate_Register(RegisterUsageBySize(size), GetRegisterID(), intermediate_stream);

			X86_Inst intermediate = {};
			intermediate.type = X86_MOV;
			intermediate.as.move.source = source;
			intermediate.as.move.destination = dest_register;
			intermediate.comment = "intermediate move to register";
			source = dest_register;
			intermediate_stream.push_back(ASMA(intermediate));

			Free_Register(dest_register->as.reg_alloc.register_allocation_id);
		}

		bool useless_move = AreEqual(source, destination);

		if (!useless_move)
		{

			X86_Inst* move = ASMA(X86_Inst());
			move->type = X86_MOV;
			move->as.move.source = source;
			move->as.move.destination = destination;
			move->comment = comment;

			intermediate_stream.push_back(move);
		}

		return destination;
	}

	void X86_BackEnd::Make_LEA(X86_Inst* source, X86_Inst* destination, std::vector<X86_Inst*>& stream)
	{
		X86_Inst* lea = ASMA(X86_Inst());
		lea->type = X86_LEA;
		lea->as.lea.source = source;
		lea->as.lea.destination = destination;

		stream.push_back(lea);
	}

	X86_Inst* X86_BackEnd::Register_Zext(X86_Inst* reg, u64 size, std::vector<X86_Inst*>& stream)
	{
		auto destination = Allocate_Register(RegisterUsageBySize(size), GetRegisterID(), stream);

		X86_Inst* move = ASMA(X86_Inst());
		move->type = X86_MOVZX;
		move->as.move.source = reg;
		move->as.move.destination = destination;

		stream.push_back(move);

		return destination;
	}

	X86_Inst* X86_BackEnd::Register_Sext(X86_Inst* reg, u64 size, std::vector<X86_Inst*>& stream, bool free_register_after_use /*= true*/)
	{
		auto destination = Allocate_Register(RegisterUsageBySize(size), GetRegisterID(), stream, free_register_after_use);

		X86_Inst* move = ASMA(X86_Inst());
		move->type = X86_MOVSXD;
		move->as.move.source = reg;
		move->as.move.destination = destination;

		stream.push_back(move);

		return destination;
	}

	X86_Inst* X86_BackEnd::Make_Register(X86_Register register_type)
	{
		X86_Inst* inst = new X86_Inst;
		inst->type = X86_REG_NAME;
		inst->as.reg_name = X86_Reg_Name_Inst{ register_type };

		return inst;
	}

	X86_Inst* X86_BackEnd::Make_Constant(i64 integer)
	{
		X86_Inst* inst = new X86_Inst;

		inst->type = X86_CONSTANT;
		memcpy(&inst->as.constant.bytes, &integer, 8);

		return inst;
	}

	X86_Inst* X86_BackEnd::Make_DeRef(X86_Inst* what)
	{
		X86_Inst* de_ref = ASMA(X86_Inst());
		de_ref->type = X86_DE_REF;
		de_ref->as.de_ref.size = (X86_Word)0;
		de_ref->as.de_ref.what = what;

		return de_ref;
	}

	X86_Inst* X86_BackEnd::Make_Constant_Float(double floating)
	{
		X86_Inst* inst = new X86_Inst;

		inst->type = X86_CONSTANT_FLOAT;
		memcpy(&inst->as.constant.bytes, &floating, 8);

		return inst;
	}

	X86_Inst* X86_BackEnd::GetIRRegister(u64 id, bool free_allocated_registers /* = true*/)
	{

		X86_Inst* ir_register_value = m_Data.IR_RegisterValues.at(id);

		const auto& free_list = m_Data.IR_RegisterFreeLists[id];

		if (free_allocated_registers)
		{
			for (size_t i = 0; i < free_list.count; i++)
			{
				Free_Register(free_list.free_list[i]);
			}
		}

		if (ir_register_value->type == X86_REG_ALLOC)
		{
			if (free_allocated_registers)
			{
				if (ir_register_value->as.reg_alloc.free_after_use)
				{
					Free_Register(ir_register_value->as.reg_alloc.register_allocation_id);
				}
			}
		}

		return ir_register_value;
	}

	TypeStorage* X86_BackEnd::GetIRRegisterType(u64 id)
	{
		return m_Data.IR_RegisterTypes.at(id);
	}

	RegisterUsage X86_BackEnd::RegisterUsageByType(TypeStorage* type)
	{
		GS_CORE_ASSERT(type);
		auto type_size = TypeSystem::GetTypeSize(type);

		bool floating_point = m_Metadata->GetTypeFlags(type->BaseID) & FLAG_FLOATING_TYPE;

		if (floating_point)
		{
			if (type_size > 4)
			{
				return RegisterUsage::REG_F64;
			}
			else if (type_size == 4)
			{
				return RegisterUsage::REG_F32;
			}
		}
		else
		{
			if (type_size > 4)
			{
				return RegisterUsage::REG_I64;
			}
			else if (type_size == 4)
			{
				return RegisterUsage::REG_I32;
			}
			else if (type_size == 2)
			{
				return RegisterUsage::REG_I16;
			}
			else if (type_size == 1)
			{
				return RegisterUsage::REG_I8;
			}
		}

		GS_CORE_ASSERT(0);
	}

	RegisterUsage X86_BackEnd::RegisterUsageByTypeNoXMM(TypeStorage* type)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);

		if (type_size > 4)
		{
			return RegisterUsage::REG_I64;
		}
		else if (type_size == 4)
		{
			return RegisterUsage::REG_I32;
		}
		else if (type_size == 2)
		{
			return RegisterUsage::REG_I16;
		}
		else if (type_size == 1)
		{
			return RegisterUsage::REG_I8;
		}

		GS_CORE_ASSERT(0);
	}

	RegisterUsage X86_BackEnd::RegisterUsageBySize(u64 type_size)
	{
		GS_CORE_ASSERT(type_size);

		if (type_size > 4)
		{
			return RegisterUsage::REG_I64;
		}
		else if (type_size == 4)
		{
			return RegisterUsage::REG_I32;
		}
		else if (type_size == 2)
		{
			return RegisterUsage::REG_I16;
		}
		else if (type_size == 1)
		{
			return RegisterUsage::REG_I8;
		}
	}

	X86_Word X86_BackEnd::InWords(TypeStorage* type)
	{
		static const std::map<u64, X86_Word> size_table = {
			{1, X86_byte},
			{2, X86_word},
			{4, X86_dword},
			{8, X86_qword},
		};

		GS_CORE_ASSERT(type);
		auto type_size = TypeSystem::GetTypeSize(type);

		if (type_size <= 8)
		{
			return size_table.at(type_size);
		}
		else
		{
			return (X86_Word)0;
		}
	}

	X86_Word X86_BackEnd::InWords(u64 type_size)
	{
		static const std::map<u64, X86_Word> size_table = {
			{1, X86_byte},
			{2, X86_word},
			{4, X86_dword},
			{8, X86_qword},
		};

		GS_CORE_ASSERT(type_size);

		if (type_size <= 8)
		{
			return size_table.at(type_size);
		}
		else
		{
			return (X86_Word)0;
		}
	}

	X86_ASM X86_BackEnd::MovByType(TypeStorage* type)
	{
		GS_CORE_ASSERT(type);
		auto flags = m_Metadata->GetTypeFlags(type->BaseID);

		auto size = TypeSystem::GetTypeSize(type);

		if (flags & FLAG_FLOATING_TYPE)
		{
			if (size <= 4)
			{
				return X86_MOVSS;
			}
			else
			{
				return X86_MOVSD;
			}
		}
		else
		{
			return X86_MOV;
		}
	}

	X86_ASM X86_BackEnd::MovByRegisterType(RegisterUsage usage)
	{
		if (usage == RegisterUsage::REG_F32)
		{
			return X86_MOVSS;
		}
		else if (usage == RegisterUsage::REG_F64)
		{
			return X86_MOVSD;
		}
		else
		{
			return X86_MOV;
		}
	}

	u64 X86_BackEnd::RegisterSize(X86_Register reg)
	{
		static const std::unordered_map<X86_Register, u64> sizes = {
			{RSP, 8},
			{RBP, 8},
			{RAX, 8},
			{RBX, 8},
			{RCX, 8},
			{RDX, 8},
			{RSI, 8},
			{RDI, 8},
			{R8, 8},
			{R9, 8},
			{R10, 8},
			{R11, 8},
			{R12, 8},
			{R13, 8},
			{R14, 8},
			{R15, 8},
			{ESP, 4},
			{EBP, 4},
			{EAX, 4},
			{EBX, 4},
			{ECX, 4},
			{EDX, 4},
			{ESI, 4},
			{EDI, 4},
			{R8d, 4},
			{R9d, 4},
			{R10d, 4},
			{R11d, 4},
			{R12d, 4},
			{R13d, 4},
			{R14d, 4},
			{R15d, 4},
			{XMM0, 16},
			{XMM1, 16},
			{XMM2, 16},
			{XMM3, 16},
			{XMM4, 16},
			{XMM5, 16},
			{XMM6, 16},
			{XMM7, 16},
			{SP, 2},
			{BP, 2},
			{AX, 2},
			{BX, 2},
			{CX, 2},
			{DX, 2},
			{SI, 2},
			{DI, 2},
			{SPL, 1},
			{BPL, 1},
			{AL, 1},
			{BL, 1},
			{CL, 1},
			{DL, 1},
			{SIL, 1},
			{DIL, 1},
		};

		return sizes.at(reg);
	}

	bool X86_BackEnd::IsXMM(X86_Register reg)
	{
		if (reg >= X86_Register::XMM0 && reg <= X86_Register::XMM15)
		{
			return true;
		}
		return false;
	}

	void X86_BackEnd::Free_Register(u32 id)
	{
		if (RegisterAllocationIDs.find(id) == RegisterAllocationIDs.end())
		{
			return;
		}

		auto phys_reg = RegisterAllocationIDs.at(id);
		auto overlap = register_overlap.at(phys_reg->as.reg_alloc.Register);
		RegisterOccupations[overlap] = false;
	}

	void X86_BackEnd::Free_All_Register()
	{
		RegisterOccupations.clear();
		RegisterAllocations.clear();
		RegisterAllocationIDs.clear();
		RegisterAllocationTypes.clear();
	}

	X86_Inst* X86_BackEnd::Allocate_Register(RegisterUsage usage, u32 id, std::vector<X86_Inst*>& spillage_stream, bool free_register_after_use /*= true*/)
	{

		static const std::map<RegisterUsage, TypeStorage*> UsageToType = {
			{RegisterUsage::REG_I8, TypeSystem::GetBasic(IR_u8)},
			{RegisterUsage::REG_I16, TypeSystem::GetBasic(IR_u16)},
			{RegisterUsage::REG_I32, TypeSystem::GetBasic(IR_u32)},
			{RegisterUsage::REG_I64, TypeSystem::GetBasic(IR_u64)},

			{RegisterUsage::REG_F32, TypeSystem::GetBasic(IR_f32)},
			{RegisterUsage::REG_F64, TypeSystem::GetBasic(IR_f64)},
		};

		const auto& slice = registers.at(usage);

		for (auto phys_reg : slice)
		{

			auto overlap = register_overlap.at(phys_reg);

			if (!RegisterOccupations[overlap])
			{

				X86_Inst* allocation_inst = ASMA(X86_Inst());
				allocation_inst->type = X86_REG_ALLOC;
				allocation_inst->as.reg_alloc.Register = phys_reg;
				allocation_inst->as.reg_alloc.register_allocation_id = id;
				allocation_inst->as.reg_alloc.free_after_use = free_register_after_use;

				RegisterAllocations[overlap] = allocation_inst;
				RegisterAllocationIDs[id] = allocation_inst;
				RegisterOccupations[overlap] = true;
				RegisterAllocationTypes[overlap] = UsageToType.at(usage);

				return allocation_inst;
			}
		}

		auto overlap = register_overlap.at(slice[0]);

		RegisterOccupations.at(overlap) = false;
		X86_Inst* previous_allocation = RegisterAllocations.at(overlap);

		auto allocation_type = RegisterAllocationTypes.at(overlap);

		auto spillage_location = AllocateStack(allocation_type);

		Make_Move(ASMA(X86_Inst(*previous_allocation)), spillage_location, spillage_stream, allocation_type, "spillage");
		*previous_allocation = *spillage_location;

		return Allocate_Register(usage, id, spillage_stream, free_register_after_use);
	}

	X86_Inst* X86_BackEnd::Allocate_Specific_Register(X86_Register reg, u32 id, std::vector<X86_Inst*>& spillage_stream)
	{
		auto overlap = register_overlap.at(reg);

		if (!RegisterOccupations[overlap])
		{

			X86_Inst* allocation_inst = ASMA(X86_Inst());
			allocation_inst->type = X86_REG_ALLOC;
			allocation_inst->as.reg_alloc.Register = reg;
			allocation_inst->as.reg_alloc.register_allocation_id = id;
			allocation_inst->as.reg_alloc.free_after_use = true;

			RegisterAllocations[overlap] = allocation_inst;
			RegisterAllocationIDs[id] = allocation_inst;
			RegisterOccupations[overlap] = true;

			return allocation_inst;
		}
		else
		{
			RegisterOccupations[overlap] = false;
			X86_Inst* previous_allocation = RegisterAllocations[overlap];

			auto allocation_size = RegisterSize(previous_allocation->as.reg_alloc.Register);

			auto spillage_location = AllocateStack(allocation_size);

			Make_Move(ASMA(X86_Inst(*previous_allocation)), spillage_location, spillage_stream, allocation_size, "spillage");
			*previous_allocation = *spillage_location;

			return Allocate_Specific_Register(reg, id, spillage_stream);
		}
	}

	bool X86_BackEnd::AreEqual(X86_Inst* a, X86_Inst* b)
	{
		if (a->type == X86_REG_NAME && b->type == X86_REG_NAME)
		{
			return a->as.reg_name.Register == b->as.reg_name.Register;
		}
		if (a->type == X86_REG_ALLOC && b->type == X86_REG_ALLOC)
		{
			return a->as.reg_alloc.Register == b->as.reg_alloc.Register;
		}
		return false;
	}

	X86_Inst* X86_BackEnd::AllocateStack(TypeStorage* type)
	{
		u64 allocation_size = TypeSystem::GetTypeSize(type);
		return AllocateStack(allocation_size);
	}

	X86_Inst* X86_BackEnd::AllocateStack(u64 allocation_size)
	{
		u64 actual_allocation_size = allocation_size;

		if (actual_allocation_size < 4)
		{
			actual_allocation_size = 4;
		}

		m_Data.CurrentFunction_StackFrameSize += actual_allocation_size;

		X86_Inst stack_frame_offset = {};
		stack_frame_offset.type = X86_CONSTANT_OFFSET;
		stack_frame_offset.as.constant_offset.from = Make_Register(RBP);
		stack_frame_offset.as.constant_offset.offset = Make_Constant((i64)m_Data.CurrentFunction_StackFrameSize);
		stack_frame_offset.as.constant_offset.size = InWords(allocation_size);
		return ASMA(stack_frame_offset);
	}

	X86_Inst* X86_BackEnd::Allocate_CallStack(u64 allocation_size)
	{
		if (allocation_size > 8)
		{
			allocation_size = 8;
		}

		if (m_Data.CurrentFunction_CallStackSize == 0)
		{
			m_Data.CurrentFunction_CallStackSize = 32; // i dont know why on windows arguments passed through the stack has to be 32 bytes offseted from stack top
			m_Data.CurrentFunction_CallStackPointer = 32;
		}

		X86_Inst* stack_top_offset = ASMA(X86_Inst());
		stack_top_offset->type = X86_CONSTANT_OFFSET;
		stack_top_offset->as.constant_offset.from = Make_Register(RSP);
		stack_top_offset->as.constant_offset.offset = Make_Constant(m_Data.CurrentFunction_CallStackPointer);
		stack_top_offset->as.constant_offset.offset_type = X86_CONSTANT_ADD;
		stack_top_offset->as.constant_offset.size = InWords(allocation_size);

		m_Data.CurrentFunction_CallStackPointer += (allocation_size < 8 ? 8 : allocation_size);
		if (m_Data.CurrentFunction_CallStackPointer >= m_Data.CurrentFunction_CallStackSize)
		{
			m_Data.CurrentFunction_CallStackSize = m_Data.CurrentFunction_CallStackPointer;
		}

		return stack_top_offset;
	}

	void X86_BackEnd::Reset_CallStackPointer()
	{
		m_Data.CurrentFunction_CallStackPointer = 32;
	}

	void X86_BackEnd::Reset_CallStackSize()
	{
		m_Data.CurrentFunction_CallStackSize = 0;
	}

	u32 X86_BackEnd::GetRegisterID()
	{
		return RegisterIDCounter++;
	}

	X86_BackEnd::ArgumentLocationInfo X86_BackEnd::GetArgumentLocationInfo(TypeStorage* type, u32 index, GetArgument_Location_Dir direction)
	{
		ArgumentLocationInfo info = {};

		auto argument_size = TypeSystem::GetTypeSize(type);

		if (index < 4)
		{

			if (argument_size > 8)
			{
				info.Type = ARG_LOC_REGISTER_PTR;
			}
			else
			{
				info.Type = ARG_LOC_REGISTER;
			}
		}
		else
		{
			if (argument_size > 8)
			{
				info.Type = ARG_LOC_PTR_IN_STACK;
			}
			else
			{
				info.Type = ARG_LOC_STACK;
			}
		}

		return info;
	}

	X86_Inst* X86_BackEnd::GetArgumentLocation(TypeStorage* type, u32 index, std::vector<X86_Inst*>& spillage_stream, GetArgument_Location_Dir direction, X86_Inst** secondary /*= nullptr*/)
	{
		// X64 Calling Convention On X86_64 Windows

		static const std::map<std::pair<u32, u32>, X86_Register> scalar_registers = {
			// index, size, reg
			{{0, 8}, RCX},
			{{1, 8}, RDX},

			{{0, 4}, ECX},
			{{1, 4}, EDX},

			{{0, 2}, CX},
			{{1, 2}, DX},

			{{0, 1}, CL},
			{{1, 1}, DL},

			{{2, 8}, R8},
			{{3, 8}, R9},

			{{2, 4}, R8d},
			{{3, 4}, R9d},
		};

		static const std::map<u32, X86_Register> float_registers = {
			// index, size, reg
			{0, XMM1},
			{1, XMM2},

			{2, XMM3},
			{3, XMM4},
		};

		u32 type_size = TypeSystem::GetTypeSize(type);

		auto type_flags = TypeSystem::GetTypeFlags(type);

		if (index < 4)
		{
			if (type_size <= 8)
			{

				if (direction == ARG_DIR_OUT && secondary && type_flags & FLAG_FLOATING_TYPE)
				{
					auto needed_register = scalar_registers.at({ index, type_size });
					*secondary = Allocate_Specific_Register(needed_register, GetRegisterID(), spillage_stream);
				}

				if (type_flags & FLAG_FLOATING_TYPE)
				{
					auto needed_register = float_registers.at(index);
					return Allocate_Specific_Register(needed_register, GetRegisterID(), spillage_stream);
				}
				else
				{
					auto needed_register = scalar_registers.at({ index, type_size });
					return Allocate_Specific_Register(needed_register, GetRegisterID(), spillage_stream);
				}
			}
			else
			{
				auto ptr_size = TypeSystem::GetTypeSize(TypeSystem::GetPtr(TypeSystem::GetVoid(), 1));
				auto needed_register = scalar_registers.at({ index, (u32)ptr_size });
				return Allocate_Specific_Register(needed_register, GetRegisterID(), spillage_stream);
			}
		}
		else
		{
			if (direction == ARG_DIR_IN)
			{

				if (m_Data.CurrentFunction_InputCallStackPointer == 0)
				{
					m_Data.CurrentFunction_InputCallStackPointer = m_Data.CurrentFunction_InputCallStackOffset + 32; // 32 magic number on windows
				}

				if (type_size > 8)
				{
					type_size = 8;
				}

				m_Data.CurrentFunction_InputCallStackPointer += type_size < 8 ? 8 : type_size;

				X86_Inst* caller_stack_top_offset = ASMA(X86_Inst());
				caller_stack_top_offset->type = X86_CONSTANT_OFFSET;
				caller_stack_top_offset->as.constant_offset.from = Make_Register(RBP);
				caller_stack_top_offset->as.constant_offset.offset = Make_Constant(m_Data.CurrentFunction_InputCallStackPointer);
				caller_stack_top_offset->as.constant_offset.offset_type = X86_CONSTANT_ADD;
				caller_stack_top_offset->as.constant_offset.size = InWords(type);

				return caller_stack_top_offset;
			}
			else
			{
				return Allocate_CallStack(type_size);
			}
		}
	}

	X86_Inst* X86_BackEnd::GetReturnLocation(TypeStorage* type, std::vector<X86_Inst*>& spillage_stream)
	{
		u64 type_size = TypeSystem::GetTypeSize(type);

		if (type_size > 4)
		{
			return Allocate_Specific_Register(RAX, GetRegisterID(), spillage_stream);
		}
		else
		{
			return Allocate_Specific_Register(EAX, GetRegisterID(), spillage_stream);
		}
	}

	X86_BackEnd::ArgumentAllocation X86_BackEnd::AllocateArgumentLocations(TSFunc* type, std::vector<X86_Inst*>& spillage_stream, GetArgument_Location_Dir direction, bool variadic /*= false*/)
	{
		u32 type_size = TypeSystem::GetTypeSize(type);
		auto type_flags = TypeSystem::GetTypeFlags(type);

		ArgumentAllocation allocation;

		u32 index = 0;

		for (auto arg_type : type->Arguments)
		{

			u32 type_size = TypeSystem::GetTypeSize(arg_type);
			auto type_flags = TypeSystem::GetTypeFlags(arg_type);

			auto needed_argument_type = arg_type;

			if (type_flags & FLAG_FLOATING_TYPE && variadic && type_size < 8)
			{
				needed_argument_type = TypeSystem::GetBasic(IR_f64);
			}

			X86_Inst* secondary_location;
			auto float_argument_location = GetArgumentLocation(needed_argument_type, index, spillage_stream, direction, &secondary_location);

			allocation.Locations.push_back({ GetArgumentLocationInfo(needed_argument_type, index, direction).Type, float_argument_location, secondary_location });

			index++;
		}

		return allocation;
	}
}