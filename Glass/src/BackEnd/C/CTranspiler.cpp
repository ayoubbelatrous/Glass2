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
		std::string header;

		header += "\n";

		header += "#include <stdint.h>\n";
		header += "#include <stdio.h>\n";
		header += "#include <string.h>\n";
		header += "#include <malloc.h>\n";

		for (auto& include : m_Includes) {
			header += fmt::format("#include \"{}\"\n", include);
		}

		header += "\n\n";

		header += "#define true 1;		\n";
		header += "#define false 0;		\n";

		header += "typedef char i8;		\n";
		header += "typedef int16_t i16;	\n";
		header += "typedef int32_t i32;	\n";
		header += "typedef int64_t i64;	\n";

		header += "\n";

		header += "typedef uint8_t u8;	\n";
		header += "typedef uint16_t u16;	\n";
		header += "typedef uint32_t u32;	\n";
		header += "typedef uint64_t u64;	\n";

		header += "typedef float f32;		\n";
		header += "typedef double f64;	\n";

		header += "typedef u8 bool;		\n";

		header += R"(
typedef struct type_info
{	u64 id;
	bool pointer;
	const char* name;
} type_info;
			)";
		header += "\n";

		{
			header += fmt::format("static const type_info __type_info_table[{}] = ", m_Metadata->m_Types.size());
			header += "{\n";

			u64 i = 0;

			for (const auto& [id, type_name] : m_Metadata->m_Types) {
				m_TypeInfoTable[id] = i;

				header += "{";
				header += fmt::format(".name=\"{}\",", type_name);
				header += fmt::format(".id={}", id);
				header += "},";

				i++;
			}

			header += "};\n";
		}
		header += "\n";

		std::string code;

		std::string forward_declaration;

		for (IRInstruction* inst : m_Program->Instructions) {
			if (inst->GetType() == IRNodeType::Function) {
				IRFunction* IRF = (IRFunction*)inst;

				const auto func_metadata = m_Metadata->GetFunctionMetadata(IRF->ID);

				const std::string& func_name = func_metadata->Name;
				std::string return_type = m_Metadata->GetType(func_metadata->ReturnType.ID);

				if (func_metadata->ReturnType.TT == TypeType::Pointer) {
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

				forward_declaration += fmt::format("{} {} ({});\n", return_type, func_name, arguments);
			}
			else if (inst->GetType() == IRNodeType::Struct) {

				IRStruct* ir_struct = (IRStruct*)inst;
				u64 struct_id = ir_struct->ID;
				const StructMetadata* metadata = m_Metadata->GetStructMetadata(struct_id);

				std::string code;

				forward_declaration += fmt::format("typedef struct {0} {0};", metadata->Name.Symbol);
			}
		}

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

		std::string var_type_table;

		var_type_table += "\n";

		{
			var_type_table += fmt::format("static const type_info __var_type_info_table[{}] = ", m_VariableTypeInfo.size());
			var_type_table += "{\n";

			u64 i = 0;

			for (const auto& [id, type] : m_VariableTypeInfo) {
				m_TypeInfoTable[id] = i;

				var_type_table += "{";
				var_type_table += fmt::format(".id={},", type.id);
				var_type_table += fmt::format(".pointer={},", (u64)type.pointer);
				var_type_table += fmt::format(".name=\"{}\",", type.name);
				var_type_table += "},";

				i++;
			}

			var_type_table += "};\n";
		}
		var_type_table += "\n";

		return fmt::format("{}{}{}{}", header, var_type_table, forward_declaration, code);
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
		case IRNodeType::AddressAsValue:
		{
			IRAddressAsValue* as_value = (IRAddressAsValue*)inst;
			return fmt::format("(u64)__tmp{}", as_value->SSA);
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
		case IRNodeType::Ref:
		{
			IRRef* ref = (IRRef*)inst;
			return fmt::format("&__tmp{}", ref->SSA);
		}
		break;
		case IRNodeType::TypeOf:
		{
			IRTypeOf* type_of = (IRTypeOf*)inst;
			m_VariableTypeInfo[m_VariableTypeInfo.size()] = type_of->Type;
			return fmt::format("(&__var_type_info_table[{}])", m_VariableTypeInfo.size() - 1);
		}
		break;
		}

		return "";
	}

	std::string CTranspiler::FunctionCodeGen(IRFunction* IRF)
	{
		std::string code;

		// 		if (IRF->Polymorphs.size() != 0) {
		// 			for (IRFunction* irf : IRF->Polymorphs) {
		// 				code += "\n";
		// 				code += FunctionCodeGen(irf);
		// 			}
		// 
		// 			return code;
		// 		}

		const auto func_metadata = m_Metadata->GetFunctionMetadata(IRF->ID);

		const std::string& func_name = func_metadata->Name;
		std::string return_type = m_Metadata->GetType(func_metadata->ReturnType.ID);

		if (func_metadata->ReturnType.TT == TypeType::Pointer) {
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

		code = fmt::format("{} {} ({})", return_type, func_name, arguments);

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
		const StructMetadata* metadata = m_Metadata->GetStructMetadata(struct_id);

		std::string code;

		code = fmt::format("typedef struct {}", metadata->Name.Symbol);
		code += "{\n";

		for (const MemberMetadata& member : metadata->Members) {
			std::string member_type = m_Metadata->GetType(member.Tipe.ID);

			if (member.Tipe.TT == TypeType::Pointer) {
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