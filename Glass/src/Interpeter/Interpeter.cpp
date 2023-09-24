#include "pch.h"

#include "Interpeter/Interpeter.h"
#include "Application.h"

namespace Glass
{

	Interpeter::Interpeter(IRInstruction* program, const MetaData* metadata)
		:m_Program((IRTranslationUnit*)program), m_Metadata(metadata)
	{
	}

	void Interpeter::Prepare()
	{
		for (IRInstruction* inst : m_Program->Instructions) {
			if (inst->GetType() == IRNodeType::Function) {
				IRFunction* IRF = (IRFunction*)inst;
				m_Functions[IRF->ID] = IRF;
			}
		}
	}

	void Interpeter::Run(u64 Function)
	{
		IRFunction* IRF = m_Functions[Function];
		ExecFunction(IRF);
	}

	void* Interpeter::Exec(IRInstruction* instruction)
	{
		switch (instruction->GetType()) {
		case IRNodeType::SSA:
		{
			IRSSA* ssa = (IRSSA*)instruction;
			void* value = Exec(ssa->Value);
			*PushRegister() = *(u64*)value;
		}
		break;
		case IRNodeType::ConstValue:
		{
			IRCONSTValue* ssa = (IRCONSTValue*)instruction;
			u64* stack_location = (u64*)Push(1);
			memcpy(stack_location, &ssa->Data, 8);
			return stack_location;
		}
		case IRNodeType::SSAValue:
		{
			IRSSAValue* ssa_val = (IRSSAValue*)instruction;

			return (void*)GetRegister(ssa_val->RegisterID);
		}
		break;
		case IRNodeType::ADD:
		{
			IRADD* add = (IRADD*)instruction;

			void* a = Exec(add->SSA_A);
			void* b = Exec(add->SSA_B);

			u64* stack_location = (u64*)Push(1);

			*stack_location = *(u64*)a + *(u64*)b;

			return (void*)stack_location;
		}
		break;
		}

		return nullptr;
	}

	void Interpeter::ExecFunction(IRFunction* Irf)
	{
		u64 stack_location = m_StackPointer;
		u64 register_location = m_RegisterPointer;

		for (IRInstruction* instruction : Irf->Instructions) {
			Exec(instruction);
		}

		PopStack(stack_location);
		PopRegisters(register_location);
	}

}