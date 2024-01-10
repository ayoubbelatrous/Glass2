#include "pch.h"

#include "Intel_Syntax_Printer.h"

namespace Glass
{
	std::unordered_map<X86_Register, std::string> Register_Names = {
		{X86_Register::RIP,"rip"},
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

	const std::string wordness_map[5] = {
		"",
		"byte ",
		"word ",
		"dword ",
		"qword ",
	};

	const std::string wordness_map_ptr[5] = {
		"",
		"byte ptr",
		"word ptr",
		"dword ptr",
		"qword ptr",
	};

	void Intel_Syntax_Printer::PrintOperand(const Assembly_Operand* operand, std::stringstream& stream)
	{

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
			if (ptr_at_deref) {
				stream << wordness_map_ptr[operand->de_reference.wordness];
			}
			else {
				stream << wordness_map[operand->de_reference.wordness];
			}
			stream << '[';
			PrintOperand(operand->de_reference.operand, stream);
			stream << ']';
			break;
		case Op_Symbol:

			if (dot_at_label) {
				stream << '.';
			}

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

	void Intel_Syntax_Printer::PrintArithmeticInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2)
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

	void Intel_Syntax_Printer::PrintMoveInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2)
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
		PrintOperand(&op1, stream);
		stream << ", ";
		PrintOperand(&op2, stream);
	}

	void Intel_Syntax_Printer::PrintComparisonInstruction(Assembly_Op_Code opcode, std::stringstream& stream, const Assembly_Operand& op1, const Assembly_Operand& op2)
	{
		switch (opcode)
		{
		case I_Cmp: stream << "cmp "; break;
		case I_And: stream << "and "; break;
		case I_Or:  stream << "or "; break;
		default:    stream << "unknown_comparison "; break;
		}
		PrintOperand(&op1, stream);
		stream << ", ";
		PrintOperand(&op2, stream);
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

}