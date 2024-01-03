#include "pch.h"

#include "BackEnd/LLR_x86.h"
#include "BackEnd/TypeSystem.h"

#include "math.h"
#include <Windows.h>

namespace Glass
{

	Assembly_Instruction Builder::Ret()
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Ret;

		return instruction;
	}

	Assembly_Instruction Builder::Build_Inst(Assembly_Op_Code op_code, Assembly_Operand* op1 /*= nullptr*/, Assembly_Operand* op2 /*= nullptr*/)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = op_code;
		instruction.Operand1 = op1;
		instruction.Operand2 = op2;

		return instruction;
	}

	Assembly_Instruction Builder::Label(Assembly_Operand* operand)
	{
		return Builder::Build_Inst(I_Label, operand);
	}

	Assembly_Instruction Builder::Push(Assembly_Operand* operand)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Push;
		instruction.Operand1 = operand;

		return instruction;
	}

	Assembly_Instruction Builder::Pop(Assembly_Operand* operand)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Pop;
		instruction.Operand1 = operand;

		return instruction;
	}

	Assembly_Instruction Builder::Add(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Add;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::AddSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_AddSS;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::AddSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_AddSD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::Sub(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Sub;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::SubSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_SubSS;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::SubSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_SubSD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::Mul(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_IMul;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MulSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MulSS;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MulSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MulSD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::IDiv(Assembly_Operand* operand1)
	{
		return Builder::Build_Inst(I_IDiv, operand1);
	}

	Assembly_Instruction Builder::Div(Assembly_Operand* operand1)
	{
		return Builder::Build_Inst(I_Div, operand1);
	}

	Assembly_Instruction Builder::DivSS(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_DivSS, operand1, operand2);
	}

	Assembly_Instruction Builder::DivSD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_DivSD, operand1, operand2);
	}

	Assembly_Instruction Builder::Call(Assembly_Operand* operand1)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Call;
		instruction.Operand1 = operand1;

		return instruction;
	}

	Assembly_Instruction Builder::Lea(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Lea;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;
		return instruction;
	}

	Assembly_Instruction Builder::Cmp(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_Cmp, operand1, operand2);
	}

	Assembly_Instruction Builder::SetNe(Assembly_Operand* op)
	{
		return Builder::Build_Inst(I_Setne, op);
	}

	Assembly_Instruction Builder::SetE(Assembly_Operand* op)
	{
		return Builder::Build_Inst(I_Sete, op);
	}

	Assembly_Instruction Builder::SetL(Assembly_Operand* op)
	{
		return Builder::Build_Inst(I_Setl, op);
	}

	Assembly_Instruction Builder::SetG(Assembly_Operand* op)
	{
		return Builder::Build_Inst(I_Setg, op);
	}

	Assembly_Instruction Builder::And(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_And, operand1, operand2);
	}

	Assembly_Instruction Builder::Or(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_Or, operand1, operand2);
	}

	Assembly_Instruction Builder::SS2SD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_CvtSS2SD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;
		return instruction;
	}

	Assembly_Instruction Builder::Mov(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_Mov;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MovD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MovD;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MovQ(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Instruction instruction = {};
		instruction.OpCode = I_MovQ;
		instruction.Operand1 = operand1;
		instruction.Operand2 = operand2;

		return instruction;
	}

	Assembly_Instruction Builder::MovZX(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_MovZX, operand1, operand2);
	}

	Assembly_Instruction Builder::MovSX(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_MovSX, operand1, operand2);
	}

	Assembly_Instruction Builder::MovSXD(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		return Builder::Build_Inst(I_MovSXD, operand1, operand2);
	}

	Assembly_Operand* Builder::Register(X86_Register reg)
	{
		Assembly_Operand operand = {};
		operand.reg.Register = reg;
		operand.type = Op_Register;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::Constant_Integer(i64 integer)
	{
		Assembly_Operand operand = {};
		operand.constant_integer.integer = integer;
		operand.type = Op_Constant_Integer;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpAdd(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Add;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpSub(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Sub;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpMul(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Mul;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::OpDiv(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		Assembly_Operand operand = {};
		operand.bin_op.operand1 = operand1;
		operand.bin_op.operand2 = operand2;

		operand.type = Op_Div;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::De_Reference(Assembly_Operand* operand1, TypeStorage* type /*= nullptr*/)
	{
		Assembly_Operand operand = {};
		operand.de_reference.operand = operand1;
		operand.de_reference.wordness = asm_none;

		if (type) {
			operand.de_reference.wordness = to_asm_size(TypeSystem::GetTypeSize(type));
		}

		operand.type = Op_De_Reference;

		return ASMA(operand);
	}

	Assembly_Operand* Builder::Symbol(const std::string& symbol)
	{
		Assembly_Operand operand = {};

		operand.symbol.symbol = ASMA(symbol)->data();

		operand.type = Op_Symbol;

		return ASMA(operand);
	}

	std::unordered_map<X86_Register, std::string> Register_Names = {
		{X86_Register::RBP,"rbp"},
		{X86_Register::RSP,"rsp"},

		{X86_Register::AL,"al"},
		{X86_Register::BL,"bl"},
		{X86_Register::CL,"cl"},
		{X86_Register::DL,"dl"},
		{X86_Register::R8b,"r8b"},
		{X86_Register::R9b,"r9b"},
		{X86_Register::R10b,"r10b"},
		{X86_Register::R11b,"r11b"},
		{X86_Register::R12b,"r12b"},
		{X86_Register::R13b,"r13b"},
		{X86_Register::R14b,"r14b"},
		{X86_Register::R15b,"r15b"},


		{X86_Register::AX,"ax"},
		{X86_Register::BX,"bx"},
		{X86_Register::CX,"cx"},
		{X86_Register::DX,"dx"},
		{X86_Register::R8w,"r8w"},
		{X86_Register::R9w,"r9w"},
		{X86_Register::R10w,"r10w"},
		{X86_Register::R11w,"r11w"},
		{X86_Register::R12w,"r12w"},
		{X86_Register::R13w,"r13w"},
		{X86_Register::R14w,"r14w"},
		{X86_Register::R15w,"r15w"},

		{X86_Register::EAX,"eax"},
		{X86_Register::EBX,"ebx"},
		{X86_Register::ECX,"ecx"},
		{X86_Register::EDX,"edx"},
		{X86_Register::R8d,"r8d"},
		{X86_Register::R9d,"r9d"},
		{X86_Register::R10d,"r10d"},
		{X86_Register::R11d,"r11d"},
		{X86_Register::R12d,"r12d"},
		{X86_Register::R13d,"r13d"},
		{X86_Register::R14d,"r14d"},
		{X86_Register::R15d,"r15d"},

		{X86_Register::RAX,"rax"},
		{X86_Register::RBX,"rbx"},
		{X86_Register::RCX,"rcx"},
		{X86_Register::RDX,"rdx"},
		{X86_Register::R8,"r8"},
		{X86_Register::R9,"r9"},
		{X86_Register::R10,"r10"},
		{X86_Register::R11,"r11"},
		{X86_Register::R12,"r12"},
		{X86_Register::R13,"r13"},
		{X86_Register::R14,"r14"},
		{X86_Register::R15,"r15"},

		{X86_Register::XMM0,"xmm0"},
		{X86_Register::XMM1,"xmm1"},
		{X86_Register::XMM2,"xmm2"},
		{X86_Register::XMM3,"xmm3"},
		{X86_Register::XMM4,"xmm4"},
		{X86_Register::XMM5,"xmm5"},
		{X86_Register::XMM6,"xmm6"},
		{X86_Register::XMM7,"xmm7"},

	};

	const std::map<std::pair<u64, X86_Register_Family>, X86_Register> register_family_map = {
		{{1,F_A},AL},
		{{2,F_A},AX},
		{{4,F_A},EAX},
		{{8,F_A},RAX},

		{{1,F_B},BL},
		{{2,F_B},BX},
		{{4,F_B},EBX},
		{{8,F_B},RBX},

		{{1,F_C},CL},
		{{2,F_C},CX},
		{{4,F_C},ECX},
		{{8,F_C},RCX},

		{{1,F_D},DL},
		{{2,F_D},DX},
		{{4,F_D},EDX},
		{{8,F_D},RDX},

		{{1,F_R8},R8b},
		{{2,F_R8},R8w},
		{{4,F_R8},R8d},
		{{8,F_R8},R8},

		{{1,F_R9},R9b},
		{{2,F_R9},R9w},
		{{4,F_R9},R9d},
		{{8,F_R9},R9},

		{{1,F_R10},R10b},
		{{2,F_R10},R10w},
		{{4,F_R10},R10d},
		{{8,F_R10},R10},

		{{1,F_R11},R11b},
		{{2,F_R11},R11w},
		{{4,F_R11},R11d},
		{{8,F_R11},R11},

		{{1,F_R12},R12b},
		{{2,F_R12},R12w},
		{{4,F_R12},R12d},
		{{8,F_R12},R12},

		{{1,F_R13},R13b},
		{{2,F_R13},R13w},
		{{4,F_R13},R13d},
		{{8,F_R13},R13},

		{{1,F_R14},R14b},
		{{2,F_R14},R14w},
		{{4,F_R14},R14d},
		{{8,F_R14},R14},

		{{1,F_R15},R15b},
		{{2,F_R15},R15w},
		{{4,F_R15},R15d},
		{{8,F_R15},R15},
	};

	const std::map<X86_Register_Family, X86_Register> register_floating_family_map = {
		{F_X0,XMM0},
		{F_X1,XMM1},
		{F_X2,XMM2},
		{F_X3,XMM3},
		{F_X4,XMM4},
		{F_X5,XMM5},
		{F_X6,XMM6},
		{F_X7,XMM7},
	};

	const std::map<X86_Register, X86_Register_Family> register_to_family_map = {
		{AL,F_A},
		{AX,F_A},
		{EAX,F_A},
		{RAX,F_A},

		{BL,F_B},
		{BX,F_B},
		{EBX,F_B},
		{RBX,F_B},

		{CL,F_C},
		{CX,F_C},
		{ECX,F_C},
		{RCX,F_C},

		{DL,F_D},
		{DX,F_D},
		{EDX,F_D},
		{RDX,F_D},

		{R8b,F_R8},
		{R8w,F_R8},
		{R8d,F_R8},
		{R8,F_R8},

		{R9b,F_R9},
		{R9w,F_R9},
		{R9d,F_R9},
		{R9,F_R9},

		{R10b,F_R10},
		{R10w,F_R10},
		{R10d,F_R10},
		{R10,F_R10},

		{R11b,F_R11},
		{R11w,F_R11},
		{R11d,F_R11},
		{R11,F_R11},

		{R12b,F_R12},
		{R12w,F_R12},
		{R12d,F_R12},
		{R12,F_R12},

		{R13b,F_R13},
		{R13w,F_R13},
		{R13d,F_R13},
		{R13,F_R13},

		{R14b,F_R14},
		{R14w,F_R14},
		{R14d,F_R14},
		{R14,F_R14},

		{R15b,F_R15},
		{R15w,F_R15},
		{R15d,F_R15},
		{R15,F_R15},

		{XMM0,F_X0},
		{XMM1,F_X1},
		{XMM2,F_X2},
		{XMM3,F_X3},
		{XMM4,F_X4},
		{XMM5,F_X5},
		{XMM6,F_X6},
		{XMM7,F_X7},
	};

	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		: m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
		Init();
	}

	void X86_BackEnd::Init()
	{
		Register_Allocator_Data.allocated = {
			{F_A, false},
			{F_B, false},
			{F_C, false},
			{F_D, false},
			{F_R8, false},
			{F_R9, false},
			{F_R10, false},
			{F_R11, false},
			{F_R12, false},
			{F_R13, false},
			{F_R14, false},
			{F_R15, false},
		};

		Register_Allocator_Data.allocated_floating = {
			{F_X0, false},
			{F_X1, false},
			{F_X2, false},
			{F_X3, false},
			{F_X4, false},
			{F_X5, false},
			{F_X6, false},
			{F_X7, false},
		};
	}

	std::string X86_BackEnd::Mangle_Name(std::string name, TypeStorage* type)
	{
		for (auto& c : name) {
			if (c == '*') {
				c = 'p';
			}
		}

		return fmt::format("{}_{}", name, (void*)type->Hash);
	}

	void X86_BackEnd::AssembleForeignLibraries()
	{
		bool has_msvcrt = false;
		bool has_kernel32 = false;

		for (const auto& [name, library] : m_Metadata->Libraries) {

			Assembly_Dynamic_Library library_import;
			library_import.Name = library.Name.Symbol;
			library_import.Path = library.Value->Symbol.Symbol;

			if (library_import.Name == "msvcrt") {
				has_msvcrt = true;
			}

			if (library_import.Name == "kernel32") {
				has_kernel32 = true;
			}

			m_Data.Library_Indices[library.Name.Symbol] = Dynamic_Libraries.size();

			Dynamic_Libraries.push_back(library_import);
		}

		if (!has_msvcrt) {

			Assembly_Dynamic_Library library_import;

			library_import.Name = "msvcrt";
			library_import.Path = "msvcrt.dll";

			m_Data.Library_Indices["msvcrt"] = Dynamic_Libraries.size();

			Dynamic_Libraries.push_back(library_import);
		}

		if (!has_kernel32) {

			Assembly_Dynamic_Library library_import;

			library_import.Name = "kernel32";
			library_import.Path = "kernel32";

			m_Data.Library_Indices["kernel32"] = Dynamic_Libraries.size();

			Dynamic_Libraries.push_back(library_import);
		}
	}

	void X86_BackEnd::AssembleForeignImport(const FunctionMetadata* function)
	{
		Assembly_Import external;
		external.Name = function->Symbol.Symbol;
		external.library_idx = m_Data.Library_Indices.at(function->Foreign_Library);

		Imports.push_back(external);
	}

	void X86_BackEnd::AssembleExternalFunction(const FunctionMetadata* function)
	{
		Assembly_External_Symbol external;
		external.Name = function->Symbol.Symbol;
		external.ExternalName = function->Symbol.Symbol;

		Externals.push_back(external);
	}

	void X86_BackEnd::AssembleExternals()
	{
		bool has_memcpy = false;
		bool has_memset = false;
		bool has_exit_process = false;

		for (const auto& function_pair : m_Metadata->m_Functions) {

			const FunctionMetadata& function = function_pair.second;

			if (function.Symbol.Symbol == "memcpy") {
				has_memcpy = true;
			}

			if (function.Symbol.Symbol == "memset") {
				has_memset = true;
			}

			if (function.Symbol.Symbol == "ExitProcess") {
				has_exit_process = true;
			}

			if (function.Foreign) {
				if (Use_Linker) {
					AssembleExternalFunction(&function);
				}
				else {
					AssembleForeignImport(&function);
				}
			}
		}

		//intrinsics

		//memcpy
		if (Use_Linker) {
			if (!has_memcpy)
			{
				Assembly_External_Symbol external;
				external.Name = "memcpy";
				external.ExternalName = "memcpy";

				Externals.push_back(external);
			}
			if (!has_memset)
			{
				Assembly_External_Symbol external;
				external.Name = "memset";
				external.ExternalName = "memset";

				Externals.push_back(external);
			}
		}
		else {
			if (!has_memcpy)
			{
				Assembly_Import external;
				external.Name = "memcpy";
				external.library_idx = m_Data.Library_Indices.at("msvcrt");

				Imports.push_back(external);
			}

			if (!has_memset)
			{
				Assembly_Import external;
				external.Name = "memset";
				external.library_idx = m_Data.Library_Indices.at("msvcrt");

				Imports.push_back(external);
			}

			if (!has_exit_process)
			{
				Assembly_Import external;
				external.Name = "ExitProcess";
				external.library_idx = m_Data.Library_Indices.at("kernel32");

				Imports.push_back(external);
			}
		}

	}

	void X86_BackEnd::AssembleTypeInfoTable()
	{
		const u64 Type_Info_Element_Size = 64;

		std::vector<TypeStorage*> type_info_map = TypeSystem::GetTypeMap();

		Assembly_TypeInfo_Table assembly_type_info_table;
		assembly_type_info_table.External_Variable_Name = "TypeInfo_Array";
		assembly_type_info_table.External_Members_Array_Name = "TypeInfo_Members_Array";
		assembly_type_info_table.External_Enum_Members_Array_Name = "TypeInfo_Enum_Members_Array";
		assembly_type_info_table.External_Func_Type_Parameters_Array_Name = "TypeInfo_Func_Type_Parameters_Array";

		TypeInfo_Table_Label = Builder::Symbol("TypeInfo_Array");

		auto Func_Type_Parameters_Array_Name = Builder::Symbol("TypeInfo_Func_Type_Parameters_Array");

		auto Enum_Members_Array_Symbol = Builder::Symbol("TypeInfo_Enum_Members_Array");

		std::unordered_map<u64, u64> Enum_Members_Offsets;

		u64 i = 0;
		for (auto& [enum_id, enum_metadata] : m_Metadata->m_Enums)
		{
			Enum_Members_Offsets[enum_id] = Type_Info_Element_Size * i;

			for (const EnumMemberMetadata& member : enum_metadata.Members) {
				auto member_name_string = Create_String_Constant(member.Name);

				Assembly_TypeInfo_Table_Entry entry = {};
				entry.elements[0] = member_name_string;
				entry.elements[1] = Builder::Constant_Integer(member.Value);
				entry.elements[2] = Builder::Constant_Integer(0);
				entry.elements[3] = Builder::Constant_Integer(0);
				entry.elements[4] = Builder::Constant_Integer(0);
				entry.elements[5] = Builder::Constant_Integer(0);
				entry.elements[6] = Builder::Constant_Integer(0);
				entry.elements[7] = Builder::Constant_Integer(0);
				assembly_type_info_table.Enum_Members_Array.push_back(entry);
				i++;
			}
		}

		std::unordered_map<u64, u64> Struct_Members_Offsets;

		auto Struct_Members_Array_Symbol = Builder::Symbol("TypeInfo_Members_Array");

		i = 0;
		for (auto& [struct_id, struct_metadata] : m_Metadata->m_StructMetadata)
		{
			Struct_Members_Offsets[struct_id] = Type_Info_Element_Size * i;

			for (const MemberMetadata& member : struct_metadata.Members) {
				auto member_name_string = Create_String_Constant(member.Name.Symbol);

				Assembly_TypeInfo_Table_Entry entry = {};
				entry.elements[0] = member_name_string;
				entry.elements[1] = Builder::Constant_Integer(TypeSystem::GetTypeInfoIndex(member.Type));
				entry.elements[2] = Builder::Constant_Integer(member.Offset);
				entry.elements[3] = Builder::Constant_Integer(0);
				entry.elements[4] = Builder::Constant_Integer(0);
				entry.elements[5] = Builder::Constant_Integer(0);
				entry.elements[6] = Builder::Constant_Integer(0);
				entry.elements[7] = Builder::Constant_Integer(0);
				assembly_type_info_table.Members_Array.push_back(entry);
				i++;
			}
		}

		u64 func_type_param_i = 0;

		for (size_t i = 0; i < type_info_map.size(); i++)
		{
			TypeStorage* type = type_info_map[i];

			if (type->Kind == TypeStorageKind::Base) {

				auto flags = TypeSystem::GetTypeFlags(type);

				if (flags & FLAG_ENUM_TYPE) {
					const EnumMetadata* enum_metadata = m_Metadata->GetEnumFromType(type->BaseID);

					GS_CORE_ASSERT(enum_metadata);

					auto type_name_string = Create_String_Constant(enum_metadata->Name.Symbol);

					Assembly_TypeInfo_Table_Entry entry = {};
					entry.elements[0] = type_name_string;
					entry.elements[1] = Builder::Constant_Integer(TI_ENUM);
					entry.elements[2] = Builder::Constant_Integer(TypeSystem::GetTypeSize(type));
					entry.elements[3] = Builder::Constant_Integer(enum_metadata->Members.size());
					entry.elements[4] = Builder::OpAdd(Enum_Members_Array_Symbol, Builder::Constant_Integer(Enum_Members_Offsets.at(enum_metadata->EnumID)));
					entry.elements[5] = Builder::Constant_Integer(0);
					entry.elements[6] = Builder::Constant_Integer(0);
					entry.elements[7] = Builder::Constant_Integer(0);

					assembly_type_info_table.Entries.push_back(entry);
				}
				else if (flags & FLAG_STRUCT_TYPE) {
					u64 struct_id = m_Metadata->GetStructIDFromType(type->BaseID);
					const StructMetadata* struct_metadata = m_Metadata->GetStructFromType(type->BaseID);
					GS_CORE_ASSERT(struct_metadata);

					auto type_name_string = Create_String_Constant(struct_metadata->Name.Symbol);

					Assembly_TypeInfo_Table_Entry entry = {};
					entry.elements[0] = type_name_string;
					entry.elements[1] = Builder::Constant_Integer(m_Metadata->GetTypeInfoFlags(type->BaseID));
					entry.elements[2] = Builder::Constant_Integer(TypeSystem::GetTypeSize(type));
					entry.elements[3] = Builder::Constant_Integer(struct_metadata->Members.size());
					entry.elements[4] = Builder::OpAdd(Struct_Members_Array_Symbol, Builder::Constant_Integer(Struct_Members_Offsets.at(struct_id)));
					entry.elements[5] = Builder::Constant_Integer(0);
					entry.elements[6] = Builder::Constant_Integer(0);
					entry.elements[7] = Builder::Constant_Integer(0);

					assembly_type_info_table.Entries.push_back(entry);
				}
				else {
					std::string type_name = TypeSystem::PrintType(type);
					auto type_name_string = Create_String_Constant(type_name);

					Assembly_TypeInfo_Table_Entry entry = {};
					entry.elements[0] = type_name_string;
					entry.elements[1] = Builder::Constant_Integer(m_Metadata->GetTypeInfoFlags(type->BaseID));
					entry.elements[2] = Builder::Constant_Integer(TypeSystem::GetTypeSize(type));
					entry.elements[3] = Builder::Constant_Integer(0);
					entry.elements[4] = Builder::Constant_Integer(0);
					entry.elements[5] = Builder::Constant_Integer(0);
					entry.elements[6] = Builder::Constant_Integer(0);
					entry.elements[7] = Builder::Constant_Integer(0);

					assembly_type_info_table.Entries.push_back(entry);
				}
			}
			else if (type->Kind == TypeStorageKind::Pointer) {
				std::string type_name = TypeSystem::PrintType(type);
				auto type_name_string = Create_String_Constant(type_name);

				Assembly_TypeInfo_Table_Entry entry = {};
				entry.elements[0] = type_name_string;
				entry.elements[1] = Builder::Constant_Integer(TI_POINTER);
				entry.elements[2] = Builder::Constant_Integer(TypeSystem::GetTypeInfoIndex(((TSPtr*)type)->Pointee));
				entry.elements[3] = Builder::Constant_Integer(((TSPtr*)type)->Indirection);
				entry.elements[4] = Builder::Constant_Integer(0);
				entry.elements[5] = Builder::Constant_Integer(0);
				entry.elements[6] = Builder::Constant_Integer(0);
				entry.elements[7] = Builder::Constant_Integer(0);

				assembly_type_info_table.Entries.push_back(entry);
			}
			else if (type->Kind == TypeStorageKind::DynArray) {
				std::string type_name = TypeSystem::PrintType(type);
				auto type_name_string = Create_String_Constant(type_name);

				Assembly_TypeInfo_Table_Entry entry = {};
				entry.elements[0] = type_name_string;
				entry.elements[1] = Builder::Constant_Integer(TI_DYN_ARRAY);
				entry.elements[2] = Builder::Constant_Integer(TypeSystem::GetTypeInfoIndex(((TSDynArray*)type)->ElementType));
				entry.elements[3] = Builder::Constant_Integer(0);
				entry.elements[4] = Builder::Constant_Integer(0);
				entry.elements[5] = Builder::Constant_Integer(0);
				entry.elements[6] = Builder::Constant_Integer(0);
				entry.elements[7] = Builder::Constant_Integer(0);

				assembly_type_info_table.Entries.push_back(entry);
			}
			else if (type->Kind == TypeStorageKind::Function) {
				TSFunc* as_func = (TSFunc*)type;

				std::string type_name = TypeSystem::PrintType(type);
				auto type_name_string = Create_String_Constant(type_name);

				Assembly_TypeInfo_Table_Entry entry = {};
				entry.elements[0] = type_name_string;
				entry.elements[1] = Builder::Constant_Integer(TI_FUNCTION);
				entry.elements[2] = Builder::Constant_Integer(TypeSystem::GetTypeInfoIndex(as_func->ReturnType));
				entry.elements[3] = Builder::Constant_Integer(as_func->Arguments.size());
				entry.elements[4] = Builder::OpAdd(Func_Type_Parameters_Array_Name, Builder::Constant_Integer(func_type_param_i * 8));
				entry.elements[5] = Builder::Constant_Integer(0);
				entry.elements[6] = Builder::Constant_Integer(0);
				entry.elements[7] = Builder::Constant_Integer(0);

				assembly_type_info_table.Entries.push_back(entry);

				for (auto parameter : as_func->Arguments) {

					Assembly_TypeInfo_Table_Entry param_entry = {};

					param_entry.elements[0] = Builder::Constant_Integer(TypeSystem::GetTypeInfoIndex(parameter));

					assembly_type_info_table.Func_Type_Parameters_Array.push_back(param_entry);

					func_type_param_i++;
				}
			}
		}

		Program_Assembly_TypeInfo_Table = assembly_type_info_table;
	}

	void X86_BackEnd::Assemble()
	{
		std::chrono::steady_clock::time_point Start;
		std::chrono::steady_clock::time_point End;

		Start = std::chrono::high_resolution_clock::now();

		AssembleTypeInfoTable();

		AssembleForeignLibraries();
		AssembleExternals();

		for (auto i : m_TranslationUnit->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions) {
					if (tl_inst->GetType() == IRNodeType::Function) {
						AssembleFunctionSymbol((IRFunction*)tl_inst);
					}
				}
			}
		}

		for (auto i : m_TranslationUnit->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions) {
					AssembleInstruction(tl_inst);
				}
			}
			else {
				AssembleInstruction(i);
			}
		}

		Assembly_File assembly;

		if (Use_Linker) {
			assembly.externals = Externals;
			assembly.output_mode = Assembler_Output_Mode::COFF_Object;
		}
		else {
			assembly.output_mode = Assembler_Output_Mode::PE_Executable;
			assembly.libraries = Dynamic_Libraries;
			assembly.imports = Imports;
		}

		assembly.globals = Globals;
		assembly.floats = Floats;
		assembly.strings = Strings;
		assembly.type_info_table = Program_Assembly_TypeInfo_Table;

		for (auto func : Functions) {
			assembly.functions.push_back(*func);
		}

		End = std::chrono::high_resolution_clock::now();

		FASM_Printer fasm_printer(&assembly);
		std::chrono::steady_clock::time_point Print_Start = std::chrono::high_resolution_clock::now();
		std::string fasm_output = fasm_printer.Print();
		std::chrono::steady_clock::time_point Print_End = std::chrono::high_resolution_clock::now();

		if (!std::filesystem::exists(".build")) {
			std::filesystem::create_directory(".build");
		}

		std::chrono::steady_clock::time_point Write_Start = std::chrono::high_resolution_clock::now();
		{
			auto file_stream = std::ofstream(".build/fasm.s");
			file_stream << fasm_output;
		}
		std::chrono::steady_clock::time_point Write_End = std::chrono::high_resolution_clock::now();

		GS_CORE_WARN("Running Fasm");

		std::chrono::steady_clock::time_point Fasm_Start = std::chrono::high_resolution_clock::now();
		int fasm_result = system("fasm .build/fasm.s");
		std::chrono::steady_clock::time_point Fasm_End = std::chrono::high_resolution_clock::now();

		std::chrono::steady_clock::time_point Linker_Start = std::chrono::high_resolution_clock::now();
		if (Use_Linker)
		{
			GS_CORE_WARN("Running Linker On Fasm Output");
			std::stringstream libraries;
			for (const auto& [name, library] : m_Metadata->Libraries) {

				if (name == "msvcrt") {
					continue;
				}

				auto library_path = fs_path(library.Value->Symbol.Symbol);
				libraries << library_path << " ";
			}

			std::string linker_command = fmt::format("clang ./.build/fasm.obj -O0 {}", libraries.str());
			system(linker_command.c_str());
		}
		else {
			if (!fasm_result) {
				std::filesystem::copy(".build/fasm.exe", "a.exe", std::filesystem::copy_options::overwrite_existing);
			}
		}

		if (fasm_result) {
			GS_CORE_WARN("Fasm Failed!");
			return;
		}

		std::chrono::steady_clock::time_point Linker_End = std::chrono::high_resolution_clock::now();

		GS_CORE_WARN("Assembly Generation Took: {} milli s", std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count());
		GS_CORE_WARN("FASM Took: {} mill s", std::chrono::duration_cast<std::chrono::milliseconds>(Fasm_End - Fasm_Start).count());
		GS_CORE_WARN("Assembly Print Took: {} mill s", std::chrono::duration_cast<std::chrono::milliseconds>(Print_End - Print_Start).count());
		//GS_CORE_WARN("Assembly Flush To File Took: {} mill s", std::chrono::duration_cast<std::chrono::milliseconds>(Write_End - Write_Start).count());
		if (Use_Linker) {
			GS_CORE_WARN("Linker Took: {} mill s", std::chrono::duration_cast<std::chrono::milliseconds>(Linker_End - Linker_Start).count());
		}
	}

	void X86_BackEnd::AssembleInstruction(IRInstruction* instruction)
	{
		switch (instruction->GetType()) {
		case IRNodeType::Function:
			AssembleFunction((IRFunction*)instruction);
			break;
		case IRNodeType::Call:
			AssembleFunctionCall((IRFunctionCall*)instruction);
			break;
		case IRNodeType::Register:
			AssembleRegister((IRRegister*)instruction);
			break;
		case IRNodeType::Argument:
			AssembleArgument((IRArgumentAllocation*)instruction);
			break;
		case IRNodeType::Alloca:
			AssembleAlloca((IRAlloca*)instruction);
			break;
		case IRNodeType::Store:
			AssembleStore((IRStore*)instruction);
			break;
		case IRNodeType::Load:
			AssembleLoad((IRLoad*)instruction);
			break;
		case IRNodeType::ConstValue:
			AssembleConstValue((IRCONSTValue*)instruction);
			break;
		case IRNodeType::Data:
			AssembleData((IRData*)instruction);
			break;
		case IRNodeType::DataValue:
			AssembleDataValue((IRDataValue*)instruction);
			break;
		case IRNodeType::ADD:
			AssembleAdd((IRADD*)instruction);
			break;
		case IRNodeType::SUB:
			AssembleSub((IRSUB*)instruction);
			break;
		case IRNodeType::MUL:
			AssembleMul((IRMUL*)instruction);
			break;
		case IRNodeType::DIV:
			AssembleDiv((IRDIV*)instruction);
			break;
		case IRNodeType::SREM:
			AssembleSRem((IRSREM*)instruction);
			break;
		case IRNodeType::And:
			AssembleAnd((IRAnd*)instruction);
			break;
		case IRNodeType::Or:
			AssembleOr((IROr*)instruction);
			break;
		case IRNodeType::Equal:
			AssembleEqual((IREQ*)instruction);
			break;
		case IRNodeType::NotEqual:
			AssembleNotEqual((IRNOTEQ*)instruction);
			break;
		case IRNodeType::LesserThan:
			AssembleLesser((IRLesser*)instruction);
			break;
		case IRNodeType::GreaterThan:
			AssembleGreater((IRGreater*)instruction);
			break;
		case IRNodeType::BitAnd:
			AssembleBitAnd((IRBitAnd*)instruction);
			break;
		case IRNodeType::BitOr:
			AssembleBitOr((IRBitOr*)instruction);
			break;
		case IRNodeType::Return:
			AssembleReturn((IRReturn*)instruction);
			break;
		case IRNodeType::MemberAccess:
			AssembleMemberAccess((IRMemberAccess*)instruction);
			break;
		case IRNodeType::ArrayAccess:
			AssembleArrayAccess((IRArrayAccess*)instruction);
			break;

		case IRNodeType::If:
			AssembleIf((IRIf*)instruction);
			break;
		case IRNodeType::While:
			AssembleWhile((IRWhile*)instruction);
			break;
		case IRNodeType::LexicalBlock:
			AssembleLexicalBlock((IRLexBlock*)instruction);
			break;

		case IRNodeType::PointerCast:
			AssemblePointerCast((IRPointerCast*)instruction);
			break;
		case IRNodeType::Int2PtrCast:
			AssembleInt2PCast((IRInt2PtrCast*)instruction);
			break;
		case IRNodeType::Ptr2IntCast:
			AssemblePtr2IntCast((IRPtr2IntCast*)instruction);
			break;
		case IRNodeType::SExtCast:
			AssembleSextCast((IRSExtCast*)instruction);
			break;
		case IRNodeType::ZExtCast:
			AssembleZextCast((IRZExtCast*)instruction);
			break;
		case IRNodeType::IntTrunc:
			AssembleIntTruncCast((IRIntTrunc*)instruction);
			break;
		case IRNodeType::Int2FP:
			AssembleInt2FPCast((IRInt2FP*)instruction);
			break;
		case IRNodeType::FP2Int:
			AssembleFP2IntCast((IRFP2Int*)instruction);
			break;
		case IRNodeType::FPExt:
			AssembleFPExtCast((IRFPExt*)instruction);
			break;
		case IRNodeType::FPTrunc:
			AssembleFPTruncCast((IRFPTrunc*)instruction);
			break;
		case IRNodeType::FuncRef:
			AssembleFuncRef((IRFuncRef*)instruction);
			break;
		case IRNodeType::CallFuncRef:
			AssembleCallFuncRef((IRCallFuncRef*)instruction);
			break;

		case IRNodeType::GlobDecl:
			AssembleGlobalDeclare((IRGlobalDecl*)instruction);
			break;

		case IRNodeType::GlobAddress:
			AssembleGlobalAddress((IRGlobalAddress*)instruction);
			break;
		case IRNodeType::TypeValue:
			AssembleTypeValue((IRTypeValue*)instruction);
			break;
		case IRNodeType::TypeInfo:
			AssembleTypeInfo((IRTypeInfo*)instruction);
			break;
		case IRNodeType::Break:
			AssembleBreak((IRBreak*)instruction);
			break;
		case IRNodeType::NullPtr:
			AssembleNullPtr((IRNullPtr*)instruction);
			break;

		case IRNodeType::Intrinsic_MemSet:
			AssembleIntrinsic_MemSet((IRIntrinsicMemSet*)instruction);
			break;

		case IRNodeType::AnyArray:
			AssembleAnyArray((IRAnyArray*)instruction);
			break;

		case IRNodeType::String_Initializer:
			AssembleString_Initializer((IRStringInitializer*)instruction);
			break;

		case IRNodeType::RegisterValue:
		{
			IRRegisterValue* ir_register_value = (IRRegisterValue*)instruction;

			if (m_Data.IR_RegisterValues.find(ir_register_value->RegisterID) == m_Data.IR_RegisterValues.end()) {
				break;
			}

			auto register_value = GetRegisterValue(ir_register_value);
			auto register_value_type = GetRegisterValueType(ir_register_value);

			if (register_value_type == Register_Value_Type::Pointer_Address || register_value_type == Register_Value_Type::Stack_Address)
			{
				auto result_location = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

				auto lea = Builder::Lea(result_location, Builder::De_Reference(register_value));
				lea.Comment = "register value";
				Code.push_back(lea);
				SetRegisterValue(result_location, CurrentRegister, Register_Value_Type::Pointer_Address);
			}
			else {
				auto result_location = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);
				auto move = Builder::Mov(result_location, register_value);
				move.Comment = "register value";
				Code.push_back(move);
				SetRegisterValue(result_location, CurrentRegister, Register_Value_Type::Register_Value);
			}

			UseRegisterValue(ir_register_value);
		}
		break;
		default:
			GS_CORE_ASSERT(nullptr, "Un Implemented Instruction");
		}
	}

	void X86_BackEnd::AssembleFunctionSymbol(IRFunction* ir_function)
	{
		const FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);

		if (metadata->PolyMorphic) {
			for (auto& [overload, instance] : metadata->PolyMorphicInstantiations) {
				AssembleFunctionSymbol(instance);
			}
			return;
		}

		if (ir_function->Overload) {
			metadata = &metadata->GetOverload((TSFunc*)ir_function->Overload);
		}

		const auto& name = metadata->Symbol.Symbol;

		Assembly_Function* assembly = ASMA(Assembly_Function());

		assembly->Name = name;

		if (name != "main") {
			assembly->Name = Mangle_Name(name, metadata->Signature);
		}

		Functions.push_back(assembly);

		if (ir_function->Overload) {
			m_Data.FunctionOverloads[ir_function->ID][(TSFunc*)ir_function->Overload] = assembly;
		}
		else {
			m_Data.Functions[ir_function->ID] = assembly;
		}
	}

	const std::map<std::pair<u64, u64>, X86_Register> argument_register_map = {
		{ {8,0}, RCX},
		{ {8,1}, RDX},
		{ {8,2}, R8},
		{ {8,3}, R9},

		{ {4,0}, ECX},
		{ {4,1}, EDX},
		{ {4,2}, R8d},
		{ {4,3}, R9d},

		{ {2,0}, CX},
		{ {2,1}, DX},
		{ {2,2}, R8w},
		{ {2,3}, R9w},

		{ {1,0}, CL},
		{ {1,1}, DL},
		{ {1,2}, R8b},
		{ {1,3}, R9b},
	};

	const std::map<u64, X86_Register> argument_float_register_map = {
		{ 0, XMM0},
		{ 1, XMM1},
		{ 2, XMM2},
		{ 3, XMM3},
	};

	void X86_BackEnd::AssembleFunction(IRFunction* ir_function)
	{
		auto metadata = m_Metadata->GetFunctionMetadata(ir_function->ID);

		if (metadata->PolyMorphic) {
			for (auto& [overload, instance] : metadata->PolyMorphicInstantiations) {
				AssembleFunction(instance);
			}
			return;
		}

		if (ir_function->Overload) {
			metadata = &metadata->GetOverload((TSFunc*)ir_function->Overload);
		}

		Code.clear();

		m_Data.IR_RegisterValues.clear();
		m_Data.IR_RegisterTypes.clear();
		m_Data.IR_RegisterValueTypes.clear();
		m_Data.IR_RegisterLifetimes.clear();
		m_Data.Stack_Size = 0;
		m_Data.Call_Stack_Pointer = 0;
		m_Data.Call_Stack_Size = 0;

		m_Metadata->m_CurrentFunction = ir_function->ID;

		Return_Storage_Location = nullptr;
		Return_Encountered = false;
		Return_Label = nullptr;

		Register_Allocator_Data.allocations.clear();
		Register_Allocator_Data.family_to_allocation.clear();

		for (auto& [family, allocated] : Register_Allocator_Data.allocated) {
			allocated = false;
		}

		for (auto& [family, allocated] : Register_Allocator_Data.allocated_floating) {
			allocated = false;
		}

		Assembly_Function* assembly = m_Data.Functions[ir_function->ID];

		Return_Label = Builder::Symbol(GetLabelName());

		if (metadata->ReturnType != TypeSystem::GetVoid()) {

			auto return_type = metadata->ReturnType;
			auto return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);

			if (return_type_size > 8) {
				return_type = TypeSystem::GetPtr(return_type, 1);
			}

			Return_Storage_Location = Stack_Alloc(return_type);
		}

		auto return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);

		if (return_type_size > 8) {
			auto return_type = TypeSystem::GetPtr(metadata->ReturnType, 1);
			Code.push_back(MoveBasedOnType(return_type, Builder::De_Reference(Return_Storage_Location, return_type), Builder::Register(RAX)));
		}

		u64 i = 0;

		for (const ArgumentMetadata& argument : metadata->Arguments) {

			CurrentRegister = argument.AllocationLocation->RegisterID;

			auto argument_type = argument.Type;

			if (TypeSystem::GetTypeSize(argument_type) > 8) {
				argument_type = TypeSystem::GetPtr(argument_type, 1);
			}

			auto type_size = TypeSystem::GetTypeSize(argument_type);

			if (i < 4)
			{
				X86_Register needed_register;

				if (!TypeSystem::IsFlt(argument_type)) {
					needed_register = argument_register_map.at({ type_size, i });
				}
				else {
					needed_register = argument_float_register_map.at(i);
				}

				SetRegisterValue(Allocate_Register(argument_type, CurrentRegister, needed_register), Register_Value_Type::Register_Value);
			}
			else {
				SetRegisterValue(Builder::De_Reference(Builder::OpAdd(Builder::Register(RBP), Builder::Constant_Integer(40 + (i - 3) * 8)), argument_type), Register_Value_Type::Memory_Value);
			}
			i++;
		}

		for (auto instruction : ir_function->Instructions) {

			if (Break_Encountered) {
				Break_Encountered = false;
				break;
			}

			if (Return_Encountered) {
				Return_Encountered = false;
				break;
			}

			AssembleInstruction(instruction);
		}


		Assembly_Operand* stack_size_constant = Builder::Constant_Integer(32);

		std::vector<Assembly_Instruction> prologue;

		prologue.push_back(Builder::Push(Builder::Register(RBP)));
		prologue.push_back(Builder::Sub(Builder::Register(RSP), stack_size_constant));
		prologue.push_back(Builder::Lea(Builder::Register(RBP), Builder::De_Reference(Builder::OpAdd(Builder::Register(RSP), stack_size_constant))));

		std::vector<std::pair<X86_Register, Assembly_Operand*>> saved_registers;

		for (auto used_register : Current_Function_Used_Registers) {
			X86_Register used_register_as_reg = (X86_Register)used_register;
			if (!Is_Volatile(used_register_as_reg)) {

				auto save_location = Builder::De_Reference(Stack_Alloc(TypeSystem::GetVoidPtr()), TypeSystem::GetVoidPtr());

				prologue.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), save_location, Builder::Register(used_register_as_reg), "Save Register"));
				saved_registers.push_back({ used_register_as_reg,save_location });
			}
		}

		stack_size_constant->constant_integer.integer += m_Data.Stack_Size;
		stack_size_constant->constant_integer.integer += m_Data.Call_Stack_Size + 32 + 8; // the push rbp offset + because the stack addressing works in reverse we must add this 8 here or atleast this how i understand
		stack_size_constant->constant_integer.integer = (i64)align_to(stack_size_constant->constant_integer.integer, 16);

		Code.push_back(Builder::Label(Return_Label));

		if (Return_Storage_Location) {

			auto return_type = metadata->ReturnType;
			auto return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);

			if (return_type_size > 8) {
				return_type = TypeSystem::GetPtr(return_type, 1);
			}

			auto return_location = GetReturnRegister(metadata->ReturnType);
			Code.push_back(MoveBasedOnType(return_type, return_location, Builder::De_Reference(Return_Storage_Location, return_type)));
		}

		if (metadata->Symbol.Symbol == "main" && Use_Linker == false) {

			if (Return_Storage_Location) {
				Code.push_back(Builder::Mov(Builder::Register(ECX), Builder::De_Reference(Return_Storage_Location, TypeSystem::GetI32())));
			}
			else {
				Code.push_back(Builder::Mov(Builder::Register(ECX), Builder::Constant_Integer(0)));
			}

			Code.push_back(Builder::Call(Builder::De_Reference(Builder::Symbol("ExitProcess"))));
		}

		for (int i = saved_registers.size() - 1; i >= 0; i--)
		{
			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), Builder::Register(saved_registers[i].first), saved_registers[i].second, "Reload"));
		}

		Code.push_back(Builder::Add(Builder::Register(RSP), stack_size_constant));
		Code.push_back(Builder::Pop(Builder::Register(RBP)));
		Code.push_back(Builder::Ret());

		for (auto& code : prologue) {
			assembly->Code.push_back(code);
		}

		for (auto& code : Code) {
			assembly->Code.push_back(code);
		}

		Function_Counter++;
		Label_Counter = 0;
	}

	void X86_BackEnd::AssembleFunctionCall(IRFunctionCall* ir_function)
	{
		const FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(ir_function->FuncID);

		if (ir_function->Overload) {
			metadata = &metadata->GetOverload((TSFunc*)ir_function->Overload);
		}

		std::string name = metadata->Symbol.Symbol;

		if (!metadata->Foreign) {
			name = Mangle_Name(name, metadata->Signature);
		}

		static std::map<u64, X86_Register> return_register_map = {
			{1, AL},
			{2, AX},
			{4, EAX},
			{8, RAX},
		};

		Spill_All_Scratch();

		std::vector<u64> argument_allocations;
		std::vector <std::pair<Assembly_Operand*, Assembly_Operand*>> argument_allocations_registers;

		for (size_t i = 0; i < ir_function->Arguments.size(); i++)
		{
			auto call_argument = (IRRegisterValue*)ir_function->Arguments[i];

			TypeStorage* argument_type = ir_function->ArgumentTypes[i];

			auto type_size = TypeSystem::GetTypeSize(argument_type);

			if (type_size > 8) {
				argument_type = TypeSystem::GetPtr(argument_type, 1);
				type_size = TypeSystem::GetTypeSize(argument_type);
			}

			if (metadata->Variadic) {
				if (type_size < 4) {
					type_size = 4;
				}
			}

			if (type_size <= 8) {

				X86_Register needed_register;

				if (i < 4)
				{
					if (!TypeSystem::IsFlt(argument_type)) {
						needed_register = argument_register_map.at({ type_size,i });
					}
					else {
						needed_register = argument_float_register_map.at(i);
					}

					auto temp_register_id = CreateTempRegister(nullptr);
					auto argument_phys_register = Allocate_Register(argument_type, temp_register_id, needed_register);

					argument_allocations_registers.push_back({ argument_phys_register , nullptr });

					SetRegisterValue(argument_phys_register, temp_register_id);

					auto call_argument_value = GetRegisterValue(call_argument);

					if (metadata->Variadic) {
						if (TypeSystem::IsFlt(argument_type)) {

							auto argument_type_equ_int = TypeSystem::GetI64();
							auto argument_type_equ_int_type_size = TypeSystem::GetTypeSize(argument_type_equ_int);

							needed_register = argument_register_map.at({ argument_type_equ_int_type_size,i });;

							auto integer_register_id = CreateTempRegister(nullptr);
							auto integer_phys_register = Allocate_Register(argument_type_equ_int, integer_register_id, needed_register);
							SetRegisterValue(integer_phys_register, integer_register_id);

							argument_allocations.push_back(integer_register_id);
							argument_allocations_registers[i].second = integer_phys_register;
						}
					}

					argument_allocations.push_back(temp_register_id);
				}
			}
		}

		for (size_t i = 0; i < ir_function->Arguments.size(); i++)
		{
			auto call_argument = (IRRegisterValue*)ir_function->Arguments[i];

			TypeStorage* argument_type = ir_function->ArgumentTypes[i];
			auto argument_type_flags = TypeSystem::GetTypeFlags(argument_type);

			auto type_size = TypeSystem::GetTypeSize(argument_type);

			if (type_size > 8) {
				argument_type = TypeSystem::GetPtr(argument_type, 1);
				type_size = TypeSystem::GetTypeSize(argument_type);
			}

			if (i < 4)
			{
				GS_CORE_ASSERT(argument_allocations_registers[i].first);
				auto argument_phys_register = argument_allocations_registers[i].first;

				auto call_argument_value = GetRegisterValue(call_argument);

				if (metadata->Variadic) {

					if (type_size < 4) {
						if (argument_type_flags & FLAG_UNSIGNED_TYPE) {
							Code.push_back(Builder::MovZX(argument_phys_register, call_argument_value));
						}
						else {
							Code.push_back(Builder::MovSX(argument_phys_register, call_argument_value));
						}
					}
					else {
						Code.push_back(MoveBasedOnType(argument_type, argument_phys_register, call_argument_value));
					}
				}
				else {
					Code.push_back(MoveBasedOnType(argument_type, argument_phys_register, call_argument_value));
				}

				if (metadata->Variadic) {
					if (TypeSystem::IsFlt(argument_type)) {

						if (type_size != 8) {
							Code.push_back(Builder::SS2SD(argument_phys_register, argument_phys_register));
						}

						GS_CORE_ASSERT(argument_allocations_registers[i].second);

						auto integer_phys_register = argument_allocations_registers[i].second;

						Code.push_back(Builder::MovQ(integer_phys_register, argument_phys_register));
					}
				}
			}
			else {
				auto call_argument_value = GetRegisterValue(call_argument);

				if (GetRegisterValueType(call_argument) != Register_Value_Type::Register_Value) {

					auto tmp_move_register_id = CreateTempRegister(nullptr);
					auto tmp_move_register = Allocate_Register(argument_type, tmp_move_register_id);
					SetRegisterValue(tmp_move_register, tmp_move_register_id, Register_Value_Type::Register_Value);
					UseRegisterValue(tmp_move_register_id);

					Code.push_back(MoveBasedOnType(argument_type, tmp_move_register, call_argument_value));

					call_argument_value = tmp_move_register;
				}

				if (metadata->Variadic) {
					if (TypeSystem::IsFlt(argument_type)) {

						if (type_size != 8) {
							Code.push_back(Builder::SS2SD(call_argument_value, call_argument_value));
							argument_type = TypeSystem::GetBasic(IR_f64);
						}
					}
				}

				auto argument_stack_location = Builder::De_Reference(Alloc_Call_StackTop(TypeSystem::GetU64()), argument_type);

				Code.push_back(MoveBasedOnType(argument_type, argument_stack_location, call_argument_value));
			}

			UseRegisterValue(call_argument);
		}

		Assembly_Operand* return_location = nullptr;

		if (metadata->ReturnType != TypeSystem::GetVoid()) {

			auto return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);
			auto return_type = metadata->ReturnType;

			if (return_type_size > 8) {
				return_type = TypeSystem::GetPtr(metadata->ReturnType, 1);
				return_type_size = TypeSystem::GetTypeSize(return_type);
			}

			if (!TypeSystem::IsFlt(metadata->ReturnType)) {
				return_location = Allocate_Register(return_type, CurrentRegister, return_register_map.at(return_type_size));
			}
			else {
				return_location = Allocate_Register(return_type, CurrentRegister, XMM0);
			}
		}

		auto return_type_size = TypeSystem::GetTypeSize(metadata->ReturnType);

		if (return_type_size > 8) {
			Code.push_back(Builder::Lea(return_location, Builder::De_Reference(Stack_Alloc(metadata->ReturnType), metadata->ReturnType)));
		}

		if (metadata->Foreign && !Use_Linker) {
			Code.push_back(Builder::Call(Builder::De_Reference(Builder::Symbol(name))));
		}
		else {
			Code.push_back(Builder::Call(Builder::Symbol(name)));
		}

		SetRegisterValue(return_location, Register_Value_Type::Register_Value);

		if (!TypeSystem::IsPointer(metadata->ReturnType) && return_type_size <= 8) {
			if (m_Metadata->GetStructIDFromType(metadata->ReturnType->BaseID) != -1) {

				auto new_return_location = Stack_Alloc(metadata->ReturnType);

				Code.push_back(Builder::Mov(Builder::De_Reference(new_return_location, metadata->ReturnType), return_location));

				return_location = new_return_location;
				SetRegisterValue(Builder::De_Reference(new_return_location, metadata->ReturnType), Register_Value_Type::Memory_Value);
			}
		}

		for (auto allocation : argument_allocations) {
			UseRegisterValue(allocation);
		}

		m_Data.Call_Stack_Pointer = 0;
	}

	void X86_BackEnd::AssembleRegister(IRRegister* ir_register)
	{
		if (!ir_register->Value) {
			return; // TODO: fix why empty ir registers get generated they do not affect the program
		}

		CurrentRegister = ir_register->ID;
		CurrentIrRegister = ir_register;
		GS_CORE_ASSERT(ir_register->Life_Time);
		Current_Register_Lifetime = ir_register->Life_Time;

		AssembleInstruction(ir_register->Value);
	}

	void X86_BackEnd::AssembleArgument(IRArgumentAllocation* ir_argument)
	{
		auto argument_input_location = GetRegisterValue(CurrentRegister);

		auto argument_input_value_type = GetRegisterValueType(CurrentRegister);

		auto argument_storage_location = Stack_Alloc(ir_argument->AllocationType);

		auto type_size = TypeSystem::GetTypeSize(ir_argument->AllocationType);

		if (argument_input_value_type == Register_Value_Type::Memory_Value) {

			auto tmp_register_type = ir_argument->AllocationType;

			if (type_size > 8) {
				tmp_register_type = TypeSystem::GetPtr(tmp_register_type, 1);
			}

			auto tmp_move_register_id = CreateTempRegister(nullptr);
			auto tmp_move_register = Allocate_Register(tmp_register_type, tmp_move_register_id);
			SetRegisterValue(tmp_move_register, tmp_move_register_id, Register_Value_Type::Register_Value);
			UseRegisterValue(tmp_move_register_id);

			Code.push_back(MoveBasedOnType(tmp_register_type, tmp_move_register, argument_input_location));

			argument_input_location = tmp_move_register;
		}

		if (type_size <= 8) {
			Code.push_back(MoveBasedOnType(ir_argument->AllocationType, Builder::De_Reference(argument_storage_location, ir_argument->AllocationType), argument_input_location));
		}
		else {

			auto mmcpy_dest_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_dest_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_dest_reg_id, X86_Register::RCX);
			SetRegisterValue(mmcpy_dest_reg, mmcpy_dest_reg_id, Register_Value_Type::Register_Value);

			auto mmcpy_src_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_src_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_src_reg_id, X86_Register::RDX);
			SetRegisterValue(mmcpy_src_reg, mmcpy_src_reg_id, Register_Value_Type::Register_Value);

			auto mmcpy_size_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_size_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_size_reg_id, X86_Register::R8);
			SetRegisterValue(mmcpy_size_reg, mmcpy_size_reg_id, Register_Value_Type::Register_Value);

			auto mmcpy_return_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_return_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_return_reg_id, X86_Register::RAX);
			SetRegisterValue(mmcpy_return_reg, mmcpy_return_reg_id, Register_Value_Type::Register_Value);

			auto argument_input_location = GetRegisterValue(CurrentRegister);

			Code.push_back(Builder::Lea(mmcpy_dest_reg, Builder::De_Reference(argument_storage_location)));
			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmcpy_src_reg, argument_input_location));
			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmcpy_size_reg, Builder::Constant_Integer(type_size)));

			Call_Memcpy();

			UseRegisterValue(mmcpy_dest_reg_id);
			UseRegisterValue(mmcpy_src_reg_id);
			UseRegisterValue(mmcpy_size_reg_id);
			UseRegisterValue(mmcpy_return_reg_id);
		}

		SetRegisterValue(argument_storage_location, Register_Value_Type::Stack_Address);

		m_Data.IR_RegisterLifetimes.at(CurrentRegister) = 1;

		UseRegisterValue(CurrentRegister);
	}

	void X86_BackEnd::AssembleAlloca(IRAlloca* ir_alloca)
	{
		auto register_value = Stack_Alloc(ir_alloca->Type);
		SetRegisterValue(register_value, Register_Value_Type::Stack_Address);
	}

	void X86_BackEnd::AssembleMemberAccess(IRMemberAccess* ir_member_access)
	{
		const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(ir_member_access->StructID);

		const MemberMetadata& member = struct_metadata->Members[ir_member_access->MemberID];

		auto offset = member.Offset;

		auto object_value = GetRegisterValue(ir_member_access->ObjectRegister);

		auto register_value_type = GetRegisterValueType(ir_member_access->ObjectRegister);

		bool has_allocation = false;

		if (ir_member_access->ReferenceAccess || register_value_type == Register_Value_Type::Memory_Value) {

			//GS_CORE_ASSERT(object_value->type != Op_Register);

			UseRegisterValue(ir_member_access->ObjectRegister);
			auto new_object_value = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

			if (register_value_type != Register_Value_Type::Memory_Value && register_value_type != Register_Value_Type::Register_Value) {
				object_value = Builder::De_Reference(object_value, TypeSystem::GetVoidPtr());
			}

			register_value_type = Register_Value_Type::Register_Value;

			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), new_object_value, object_value, "member access"));

			object_value = new_object_value;

			has_allocation = true;
		}

		if (register_value_type == Register_Value_Type::Stack_Address) {

			GS_CORE_ASSERT(object_value->bin_op.operand1->reg.Register == RBP);
			GS_CORE_ASSERT(object_value->bin_op.operand2->type == Op_Constant_Integer);

			SetRegisterValue(Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(object_value->bin_op.operand2->constant_integer.integer - offset)), CurrentRegister, Register_Value_Type::Stack_Address);
			UseRegisterValue(ir_member_access->ObjectRegister);
		}
		else if (register_value_type == Register_Value_Type::Pointer_Address) {
			GS_CORE_ASSERT(object_value->bin_op.operand1->reg.Register != RBP);
			GS_CORE_ASSERT(object_value->bin_op.operand2->type == Op_Constant_Integer);

			UseRegisterValue(ir_member_access->ObjectRegister);
			Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister, object_value->bin_op.operand1->reg.Register);

			SetRegisterValue(Builder::OpAdd(Builder::Register(object_value->bin_op.operand1->reg.Register), Builder::Constant_Integer(object_value->bin_op.operand2->constant_integer.integer + offset)), CurrentRegister, Register_Value_Type::Pointer_Address);
		}
		else if (register_value_type == Register_Value_Type::Register_Value) {

			if (!has_allocation) {
				UseRegisterValue(ir_member_access->ObjectRegister);
				Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister, object_value->reg.Register);
			}

			SetRegisterValue(Builder::OpAdd(Builder::Register(object_value->reg.Register), Builder::Constant_Integer(offset)), CurrentRegister, Register_Value_Type::Pointer_Address);
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}
	}

	void X86_BackEnd::AssembleArrayAccess(IRArrayAccess* ir_array_access)
	{
		GS_CORE_ASSERT(ir_array_access->Type);
		GS_CORE_ASSERT(ir_array_access->Index_Type);

		auto array_address_value = GetRegisterValue(ir_array_access->ArrayAddress);
		auto register_value_type = GetRegisterValueType(ir_array_access->ArrayAddress);

		if (register_value_type == Register_Value_Type::Memory_Value) {

			GS_CORE_ASSERT(array_address_value->type != Op_Register);

			UseRegisterValue(ir_array_access->ArrayAddress);
			auto new_array_address_value = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

			if (register_value_type != Register_Value_Type::Memory_Value && register_value_type != Register_Value_Type::Register_Value) {
				array_address_value = Builder::De_Reference(new_array_address_value, TypeSystem::GetVoidPtr());
			}

			register_value_type = Register_Value_Type::Register_Value;

			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), new_array_address_value, new_array_address_value));

			array_address_value = new_array_address_value;
		}
		else if (register_value_type == Register_Value_Type::Register_Value) {
			UseRegisterValue(ir_array_access->ArrayAddress);

			GS_CORE_ASSERT(array_address_value->type == Op_Register);

			Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister, array_address_value->reg.Register);
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}

		auto array_index_value = GetRegisterValue(ir_array_access->ElementIndexRegister);
		auto array_index_type = GetRegisterValueType(ir_array_access->ElementIndexRegister);

		if (array_index_type != Register_Value_Type::Immediate_Value)
		{
			auto index_type_size = TypeSystem::GetTypeSize(ir_array_access->Index_Type);

			auto temp_array_index_register_id = CreateTempRegister(nullptr);
			auto new_array_index_value = Allocate_Register(TypeSystem::GetVoidPtr(), temp_array_index_register_id);
			SetRegisterValue(new_array_index_value, temp_array_index_register_id, Register_Value_Type::Register_Value);

			array_index_type = Register_Value_Type::Register_Value;

			if (index_type_size <= 2) {
				Code.push_back(Builder::MovZX(new_array_index_value, array_index_value));
			}
			else {

				auto integer_register_family = register_to_family_map.at(new_array_index_value->reg.Register);
				auto section = register_family_map.at({ index_type_size, integer_register_family });

				Code.push_back(Builder::Mov(Builder::Register(section), array_index_value));
			}

			array_index_value = new_array_index_value;

			UseRegisterValue(temp_array_index_register_id);
		}

		auto element_type_size = TypeSystem::GetTypeSize(ir_array_access->Type);

		if (array_index_type == Register_Value_Type::Immediate_Value) {

			if (register_value_type == Register_Value_Type::Register_Value) {

				if (element_type_size < 8) {
					SetRegisterValue(Builder::OpAdd(IR(*array_address_value), Builder::OpMul(IR(*array_index_value), Builder::Constant_Integer(element_type_size))), CurrentRegister, Register_Value_Type::Pointer_Address);
				}
				else {
					SetRegisterValue(Builder::OpAdd(IR(*array_address_value), Builder::Constant_Integer(element_type_size * array_index_value->constant_integer.integer)), CurrentRegister, Register_Value_Type::Pointer_Address);
				}
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			Code.push_back(Builder::Mul(array_index_value, Builder::Constant_Integer(element_type_size)));
			Code.push_back(Builder::Add(array_address_value, array_index_value));
			SetRegisterValue(array_address_value, CurrentRegister, Register_Value_Type::Register_Value);
		}

		UseRegisterValue(ir_array_access->ElementIndexRegister);
	}

	void X86_BackEnd::AssembleLexicalBlock(IRLexBlock* ir_lex_block)
	{
		for (auto inst : ir_lex_block->Instructions) {

			if (Break_Encountered) {
				Break_Encountered = false;
				break;
			}

			if (Return_Encountered) {
				Return_Encountered = false;
				break;
			}

			AssembleInstruction(inst);
		}

		Break_Encountered = false;
		Return_Encountered = false;
	}


	void X86_BackEnd::EndBlock()
	{
		for (const auto& [family, used] : Register_Allocator_Data.allocated) {

			if (used) {

				auto allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation[family]];

				auto spillage_location = Stack_Alloc(allocation.type);
				spillage_location = Builder::De_Reference(spillage_location, allocation.type);

				auto value_type = m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id);

				Assembly_Instruction spill;

				if (value_type == Register_Value_Type::Pointer_Address) {
					Code.push_back(Builder::Lea(Builder::Register(allocation.reg), Builder::De_Reference(m_Data.IR_RegisterValues.at(allocation.virtual_register_id))));
					spill = Builder::Mov(spillage_location, Builder::Register(allocation.reg));
				}
				else {
					spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));
				}

				spill.Comment = "Spillage";

				Code.push_back(spill);

				m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
				m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

				allocation.spillage_location = spillage_location;
			}
		}

		for (const auto& [family, used] : Register_Allocator_Data.allocated_floating) {

			if (used) {

				auto allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation[family]];

				auto spillage_location = Stack_Alloc(allocation.type);
				spillage_location = Builder::De_Reference(spillage_location, allocation.type);

				auto value_type = m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id);

				Assembly_Instruction spill;

				if (value_type == Register_Value_Type::Pointer_Address) {
					spill = Builder::Lea(spillage_location, Builder::De_Reference(m_Data.IR_RegisterValues.at(allocation.virtual_register_id)));
				}
				else {
					spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));
				}

				spill.Comment = "Spillage";

				Code.push_back(spill);

				m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
				m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

				allocation.spillage_location = spillage_location;
			}
		}
	}

	void X86_BackEnd::AssembleIf(IRIf* ir_if)
	{
		std::string skip_label_name = GetLabelName();
		Assembly_Operand* skip_label = Builder::Symbol(skip_label_name);
		Current_Skip_Target = skip_label;

		auto condition_ir_register = m_Metadata->GetRegister(ir_if->ConditionRegister);

		Assembly_Op_Code jump_op_code;

		if (condition_ir_register->IsCondition) {

			auto ir_condition_type = condition_ir_register->Value->GetType();

			GS_CORE_ASSERT(ir_if->ConditionType);

			auto condition_type_flags = TypeSystem::GetTypeFlags(ir_if->ConditionType);

			bool is_unsigned_or_floating = (condition_type_flags & FLAG_UNSIGNED_TYPE) || (condition_type_flags & FLAG_FLOATING_TYPE);

			if (ir_condition_type == IRNodeType::Equal) {
				jump_op_code = I_Jne;
			}
			else if (ir_condition_type == IRNodeType::NotEqual) {
				jump_op_code = I_Je;
			}
			else if (ir_condition_type == IRNodeType::LesserThan) {

				if (is_unsigned_or_floating) {
					jump_op_code = I_Jae;
				}
				else {
					jump_op_code = I_Jge;
				}
			}
			else if (ir_condition_type == IRNodeType::GreaterThan) {
				if (is_unsigned_or_floating) {
					jump_op_code = I_Jbe;
				}
				else {
					jump_op_code = I_Jle;
				}
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {

			auto register_value = GetRegisterValue(ir_if->ConditionRegister);
			UseRegisterValue(ir_if->ConditionRegister);

			Code.push_back(Builder::Cmp(register_value, Builder::Constant_Integer(0)));
			jump_op_code = I_Je;
		}

		EndBlock();

		Code.push_back(Builder::Build_Inst(jump_op_code, skip_label));

		for (auto inst : ir_if->Instructions) {

			if (Break_Encountered) {
				Break_Encountered = false;
				break;
			}

			if (Return_Encountered) {
				Return_Encountered = false;
				break;
			}
			AssembleInstruction(inst);
		}

		Break_Encountered = false;
		Return_Encountered = false;

		EndBlock();

		Assembly_Operand* skip_else_label = nullptr;

		if (ir_if->ElseBlock) {

			std::string skip_else_label_name = GetLabelName();

			skip_else_label = Builder::Symbol(skip_else_label_name);

			Code.push_back(Builder::Build_Inst(I_Jmp, skip_else_label));
		}

		Code.push_back(Builder::Label(skip_label));

		if (ir_if->ElseBlock) {
			AssembleInstruction(ir_if->ElseBlock);
			Code.push_back(Builder::Label(skip_else_label));
			EndBlock();
		}
	}

	void X86_BackEnd::AssembleWhile(IRWhile* ir_while)
	{
		std::string skip_label_name = GetLabelName();
		std::string start_label_name = GetLabelName();

		Assembly_Operand* skip_label = Builder::Symbol(skip_label_name);
		Assembly_Operand* start_label = Builder::Symbol(start_label_name);

		auto condition_ir_register = m_Metadata->GetRegister(ir_while->ConditionRegisterID);

		Assembly_Op_Code jump_op_code;

		EndBlock();

		Code.push_back(Builder::Label(start_label));

		for (auto inst : ir_while->ConditionBlock) {
			AssembleInstruction(inst);
		}

		if (condition_ir_register->IsCondition) {

			auto ir_condition_type = condition_ir_register->Value->GetType();

			if (ir_condition_type == IRNodeType::Equal) {
				jump_op_code = I_Jne;
			}
			else if (ir_condition_type == IRNodeType::LesserThan) {
				jump_op_code = I_Jge;
			}
			else if (ir_condition_type == IRNodeType::GreaterThan) {
				jump_op_code = I_Jle;
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {

			auto register_value = GetRegisterValue(ir_while->ConditionRegisterID);
			UseRegisterValue(ir_while->ConditionRegisterID);

			Code.push_back(Builder::Cmp(register_value, Builder::Constant_Integer(0)));
			jump_op_code = I_Je;
		}

		Code.push_back(Builder::Build_Inst(jump_op_code, skip_label));

		Skip_Target_Stack.push(skip_label);

		for (auto inst : ir_while->Instructions) {

			if (Break_Encountered) {
				Break_Encountered = false;
				break;
			}

			if (Return_Encountered) {
				Return_Encountered = false;
				break;
			}

			AssembleInstruction(inst);
		}

		Break_Encountered = false;
		Return_Encountered = false;

		Skip_Target_Stack.pop();

		Code.push_back(Builder::Build_Inst(I_Jmp, start_label));

		Code.push_back(Builder::Label(skip_label));

		EndBlock();
	}

	void X86_BackEnd::AssembleStore(IRStore* ir_store)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_store->Type);

		auto pointer_register_value = GetRegisterValue(ir_store->AddressRegister);
		auto pointer_register_value_type = GetRegisterValueType(ir_store->AddressRegister);

		auto data_register_value = GetRegisterValue((IRRegisterValue*)ir_store->Data);
		auto data_register_value_type = GetRegisterValueType((IRRegisterValue*)ir_store->Data);

		if (data_register_value_type != Register_Value_Type::Register_Value) {

			auto store_type = ir_store->Type;

			if (type_size > 8) {
				store_type = TypeSystem::GetPtr(ir_store->Type, 1);
			}

			auto temp_register_id = CreateTempRegister(nullptr);

			auto temp_phys_register = Allocate_Register(store_type, temp_register_id);

			SetRegisterValue(temp_phys_register, temp_register_id);

			Code.push_back(MoveBasedOnType(store_type, temp_phys_register, data_register_value, "Store"));

			UseRegisterValue(temp_register_id);

			data_register_value = temp_phys_register;
		}

		if (pointer_register_value_type == Register_Value_Type::Memory_Value) {

			auto temp_register_id = CreateTempRegister(nullptr);
			auto temp_phys_register = Allocate_Register(TypeSystem::GetVoidPtr(), temp_register_id);
			SetRegisterValue(temp_phys_register, temp_register_id);

			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), temp_phys_register, pointer_register_value, "Reload Ptr"));

			UseRegisterValue(temp_register_id);

			pointer_register_value = temp_phys_register;
		}

		if (type_size <= 8) {
			auto move = MoveBasedOnType(ir_store->Type, Builder::De_Reference(pointer_register_value, ir_store->Type), data_register_value, "store");
			Code.push_back(move);
		}
		else {

			auto mmcpy_dest_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_dest_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_dest_reg_id, X86_Register::RCX);
			SetRegisterValue(mmcpy_dest_reg, mmcpy_dest_reg_id, Register_Value_Type::Register_Value);

			auto mmcpy_src_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_src_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_src_reg_id, X86_Register::RDX);
			SetRegisterValue(mmcpy_src_reg, mmcpy_src_reg_id, Register_Value_Type::Register_Value);

			auto mmcpy_size_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_size_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_size_reg_id, X86_Register::R8);
			SetRegisterValue(mmcpy_size_reg, mmcpy_size_reg_id, Register_Value_Type::Register_Value);

			auto mmcpy_return_reg_id = CreateTempRegister(nullptr);
			auto mmcpy_return_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_return_reg_id, X86_Register::RAX);
			SetRegisterValue(mmcpy_return_reg, mmcpy_return_reg_id, Register_Value_Type::Register_Value);

			Code.push_back(Builder::Lea(mmcpy_dest_reg, Builder::De_Reference(pointer_register_value)));
			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmcpy_src_reg, data_register_value));
			Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmcpy_size_reg, Builder::Constant_Integer(type_size)));

			Call_Memcpy();

			UseRegisterValue(mmcpy_dest_reg_id);
			UseRegisterValue(mmcpy_src_reg_id);
			UseRegisterValue(mmcpy_size_reg_id);
			UseRegisterValue(mmcpy_return_reg_id);
		}

		UseRegisterValue(ir_store->AddressRegister);
		UseRegisterValue((IRRegisterValue*)ir_store->Data);
	}

	void X86_BackEnd::AssembleLoad(IRLoad* ir_load)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_load->Type);

		auto pointer_register_value = GetRegisterValue(ir_load->AddressRegister);
		auto pointer_register_value_type = GetRegisterValueType(ir_load->AddressRegister);

		if (type_size <= 8) {
			auto loaded_data_register = Allocate_Register(ir_load->Type, CurrentRegister);

			if (pointer_register_value_type == Register_Value_Type::Memory_Value) {
				auto tmp_reg_id = CreateTempRegister(nullptr);
				auto tmp_reg = Allocate_Register(TypeSystem::GetVoidPtr(), tmp_reg_id);
				SetRegisterValue(tmp_reg, tmp_reg_id);
				UseRegisterValue(tmp_reg_id);
				Code.push_back(Builder::Mov(tmp_reg, pointer_register_value));

				pointer_register_value = tmp_reg;
			}

			Code.push_back(MoveBasedOnType(ir_load->Type, loaded_data_register, Builder::De_Reference(pointer_register_value, ir_load->Type), ASMA("Load " + TypeSystem::PrintType(ir_load->Type))->data()));
			SetRegisterValue(loaded_data_register, Register_Value_Type::Register_Value);
		}
		else {

			auto pointer_type = TypeSystem::GetPtr(ir_load->Type, 1);
			auto loaded_data_register = Allocate_Register(pointer_type, CurrentRegister);
			auto lea = Builder::Lea(loaded_data_register, Builder::De_Reference(pointer_register_value, pointer_type));
			lea.Comment = "Load";
			Code.push_back(lea);
			SetRegisterValue(loaded_data_register, Register_Value_Type::Pointer_Address);
		}

		UseRegisterValue(ir_load->AddressRegister);
	}

	void X86_BackEnd::AssembleAdd(IRADD* ir_add)
	{
		auto result_location = Allocate_Register(ir_add->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_add->RegisterA);
		auto b_value = GetRegisterValue(ir_add->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			Code.push_back(MoveBasedOnType(ir_add->Type, result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		if (TypeSystem::IsFlt(ir_add->Type)) {

			auto type_size = TypeSystem::GetTypeSize(ir_add->Type);

			if (type_size == 4) {
				Code.push_back(Builder::AddSS(result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::AddSD(result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			Code.push_back(Builder::Add(result_location, b_value));
		}

		UseRegisterValue(ir_add->RegisterA);
		UseRegisterValue(ir_add->RegisterB);

	}

	void X86_BackEnd::AssembleSub(IRSUB* ir_sub)
	{
		auto result_location = Allocate_Register(ir_sub->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_sub->RegisterA);
		auto b_value = GetRegisterValue(ir_sub->RegisterB);

		if (!Are_Equal(result_location, a_value)) {
			Code.push_back(MoveBasedOnType(ir_sub->Type, result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		if (TypeSystem::IsFlt(ir_sub->Type)) {

			auto type_size = TypeSystem::GetTypeSize(ir_sub->Type);

			if (type_size == 4) {
				Code.push_back(Builder::SubSS(result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::SubSD(result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			Code.push_back(Builder::Sub(result_location, b_value));
		}

		UseRegisterValue(ir_sub->RegisterA);
		UseRegisterValue(ir_sub->RegisterB);
	}

	void X86_BackEnd::AssembleMul(IRMUL* ir_mul)
	{
		auto result_location = Allocate_Register(ir_mul->Type, CurrentRegister);

		auto a_value = GetRegisterValue(ir_mul->RegisterA);
		auto b_value = GetRegisterValue(ir_mul->RegisterB);

		if (!Are_Equal(result_location, a_value)) {

			Code.push_back(MoveBasedOnType(ir_mul->Type, result_location, a_value));
		}

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		if (TypeSystem::IsFlt(ir_mul->Type)) {

			auto type_size = TypeSystem::GetTypeSize(ir_mul->Type);

			if (type_size == 4) {
				Code.push_back(Builder::MulSS(result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::MulSD(result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			Code.push_back(Builder::Mul(result_location, b_value));
		}

		UseRegisterValue(ir_mul->RegisterA);
		UseRegisterValue(ir_mul->RegisterB);
	}

	struct Division_Inst_Registers {
		X86_Register first_half;
		X86_Register second_half;
		X86_Register result;
		X86_Register remainder;
	};

	const std::map<u64, Division_Inst_Registers> divide_register_table = {
		{2, {AX, DX, AX, DX}},
		{4, {EAX, EDX, EAX, EDX}},
		{8, {RAX, RDX, RAX, RDX}},
	};

	void X86_BackEnd::AssembleDiv(IRDIV* ir_div)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_div->Type);
		auto type_flags = TypeSystem::GetTypeFlags(ir_div->Type);

		if (TypeSystem::IsFlt(ir_div->Type)) {

			auto input_result_location = Allocate_Register(ir_div->Type, CurrentRegister);

			auto a_value = GetRegisterValue(ir_div->RegisterA);
			auto b_value = GetRegisterValue(ir_div->RegisterB);

			Code.push_back(MoveBasedOnType(ir_div->Type, input_result_location, a_value));

			if (b_value->type == Op_Constant_Integer)
			{
				auto divisor_reg_id = CreateTempRegister(nullptr);

				auto divisor_reg_value = Allocate_Register(ir_div->Type, divisor_reg_id);
				SetRegisterValue(divisor_reg_value, divisor_reg_id, Register_Value_Type::Register_Value);

				Code.push_back(MoveBasedOnType(ir_div->Type, divisor_reg_value, b_value));

				UseRegisterValue(divisor_reg_id);
				b_value = divisor_reg_value;
			}

			if (type_size == 4) {
				Code.push_back(Builder::DivSS(input_result_location, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::DivSD(input_result_location, b_value));
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}

			SetRegisterValue(input_result_location, Register_Value_Type::Register_Value);
		}
		else {

			bool is_unsigned = (type_flags & FLAG_UNSIGNED_TYPE);

			const auto& register_table = divide_register_table.at(type_size);

			auto remainder_register = Builder::Register(register_table.remainder);
			auto input_result_location = Allocate_Register(ir_div->Type, CurrentRegister, register_table.result);

			auto rem_reg_id = CreateTempRegister(remainder_register);
			remainder_register = Allocate_Register(ir_div->Type, rem_reg_id, register_table.remainder);

			if (is_unsigned) {
				Code.push_back(Builder::Mov(remainder_register, Builder::Constant_Integer(0)));
			}

			auto a_value = GetRegisterValue(ir_div->RegisterA);
			auto b_value = GetRegisterValue(ir_div->RegisterB);

			if (b_value->type == Op_Constant_Integer)
			{
				auto divisor_reg_id = CreateTempRegister(nullptr);

				auto divisor_reg_value = Allocate_Register(ir_div->Type, divisor_reg_id);
				SetRegisterValue(divisor_reg_value, divisor_reg_id, Register_Value_Type::Register_Value);

				Code.push_back(Builder::Mov(divisor_reg_value, b_value));

				UseRegisterValue(divisor_reg_id);
				b_value = divisor_reg_value;
			}

			Code.push_back(Builder::Mov(input_result_location, a_value));

			if (!is_unsigned)
			{
				if (type_size == 1) {
					Code.push_back(Builder::Build_Inst(I_CBW));
				}
				else if (type_size == 2) {
					Code.push_back(Builder::Build_Inst(I_CWD));
				}
				else if (type_size == 4) {
					Code.push_back(Builder::Build_Inst(I_CDQ));
				}
				else if (type_size == 8) {
					Code.push_back(Builder::Build_Inst(I_CQO));
				}
				else {
					GS_CORE_ASSERT(nullptr);
				}
			}

			if (is_unsigned) {
				Code.push_back(Builder::Div(b_value));
			}
			else {
				Code.push_back(Builder::IDiv(b_value));
			}

			UseRegisterValue(rem_reg_id);
			SetRegisterValue(input_result_location, Register_Value_Type::Register_Value);
		}

		UseRegisterValue(ir_div->RegisterA);
		UseRegisterValue(ir_div->RegisterB);
	}

	void X86_BackEnd::AssembleSRem(IRSREM* ir_srem)
	{
		auto type_size = TypeSystem::GetTypeSize(ir_srem->Type);
		auto type_flags = TypeSystem::GetTypeFlags(ir_srem->Type);

		if (TypeSystem::IsFlt(ir_srem->Type)) {
			GS_CORE_ASSERT(0);
		}
		else {

			bool is_unsigned = (type_flags & FLAG_UNSIGNED_TYPE);

			const auto& register_table = divide_register_table.at(type_size);

			auto input_reg_id = CreateTempRegister(nullptr);
			auto input_register = Allocate_Register(ir_srem->Type, input_reg_id, register_table.result);

			auto result_location = Allocate_Register(ir_srem->Type, CurrentRegister, register_table.remainder);

			if (is_unsigned) {
				Code.push_back(Builder::Mov(result_location, Builder::Constant_Integer(0)));
			}

			auto a_value = GetRegisterValue(ir_srem->RegisterA);
			auto b_value = GetRegisterValue(ir_srem->RegisterB);

			if (b_value->type == Op_Constant_Integer)
			{
				auto divisor_reg_id = CreateTempRegister(nullptr);
				auto divisor_reg_value = Allocate_Register(ir_srem->Type, divisor_reg_id);
				SetRegisterValue(divisor_reg_value, divisor_reg_id, Register_Value_Type::Register_Value);

				Code.push_back(Builder::Mov(divisor_reg_value, b_value));

				UseRegisterValue(divisor_reg_id);
				b_value = divisor_reg_value;
			}

			Code.push_back(Builder::Mov(input_register, a_value));

			if (!is_unsigned)
			{
				if (type_size == 1) {
					Code.push_back(Builder::Build_Inst(I_CBW));
				}
				else if (type_size == 2) {
					Code.push_back(Builder::Build_Inst(I_CWD));
				}
				else if (type_size == 4) {
					Code.push_back(Builder::Build_Inst(I_CDQ));
				}
				else if (type_size == 8) {
					Code.push_back(Builder::Build_Inst(I_CQO));
				}
				else {
					GS_CORE_ASSERT(nullptr);
				}
			}

			if (is_unsigned) {
				Code.push_back(Builder::Div(b_value));
			}
			else {
				Code.push_back(Builder::IDiv(b_value));
			}

			UseRegisterValue(input_reg_id);
			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}

		UseRegisterValue(ir_srem->RegisterA);
		UseRegisterValue(ir_srem->RegisterB);
	}

	void X86_BackEnd::AssembleAnd(IRAnd* ir_and)
	{
		auto a_value = GetRegisterValue(ir_and->RegisterA);
		auto b_value = GetRegisterValue(ir_and->RegisterB);

		if (CurrentIrRegister->IsCondition) {

			Code.push_back(Builder::Cmp(a_value, b_value));

			UseRegisterValue(ir_and->RegisterA);
			UseRegisterValue(ir_and->RegisterB);
			return;
		}

		auto result_location = Allocate_Register(ir_and->Type, CurrentRegister);

		auto temp_reg_id = CreateTempRegister(nullptr);
		auto temp_location = Allocate_Register(ir_and->Type, temp_reg_id);
		SetRegisterValue(temp_location, temp_reg_id, Register_Value_Type::Register_Value);

		Code.push_back(MoveBasedOnType(ir_and->Type, result_location, a_value));
		Code.push_back(MoveBasedOnType(ir_and->Type, temp_location, b_value));

		Code.push_back(Builder::Cmp(temp_location, Builder::Constant_Integer(0)));
		Code.push_back(Builder::SetNe(temp_location));

		Code.push_back(Builder::Cmp(result_location, Builder::Constant_Integer(0)));
		Code.push_back(Builder::SetNe(result_location));

		Code.push_back(Builder::And(result_location, temp_location));

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		UseRegisterValue(ir_and->RegisterA);
		UseRegisterValue(ir_and->RegisterB);
		UseRegisterValue(temp_reg_id);
	}

	void X86_BackEnd::AssembleOr(IROr* ir_or)
	{
		auto a_value = GetRegisterValue(ir_or->RegisterA);
		auto b_value = GetRegisterValue(ir_or->RegisterB);

		if (CurrentIrRegister->IsCondition) {

			Code.push_back(Builder::Cmp(a_value, b_value));

			UseRegisterValue(ir_or->RegisterA);
			UseRegisterValue(ir_or->RegisterB);
			return;
		}

		auto result_location = Allocate_Register(ir_or->Type, CurrentRegister);

		auto temp_reg_id = CreateTempRegister(nullptr);
		auto temp_location = Allocate_Register(ir_or->Type, temp_reg_id);
		SetRegisterValue(temp_location, temp_reg_id, Register_Value_Type::Register_Value);

		Code.push_back(MoveBasedOnType(ir_or->Type, result_location, a_value));
		Code.push_back(MoveBasedOnType(ir_or->Type, temp_location, b_value));

		Code.push_back(Builder::Cmp(temp_location, Builder::Constant_Integer(0)));
		Code.push_back(Builder::SetNe(temp_location));

		Code.push_back(Builder::Cmp(result_location, Builder::Constant_Integer(0)));
		Code.push_back(Builder::SetNe(result_location));

		Code.push_back(Builder::Or(result_location, temp_location));

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		UseRegisterValue(ir_or->RegisterA);
		UseRegisterValue(ir_or->RegisterB);
		UseRegisterValue(temp_reg_id);
	}

	void X86_BackEnd::AssembleEqual(IREQ* ir_eq)
	{
		auto a_value = GetRegisterValue(ir_eq->RegisterA);
		auto b_value = GetRegisterValue(ir_eq->RegisterB);

		auto a_value_type = GetRegisterValueType(ir_eq->RegisterA);
		auto b_value_type = GetRegisterValueType(ir_eq->RegisterB);

		u64 a_temp_reg_id = -1;
		u64 b_temp_reg_id = -1;

		if (a_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_eq->Type);

			Code.push_back(MoveBasedOnType(ir_eq->Type, temp_reg, a_value));

			a_temp_reg_id = temp_reg_id;
			a_value = temp_reg;
		}

		if (b_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_eq->Type);

			Code.push_back(MoveBasedOnType(ir_eq->Type, temp_reg, b_value));

			b_temp_reg_id = temp_reg_id;
			b_value = temp_reg;
		}

		auto type_size = TypeSystem::GetTypeSize(ir_eq->Type);

		if (CurrentIrRegister->IsCondition) {

			if (TypeSystem::IsFlt(ir_eq->Type)) {
				if (type_size == 4) {
					Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
				}
				else if (type_size == 8) {
					Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
				}
				else {
					GS_CORE_ASSERT(0);
				}
			}
			else {
				Code.push_back(Builder::Cmp(a_value, b_value));
			}

			UseRegisterValue(ir_eq->RegisterA);
			UseRegisterValue(ir_eq->RegisterB);
			return;
		}

		if (TypeSystem::IsFlt(ir_eq->Type)) {
			if (type_size == 4) {
				Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
			}
			else {
				GS_CORE_ASSERT(0);
			}

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			Code.push_back(Builder::SetE(result_location));

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}
		else {

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			auto register_family = register_to_family_map.at(result_location->reg.Register);
			auto section = Builder::Register(register_family_map.at({ TypeSystem::GetTypeSize(ir_eq->Type), register_family }));

			Code.push_back(MoveBasedOnType(ir_eq->Type, section, a_value));

			Code.push_back(Builder::Build_Inst(I_Cmp, section, b_value));
			Code.push_back(Builder::SetE(result_location));

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}

		if (a_temp_reg_id != -1) {
			UseRegisterValue(a_temp_reg_id);
		}

		if (b_temp_reg_id != -1) {
			UseRegisterValue(b_temp_reg_id);
		}

		UseRegisterValue(ir_eq->RegisterA);
		UseRegisterValue(ir_eq->RegisterB);
	}

	void X86_BackEnd::AssembleNotEqual(IRNOTEQ* ir_not_eq)
	{
		auto a_value = GetRegisterValue(ir_not_eq->RegisterA);
		auto b_value = GetRegisterValue(ir_not_eq->RegisterB);

		auto a_value_type = GetRegisterValueType(ir_not_eq->RegisterA);
		auto b_value_type = GetRegisterValueType(ir_not_eq->RegisterB);

		u64 a_temp_reg_id = -1;
		u64 b_temp_reg_id = -1;

		if (a_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_not_eq->Type);

			Code.push_back(MoveBasedOnType(ir_not_eq->Type, temp_reg, a_value));

			a_temp_reg_id = temp_reg_id;
			a_value = temp_reg;
		}

		if (b_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_not_eq->Type);

			Code.push_back(MoveBasedOnType(ir_not_eq->Type, temp_reg, b_value));

			b_temp_reg_id = temp_reg_id;
			b_value = temp_reg;
		}

		auto type_size = TypeSystem::GetTypeSize(ir_not_eq->Type);

		if (CurrentIrRegister->IsCondition) {

			if (TypeSystem::IsFlt(ir_not_eq->Type)) {
				if (type_size == 4) {
					Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
				}
				else if (type_size == 8) {
					Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
				}
				else {
					GS_CORE_ASSERT(0);
				}
			}
			else {
				Code.push_back(Builder::Cmp(a_value, b_value));
			}

			UseRegisterValue(ir_not_eq->RegisterA);
			UseRegisterValue(ir_not_eq->RegisterB);
			return;
		}

		if (TypeSystem::IsFlt(ir_not_eq->Type)) {
			if (type_size == 4) {
				Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
			}
			else {
				GS_CORE_ASSERT(0);
			}

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			Code.push_back(Builder::SetNe(result_location));

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}
		else {

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			auto register_family = register_to_family_map.at(result_location->reg.Register);
			auto section = Builder::Register(register_family_map.at({ TypeSystem::GetTypeSize(ir_not_eq->Type), register_family }));

			Code.push_back(MoveBasedOnType(ir_not_eq->Type, section, a_value));

			Code.push_back(Builder::Build_Inst(I_Cmp, section, b_value));
			Code.push_back(Builder::SetNe(result_location));

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}

		if (a_temp_reg_id != -1) {
			UseRegisterValue(a_temp_reg_id);
		}

		if (b_temp_reg_id != -1) {
			UseRegisterValue(b_temp_reg_id);
		}

		UseRegisterValue(ir_not_eq->RegisterA);
		UseRegisterValue(ir_not_eq->RegisterB);
	}

	void X86_BackEnd::AssembleLesser(IRLesser* ir_lesser)
	{
		auto a_value = GetRegisterValue(ir_lesser->RegisterA);
		auto b_value = GetRegisterValue(ir_lesser->RegisterB);

		auto a_value_type = GetRegisterValueType(ir_lesser->RegisterA);
		auto b_value_type = GetRegisterValueType(ir_lesser->RegisterB);

		u64 a_temp_reg_id = -1;
		u64 b_temp_reg_id = -1;

		if (a_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_lesser->Type);

			Code.push_back(MoveBasedOnType(ir_lesser->Type, temp_reg, a_value));

			a_temp_reg_id = temp_reg_id;
			a_value = temp_reg;
		}

		if (b_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_lesser->Type);

			Code.push_back(MoveBasedOnType(ir_lesser->Type, temp_reg, b_value));

			b_temp_reg_id = temp_reg_id;
			b_value = temp_reg;
		}

		auto type_size = TypeSystem::GetTypeSize(ir_lesser->Type);

		if (CurrentIrRegister->IsCondition) {

			if (TypeSystem::IsFlt(ir_lesser->Type)) {
				if (type_size == 4) {
					Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
				}
				else if (type_size == 8) {
					Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
				}
				else {
					GS_CORE_ASSERT(0);
				}
			}
			else {
				Code.push_back(Builder::Cmp(a_value, b_value));
			}

			UseRegisterValue(ir_lesser->RegisterA);
			UseRegisterValue(ir_lesser->RegisterB);
			return;
		}

		if (TypeSystem::IsFlt(ir_lesser->Type)) {
			if (type_size == 4) {
				Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
			}
			else {
				GS_CORE_ASSERT(0);
			}

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			Code.push_back(Builder::Build_Inst(I_Setb, result_location));

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}
		else {

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			auto register_family = register_to_family_map.at(result_location->reg.Register);
			auto section = Builder::Register(register_family_map.at({ TypeSystem::GetTypeSize(ir_lesser->Type), register_family }));

			Code.push_back(MoveBasedOnType(ir_lesser->Type, section, a_value));

			Code.push_back(Builder::Build_Inst(I_Cmp, section, b_value));

			if (TypeSystem::IsUnSigned(ir_lesser->Type)) {
				Code.push_back(Builder::Build_Inst(I_Setb, result_location));
			}
			else {
				Code.push_back(Builder::Build_Inst(I_Setl, result_location));
			}

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}

		if (a_temp_reg_id != -1) {
			UseRegisterValue(a_temp_reg_id);
		}

		if (b_temp_reg_id != -1) {
			UseRegisterValue(b_temp_reg_id);
		}

		UseRegisterValue(ir_lesser->RegisterA);
		UseRegisterValue(ir_lesser->RegisterB);
	}

	void X86_BackEnd::AssembleGreater(IRGreater* ir_greater)
	{
		auto a_value = GetRegisterValue(ir_greater->RegisterA);
		auto b_value = GetRegisterValue(ir_greater->RegisterB);

		auto a_value_type = GetRegisterValueType(ir_greater->RegisterA);
		auto b_value_type = GetRegisterValueType(ir_greater->RegisterB);

		u64 a_temp_reg_id = -1;
		u64 b_temp_reg_id = -1;

		if (a_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_greater->Type);

			Code.push_back(MoveBasedOnType(ir_greater->Type, temp_reg, a_value));

			a_temp_reg_id = temp_reg_id;
			a_value = temp_reg;
		}

		if (b_value_type != Register_Value_Type::Register_Value) {
			auto& [temp_reg, temp_reg_id] = Allocate_Temp_Physical_Register(ir_greater->Type);

			Code.push_back(MoveBasedOnType(ir_greater->Type, temp_reg, b_value));

			b_temp_reg_id = temp_reg_id;
			b_value = temp_reg;
		}

		auto type_size = TypeSystem::GetTypeSize(ir_greater->Type);

		if (CurrentIrRegister->IsCondition) {

			if (TypeSystem::IsFlt(ir_greater->Type)) {
				if (type_size == 4) {
					Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
				}
				else if (type_size == 8) {
					Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
				}
				else {
					GS_CORE_ASSERT(0);
				}
			}
			else {
				Code.push_back(Builder::Cmp(a_value, b_value));
			}

			UseRegisterValue(ir_greater->RegisterA);
			UseRegisterValue(ir_greater->RegisterB);
			return;
		}

		if (TypeSystem::IsFlt(ir_greater->Type)) {
			if (type_size == 4) {
				Code.push_back(Builder::Build_Inst(I_UCOMISS, a_value, b_value));
			}
			else if (type_size == 8) {
				Code.push_back(Builder::Build_Inst(I_UCOMISD, a_value, b_value));
			}
			else {
				GS_CORE_ASSERT(0);
			}

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			Code.push_back(Builder::Build_Inst(I_Seta, result_location));

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}
		else {

			auto result_location = Allocate_Register(TypeSystem::GetBool(), CurrentRegister);

			auto register_family = register_to_family_map.at(result_location->reg.Register);
			auto section = Builder::Register(register_family_map.at({ TypeSystem::GetTypeSize(ir_greater->Type), register_family }));

			Code.push_back(MoveBasedOnType(ir_greater->Type, section, a_value));

			Code.push_back(Builder::Build_Inst(I_Cmp, section, b_value));

			if (TypeSystem::IsUnSigned(ir_greater->Type)) {
				Code.push_back(Builder::Build_Inst(I_Seta, result_location));
			}
			else {
				Code.push_back(Builder::Build_Inst(I_Setg, result_location));
			}

			SetRegisterValue(result_location, Register_Value_Type::Register_Value);
		}

		if (a_temp_reg_id != -1) {
			UseRegisterValue(a_temp_reg_id);
		}

		if (b_temp_reg_id != -1) {
			UseRegisterValue(b_temp_reg_id);
		}

		UseRegisterValue(ir_greater->RegisterA);
		UseRegisterValue(ir_greater->RegisterB);
	}

	void X86_BackEnd::AssembleBitAnd(IRBitAnd* ir_bit_and)
	{
		auto a_value = GetRegisterValue(ir_bit_and->RegisterA);
		auto b_value = GetRegisterValue(ir_bit_and->RegisterB);

		auto result_location = Allocate_Register(ir_bit_and->Type, CurrentRegister);

		Code.push_back(MoveBasedOnType(ir_bit_and->Type, result_location, a_value));

		Code.push_back(Builder::And(result_location, b_value));

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		UseRegisterValue(ir_bit_and->RegisterA);
		UseRegisterValue(ir_bit_and->RegisterB);
	}

	void X86_BackEnd::AssembleBitOr(IRBitOr* ir_bit_or)
	{
		auto a_value = GetRegisterValue(ir_bit_or->RegisterA);
		auto b_value = GetRegisterValue(ir_bit_or->RegisterB);

		auto result_location = Allocate_Register(ir_bit_or->Type, CurrentRegister);

		Code.push_back(MoveBasedOnType(ir_bit_or->Type, result_location, a_value));

		Code.push_back(Builder::Or(result_location, b_value));

		SetRegisterValue(result_location, Register_Value_Type::Register_Value);

		UseRegisterValue(ir_bit_or->RegisterA);
		UseRegisterValue(ir_bit_or->RegisterB);
	}

	void X86_BackEnd::AssemblePointerCast(IRPointerCast* ir_pointer_cast)
	{
		auto pointer = GetRegisterValue(ir_pointer_cast->PointerRegister);
		UseRegisterValue(ir_pointer_cast->PointerRegister);

		auto result = Allocate_Register(ir_pointer_cast->Type, CurrentRegister);
		Code.push_back(MoveBasedOnType(ir_pointer_cast->Type, result, pointer));

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleInt2PCast(IRInt2PtrCast* ir_int_2_ptr)
	{
		GS_CORE_ASSERT(ir_int_2_ptr->From);

		auto from_type_size = TypeSystem::GetTypeSize(ir_int_2_ptr->From);
		auto from_type_flags = TypeSystem::GetTypeFlags(ir_int_2_ptr->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_int_2_ptr->Type);

		GS_CORE_ASSERT(to_type_size == 8);

		auto integer = GetRegisterValue(ir_int_2_ptr->IntegerRegister);
		UseRegisterValue(ir_int_2_ptr->IntegerRegister);

		auto result = Allocate_Register(ir_int_2_ptr->Type, CurrentRegister);

		if (from_type_size == to_type_size) {
			Code.push_back(Builder::Mov(result, integer));
		}
		else {
			if (!(from_type_flags & FLAG_UNSIGNED_TYPE)) {
				if (from_type_size <= 2) {
					Code.push_back(Builder::MovSX(result, integer));
				}
				else {
					Code.push_back(Builder::MovSXD(result, integer));
				}
			}
			else {
				if (from_type_size <= 2) {
					Code.push_back(Builder::MovZX(result, integer));
				}
				else {

					GS_CORE_ASSERT(result->type == Op_Register);

					auto pointer_register_family = register_to_family_map.at(result->reg.Register);
					auto section = register_family_map.at({ from_type_size, pointer_register_family });

					Code.push_back(Builder::Mov(Builder::Register(section), integer));
				}
			}
		}

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssemblePtr2IntCast(IRPtr2IntCast* ir_ptr_2_int)
	{
		GS_CORE_ASSERT(ir_ptr_2_int->From);

		auto from_type_size = TypeSystem::GetTypeSize(ir_ptr_2_int->From);

		auto to_type_flags = TypeSystem::GetTypeFlags(ir_ptr_2_int->Type);
		auto to_type_size = TypeSystem::GetTypeSize(ir_ptr_2_int->Type);

		GS_CORE_ASSERT(from_type_size == 8);

		auto pointer = GetRegisterValue(ir_ptr_2_int->PointerRegister);
		UseRegisterValue(ir_ptr_2_int->PointerRegister);

		auto result = Allocate_Register(ir_ptr_2_int->Type, CurrentRegister);

		GS_CORE_ASSERT(result->type == Op_Register);

		auto integer_register_family = register_to_family_map.at(result->reg.Register);
		auto section = register_family_map.at({ 8, integer_register_family });

		Code.push_back(Builder::Mov(Builder::Register(section), pointer));

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleSextCast(IRSExtCast* ir_sext)
	{
		GS_CORE_ASSERT(ir_sext->From);

		auto from_type_size = TypeSystem::GetTypeSize(ir_sext->From);
		auto from_type_flags = TypeSystem::GetTypeFlags(ir_sext->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_sext->Type);

		auto integer = GetRegisterValue(ir_sext->IntegerRegister);

		auto result = Allocate_Register(ir_sext->Type, CurrentRegister);

		auto integer_value_type = GetRegisterValueType(ir_sext->IntegerRegister);

		if (integer_value_type != Register_Value_Type::Immediate_Value) {
			if (from_type_size <= 2) {
				Code.push_back(Builder::MovSX(result, integer));
			}
			else {
				Code.push_back(Builder::MovSXD(result, integer));
			}
		}
		else {
			Code.push_back(Builder::Mov(result, integer));
		}

		UseRegisterValue(ir_sext->IntegerRegister);

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleZextCast(IRZExtCast* ir_zext)
	{
		GS_CORE_ASSERT(ir_zext->From);

		auto from_type_size = TypeSystem::GetTypeSize(ir_zext->From);
		auto from_type_flags = TypeSystem::GetTypeFlags(ir_zext->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_zext->Type);

		auto integer = GetRegisterValue(ir_zext->IntegerRegister);

		auto result = Allocate_Register(ir_zext->Type, CurrentRegister);

		if (from_type_size <= 2) {
			Code.push_back(Builder::MovZX(result, integer));
		}
		else {

			GS_CORE_ASSERT(result->type == Op_Register);

			auto integer_register_family = register_to_family_map.at(result->reg.Register);
			auto section = register_family_map.at({ from_type_size, integer_register_family });

			Code.push_back(Builder::Mov(Builder::Register(section), integer));
		}

		UseRegisterValue(ir_zext->IntegerRegister);

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleIntTruncCast(IRIntTrunc* ir_int_trunc)
	{
		GS_CORE_ASSERT(ir_int_trunc->From);

		auto from_type_size = TypeSystem::GetTypeSize(ir_int_trunc->From);
		auto from_type_flags = TypeSystem::GetTypeFlags(ir_int_trunc->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_int_trunc->Type);

		auto integer = GetRegisterValue(ir_int_trunc->IntegerRegister);

		auto result = Allocate_Register(ir_int_trunc->Type, CurrentRegister);

		GS_CORE_ASSERT(result->type == Op_Register);

		auto integer_register_family = register_to_family_map.at(result->reg.Register);
		auto section = register_family_map.at({ from_type_size, integer_register_family });

		auto the_move = Builder::Mov(Builder::Register(section), integer);
		the_move.Comment = "int trunc cast";
		Code.push_back(the_move);

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);

		UseRegisterValue(ir_int_trunc->IntegerRegister);
	}

	void X86_BackEnd::AssembleInt2FPCast(IRInt2FP* ir_int_2_fp)
	{
		GS_CORE_ASSERT(ir_int_2_fp->From);
		GS_CORE_ASSERT(!TypeSystem::IsFlt(ir_int_2_fp->From));
		GS_CORE_ASSERT(TypeSystem::IsFlt(ir_int_2_fp->Type));

		auto from_type_size = TypeSystem::GetTypeSize(ir_int_2_fp->From);
		auto from_type_flags = TypeSystem::GetTypeFlags(ir_int_2_fp->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_int_2_fp->Type);

		auto integer = GetRegisterValue(ir_int_2_fp->IntegerRegister);

		if (from_type_size <= 2) {

			auto tmp_register_id = CreateTempRegister(nullptr);
			auto tmp_register = Allocate_Register(TypeSystem::GetI32(), tmp_register_id);
			SetRegisterValue(tmp_register, tmp_register_id, Register_Value_Type::Register_Value);

			if (from_type_flags & FLAG_UNSIGNED_TYPE) {
				Code.push_back(Builder::MovZX(tmp_register, integer));
			}
			else {
				Code.push_back(Builder::MovSX(tmp_register, integer));
			}

			integer = tmp_register;
			UseRegisterValue(tmp_register_id);
		}

		auto result = Allocate_Register(ir_int_2_fp->Type, CurrentRegister);

		if (to_type_size == 4) {
			Code.push_back(Builder::Build_Inst(I_CvtSI2SS, result, integer));
		}
		else if (to_type_size == 8) {
			Code.push_back(Builder::Build_Inst(I_CvtSI2SD, result, integer));
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}

		UseRegisterValue(ir_int_2_fp->IntegerRegister);

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleFP2IntCast(IRFP2Int* ir_fp_2_int)
	{
		GS_CORE_ASSERT(ir_fp_2_int->From);
		GS_CORE_ASSERT(TypeSystem::IsFlt(ir_fp_2_int->From));
		GS_CORE_ASSERT(!TypeSystem::IsFlt(ir_fp_2_int->Type));

		auto from_type_size = TypeSystem::GetTypeSize(ir_fp_2_int->From);
		auto from_type_flags = TypeSystem::GetTypeFlags(ir_fp_2_int->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_fp_2_int->Type);

		auto floating = GetRegisterValue(ir_fp_2_int->FloatRegister);

		auto result = Allocate_Register(ir_fp_2_int->Type, CurrentRegister);
		auto result_section = result;

		if (to_type_size <= 2) {

			auto integer_register_family = register_to_family_map.at(result->reg.Register);
			auto section = register_family_map.at({ 4, integer_register_family });

			result_section = Builder::Register(section);
		}

		if (from_type_size == 4) {
			Code.push_back(Builder::Build_Inst(I_CvtSS2SI, result_section, floating));
		}
		else if (from_type_size == 8) {
			Code.push_back(Builder::Build_Inst(I_CvtSD2SI, result_section, floating));
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}

		UseRegisterValue(ir_fp_2_int->FloatRegister);

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleFPExtCast(IRFPExt* ir_fp_ext)
	{
		GS_CORE_ASSERT(ir_fp_ext->From);
		GS_CORE_ASSERT(TypeSystem::IsFlt(ir_fp_ext->From));
		GS_CORE_ASSERT(TypeSystem::IsFlt(ir_fp_ext->Type));

		auto from_type_size = TypeSystem::GetTypeSize(ir_fp_ext->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_fp_ext->Type);

		GS_CORE_ASSERT(to_type_size > from_type_size);

		auto floating = GetRegisterValue(ir_fp_ext->FloatRegister);

		auto result = Allocate_Register(ir_fp_ext->Type, CurrentRegister);

		Code.push_back(Builder::Build_Inst(I_CvtSS2SD, result, floating));

		UseRegisterValue(ir_fp_ext->FloatRegister);

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleFPTruncCast(IRFPTrunc* ir_fp_trunc)
	{
		GS_CORE_ASSERT(ir_fp_trunc->From);
		GS_CORE_ASSERT(TypeSystem::IsFlt(ir_fp_trunc->From));
		GS_CORE_ASSERT(TypeSystem::IsFlt(ir_fp_trunc->Type));

		auto from_type_size = TypeSystem::GetTypeSize(ir_fp_trunc->From);
		auto to_type_size = TypeSystem::GetTypeSize(ir_fp_trunc->Type);

		GS_CORE_ASSERT(to_type_size < from_type_size);

		auto floating = GetRegisterValue(ir_fp_trunc->FloatRegister);

		auto result = Allocate_Register(ir_fp_trunc->Type, CurrentRegister);

		Code.push_back(Builder::Build_Inst(I_CvtSD2SS, result, floating));

		UseRegisterValue(ir_fp_trunc->FloatRegister);

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleFuncRef(IRFuncRef* ir_func_ref)
	{
		const FunctionMetadata* metadata = m_Metadata->GetFunctionMetadata(ir_func_ref->FunctionID);
		GS_CORE_ASSERT(metadata);

		std::string name = metadata->Symbol.Symbol;

		if (!metadata->Foreign && metadata->Symbol.Symbol != "main") {
			name = Mangle_Name(name, metadata->Signature);
		}

		auto result = Allocate_Register(metadata->Signature, CurrentRegister);

		if (!Use_Linker) {
			if (metadata->Foreign) {
				Code.push_back(Builder::Mov(result, Builder::De_Reference(Builder::Symbol(name))));
			}
			else {
				Code.push_back(Builder::Mov(result, Builder::Symbol(name)));
			}
		}
		else {
			Code.push_back(Builder::Mov(result, Builder::Symbol(name)));
		}

		SetRegisterValue(result, CurrentRegister);
	}

	void X86_BackEnd::AssembleCallFuncRef(IRCallFuncRef* ir_call_func_ref)
	{
		static std::map<u64, X86_Register> return_register_map = {
			{1, AL},
			{2, AX},
			{4, EAX},
			{8, RAX},
		};

		std::vector<u64> argument_allocations;
		std::vector <std::pair<Assembly_Operand*, Assembly_Operand*>> argument_allocations_registers;

		TSFunc* signature = (TSFunc*)ir_call_func_ref->Signature;

		for (size_t i = 0; i < ir_call_func_ref->Arguments.size(); i++)
		{
			auto call_argument_ir_id = ir_call_func_ref->Arguments[i];

			TypeStorage* argument_type = signature->Arguments[i];

			auto type_size = TypeSystem::GetTypeSize(argument_type);

			if (type_size > 8) {
				argument_type = TypeSystem::GetPtr(argument_type, 1);
				type_size = TypeSystem::GetTypeSize(argument_type);
			}

			if (type_size <= 8) {

				X86_Register needed_register;

				if (i < 4)
				{
					if (!TypeSystem::IsFlt(argument_type)) {
						needed_register = argument_register_map.at({ type_size,i });
					}
					else {
						needed_register = argument_float_register_map.at(i);
					}

					auto temp_register_id = CreateTempRegister(nullptr);
					auto argument_phys_register = Allocate_Register(argument_type, temp_register_id, needed_register);

					argument_allocations_registers.push_back({ argument_phys_register , nullptr });

					SetRegisterValue(argument_phys_register, temp_register_id);

					auto call_argument_value = GetRegisterValue(call_argument_ir_id);

					argument_allocations.push_back(temp_register_id);
				}
			}
		}

		for (size_t i = 0; i < ir_call_func_ref->Arguments.size(); i++)
		{
			auto call_argument_ir_id = ir_call_func_ref->Arguments[i];

			TypeStorage* argument_type = signature->Arguments[i];
			auto argument_type_flags = TypeSystem::GetTypeFlags(argument_type);

			auto type_size = TypeSystem::GetTypeSize(argument_type);

			if (type_size > 8) {
				argument_type = TypeSystem::GetPtr(argument_type, 1);
				type_size = TypeSystem::GetTypeSize(argument_type);
			}

			if (i < 4)
			{
				GS_CORE_ASSERT(argument_allocations_registers[i].first);
				auto argument_phys_register = argument_allocations_registers[i].first;

				auto call_argument_value = GetRegisterValue(call_argument_ir_id);

				Code.push_back(MoveBasedOnType(argument_type, argument_phys_register, call_argument_value));

			}
			else {
				auto call_argument_value = GetRegisterValue(call_argument_ir_id);

				if (GetRegisterValueType(call_argument_ir_id) != Register_Value_Type::Register_Value) {

					auto tmp_move_register_id = CreateTempRegister(nullptr);
					auto tmp_move_register = Allocate_Register(argument_type, tmp_move_register_id);
					SetRegisterValue(tmp_move_register, tmp_move_register_id, Register_Value_Type::Register_Value);
					UseRegisterValue(tmp_move_register_id);

					Code.push_back(MoveBasedOnType(argument_type, tmp_move_register, call_argument_value));

					call_argument_value = tmp_move_register;
				}

				auto argument_stack_location = Builder::De_Reference(Alloc_Call_StackTop(TypeSystem::GetU64()), argument_type);

				Code.push_back(MoveBasedOnType(argument_type, argument_stack_location, call_argument_value));
			}

			UseRegisterValue(call_argument_ir_id);
		}

		Assembly_Operand* return_location = nullptr;

		if (signature->ReturnType != TypeSystem::GetVoid()) {

			auto return_type_size = TypeSystem::GetTypeSize(signature->ReturnType);
			auto return_type = signature->ReturnType;

			if (return_type_size > 8) {
				return_type = TypeSystem::GetPtr(signature->ReturnType, 1);
				return_type_size = TypeSystem::GetTypeSize(return_type);
			}

			if (!TypeSystem::IsFlt(signature->ReturnType)) {
				return_location = Allocate_Register(return_type, CurrentRegister, return_register_map.at(return_type_size));
			}
			else {
				return_location = Allocate_Register(return_type, CurrentRegister, XMM0);
			}
		}

		auto return_type_size = TypeSystem::GetTypeSize(signature->ReturnType);

		if (return_type_size > 8) {
			Code.push_back(Builder::Lea(return_location, Builder::De_Reference(Stack_Alloc(signature->ReturnType), signature->ReturnType)));
		}

		Code.push_back(Builder::Call(GetRegisterValue(ir_call_func_ref->PtrRegister)));

		SetRegisterValue(return_location, Register_Value_Type::Register_Value);

		if (!TypeSystem::IsPointer(signature->ReturnType) && return_type_size <= 8) {
			if (m_Metadata->GetStructIDFromType(signature->ReturnType->BaseID) != -1) {

				auto new_return_location = Stack_Alloc(signature->ReturnType);

				Code.push_back(Builder::Mov(Builder::De_Reference(new_return_location, signature->ReturnType), return_location));

				return_location = new_return_location;
			}

			SetRegisterValue(return_location, Register_Value_Type::Stack_Address);
		}

		for (auto allocation : argument_allocations) {
			UseRegisterValue(allocation);
		}

		UseRegisterValue(ir_call_func_ref->PtrRegister);

		m_Data.Call_Stack_Pointer = 0;
	}

	void X86_BackEnd::AssembleGlobalDeclare(IRGlobalDecl* ir_global)
	{
		const VariableMetadata* global_variable_metadata = m_Metadata->GetVariableMetadataRecursive(1, ir_global->GlobID);

		GS_CORE_ASSERT(global_variable_metadata);
		GS_CORE_ASSERT(global_variable_metadata->Global);

		GS_CORE_ASSERT(!global_variable_metadata->Foreign);

		Assembly_Global global;
		global.Allocation_Size = TypeSystem::GetTypeSize(global_variable_metadata->Tipe);
		global.Name = global_variable_metadata->Name.Symbol;
		global.Linkage = Assembly_Global_Linkage::External;

		Assembly_Global_Initializer initializer;

		if (!ir_global->Initializer) {
			initializer.Type = Assembly_Global_Initializer_Type::Zero_Initilizer;
		}
		else {
			auto type_size = TypeSystem::GetTypeSize(ir_global->Type);

			for (size_t i = 0; i < type_size; i++)
			{
				initializer.Initializer_Data.push_back(ir_global->Initializer->Data[i]);
			}

			initializer.Type = Assembly_Global_Initializer_Type::Bytes_Initilizer;
		}


		global.Initializer = initializer;

		Globals.push_back(global);
	}

	void X86_BackEnd::AssembleGlobalAddress(IRGlobalAddress* ir_global_addr)
	{
		const VariableMetadata* global_variable_metadata = m_Metadata->GetVariableMetadataRecursive(1, ir_global_addr->GlobID);

		GS_CORE_ASSERT(global_variable_metadata);
		GS_CORE_ASSERT(global_variable_metadata->Global);

		auto result = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);
		Code.push_back(Builder::Lea(result, Builder::De_Reference(Builder::Symbol(global_variable_metadata->Name.Symbol))));

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleTypeValue(IRTypeValue* ir_type_value)
	{
		auto type_index = TypeSystem::GetTypeInfoIndex(ir_type_value->Type);
		SetRegisterValue(Builder::Constant_Integer(type_index), CurrentRegister, Register_Value_Type::Immediate_Value);
	}

	void X86_BackEnd::AssembleTypeInfo(IRTypeInfo* ir_type_info)
	{
		auto argument_value = GetRegisterValue(ir_type_info->ArgumentRegister);

		auto result = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

		UseRegisterValue(ir_type_info->ArgumentRegister);

		Code.push_back(Builder::Mov(result, Builder::Constant_Integer(64)));

		Code.push_back(Builder::Mul(result, argument_value));

		if (!Use_Linker) {
			Code.push_back(Builder::Lea(result, Builder::De_Reference(Builder::OpAdd(TypeInfo_Table_Label, result))));
		}
		else {

			auto tmp_reg_id = CreateTempRegister(nullptr);
			auto tmp_register = Allocate_Register(TypeSystem::GetVoidPtr(), tmp_reg_id);
			SetRegisterValue(tmp_register, tmp_reg_id);
			UseRegisterValue(tmp_reg_id);

			Code.push_back(Builder::Lea(tmp_register, Builder::De_Reference(TypeInfo_Table_Label)));
			Code.push_back(Builder::Add(result, tmp_register));
		}

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleBreak(IRBreak* ir_break)
	{
		Break_Encountered = true;
		GS_CORE_ASSERT(Skip_Target_Stack.top());
		Code.push_back(Builder::Build_Inst(I_Jmp, Skip_Target_Stack.top()));
	}

	void X86_BackEnd::AssembleNullPtr(IRNullPtr* ir_null_ptr)
	{
		SetRegisterValue(Builder::Constant_Integer(0), CurrentRegister, Register_Value_Type::Immediate_Value);
	}

	void X86_BackEnd::AssembleIntrinsic_MemSet(IRIntrinsicMemSet* ir_memset)
	{
		GS_CORE_ASSERT(ir_memset->Type);

		auto type_size = TypeSystem::GetTypeSize(ir_memset->Type);

		auto pointer_register_value = GetRegisterValue(ir_memset->Pointer_Ir_Register);

		auto mmset_return_reg_id = CreateTempRegister(nullptr);
		auto mmset_return_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmset_return_reg_id, X86_Register::RAX);
		SetRegisterValue(mmset_return_reg, mmset_return_reg_id, Register_Value_Type::Register_Value);

		auto mmset_buffer_reg_id = CreateTempRegister(nullptr);
		auto mmset_buffer_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmset_buffer_reg_id, X86_Register::RCX);
		SetRegisterValue(mmset_buffer_reg, mmset_buffer_reg_id, Register_Value_Type::Register_Value);

		auto mmset_value_reg_id = CreateTempRegister(nullptr);
		auto mmset_value_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmset_value_reg_id, X86_Register::RDX);
		SetRegisterValue(mmset_value_reg, mmset_value_reg_id, Register_Value_Type::Register_Value);

		auto mmset_size_reg_id = CreateTempRegister(nullptr);
		auto mmset_size_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmset_value_reg_id, X86_Register::R8);
		SetRegisterValue(mmset_size_reg, mmset_size_reg_id, Register_Value_Type::Register_Value);

		Code.push_back(Builder::Lea(mmset_buffer_reg, Builder::De_Reference(pointer_register_value)));
		Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmset_value_reg, Builder::Constant_Integer(ir_memset->Value)));
		Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmset_size_reg, Builder::Constant_Integer(type_size)));

		UseRegisterValue(mmset_buffer_reg_id);
		UseRegisterValue(mmset_value_reg_id);
		UseRegisterValue(mmset_size_reg_id);
		UseRegisterValue(mmset_return_reg_id);

		if (!Use_Linker) {
			Code.push_back(Builder::Call(Builder::De_Reference(Builder::Symbol("memset"))));
		}
		else {
			Code.push_back(Builder::Call(Builder::Symbol("memset")));
		}
	}

	void X86_BackEnd::AssembleAnyArray(IRAnyArray* ir_any_array)
	{
		auto [tmp_register, temp_reg_id] = Allocate_Temp_Physical_Register(TypeSystem::GetVoidPtr());

		auto any_type_size = TypeSystem::GetTypeSize(TypeSystem::GetAny());

		auto array_size = any_type_size * ir_any_array->Arguments.size();

		auto any_array_struct = Stack_Alloc(TypeSystem::GetArray());
		auto any_array_data = Stack_Alloc(array_size);

		const StructMetadata* any_struct_metadata = m_Metadata->GetStructFromType(TypeSystem::GetAny()->BaseID);
		GS_CORE_ASSERT(any_struct_metadata);

		const MemberMetadata& any_struct_data_member = any_struct_metadata->Members[any_struct_metadata->FindMember("data")];
		const MemberMetadata& any_struct_type_member = any_struct_metadata->Members[any_struct_metadata->FindMember("type")];

		u64 i = 0;
		for (IRAny& element : ir_any_array->Arguments) {

			auto any_array_data_element_location = i * any_type_size;
			auto any_data_member_location = Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(any_array_data->bin_op.operand2->constant_integer.integer - (any_array_data_element_location + any_struct_data_member.Offset)));
			auto any_type_member_location = Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(any_array_data->bin_op.operand2->constant_integer.integer - (any_array_data_element_location + any_struct_type_member.Offset)));

			auto data_register_value = GetRegisterValue(element.DataRegister);
			auto data_register_value_type = GetRegisterValueType(element.DataRegister);
			GS_CORE_ASSERT(data_register_value_type == Register_Value_Type::Stack_Address);
			UseRegisterValue(element.DataRegister);

			Code.push_back(Builder::Lea(tmp_register, Builder::De_Reference(data_register_value)));
			Code.push_back(Builder::Mov(Builder::De_Reference(any_data_member_location), tmp_register));
			Code.push_back(Builder::Mov(Builder::De_Reference(any_type_member_location, TypeSystem::GetType()), Builder::Constant_Integer(TypeSystem::GetTypeInfoIndex(element.Type))));

			i++;
		}

		const StructMetadata* array_struct_type_metadata = m_Metadata->GetStructFromType(TypeSystem::GetArray()->BaseID);
		GS_CORE_ASSERT(array_struct_type_metadata);

		const MemberMetadata& array_struct_count_member = array_struct_type_metadata->Members[array_struct_type_metadata->FindMember("count")];
		const MemberMetadata& array_struct_data_member = array_struct_type_metadata->Members[array_struct_type_metadata->FindMember("data")];

		auto any_array_struct_data_member_offset = Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(any_array_struct->bin_op.operand2->constant_integer.integer - array_struct_data_member.Offset));
		auto any_array_struct_count_member_offset = Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(any_array_struct->bin_op.operand2->constant_integer.integer - array_struct_count_member.Offset));

		Code.push_back(Builder::Lea(tmp_register, Builder::De_Reference(any_array_data)));
		Code.push_back(Builder::Mov(Builder::De_Reference(any_array_struct_data_member_offset), tmp_register));
		UseRegisterValue(temp_reg_id);

		Code.push_back(Builder::Mov(Builder::De_Reference(any_array_struct_count_member_offset, array_struct_count_member.Type), Builder::Constant_Integer(ir_any_array->Arguments.size())));

		auto result = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);
		Code.push_back(Builder::Lea(result, Builder::De_Reference(any_array_struct)));

		SetRegisterValue(result, CurrentRegister);
	}

	void X86_BackEnd::AssembleString_Initializer(IRStringInitializer* ir_string_initializer)
	{
		auto result = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

		auto data_register_value = GetRegisterValue(ir_string_initializer->Data_Register_ID);
		auto count_register_value = GetRegisterValue(ir_string_initializer->Count_Register_ID);

		const StructMetadata* string_struct_metadata = m_Metadata->GetStructFromType(TypeSystem::GetString()->BaseID);
		GS_CORE_ASSERT(string_struct_metadata);

		const MemberMetadata& string_struct_data_member = string_struct_metadata->Members[string_struct_metadata->FindMember("data")];
		const MemberMetadata& string_struct_count_member = string_struct_metadata->Members[string_struct_metadata->FindMember("count")];

		auto storage = Stack_Alloc(TypeSystem::GetString());

		auto string_struct_data_member_offset = Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(storage->bin_op.operand2->constant_integer.integer - string_struct_data_member.Offset));
		auto string_struct_count_member_offset = Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(storage->bin_op.operand2->constant_integer.integer - string_struct_count_member.Offset));

		Code.push_back(Builder::Mov(Builder::De_Reference(string_struct_data_member_offset, TypeSystem::GetVoidPtr()), data_register_value));
		Code.push_back(Builder::Mov(Builder::De_Reference(string_struct_count_member_offset, TypeSystem::GetU64()), count_register_value));

		Code.push_back(Builder::Lea(result, Builder::De_Reference(storage)));

		SetRegisterValue(result, CurrentRegister, Register_Value_Type::Register_Value);
	}

	void X86_BackEnd::AssembleReturn(IRReturn* ir_return)
	{
		Return_Encountered = true;

		auto ir_register_value = (IRRegisterValue*)ir_return->Value;

		GS_CORE_ASSERT(ir_return->Type);

		if (TypeSystem::GetVoid() != ir_return->Type) {

			GS_CORE_ASSERT(ir_return->Value);

			auto type_size = TypeSystem::GetTypeSize(ir_return->Type);

			auto data_register = GetRegisterValue(ir_register_value);
			UseRegisterValue(ir_register_value);

			if (data_register->type != Op_Register) {

				auto type_size = TypeSystem::GetTypeSize(ir_return->Type);
				auto return_type = ir_return->Type;

				if (type_size > 8) {
					return_type = TypeSystem::GetPtr(ir_return->Type, 1);
					type_size = 8;
				}

				auto temp_register_id = CreateTempRegister(nullptr);

				auto temp_phys_register = Allocate_Register(return_type, temp_register_id);

				SetRegisterValue(temp_phys_register, temp_register_id);

				Code.push_back(Builder::Mov(temp_phys_register, data_register));

				UseRegisterValue(temp_register_id);

				data_register = temp_phys_register;
			}

			if (type_size <= 8) {
				Code.push_back(MoveBasedOnType(ir_return->Type, Builder::De_Reference(Return_Storage_Location, ir_return->Type), data_register));
			}
			else {
				auto mmcpy_dest_reg_id = CreateTempRegister(nullptr);
				auto mmcpy_dest_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_dest_reg_id, X86_Register::RCX);
				SetRegisterValue(mmcpy_dest_reg, mmcpy_dest_reg_id, Register_Value_Type::Register_Value);

				auto mmcpy_src_reg_id = CreateTempRegister(nullptr);
				auto mmcpy_src_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_src_reg_id, X86_Register::RDX);
				SetRegisterValue(mmcpy_src_reg, mmcpy_src_reg_id, Register_Value_Type::Register_Value);

				auto mmcpy_size_reg_id = CreateTempRegister(nullptr);
				auto mmcpy_size_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_size_reg_id, X86_Register::R8);
				SetRegisterValue(mmcpy_size_reg, mmcpy_size_reg_id, Register_Value_Type::Register_Value);

				auto mmcpy_return_reg_id = CreateTempRegister(nullptr);
				auto mmcpy_return_reg = Allocate_Register(TypeSystem::GetVoidPtr(), mmcpy_return_reg_id, X86_Register::RAX);
				SetRegisterValue(mmcpy_return_reg, mmcpy_return_reg_id, Register_Value_Type::Register_Value);

				Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmcpy_dest_reg, Builder::De_Reference(Return_Storage_Location, TypeSystem::GetVoidPtr())));
				Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmcpy_src_reg, data_register));
				Code.push_back(MoveBasedOnType(TypeSystem::GetVoidPtr(), mmcpy_size_reg, Builder::Constant_Integer(type_size)));

				Call_Memcpy();

				UseRegisterValue(mmcpy_dest_reg_id);
				UseRegisterValue(mmcpy_src_reg_id);
				UseRegisterValue(mmcpy_size_reg_id);
				UseRegisterValue(mmcpy_return_reg_id);
			}
		}
		else {
			GS_CORE_ASSERT(!ir_return->Value);
		}

		GS_CORE_ASSERT(Return_Label);
		Code.push_back(Builder::Build_Inst(I_Jmp, Return_Label));
	}

	void X86_BackEnd::AssembleConstValue(IRCONSTValue* ir_constant)
	{
		if (!TypeSystem::IsFlt(TypeSystem::GetBasic(ir_constant->Type))) {
			SetRegisterValue(Builder::Constant_Integer(*(i64*)ir_constant->Data), Register_Value_Type::Immediate_Value);
		}
		else {
			auto type_size = TypeSystem::GetTypeSize(TypeSystem::GetBasic(ir_constant->Type));

			double data = *(double*)ir_constant->Data;

			SetRegisterValue(Create_Floating_Constant(type_size, data), Register_Value_Type::Memory_Value);
		}
	}

	void X86_BackEnd::AssembleData(IRData* ir_data)
	{
		std::string string;

		for (auto c : ir_data->Data) {
			string.push_back(c);
		}

		data_values[ir_data->ID] = Create_String_Constant(string, ir_data->ID);
	}

	void X86_BackEnd::AssembleDataValue(IRDataValue* ir_data_value)
	{
		auto data_location = data_values[ir_data_value->DataID];

		auto address_register = Allocate_Register(TypeSystem::GetVoidPtr(), CurrentRegister);

		auto lea = Builder::Lea(address_register, Builder::De_Reference(data_location));

		lea.Comment = "load string";

		Code.push_back(lea);

		SetRegisterValue(address_register, Register_Value_Type::Register_Value);
	}

	TypeStorage* X86_BackEnd::GetIRNodeType(IRInstruction* inst)
	{
		TypeStorage* type = nullptr;
		IRNodeType node_type = inst->GetType();
		return type;
	}

	std::string X86_BackEnd::GetLabelName()
	{
		std::string label_name = fmt::format("L{}_{}", Function_Counter, Label_Counter);
		Label_Counter++;
		return label_name;
	}

	void X86_BackEnd::Call_Memcpy()
	{
		//Spill_All_Scratch();

		if (!Use_Linker) {
			Code.push_back(Builder::Call(Builder::De_Reference(Builder::Symbol("memcpy"))));
		}
		else {
			Code.push_back(Builder::Call(Builder::Symbol("memcpy")));
		}
	}

	Assembly_Operand* X86_BackEnd::Stack_Alloc(TypeStorage* type)
	{
		auto type_size = TypeSystem::GetTypeSize(type);
		return Stack_Alloc(type_size);
	}

	Assembly_Operand* X86_BackEnd::Stack_Alloc(u64 size)
	{
		size = align_to(size, 4);
		m_Data.Stack_Size += size;
		return Builder::OpSub(Builder::Register(RBP), Builder::Constant_Integer(m_Data.Stack_Size));
	}

	Assembly_Operand* X86_BackEnd::Alloc_Call_StackTop(TypeStorage* type)
	{
		auto type_size = TypeSystem::GetTypeSize(type);

		auto inst = Builder::OpAdd(Builder::Register(RSP), Builder::Constant_Integer(m_Data.Call_Stack_Pointer + 32));

		m_Data.Call_Stack_Pointer += type_size;

		if (m_Data.Call_Stack_Pointer >= m_Data.Stack_Size) {
			m_Data.Call_Stack_Size = m_Data.Call_Stack_Pointer;
		}

		return inst;
	}

	Assembly_Operand* X86_BackEnd::GetReturnRegister(TypeStorage* type)
	{
		const std::unordered_map<u64, X86_Register> return_registers = {
			{1,AL},
			{2,AX},
			{4,EAX},
			{8,RAX},
		};

		auto type_size = TypeSystem::GetTypeSize(type);

		if (!TypeSystem::IsFlt(type)) {
			if (type_size > 8) {
				return Builder::Register(RAX);
			}
			else {
				return Builder::Register(return_registers.at(type_size));
			}
		}
		else {
			return Builder::Register(XMM0);
		}
	}

	Assembly_Instruction X86_BackEnd::MoveBasedOnType(TypeStorage* type, Assembly_Operand* op1, Assembly_Operand* op2, const char* comment /*= nullptr*/)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);

		Assembly_Instruction instruction = {};
		instruction.Operand1 = op1;
		instruction.Operand2 = op2;
		instruction.Comment = comment;

		if (TypeSystem::IsFlt(type)) {
			if (type_size == 4) {
				instruction.OpCode = I_MovSS;
			}
			else if (type_size == 8) {
				instruction.OpCode = I_MovSD;
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
		}
		else {
			instruction.OpCode = I_Mov;
		}

		return instruction;
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, Register_Value_Type value_type)
	{
		SetRegisterValue(register_value, CurrentRegister, value_type);
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, u64 register_id)
	{
		SetRegisterValue(register_value, register_id, Register_Value_Type::Memory_Value);
	}

	void X86_BackEnd::SetRegisterValue(Assembly_Operand* register_value, u64 register_id, Register_Value_Type value_type)
	{
		m_Data.IR_RegisterValues[register_id] = register_value;
		m_Data.IR_RegisterLifetimes[register_id] = Current_Register_Lifetime;
		m_Data.IR_RegisterValueTypes[register_id] = value_type;
	}

	u64 X86_BackEnd::CreateTempRegister(Assembly_Operand* register_value)
	{
		auto tmp_register_id = Temporary_Register_ID_Counter++;

		m_Data.IR_RegisterValues[tmp_register_id] = register_value;
		m_Data.IR_RegisterLifetimes[tmp_register_id] = 1;

		return tmp_register_id;
	}

	std::tuple<Assembly_Operand*, u64> X86_BackEnd::Allocate_Temp_Physical_Register(TypeStorage* type)
	{
		auto temp_reg_id = CreateTempRegister(nullptr);
		auto temp_reg = Allocate_Register(type, temp_reg_id);
		SetRegisterValue(temp_reg, temp_reg_id);

		return { temp_reg, temp_reg_id };
	}

	std::tuple<Assembly_Operand*, u64> X86_BackEnd::Allocate_Temp_Physical_Register(TypeStorage* type, X86_Register physical_register)
	{
		auto temp_reg_id = CreateTempRegister(nullptr);
		auto temp_reg = Allocate_Register(type, temp_reg_id, physical_register);
		SetRegisterValue(temp_reg, temp_reg_id);

		return { temp_reg, temp_reg_id };
	}

	Assembly_Operand* X86_BackEnd::GetRegisterValue(u64 ir_register)
	{
		return m_Data.IR_RegisterValues.at(ir_register);
	}

	Assembly_Operand* X86_BackEnd::GetRegisterValue(IRRegisterValue* ir_register)
	{
		return m_Data.IR_RegisterValues.at(ir_register->RegisterID);
	}

	Register_Value_Type X86_BackEnd::GetRegisterValueType(IRRegisterValue* ir_register)
	{
		return GetRegisterValueType(ir_register->RegisterID);
	}

	Register_Value_Type X86_BackEnd::GetRegisterValueType(u64 register_id)
	{
		return m_Data.IR_RegisterValueTypes.at(register_id);
	}

	void X86_BackEnd::UseRegisterValue(IRRegisterValue* ir_register)
	{
		return UseRegisterValue(ir_register->RegisterID);
	}

	void X86_BackEnd::UseRegisterValue(u64 ir_register)
	{
		Assembly_Operand* value = m_Data.IR_RegisterValues.at(ir_register);

		if (Register_Allocator_Data.allocations.find(ir_register) != Register_Allocator_Data.allocations.end()) {

			auto& liveness = m_Data.IR_RegisterLifetimes.at(ir_register);

			liveness--;

			if (liveness == 0) {
				Register_Allocation& allocation = Register_Allocator_Data.allocations.at(ir_register);
				if (!TypeSystem::IsFlt(allocation.type)) {
					Register_Allocator_Data.allocated.at(allocation.family) = false;
				}
				else {
					Register_Allocator_Data.allocated_floating.at(allocation.family) = false;
				}
			}
		}
	}

	Assembly_Operand* X86_BackEnd::Allocate_Register(TypeStorage* type, u64 ir_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		if (TypeSystem::IsFlt(type)) {
			return Allocate_Float_Register(type, ir_register);
		}

		for (auto& [family, used] : Register_Allocator_Data.allocated) {
			if (!used) {
				used = true;

				X86_Register physical_register = register_family_map.at({ type_size, family });;

				Register_Allocation allocation = { };

				allocation.family = family;
				allocation.reg = physical_register;
				allocation.type = type;
				allocation.virtual_register_id = ir_register;

				Register_Allocator_Data.allocations[ir_register] = allocation;
				Register_Allocator_Data.family_to_allocation[family] = ir_register;

				Push_Used_Register(physical_register);

				return Builder::Register(allocation.reg);
			}
		}

		for (auto [family, used] : Register_Allocator_Data.allocated) {

			auto allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation[family]];

			auto spillage_location = Builder::De_Reference(Stack_Alloc(allocation.type), allocation.type);

			auto value_type = m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id);

			Assembly_Instruction spill;

			if (value_type == Register_Value_Type::Pointer_Address) {
				Code.push_back(Builder::Lea(Builder::Register(allocation.reg), Builder::De_Reference(m_Data.IR_RegisterValues.at(allocation.virtual_register_id))));
				spill = Builder::Mov(spillage_location, Builder::Register(allocation.reg));
			}
			else {
				spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));
			}

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation.spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = register_family_map.at({ type_size, family });
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = ir_register;

			return Builder::Register(new_allocation.reg);
		}

		return nullptr;
	}

	Assembly_Operand* X86_BackEnd::Allocate_Register(TypeStorage* type, u64 ir_register, X86_Register x86_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		if (TypeSystem::IsFlt(type)) {
			return Allocate_Float_Register(type, ir_register, x86_register);
		}

		auto family = register_to_family_map.at(x86_register);

		bool& used = Register_Allocator_Data.allocated.at(family);

		if (!used) {
			used = true;

			Register_Allocation allocation = { };

			allocation.family = family;
			allocation.reg = x86_register;
			allocation.type = type;
			allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = allocation;
			Register_Allocator_Data.family_to_allocation[family] = ir_register;

			Push_Used_Register(x86_register);

			return Builder::Register(allocation.reg);
		}
		else {
			auto allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation.at(family)];

			auto spillage_location = Builder::De_Reference(Stack_Alloc(allocation.type), allocation.type);

			auto value_type = m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id);

			Assembly_Instruction spill;

			if (value_type == Register_Value_Type::Pointer_Address) {
				Code.push_back(Builder::Lea(Builder::Register(allocation.reg), Builder::De_Reference(m_Data.IR_RegisterValues.at(allocation.virtual_register_id))));
				spill = Builder::Mov(spillage_location, Builder::Register(allocation.reg));
			}
			else {
				spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));
			}

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation.spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = x86_register;
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = ir_register;

			return Builder::Register(new_allocation.reg);
		}
	}

	Assembly_Operand* X86_BackEnd::Allocate_Float_Register(TypeStorage* type, u64 ir_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		for (auto& [family, used] : Register_Allocator_Data.allocated_floating) {
			if (!used) {
				used = true;

				X86_Register physical_register = register_floating_family_map.at(family);

				Register_Allocation allocation = { };

				allocation.family = family;
				allocation.reg = physical_register;
				allocation.type = type;
				allocation.virtual_register_id = ir_register;

				Register_Allocator_Data.allocations[ir_register] = allocation;
				Register_Allocator_Data.family_to_allocation[family] = ir_register;

				return Builder::Register(allocation.reg);
			}
		}

		for (auto [family, used] : Register_Allocator_Data.allocated_floating) {

			auto& allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation[family]];

			auto spillage_location = Stack_Alloc(allocation.type);
			spillage_location = Builder::De_Reference(spillage_location, allocation.type);

			auto spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation.spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = register_floating_family_map.at(family);
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = ir_register;

			return Builder::Register(new_allocation.reg);
		}

		return nullptr;
	}

	Assembly_Operand* X86_BackEnd::Allocate_Float_Register(TypeStorage* type, u64 ir_register, X86_Register x86_register)
	{
		GS_CORE_ASSERT(type);

		auto type_size = TypeSystem::GetTypeSize(type);
		GS_CORE_ASSERT(type_size <= 8);

		auto family = register_to_family_map.at(x86_register);

		bool& used = Register_Allocator_Data.allocated_floating.at(family);

		if (!used) {
			used = true;

			Register_Allocation allocation = { };

			allocation.family = family;
			allocation.reg = x86_register;
			allocation.type = type;
			allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = allocation;
			Register_Allocator_Data.family_to_allocation[family] = ir_register;

			return Builder::Register(allocation.reg);
		}
		else {
			auto allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation[family]];

			auto spillage_location = Stack_Alloc(allocation.type);
			spillage_location = Builder::De_Reference(spillage_location, allocation.type);
			auto spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));

			spill.Comment = "Spillage";

			Code.push_back(spill);

			m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
			m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

			allocation.spillage_location = spillage_location;

			Register_Allocation new_allocation = { };

			new_allocation.family = family;
			new_allocation.reg = x86_register;
			new_allocation.type = type;
			new_allocation.virtual_register_id = ir_register;

			Register_Allocator_Data.allocations[ir_register] = new_allocation;
			Register_Allocator_Data.family_to_allocation[family] = ir_register;

			return Builder::Register(new_allocation.reg);
		}
	}

	Assembly_Operand* X86_BackEnd::Create_Floating_Constant(u64 size, double value)
	{
		Float_Constant_Counter++;

		Assembly_Float_Constant constant;
		constant.index = Float_Constant_Counter;
		constant.size = size;
		constant.value = value;

		Floats.push_back(constant);

		return Builder::De_Reference(Builder::Symbol(fmt::format("fl_{}", Float_Constant_Counter)));
	}

	Assembly_Operand* X86_BackEnd::Create_String_Constant(const std::string& data, u64 id)
	{
		Assembly_String_Constant constant;

		constant.id = id;
		constant.value = data;

		Strings.push_back(constant);

		return Builder::Symbol(fmt::format("str_{}", id));
	}

	Assembly_Operand* X86_BackEnd::Create_String_Constant(const std::string& data)
	{
		String_Id_Counter++;
		return Create_String_Constant(data, String_Id_Counter);
	}

	const std::set<u64> scratch_register_families = {
		(u64)F_B,
		(u64)F_R12,
		(u64)F_R13,
		(u64)F_R14,
		(u64)F_R15,

		(u64)XMM6,
		(u64)XMM7,
	};

	void X86_BackEnd::Push_Used_Register(X86_Register physical_register)
	{
		X86_Register_Family register_family = register_to_family_map.at(physical_register);
		Current_Function_Used_Registers.insert(register_family_map.at({ 8, register_family }));
	}

	void X86_BackEnd::Spill_All_Scratch()
	{
		for (auto& [family, used] : Register_Allocator_Data.allocated) {

			if (used) {

				auto allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation[family]];

				if (Is_Volatile(allocation.reg)) {

					auto spillage_location = Stack_Alloc(allocation.type);
					spillage_location = Builder::De_Reference(spillage_location, allocation.type);

					auto value_type = m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id);

					Assembly_Instruction spill;

					if (value_type == Register_Value_Type::Pointer_Address) {
						Code.push_back(Builder::Lea(Builder::Register(allocation.reg), Builder::De_Reference(m_Data.IR_RegisterValues.at(allocation.virtual_register_id))));
						spill = Builder::Mov(spillage_location, Builder::Register(allocation.reg));
					}
					else {
						spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));
					}

					m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
					m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

					spill.Comment = "Save Scratch Register";

					Code.push_back(spill);

					allocation.spillage_location = spillage_location;
					used = false;
				}
			}
		}

		for (auto& [family, used] : Register_Allocator_Data.allocated_floating) {

			if (used) {

				auto allocation = Register_Allocator_Data.allocations[Register_Allocator_Data.family_to_allocation[family]];

				if (Is_Volatile(allocation.reg)) {
					auto spillage_location = Stack_Alloc(allocation.type);
					spillage_location = Builder::De_Reference(spillage_location, allocation.type);

					auto value_type = m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id);

					Assembly_Instruction spill;

					if (value_type == Register_Value_Type::Pointer_Address) {
						spill = Builder::Lea(spillage_location, Builder::De_Reference(m_Data.IR_RegisterValues.at(allocation.virtual_register_id)));
					}
					else {
						spill = MoveBasedOnType(allocation.type, spillage_location, Builder::Register(allocation.reg));
					}

					spill.Comment = "Spillage";

					Code.push_back(spill);

					m_Data.IR_RegisterValues.at(allocation.virtual_register_id) = spillage_location;
					m_Data.IR_RegisterValueTypes.at(allocation.virtual_register_id) = Register_Value_Type::Memory_Value;

					allocation.spillage_location = spillage_location;
					used = false;
				}
			}
		}
	}

	bool X86_BackEnd::Is_Volatile(X86_Register physical_register)
	{
		X86_Register_Family register_family = register_to_family_map.at(physical_register);
		return scratch_register_families.find((u64)register_family) == scratch_register_families.end();
	}

	bool X86_BackEnd::Are_Equal(Assembly_Operand* operand1, Assembly_Operand* operand2)
	{
		if (operand1->type != operand2->type) {
			return false;
		}

		if (operand1->type == Op_Register) {
			if (operand1->reg.Register == operand2->reg.Register) {
				return true;
			}
		}

		return false;
	}

	FASM_Printer::FASM_Printer(Assembly_File* assembly)
	{
		Assembly = assembly;
	}

	std::string doubleToHexString(double value) {
		std::ostringstream stream;
		stream << std::setprecision(15) << std::hexfloat << value;
		std::string hexString = stream.str();

		return hexString;
	}

	std::string floatToHexString(float value) {
		std::ostringstream stream;
		stream << std::setprecision(8) << std::hexfloat << value;
		std::string hexString = stream.str();

		return hexString;
	}

	std::string FASM_Printer::Print()
	{
		std::stringstream stream;

		if (Assembly->output_mode == Assembler_Output_Mode::COFF_Object) {
			stream << "format MS64 COFF" << "\n" << "\n";

			for (Assembly_Function& func : Assembly->functions) {
				stream << "public " << func.Name << '\n';
			}

		}
		else if (Assembly->output_mode == Assembler_Output_Mode::PE_Executable) {
			stream << "format PE64 CONSOLE" << "\n" << "\n";
			stream << "entry main" << "\n" << "\n";
		}
		else {
			GS_CORE_ASSERT(nullptr);
		}
		if (Assembly->output_mode == Assembler_Output_Mode::PE_Executable) {

			{
				stream <<
					R"(
macro library[name, string]
{ common
import.data:
forward
local _label
if defined name#.redundant
if ~name#.redundant
	dd RVA name#.lookup,0,0,RVA _label,RVA name#.address
end if
end if
name#.referred = 1
common
dd 0,0,0,0,0
forward
if defined name#.redundant
if ~name#.redundant
	_label db string,0
	rb RVA $ and 1
end if
end if }

macro import name, [label, string]
{ common
	rb(-rva $) and 7
	if defined name#.referred
		name#.lookup:
forward
	if used label
		if string eqtype ''
			local _label
			dq RVA _label
		else
			dq 8000000000000000h + string
			end if
			end if
			common
			if $ > name#.lookup
				name#.redundant = 0
				dq 0
			else
				name#.redundant = 1
				end if
				name#.address:
forward
	if used label
		if string eqtype ''
			label dq RVA _label
		else
			label dq 8000000000000000h + string
			end if
			end if
			common
			if ~name#.redundant
				dq 0
				end if
				forward
				if used label& string eqtype ''
					_label dw 0
					db string, 0
					rb RVA $ and 1
					end if
					common
					end if }

	macro api[name]{})"
					;
			}

			stream << "\nsection '.idata' import data readable\n";

			std::unordered_map<std::string, std::vector<std::string>> import_per_library;

			for (size_t i = 0; i < Assembly->imports.size(); i++)
			{
				Assembly_Import& foreign_import = Assembly->imports[i];
				import_per_library[Assembly->libraries[foreign_import.library_idx].Name].push_back(foreign_import.Name);
			}

			stream << "library ";

			for (size_t i = 0; i < Assembly->libraries.size(); i++)
			{
				Assembly_Dynamic_Library& dynamic_library = Assembly->libraries[i];

				stream << dynamic_library.Name << ", " << fmt::format("'{}'", dynamic_library.Path + ".dll");

				if (i != Assembly->libraries.size() - 1) {
					stream << ", \\ \n";
				}
			}

			for (size_t i = 0; i < Assembly->libraries.size(); i++)
			{
				Assembly_Dynamic_Library& dynamic_library = Assembly->libraries[i];

				stream << "\nimport " << dynamic_library.Name;

				for (auto& import_name : import_per_library[dynamic_library.Name]) {
					stream << fmt::format(", {0}, '{0}'", import_name);
				}

				stream << "\n";
			}
		}

		for (Assembly_External_Symbol external : Assembly->externals) {
			stream << "extrn '" << external.ExternalName << "' " << "as " << external.ExternalName << "\n";
		}

		for (Assembly_Global& global : Assembly->globals) {
			if (global.Initializer.Type == Assembly_Global_Initializer_Type::Zero_Initilizer) {
				stream << "\nsection '.bss' data writable readable\n";
				break;
			}
		}

		for (Assembly_Global& global : Assembly->globals) {

			GS_CORE_ASSERT(global.Allocation_Size);

			if (global.Initializer.Type == Assembly_Global_Initializer_Type::Zero_Initilizer) {
				stream << global.Name << " " << "rb " << global.Allocation_Size << "\n";
			}
		}

		for (Assembly_Global& global : Assembly->globals) {
			if (global.Initializer.Type != Assembly_Global_Initializer_Type::Zero_Initilizer) {
				stream << "\nsection '.data' data writable readable\n";
				break;
			}
		}

		for (Assembly_Global& global : Assembly->globals) {

			GS_CORE_ASSERT(global.Allocation_Size);

			if (global.Initializer.Type != Assembly_Global_Initializer_Type::Zero_Initilizer) {
				stream << global.Name << " ";
				if (global.Initializer.Type == Assembly_Global_Initializer_Type::Bytes_Initilizer) {

					stream << "db ";

					for (size_t i = 0; i < global.Initializer.Initializer_Data.size(); i++)
					{
						stream << fmt::format("{}", global.Initializer.Initializer_Data[i]);

						if (i != global.Initializer.Initializer_Data.size() - 1) {
							stream << ", ";
						}
					}
					stream << "\n";
				}
				else {
					GS_CORE_ASSERT(nullptr);
				}
			}
		}

		//data
		stream << "\n";
		stream << "section '.rdata' data readable\n";

		stream << Assembly->type_info_table.External_Func_Type_Parameters_Array_Name;
		stream << " dq ";

		for (size_t i = 0; i < Assembly->type_info_table.Func_Type_Parameters_Array.size(); i++)
		{
			auto& entry = Assembly->type_info_table.Func_Type_Parameters_Array[i];

			if (i != 0) {
				stream << ", ";
			}

			Intel_Syntax_Printer::PrintOperand(entry.elements[0], stream);
		}

		stream << "\n";
		stream << Assembly->type_info_table.External_Enum_Members_Array_Name;
		stream << " dq ";

		for (size_t i = 0; i < Assembly->type_info_table.Enum_Members_Array.size(); i++)
		{
			auto& entry = Assembly->type_info_table.Enum_Members_Array[i];

			if (i != 0) {
				stream << ", ";
			}

			for (size_t j = 0; j < 8; j++)
			{
				Intel_Syntax_Printer::PrintOperand(entry.elements[j], stream);

				if (j != 7) {
					stream << ", ";
				}
			}
		}

		stream << "\n";
		stream << Assembly->type_info_table.External_Members_Array_Name;
		stream << " dq ";

		for (size_t i = 0; i < Assembly->type_info_table.Members_Array.size(); i++)
		{
			auto& entry = Assembly->type_info_table.Members_Array[i];

			if (i != 0) {
				stream << ", ";
			}

			for (size_t j = 0; j < 8; j++)
			{
				Intel_Syntax_Printer::PrintOperand(entry.elements[j], stream);

				if (j != 7) {
					stream << ", ";
				}
			}
		}

		stream << "\n";

		stream << Assembly->type_info_table.External_Variable_Name;
		stream << " dq ";

		for (size_t i = 0; i < Assembly->type_info_table.Entries.size(); i++)
		{
			auto& entry = Assembly->type_info_table.Entries[i];

			if (i != 0) {
				stream << ", ";
			}

			for (size_t j = 0; j < 8; j++)
			{
				Intel_Syntax_Printer::PrintOperand(entry.elements[j], stream);

				if (j != 7) {
					stream << ", ";
				}
			}
		}

		stream << "\n";

		for (Assembly_Float_Constant& floating_constant : Assembly->floats) {

			stream << "fl_";
			stream << floating_constant.index;
			stream << " ";

			if (floating_constant.size == 4) {
				float data = (float)floating_constant.value;
				stream << "dd ";
				stream << fmt::format("0x{0:x}", *(u32*)&data);
				stream << " ; ";
				stream << fmt::format("{}", data);
			}
			else if (floating_constant.size == 8) {
				double data = (double)floating_constant.value;
				stream << "dq ";
				stream << fmt::format("0x{0:x}", *(u64*)&data);
				stream << " ; ";
				stream << fmt::format("{}", data);
			}
			else {
				GS_CORE_ASSERT(nullptr);
			}
			stream << '\n';

		}

		for (Assembly_String_Constant& constant_string : Assembly->strings) {

			stream << "str_";
			stream << constant_string.id;
			stream << " db \"";

			for (size_t i = 0; i < constant_string.value.size(); i++)
			{
				char c = constant_string.value[i];

				if (c == '\\') {

					if (constant_string.value[i + 1] == 'n') {
						stream << "\"";

						stream << ", 0ah, ";

						stream << "\"";
						i++;
						continue;
					}
				}
				else {
					stream << c;
				}
			}

			stream << "\", 0\n";
		}

		PrintCode(stream);

		return stream.str();
	}

	void Intel_Syntax_Printer::PrintOperand(const Assembly_Operand* operand, std::stringstream& stream)
	{
		static const std::unordered_map<Assembly_Size, std::string> wordness_map = {
			{asm_none, ""},
			{asm_byte,"byte "},
			{asm_word,"word "},
			{asm_dword,"dword "},
			{asm_qword,"qword "},
		};

		switch (operand->type)
		{
		case Op_Register:
			stream << Register_Names.at(operand->reg.Register);
			break;
		case Op_Constant_Integer:
			stream << operand->constant_integer.integer;
			break;

		case Op_Sub:
			PrintOperand(operand->bin_op.operand1, stream);
			stream << " - ";
			PrintOperand(operand->bin_op.operand2, stream);
			break;

		case Op_Add:
			PrintOperand(operand->bin_op.operand1, stream);
			stream << " + ";
			PrintOperand(operand->bin_op.operand2, stream);
			break;

		case Op_Mul:
			PrintOperand(operand->bin_op.operand1, stream);
			stream << " * ";
			PrintOperand(operand->bin_op.operand2, stream);
			break;

		case Op_De_Reference:
			stream << wordness_map.at(operand->de_reference.wordness);
			stream << '[';
			PrintOperand(operand->de_reference.operand, stream);
			stream << ']';
			break;
		case Op_Symbol:
			stream << operand->symbol.symbol;
			break;
		default:
			GS_CORE_ASSERT(nullptr, "Un Implemented Operand Instruction");
			break;
		}
	}

	std::string GetJumpInstructionName(Assembly_Op_Code opcode)
	{
		switch (opcode)
		{
		case I_Je:   return "je";
		case I_Jne:  return "jne";
		case I_Jg:   return "jg";
		case I_Jl:   return "jl";
		case I_Jge:  return "jge";
		case I_Ja:   return "ja";
		case I_Jb:   return "jb";
		case I_Jbe:  return "jbe";
		case I_Jae:  return "jae";
		case I_Jle:  return "jle";
		case I_Jmp:  return "jmp";
		default:     return "unknown_jump";
		}
	}

	std::string GetSetInstructionName(Assembly_Op_Code opcode)
	{
		switch (opcode)
		{
		case I_Setne: return "setne";
		case I_Sete:  return "sete";
		case I_Setg:  return "setg";
		case I_Setl:  return "setl";
		case I_Seta:  return "seta";
		case I_Setb:  return "setb";
		default:      return "unknown_set";
		}
	}

	std::string GetSignExtensionInstructionName(Assembly_Op_Code opcode)
	{
		switch (opcode)
		{
		case I_CBW: return "cbw";
		case I_CWD: return "cwd";
		case I_CDQ: return "cdq";
		case I_CQO: return "cqo";
		default:    return "unknown_sign_extension";
		}
	}

	std::string GetFloatingPointInstructionName(Assembly_Op_Code opcode, const std::string& baseName)
	{
		switch (opcode)
		{
		case I_UCOMISS: return baseName + "s";
		case I_UCOMISD: return baseName + "d";
		default:        return "unknown_floating_point";
		}
	}

	void PrintArithmeticInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2)
	{
		switch (opcode)
		{
		case I_Add:   stream << "add "; break;
		case I_AddSS: stream << "addss "; break;
		case I_AddSD: stream << "addsd "; break;
		case I_Sub:   stream << "sub "; break;
		case I_SubSS: stream << "subss "; break;
		case I_SubSD: stream << "subsd "; break;
		case I_IDiv:  stream << "idiv "; break;
		case I_Div:   stream << "div "; break;
		case I_IMul:  stream << "imul "; break;
		case I_MulSS: stream << "mulss "; break;
		case I_MulSD: stream << "mulsd "; break;
		case I_DivSS: stream << "divss "; break;
		case I_DivSD: stream << "divsd "; break;
		default:      stream << "unknown_arithmetic "; break;
		}
		Intel_Syntax_Printer::PrintOperand(&op1, stream);
		if (opcode != I_IDiv && opcode != I_Div)
		{
			stream << ", ";
			Intel_Syntax_Printer::PrintOperand(&op2, stream);
		}
	}

	void PrintMoveInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2)
	{
		switch (opcode)
		{
		case I_Mov:   stream << "mov "; break;
		case I_MovD:  stream << "movd "; break;
		case I_MovQ:  stream << "movq "; break;
		case I_MovSS: stream << "movss "; break;
		case I_MovSD: stream << "movsd "; break;
		case I_MovZX: stream << "movzx "; break;
		case I_MovSX: stream << "movsx "; break;
		case I_MovSXD: stream << "movsxd "; break;
		case I_Lea:   stream << "lea "; break;
		case I_CvtSI2SS: stream << "cvtsi2ss "; break;
		case I_CvtSI2SD: stream << "cvtsi2sd "; break;
		case I_CvtSS2SI: stream << "cvtss2si "; break;
		case I_CvtSD2SI: stream << "cvtsd2si "; break;
		case I_CvtSS2SD: stream << "cvtss2sd "; break;
		case I_CvtSD2SS: stream << "cvtsd2ss "; break;
		default:        stream << "unknown_move "; break;
		}
		Intel_Syntax_Printer::PrintOperand(&op1, stream);
		stream << ", ";
		Intel_Syntax_Printer::PrintOperand(&op2, stream);
	}

	void PrintComparisonInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2)
	{
		switch (opcode)
		{
		case I_Cmp: stream << "cmp "; break;
		case I_And: stream << "and "; break;
		case I_Or:  stream << "or "; break;
		default:    stream << "unknown_comparison "; break;
		}
		Intel_Syntax_Printer::PrintOperand(&op1, stream);
		stream << ", ";
		Intel_Syntax_Printer::PrintOperand(&op2, stream);
	}

	void Intel_Syntax_Printer::PrintInstruction(const Assembly_Instruction& instruction, std::stringstream& stream)
	{
		switch (instruction.OpCode)
		{
		case I_Label:
			PrintOperand(instruction.Operand1, stream);
			stream << ':';
			break;
		case I_Je:
		case I_Jne:
		case I_Jg:
		case I_Jl:
		case I_Jge:
		case I_Ja:
		case I_Jb:
		case I_Jbe:
		case I_Jae:
		case I_Jle:
		case I_Jmp:
			stream << GetJumpInstructionName(instruction.OpCode) << " ";
			PrintOperand(instruction.Operand1, stream);
			break;

		case I_Add:
		case I_AddSS:
		case I_AddSD:
		case I_Sub:
		case I_SubSS:
		case I_SubSD:
		case I_IDiv:
		case I_Div:
		case I_IMul:
		case I_MulSS:
		case I_MulSD:
		case I_DivSS:
		case I_DivSD:
			PrintArithmeticInstruction(instruction.OpCode, stream, *instruction.Operand1, *instruction.Operand2);
			break;

		case I_Mov:
		case I_MovD:
		case I_MovQ:
		case I_MovSS:
		case I_MovSD:
		case I_MovZX:
		case I_MovSX:
		case I_MovSXD:
		case I_Lea:
		case I_CvtSI2SS:
		case I_CvtSI2SD:
		case I_CvtSS2SI:
		case I_CvtSD2SI:
		case I_CvtSS2SD:
		case I_CvtSD2SS:
			PrintMoveInstruction(instruction.OpCode, stream, *instruction.Operand1, *instruction.Operand2);
			break;

		case I_Cmp:
		case I_And:
		case I_Or:
			PrintComparisonInstruction(instruction.OpCode, stream, *instruction.Operand1, *instruction.Operand2);
			break;

		case I_Setne:
		case I_Sete:
		case I_Setg:
		case I_Setl:
		case I_Seta:
		case I_Setb:
			stream << GetSetInstructionName(instruction.OpCode) << " ";
			PrintOperand(instruction.Operand1, stream);
			break;

		case I_Push:
			stream << "push ";
			PrintOperand(instruction.Operand1, stream);
			break;
		case I_Pop:
			stream << "pop ";
			PrintOperand(instruction.Operand1, stream);
			break;
		case I_Ret:
			stream << "ret";
			break;
		case I_Call:
			stream << "call ";
			PrintOperand(instruction.Operand1, stream);
			break;
		case I_CBW:
		case I_CWD:
		case I_CDQ:
		case I_CQO:
			stream << GetSignExtensionInstructionName(instruction.OpCode);
			break;

		case I_UCOMISS:
		case I_UCOMISD:
			stream << GetFloatingPointInstructionName(instruction.OpCode, "ucomis") << " ";
			PrintOperand(instruction.Operand1, stream);
			stream << ", ";
			PrintOperand(instruction.Operand2, stream);
			break;
		default:
			GS_CORE_ASSERT(nullptr, "Unimplemented Assembly Instruction");
			break;
		}
	}

	void FASM_Printer::PrintCode(std::stringstream& stream)
	{
		stream << "\n";
		stream << "section '.code' code readable executable\n";
		stream << "\n";

		for (Assembly_Function& asm_function : Assembly->functions) {
			stream << asm_function.Name << ":" << "\n";

			for (auto& code : asm_function.Code) {

				if (code.OpCode != I_Label) {
					stream << "\t";
				}

				Intel_Syntax_Printer::PrintInstruction(code, stream);

				if (code.Comment) {
					stream << "\t; ";
					stream << code.Comment;
				}

				stream << "\n";
			}
			stream << "\n";
			stream << "\n";
		}
	}
}