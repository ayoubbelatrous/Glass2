#include "pch.h"

#include "BackEnd/LLR_x86.h"
#include "BackEnd/TypeSystem.h"

#include "math.h"

namespace Glass
{
	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		: m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
		Init();
	}

	void X86_BackEnd::Init()
	{
	}

	TypeStorage* X86_BackEnd::GetIRNodeType(IRInstruction* inst)
	{
		TypeStorage* type = nullptr;
		IRNodeType node_type = inst->GetType();
		return type;
	}

	// 	//
	// 	// 
	// 	switch (node_type)
	// 	{
	// 	case IRNodeType::ConstValue:
	// 	{
	// 		IRCONSTValue* as_const_value = (IRCONSTValue*)inst;
	// 		type = TypeSystem::GetBasic(as_const_value->Type);
	// 	}
	// 	break;
	// 	case IRNodeType::DataValue:
	// 	{
	// 		type = TypeSystem::GetPtr(TypeSystem::GetBasic(IR_u8), 1);
	// 	}
	// 	break;
	// 	case IRNodeType::Alloca:
	// 	{
	// 		IRAlloca* as_alloca = (IRAlloca*)inst;
	// 		type = as_alloca->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Load:
	// 	{
	// 		IRLoad* as_load = (IRLoad*)inst;
	// 		type = as_load->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::MemberAccess:
	// 	{
	// 		auto member_access = (IRMemberAccess*)inst;
	// 
	// 		const StructMetadata* struct_metadata = m_Metadata->GetStructMetadata(member_access->StructID);
	// 		GS_CORE_ASSERT(struct_metadata);
	// 		GS_CORE_ASSERT(member_access->MemberID < struct_metadata->Members.size());
	// 
	// 		const MemberMetadata& member = struct_metadata->Members[member_access->MemberID];
	// 		type = TypeSystem::GetPtr(member.Type, 1);
	// 	}
	// 	break;
	// 	case IRNodeType::ArrayAccess:
	// 	{
	// 		auto array_access = (IRArrayAccess*)inst;
	// 		type = TypeSystem::GetPtr(array_access->Type, 1);
	// 	}
	// 	break;
	// 	case IRNodeType::Store:
	// 	{
	// 		IRStore* as_store = (IRStore*)inst;
	// 		type = as_store->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Call:
	// 	{
	// 		IRFunctionCall* as_call = (IRFunctionCall*)inst;
	// 		type = m_Metadata->GetFunctionMetadata(as_call->FuncID)->ReturnType;
	// 	}
	// 	break;
	// 	case IRNodeType::ADD:
	// 	case IRNodeType::SUB:
	// 	case IRNodeType::MUL:
	// 	case IRNodeType::DIV:
	// 	{
	// 		IRBinOp* as_binop = (IRBinOp*)inst;
	// 		type = as_binop->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Argument:
	// 	{
	// 		GS_CORE_ASSERT(m_CurrentFunction);
	// 
	// 		IRArgumentAllocation* argument = (IRArgumentAllocation*)inst;
	// 		type = m_CurrentFunction->Arguments[argument->ArgumentIndex].Type;
	// 	}
	// 	break;
	// 	case IRNodeType::RegisterValue:
	// 	{
	// 		type = m_Data.IR_RegisterTypes.at(((IRRegisterValue*)inst)->RegisterID);
	// 	}
	// 	break;
	// 	case IRNodeType::GlobAddress:
	// 	{
	// 		type = m_Metadata->GetVariableMetadata(((IRGlobalAddress*)inst)->GlobID)->Tipe;
	// 	}
	// 	break;
	// 	case IRNodeType::NullPtr:
	// 	{
	// 		type = TypeSystem::GetPtr(TypeSystem::GetBasic(((IRNullPtr*)inst)->TypeID), 1);
	// 	}
	// 	break;
	// 	case IRNodeType::PointerCast:
	// 	{
	// 		type = ((IRPointerCast*)inst)->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::IntTrunc:
	// 	case IRNodeType::Int2PtrCast:
	// 	case IRNodeType::Ptr2IntCast:
	// 	case IRNodeType::SExtCast:
	// 	case IRNodeType::ZExtCast:
	// 	case IRNodeType::FPTrunc:
	// 	case IRNodeType::FPExt:
	// 	{
	// 		type = ((IRIntTrunc*)inst)->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::Int2FP:
	// 	{
	// 		type = ((IRInt2FP*)inst)->Type;
	// 	}
	// 	break;
	// 	case IRNodeType::GreaterThan:
	// 	case IRNodeType::LesserThan:
	// 	case IRNodeType::Equal:
	// 	case IRNodeType::NotEqual:
	// 	case IRNodeType::BitAnd:
	// 	case IRNodeType::BitOr:
	// 	case IRNodeType::And:
	// 	case IRNodeType::Or:
	// 	{
	// 		type = ((IRBinOp*)inst)->Type;
	// 	}
	// 	break;
	// 
	// 	case IRNodeType::TypeValue:
	// 	{
	// 		return TypeSystem::GetBasic(IR_type);
	// 	}
	// 	break;
	// 	case IRNodeType::TypeInfo:
	// 	case IRNodeType::TypeOf:
	// 	{
	// 		return TypeSystem::GetPtr(TypeSystem::GetBasic(IR_typeinfo), 1);
	// 	}
	// 	break;
	// 	case IRNodeType::Any:
	// 	{
	// 		return TypeSystem::GetBasic(IR_any);
	// 	}
	// 	case IRNodeType::AnyArray:
	// 	{
	// 		return TypeSystem::GetDynArray(TypeSystem::GetAny());
	// 	}
	// 	break;
	// 	case IRNodeType::FuncRef:
	// 	{
	// 		type = m_Metadata->GetFunctionMetadata(((IRFuncRef*)inst)->FunctionID)->Signature;
	// 	}
	// 	break;
	// 	case IRNodeType::CallFuncRef:
	// 	{
	// 		type = ((TSFunc*)((IRCallFuncRef*)inst)->Signature)->ReturnType;
	// 	}
	// 	break;
	// 
	// 	default:
	// 		GS_CORE_ASSERT(0);
	// 		break;
	// 	}

		//

}