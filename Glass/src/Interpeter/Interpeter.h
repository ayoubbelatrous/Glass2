#pragma once

#include "BackEnd/Compiler.h"

#define REGISTER_N 512 * 16

namespace Glass
{

	class Interpeter
	{
	public:

		Interpeter(IRInstruction* program, const Compiler::MetaData* metadata);

		void Prepare();

		void Run(u64 Function);

		void* Exec(IRInstruction* instruction);

		void ExecFunction(IRFunction* Irf);

	private:

		u64* PushRegister()
		{
			m_RegisterPointer++;

			return &m_Registers[m_RegisterPointer];
		}

		u64* GetRegister(u64 Index)
		{
			return &m_Registers[Index];
		}

		void* Push(u64 size)
		{
			m_StackPointer += size;
			return (void*)&m_Stack[m_StackPointer + size];
		}

		void PopStack(u64 location)
		{
			m_StackPointer = location;
		}

		void PopRegisters(u64 location)
		{
			m_RegisterPointer = location;
		}

		std::unordered_map<u64, IRFunction*> m_Functions;
		IRTranslationUnit* m_Program = nullptr;
		const Compiler::MetaData* m_Metadata = nullptr;

		u64 m_RegisterPointer = 0;
		u64 m_StackPointer = 0;

		u64 m_Registers[REGISTER_N / 2];
		u64 m_Stack[REGISTER_N / 2];
	};
}