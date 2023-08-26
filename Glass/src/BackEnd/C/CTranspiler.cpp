#include "pch.h"

#include "BackEnd/C/CTranspiler.h"

//#define DBG_COMMENT(x) x
#define DBG_COMMENT(x)

namespace Glass
{
	CTranspiler::CTranspiler(IRTranslationUnit* program, const std::vector<std::string>& includes, const MetaData* metadata)
		:m_Program(program), m_Includes(includes), m_Metadata(metadata)
	{
	}

	std::string CTranspiler::Codegen()
	{
		std::string header;

		header += "\n";

		//header += "#include <stdint.h>\n";
// 		header += "#include <stdio.h>\n";
// 		header += "#include <string.h>\n";
// 		header += "#include <malloc.h>\n";

		for (auto& include : m_Includes) {
			header += fmt::format("#include \"{}\"\n", include);
		}

		header += "\n\n";

		header += "#define true 1;		\n";
		header += "#define false 0;		\n";

		header += "typedef signed char i8;		\n";
		header += "typedef short i16;	\n";
		header += "typedef int i32;	\n";
		header += "typedef long long i64;	\n";

		header += "\n";

		header += "typedef unsigned char u8;	\n";
		header += "typedef unsigned short u16;	\n";
		header += "typedef unsigned int u32;	\n";
		header += "typedef unsigned long long u64;	\n";

		header += "typedef float f32;		\n";
		header += "typedef double f64;	\n";

		header += "typedef u8 bool;		\n";

		header += "\n";

		header += R"(
typedef struct Array
{	
	u64 count;
	u64 data;
} Array;
			)";
		header += "\n";

		header += "typedef u64 TypeInfo_Flags;\n";
		header += "\n";

		header += R"(
typedef struct TypeInfo
{	
	const u64 id;
	const bool pointer;
	const char* const name;
	const u64 size;
	const TypeInfo_Flags flags;
} TypeInfo;
			)";
		header += "\n";

		header += R"(
typedef struct TypeInfo_Struct
{	
	const u64 id;
	bool pointer;
	const char* const name;
	const u64 size;
	const TypeInfo_Flags flags;
	
	const Array members;
} TypeInfo_Struct;
			)";

		header += "\n";

		header += R"(
typedef struct TypeInfo_Member
{
	const u64 id;
	bool pointer;
	const char* const name;
	const u64 size;
	const TypeInfo_Flags flags;

	const char* const member_name;
} TypeInfo_Member;
			)";

		header += "\n";

		header += R"(
typedef struct Any
{	
	u64 type;
	u64* data;
} Any;
			)";
		header += "\n";

		{
			header += fmt::format("static const TypeInfo __type_info_table[{}] = ", m_Metadata->m_Types.size());
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

		for (const auto& [ID, metadata] : m_Metadata->m_StructMetadata) {
			if (metadata.Foreign) {
				forward_declaration += fmt::format("typedef struct {0} {0};", metadata.Name.Symbol);
			}
		}

		for (const auto& [ID, metadata] : m_Metadata->m_Enums) {
			forward_declaration += fmt::format("typedef u64 {0};", metadata.Name.Symbol);
		}

		for (IRInstruction* inst : m_Program->Instructions) {
			if (inst->GetType() == IRNodeType::Function) {
				IRFunction* IRF = (IRFunction*)inst;

				const auto func_metadata = m_Metadata->GetFunctionMetadata(IRF->ID);

				const std::string& func_name = func_metadata->Name;
				std::string return_type = m_Metadata->GetType(func_metadata->ReturnType.ID);

				if (func_metadata->ReturnType.Pointer) {
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

		for (const auto& [ID, metadata] : m_Metadata->m_Functions) {
			if (metadata.Foreign) {

				const std::string& func_name = metadata.Name;
				std::string return_type = m_Metadata->GetType(metadata.ReturnType.ID);

				for (u64 i = 0; i < metadata.ReturnType.Pointer; i++) {
					return_type.push_back('*');
				}

				std::string arguments;

				u64 i = 0;
				for (const ArgumentMetadata& arg_metadata : metadata.Arguments) {

					std::string type = m_Metadata->GetType(arg_metadata.Tipe.ID);

					for (u64 i = 0; i < arg_metadata.Tipe.Pointer; i++) {
						type.push_back('*');
					}

					arguments += fmt::format("{} {}", type, arg_metadata.Name);

					if (i == metadata.Arguments.size() - 1) {
					}
					else {
						arguments += ", ";
					}
					i++;
				}

				if (metadata.Variadic) {
					arguments.append(",...");
				}

				forward_declaration += fmt::format("{} {} ({});\n", return_type, func_name, arguments);
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
			if (Type == IRNodeType::GlobDecl) {
				code += IRCodeGen(inst) + ";";
			}
		}

		std::string type_info_code;
		std::string type_info_struct_member_data;

		//Type Info
		{
			{//Base Type Info
				std::string var_type_table;

				if (m_VariableTypeInfo.size() > 0)
				{
					var_type_table += "\n";

					{
						var_type_table += fmt::format("static const TypeInfo __var_type_info_table[{}] = ", m_VariableTypeInfo.size());
						var_type_table += "{\n";

						u64 i = 0;

						for (const auto [id, type] : m_VariableTypeInfo) {
							m_TypeInfoTable[id] = i;

							var_type_table += "{";
							//var_type_table += fmt::format(".id={},", type.id);
							//var_type_table += fmt::format(".pointer={},", (u64)type.pointer);
							//var_type_table += fmt::format(".name=\"{}\",", type.name);
							//var_type_table += fmt::format(".size={},", type.size);
							//
							//var_type_table += fmt::format(".flags={},", type.flags);
							var_type_table += "},";

							i++;
						}

						var_type_table += "};\n";
					}
					var_type_table += "\n";
				}

				type_info_code += var_type_table;
			}

			{//Struct Type Info
				std::string var_struct_type_table;

				if (m_VariableStructTypeInfo.size() > 0)
				{
					var_struct_type_table += "\n";

					{
						var_struct_type_table += fmt::format("static const TypeInfo_Struct __var_type_info_struct_table[{}] = ", m_VariableStructTypeInfo.size());
						var_struct_type_table += "{\n";

						u64 i = 0;

						for (const auto [id, type] : m_VariableStructTypeInfo) {
							m_TypeInfoTable[id] = i;

							var_struct_type_table += "{";
							//var_struct_type_table += fmt::format(".id={},", type.id);
							//var_struct_type_table += fmt::format(".pointer={},", (u64)type.pointer);
							//var_struct_type_table += fmt::format(".name=\"{}\",", type.name);
							//var_struct_type_table += fmt::format(".size={},", type.size);
							//var_struct_type_table += fmt::format(".flags={},", type.flags);

							std::string member_type_info_name = fmt::format("__struct_{}_members", id);

							type_info_struct_member_data += fmt::format("static const TypeInfo_Member {} [{}]", member_type_info_name, type.members.size());
							type_info_struct_member_data += "= { \n";
							for (const TypeInfoMember& member : type.members) {
								type_info_struct_member_data += "{";

								//type_info_struct_member_data += fmt::format(".id={},", member.id);
								//type_info_struct_member_data += fmt::format(".pointer={},", (u64)member.pointer);
								//type_info_struct_member_data += fmt::format(".name=\"{}\",", m_Metadata->GetType(member.id));
								//type_info_struct_member_data += fmt::format(".size={},", m_Metadata->GetTypeSize(member.id));
								//type_info_struct_member_data += fmt::format(".flags={},", member.flags);

								type_info_struct_member_data += fmt::format(".member_name=\"{}\",", member.member_name);

								type_info_struct_member_data += "},";
							}
							type_info_struct_member_data += "};\n";

							var_struct_type_table += "\n.members = {";
							var_struct_type_table += fmt::format(".data = &{},", member_type_info_name);
							var_struct_type_table += fmt::format(".count = {}", type.members.size());
							var_struct_type_table += "}\n";

							var_struct_type_table += "},";

							i++;
						}

						var_struct_type_table += "};\n";
					}
					var_struct_type_table += "\n";
				}

				type_info_code += var_struct_type_table;
			}

		}

		return fmt::format("{}{}{}{}{}", header, type_info_struct_member_data, type_info_code, forward_declaration, code);
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
		case IRNodeType::Equal:
		case IRNodeType::NotEqual:
		case IRNodeType::GreaterThan:
		case IRNodeType::LesserThan:
		case IRNodeType::BitAnd:
		case IRNodeType::BitOr:
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
		case IRNodeType::Store:
		{
			IRStore* store = (IRStore*)inst;

			std::string ptr = "";

			for (u64 i = 0; i < store->Pointer; i++)
			{
				ptr += "*";
			}


			std::string code;
			DBG_COMMENT(code += "// Store\n");

			code += fmt::format("*(({0}{1}*)__tmp{2}) = {3};", m_Metadata->GetType(store->Type), ptr, store->AddressSSA, IRCodeGen(store->Data));
			return code;
		}
		break;
		case IRNodeType::Load:
		{
			IRLoad* load = (IRLoad*)inst;

			std::string ptr;

			for (u64 i = 0; i < load->ReferencePointer; i++)
			{
				ptr += "*";
			}
			std::string code;
			DBG_COMMENT(code += "// Load\n");
			code += fmt::format("*({} {}*)__tmp{}", m_Metadata->GetType(load->Type), ptr, load->SSAddress);
			return code;
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

			std::string code;
			DBG_COMMENT(code += "// MemberAccess\n");
			code += fmt::format("((u64)&({1}({0} {1}*)__tmp{2})->{3})", type, ptr, ssa, member);
			return code;
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

			u64 label_id = PushLabel();

			std::string code;
			std::string goto_code = fmt::format("goto _l{}", label_id);

			code += "_l" + std::to_string(label_id) + ":\n";

			for (IRInstruction* inst : while_->ConditionBlock) {
				IRNodeType Type = inst->GetType();
				code += IRCodeGen(inst) + ";";
			}

			code += fmt::format("if(__tmp{})", while_->SSA);

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
			code += goto_code + ";";
			code += "}\n";

			//PushLabelCode(code);

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
		case IRNodeType::DeRef:
		{
			IRDeRef* de_ref = (IRDeRef*)inst;
			return fmt::format("*__tmp{}", de_ref->SSA);
		}
		break;
		case IRNodeType::TypeOf:
		{
			return TypeOfCodeGen((IRTypeOf*)inst);
		}
		break;
		case IRNodeType::ArrayAllocate:
		{
			IRArrayAllocate* array_allocate = (IRArrayAllocate*)inst;

			u64 counter = 0;

			std::string data_str = "{";

			for (auto data : array_allocate->Data) {

				data_str.append("__tmp");
				data_str.append(std::to_string(data));

				if (counter != array_allocate->Data.size() - 1) {
					data_str.push_back((','));
				}

				counter++;
			}

			data_str += "};";

			return data_str;
		}
		break;
		case IRNodeType::FuncPtr:
		{
			IRFuncPtr* func_ptr = (IRFuncPtr*)inst;
			return fmt::format("&{}", m_Metadata->GetFunctionMetadata(func_ptr->FunctionID)->Name);
		}
		break;
		case IRNodeType::GlobAddress:
		{
			IRGlobalAddress* address_of_global = (IRGlobalAddress*)inst;
			return fmt::format("(u64)&__glob{}", address_of_global->GlobID);
		}
		break;
		case IRNodeType::GlobDecl:
		{
			IRGlobalDecl* global_decl = (IRGlobalDecl*)inst;

			std::string type = m_Metadata->GetType(global_decl->Type);

			for (u64 i = 0; i < global_decl->Pointer; i++)
			{
				type.push_back('*');
			}

			return fmt::format("{} __glob{}", type, global_decl->GlobID);
		}
		break;
		}

		return "";
	}

	std::string CTranspiler::TypeOfCodeGen(IRTypeOf* type_of)
	{
		// 		if (type_of->typeInfoType == TypeInfoType::Base) {
		// 			m_VariableTypeInfo[m_VariableTypeInfo.size()] = *type_of->Type;
		// 
		// 			return fmt::format("(&__var_type_info_table[{}])", m_VariableTypeInfo.size() - 1);
		// 		}
		// 		else if (type_of->typeInfoType == TypeInfoType::Struct) {
		// 			m_VariableStructTypeInfo[m_VariableStructTypeInfo.size()] = *(TypeInfoStruct*)type_of->Type;
		// 
		// 			return fmt::format("(&__var_type_info_struct_table[{}])", m_VariableStructTypeInfo.size() - 1);
		// 		}

		return "";
	}

	std::string CTranspiler::FunctionCodeGen(IRFunction* IRF)
	{
		std::string code;

		const auto func_metadata = m_Metadata->GetFunctionMetadata(IRF->ID);

		const std::string& func_name = func_metadata->Name;
		std::string return_type = m_Metadata->GetType(func_metadata->ReturnType.ID);

		if (func_metadata->ReturnType.Pointer) {
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

		std::string header = fmt::format("{} {} ({})", return_type, func_name, arguments);

		header += "{\n";

		for (IRInstruction* inst : IRF->Instructions) {

			IRNodeType Type = inst->GetType();

			switch (Type)
			{
			case IRNodeType::SSA:
			{
				code += SSACodeGen((IRSSA*)inst) + "\n";
			}
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

		code += m_LabelCode;

		ClearLabelCode();

		std::string final_code = fmt::format("{} {} {}", header, m_SSAHeader, code);

		final_code += "}\n";

		ClearSSAHeader();

		return final_code;
	}

	std::string CTranspiler::SSACodeGen(IRSSA* SSA)
	{
		std::string type = m_Metadata->GetType(SSA->Type);


		for (u64 i = 0; i < SSA->Pointer; i++)
		{
			type.push_back('*');
		}

		if (SSA->Array) {
		}
		else {
			PushSSAHeader(fmt::format("{} __tmp{};", type, SSA->ID));
		}

		if (SSA->Array) {
			IRArrayAllocate* array_allocate = (IRArrayAllocate*)SSA->Value;

			std::string value = IRCodeGen(array_allocate);
			u64 count = array_allocate->Data.size();

			return fmt::format("{} __tmp{}[{}] = {};", type, SSA->ID, count, value);
		}

		if (SSA->Value && !SSA->Array) {
			return fmt::format("__tmp{} = {};", SSA->ID, IRCodeGen(SSA->Value));
		}

		return "";
	}

	std::string CTranspiler::StructCodeGen(IRStruct* ir_struct)
	{
		u64 struct_id = ir_struct->ID;
		const StructMetadata* metadata = m_Metadata->GetStructMetadata(struct_id);

		std::string code;

		code = fmt::format("typedef struct {}", metadata->Name.Symbol);
		code += "{\n";

		for (const MemberMetadata& member : metadata->Members) {
			std::string member_type;

			if (member.Tipe.Array) {
				member_type = m_Metadata->GetType(IR_array);
			}
			else {
				member_type = m_Metadata->GetType(member.Tipe.ID);
			}

			for (u64 i = 0; i < member.Tipe.Pointer; i++)
			{
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
		case IRNodeType::Equal:
		{
			IREQ* eq = (IREQ*)op;

			code = fmt::format("__tmp{} == __tmp{}", eq->SSA_A->SSA, eq->SSA_B->SSA);
		}
		break;
		case IRNodeType::NotEqual:
		{
			IRNOTEQ* neq = (IRNOTEQ*)op;

			code = fmt::format("__tmp{} != __tmp{}", neq->SSA_A->SSA, neq->SSA_B->SSA);
		}
		break;
		case IRNodeType::GreaterThan:
		{
			IRGreater* greater = (IRGreater*)op;

			code = fmt::format("__tmp{} > __tmp{}", greater->SSA_A->SSA, greater->SSA_B->SSA);
		}
		break;
		case IRNodeType::LesserThan:
		{
			IRLesser* lesser = (IRLesser*)op;

			code = fmt::format("__tmp{} < __tmp{}", lesser->SSA_A->SSA, lesser->SSA_B->SSA);
		}
		break;
		case IRNodeType::BitAnd:
		{
			IRBitAnd* bit_and = (IRBitAnd*)op;

			code = fmt::format("__tmp{} & __tmp{}", bit_and->SSA_A->SSA, bit_and->SSA_B->SSA);
		}
		break;
		case IRNodeType::BitOr:
		{
			IRBitOr* bit_or = (IRBitOr*)op;

			code = fmt::format("__tmp{} | __tmp{}", bit_or->SSA_A->SSA, bit_or->SSA_B->SSA);
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