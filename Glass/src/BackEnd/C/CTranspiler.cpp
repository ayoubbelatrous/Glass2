#include "pch.h"

#include "BackEnd/C/CTranspiler.h"

namespace Glass
{
	CTranspiler::CTranspiler(IRTranslationUnit* program, const std::vector<std::string>& includes, const Compiler::MetaData* metadata)
		:m_Program(program), m_Includes(includes), m_Metadata(metadata)
	{
	}

	std::string CTranspiler::Codegen()
	{
		std::string code;

		code += "\n";

		code += "#include <stdint.h>\n";
		code += "#include <stdio.h>\n";
		code += "#include <string.h>\n";
		code += "#include <malloc.h>\n";

		for (auto& include : m_Includes) {
			code += fmt::format("#include \"{}\"\n", include);
		}

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

		code += "typedef float f32;		\n";
		code += "typedef double f64;	\n";

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
			IRCONSTValue* constant = (IRCONSTValue*)inst;
			if (constant->Type == IR_float) {
				float data = 0;
				memcpy(&data, &constant->Data, sizeof(float));
				return std::to_string(data);
			}
			else {
				i32 data = 0;
				memcpy(&data, &constant->Data, sizeof(i32));
				return std::to_string(data);
			}
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
			return fmt::format("(u64)(&{})", IRCodeGen(add_of->SSA));
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

			std::string ptr = "";

			if (store->Pointer) {
				ptr = "*";
			}

			return fmt::format("*(({0}{1}*)__tmp{2}) = ({0}{1}){3};", m_Metadata->GetType(store->Type), ptr, store->AddressSSA, IRCodeGen(store->Data));
		}
		break;
		case IRNodeType::Load:
		{
			IRLoad* load = (IRLoad*)inst;

			std::string ptr;

			if (load->ReferencePointer) {
				ptr = "*";
			}

			return fmt::format("*({} {}*)__tmp{}", m_Metadata->GetType(load->Type), ptr, load->SSAddress);
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

			std::string ptr;

			if (member_access->ReferenceAccess) {
				ptr = "*";
			}

			return fmt::format("((u64)&({1}({0} {1}*)__tmp{2})->{3})", type, ptr, ssa, member);
		}
		break;
		case IRNodeType::If:
		{
			IRIf* if_ = (IRIf*)inst;
			std::string code = fmt::format("if (__tmp{})", if_->SSA);

			code += " {\n";

			for (IRInstruction* inst : if_->Instructions) {
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
		break;
		case IRNodeType::While:
		{
			IRWhile* while_ = (IRWhile*)inst;
			std::string code = fmt::format("while (__tmp{})", while_->SSA);

			code += " {\n";

			for (IRInstruction* inst : while_->Instructions) {
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
		break;
		case IRNodeType::SizeOf:
		{
			IRSizeOF* size_of = (IRSizeOF*)inst;
			return fmt::format("sizeof({})", m_Metadata->GetType(size_of->Type));
		}
		break;
		case IRNodeType::ARGValue:
		{
			IRARGValue* arg_val = (IRARGValue*)inst;
			return fmt::format("__arg{}", arg_val->SSA);
		}
		break;
		case IRNodeType::Break:
		{
			return "break";
		}
		break;
		}

		return "";
	}

	std::string CTranspiler::FunctionCodeGen(IRFunction* IRF)
	{
		const auto func_metadata = m_Metadata->GetFunctionMetadata(IRF->ID);

		const std::string& func_name = func_metadata->Name;
		std::string return_type = m_Metadata->GetType(func_metadata->ReturnType.ID);

		if (func_metadata->ReturnType.TT == Compiler::TypeType::Pointer) {
			return_type += '*';
		}

		std::string arguments;

		u64 i = 0;
		for (IRSSA* arg : IRF->Arguments) {

			std::string type = m_Metadata->GetType(arg->Type);

			if (arg->Pointer) {
				type.push_back('*');
			}

			arguments += fmt::format("{} {}", type, "__arg" + std::to_string(arg->ID));

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

			code = fmt::format("__tmp{} - __tmp{}", o->SSA_A->SSA, o->SSA_B->SSA);
		}
		break;
		case IRNodeType::MUL:
		{
			IRMUL* o = (IRMUL*)op;

			code = fmt::format("__tmp{} * __tmp{}", o->SSA_A->SSA, o->SSA_B->SSA);
		}
		break;
		case IRNodeType::DIV:
		{
			IRDIV* o = (IRDIV*)op;

			code = fmt::format("__tmp{} / __tmp{}", o->SSA_A->SSA, o->SSA_B->SSA);
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