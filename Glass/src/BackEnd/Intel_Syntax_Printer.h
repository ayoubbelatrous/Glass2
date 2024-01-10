
#include "Assembly.h"

namespace Glass
{
	struct Intel_Syntax_Printer {
		void PrintOperand(const Assembly_Operand* operand, std::stringstream& stream);
		void PrintInstruction(const Assembly_Instruction& instruction, std::stringstream& stream);

		void PrintComparisonInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2);
		void PrintArithmeticInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2);
		void PrintMoveInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2);

		bool ptr_at_deref = false;
		bool dot_at_label = false;
	};
}