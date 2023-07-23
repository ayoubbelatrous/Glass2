#include "pch.h"

#include "BackEnd/C/CTranspiler.h"

namespace Glass
{
	CTranspiler::CTranspiler(IRTranslationUnit* program, const Compiler::MetaData* metadata)
		:m_Program(program), m_Metadata(metadata)
	{
	}

	std::string CTranspiler::Codegen()
	{
		std::string code;

		code += "\n";

		code += "#include <stdint.h>\n";
		code += "#include <stdio.h>\n";

		code += "\n\n";

		code += "typedef char i8;		\n";
		code += "typedef int16_t i16;	\n";
		code += "typedef int32_t i32;	\n";
		code += "typedef int64_t i64;	\n";

		code += "\n";

		code += "typedef uint8_t u8;	\n";
		code += "typedef uint16_t u16;	\n";
		code += "typedef uint32_t u32;	\n";
		code += "typedef uint64_t u64;	\n";

		code += "\n";

		for (IRInstruction* inst : m_Program->Instructions) {

			IRNodeType Type = inst->GetType();

			if (Type == IRNodeType::Function) {
				code += FunctionCodeGen((IRFunction*)inst) + "\n";
			}
			if (Type == IRNodeType::Struct) {
				code += StructCodeGen((IRStruct*)inst) + "\n";
			}
			if (Type == IRNodeType::Data) {
				IRData* data = (IRData*)inst;

				std::string bytes;

				for (auto c : data->Data) {
					bytes.push_back(c);
				}

				code += fmt::format("const char* __data{} = \"{}\";\n", data->ID, bytes);
			}
		}

		return code;
	}

	std::string CTranspiler::IRCodeGen(IRInstruction* inst)
	{
		IRNodeType Type = inst->GetType();

		switch (Type)
		{
		case IRNodeType::ConstValue:
		{
			return ((IRCONSTValue*)inst)->ToString();
		}
		break;
		case IRNodeType::SSA:
			return SSACodeGen((IRSSA*)inst) + ";";
			break;
		case IRNodeType::ADD:
		case IRNodeType::SUB:
		case IRNodeType::MUL:
		case IRNodeType::DIV:
			return OpCodeGen(inst) + ";";
			break;
		case IRNodeType::SSAValue:
			return SSAValueCodeGen((IRSSAValue*)inst);
			break;
		case IRNodeType::Call:
			return CallCodeGen((IRFunctionCall*)inst);
			break;
		case IRNodeType::AddressOf:
		{
			IRAddressOf* add_of = (IRAddressOf*)inst;
			return fmt::format("(u64)(&__tmp{})", add_of->SSA);
		}
		break;
		case IRNodeType::AsAddress:
		{
			IRAsAddress* as_add = (IRAsAddress*)inst;
			return fmt::format("(void *)__tmp{}", as_add->SSA);
		}
		break;
		case IRNodeType::Store:
		{
			IRStore* store = (IRStore*)inst;
			return fmt::format("*((u64*)__tmp{}) = (u64){};", store->AddressSSA, IRCodeGen(store->Data));
		}
		break;
		case IRNodeType::Load:
		{
			IRLoad* load = (IRLoad*)inst;
			return fmt::format("*({}*)__tmp{}", m_Metadata->GetType(load->Type), load->SSAddress);
		}
		break;
		case IRNodeType::DataValue:
		{
			IRDataValue* data_val = (IRDataValue*)inst;
			return fmt::format("(u8*)__data{}", data_val->DataID);
		}
		break;
		case IRNodeType::MemberAccess:
		{
			IRMemberAccess* member_access = (IRMemberAccess*)inst;

			const auto struct_metadata = m_Metadata->GetStructMetadata(member_access->StructID);

			u64 ssa = member_access->ObjectSSA;
			std::string type = struct_metadata->Name.Symbol;
			std::string member = struct_metadata->Members[member_access->MemberID].Name.Symbol;

			return fmt::format("(u64)(&(({} *)__tmp{})->{})", type, ssa, member);
		}
		break;
		}

		return "";
	}

	std::string CTranspiler::FunctionCodeGen(IRFunction* IRF)
	{
		const auto func_metadata = m_Metadata->GetFunctionMetadata(IRF->ID);

		const std::string& func_name = func_metadata->Name;
		const std::string& return_type = m_Metadata->GetType(func_metadata->ReturnType);
		std::string arguments;

		u64 i = 0;
		for (IRSSA* arg : IRF->Arguments) {

			std::string type = m_Metadata->GetType(arg->Type);

			if (arg->Pointer) {
				type.push_back('*');
			}

			arguments += fmt::format("{} {}", type, "__tmp" + std::to_string(arg->ID));

			if (i == IRF->Arguments.size() - 1) {
			}
			else {
				arguments += ", ";
			}
			i++;
		}

		std::string code = fmt::format("{} {} ({})", return_type, func_name, arguments);

		code += "{\n";

		for (IRInstruction* inst : IRF->Instructions) {

			IRNodeType Type = inst->GetType();

			switch (Type)
			{
			case IRNodeType::SSA:
				code += SSACodeGen((IRSSA*)inst) + "\n";
				break;
			case IRNodeType::Return:
			{
				IRReturn* ret = (IRReturn*)inst;
				code += "return " + IRCodeGen(ret->Value) + ";\n";
			}
			break;
			default:
				code += IRCodeGen(inst) + ";";
				break;
			}
		}

		code += "}\n";

		return code;
	}

	std::string CTranspiler::SSACodeGen(IRSSA* SSA)
	{
		std::string type = m_Metadata->GetType(SSA->Type);

		if (SSA->Pointer) {
			type.push_back('*');
		}
		if (SSA->Value) {
			return fmt::format("const {} __tmp{} = {};", type, SSA->ID, IRCodeGen(SSA->Value));
		}
		else {
			return fmt::format("const {} __tmp{};", type, SSA->ID);
		}
	}

	std::string CTranspiler::StructCodeGen(IRStruct* ir_struct)
	{
		u64 struct_id = ir_struct->ID;
		const Compiler::StructMetadata* metadata = m_Metadata->GetStructMetadata(struct_id);

		std::string code;

		code = fmt::format("typedef struct {}", metadata->Name.Symbol);
		code += "{\n";

		for (const Compiler::MemberMetadata& member : metadata->Members) {
			std::string member_type = m_Metadata->GetType(member.Tipe.ID);

			if (member.Tipe.TT == Compiler::TypeType::Pointer) {
				member_type += "*";
			}

			code += fmt::format("\t{} {};\n", member_type, member.Name.Symbol);
		}

		code += "}";
		code += metadata->Name.Symbol;
		code += ";\n";

		return code;
	}

	std::string CTranspiler::SSAValueCodeGen(IRSSAValue* ssaVal)
	{
		return fmt::format("__tmp{}", ssaVal->SSA);
	}

	std::string CTranspiler::OpCodeGen(IRInstruction* op)
	{
		std::string code;

		switch (op->GetType())
		{
		case IRNodeType::ADD:
		{
			IRADD* o = (IRADD*)op;

			code = fmt::format("__tmp{} + __tmp{}", o->SSA_A->SSA, o->SSA_B->SSA);
		}
		break;
		case IRNodeType::SUB:
		{
			IRSUB* o = (IRSUB*)op;

			code = fmt::format("__tmp{} + __tmp{}", o->SSA_A->SSA, o->SSA_B->SSA);
		}
		break;
		case IRNodeType::MUL:
		{
			IRMUL* o = (IRMUL*)op;

			code = fmt::format("__tmp{} + __tmp{}", o->SSA_A->SSA, o->SSA_B->SSA);
		}
		break;
		case IRNodeType::DIV:
		{
			IRDIV* o = (IRDIV*)op;

			code = fmt::format("__tmp{} + __tmp{}", o->SSA_A->SSA, o->SSA_B->SSA);
		}
		break;
		}

		return code;
	}

	std::string CTranspiler::CallCodeGen(IRFunctionCall* call)
	{
		std::string arg_code;

		u64 counter = 0;
		for (IRInstruction* arg : call->Arguments) {
			arg_code += IRCodeGen(arg);

			if (counter < call->Arguments.size() - 1) {
				arg_code += ", ";
			}

			counter++;
		}

		return fmt::format("{}({})", m_Metadata->GetFunctionMetadata(call->FuncID)->Name, arg_code);
	}

	std::string CTranspiler::GetType(u64 ID)
	{
		return m_TypeMap[ID];
	}
}