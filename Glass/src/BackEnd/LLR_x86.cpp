#include "pch.h"

#include "BackEnd/LLR_x86.h"

namespace Glass
{
	X86_BackEnd::X86_BackEnd(IRTranslationUnit* translation_unit, MetaData* metadata)
		:m_TranslationUnit(translation_unit), m_Metadata(metadata)
	{
	}

	std::vector<X86_Inst*> X86_BackEnd::Assemble()
	{
		std::vector<X86_Inst*> build_stream;

		for (auto i : m_TranslationUnit->Instructions) {
			if (i->GetType() == IRNodeType::File) {

				IRFile* ir_file = (IRFile*)i;

				for (auto tl_inst : ir_file->Instructions) {
					if (tl_inst->GetType() == IRNodeType::Function) {
						Assemble((IRFunction*)tl_inst, build_stream);
					}
				}
			}
		}

		return build_stream;
	}

	void X86_BackEnd::Assemble(IRInstruction* inst, std::vector<X86_Inst*>& stream)
	{
		IRNodeType node_type = inst->GetType();

		switch (node_type)
		{
		default:
			break;
		}

	}

	std::string X86_BackEnd::Print(const std::vector<X86_Inst>& assm)
	{
		std::string stream;

		u32 indentation = 0;

		for (size_t i = 0; i < assm.size(); i++)
		{
			X86_Inst inst = assm[i];
			Print(inst, stream, indentation);
			stream += '\n';

			for (u32 i = 0; i < indentation; i++)
			{
				stream += "  ";
			}
		}

		return stream;
	}

	void X86_BackEnd::Print(X86_Inst inst, std::string& stream, u32& indentation)
	{
		switch (inst.type) {
		case X86_LABEL:
			stream += inst.as.label.name;
			stream += ":";
			indentation++;
			break;

		case X86_PUSH:
		case X86_PUSHQ:
			stream += "push";
			stream += "\t";
			Print(*inst.as.push.source, stream, indentation);
			break;

		case X86_POP:
		case X86_POPQ:
			stream += "pop";
			stream += "\t";
			Print(*inst.as.push.source, stream, indentation);
			break;

		case X86_MOV:
		case X86_MOVQ:
			stream += "mov";
			stream += "\t";
			Print(*inst.as.move.destination, stream, indentation);
			stream += ", ";
			Print(*inst.as.move.source, stream, indentation);
			break;

		case X86_REG_NAME:
			stream += RegisterToString(inst.as.reg_name.Register);
			break;

		case X86_RET:
			stream += "ret";
			break;

		case X86_ISUB:
			stream += "sub";
			stream += "\t";
			Print(*inst.as.bin_op.destination, stream, indentation);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, indentation);
			break;
		case X86_IADD:
			stream += "add";
			stream += "\t";
			Print(*inst.as.bin_op.destination, stream, indentation);
			stream += ", ";
			Print(*inst.as.bin_op.value, stream, indentation);
			break;

		case X86_CONSTANT:
			stream += std::to_string(*(i64*)&inst.as.constant.bytes);
			break;

		case X86_CONSTANT_OFFSET: {
			static std::vector<const char*> words = { "word","dword","qword" };
			stream += words[inst.as.constant_offset.size];
			stream += " [";
			Print(*inst.as.constant_offset.from, stream, indentation);
			stream += " - ";
			Print(*inst.as.constant_offset.offset, stream, indentation);
			stream += "]";
			break;
		}

		default:
			stream += "un-implemented print inst";
			break;
		}
	}

	std::string X86_BackEnd::RegisterToString(X86_Register reg)
	{
		static std::vector<std::string> names = {
			"rsp", "rbp", "rax", "rcx", "rdx", "rbx", "rsi", "rdi",
			"esp", "ebp", "eax", "ecx", "edx", "ebx", "esi", "edi",
			"sp", "bp", "ax", "cx", "dx", "bx", "si", "di",
			"spl", "bpl", "ah", "al", "ch", "cl", "dh", "dl", "bh", "bl", "sil", "dil",
			"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
		};

		return names[reg];
	}

	X86_Inst* X86_BackEnd::Make_Register(X86_Register register_type)
	{
		X86_Inst* inst = new X86_Inst;
		inst->type = X86_REG_NAME;
		inst->as.reg_name = X86_Reg_Name_Inst{ register_type };

		return inst;
	}

	X86_Inst* X86_BackEnd::Make_Constant(i64 integer)
	{
		X86_Inst* inst = new X86_Inst;

		inst->type = X86_CONSTANT;
		memcpy(&inst->as.constant.bytes, &integer, 8);

		return inst;
	}
}

/*X86_Inst x86_inst = { };

#define PUSH_VALUE(x) stream.push_back(x); x86_inst = {}

		switch (inst.type)
		{
		case LLR_InstType::SymbolicLabel: {

			x86_inst.type = X86_LABEL;
			x86_inst.as.label = X86_Label_Inst{ inst.as.sym_label.name };

			PUSH_VALUE(x86_inst);
		}
										break;
		case LLR_InstType::Prologue: {

			//push	rbp
			x86_inst.type = X86_PUSH;
			x86_inst.as.push = X86_Push_Inst{ Make_Register(RBP) };
			PUSH_VALUE(x86_inst);

			//mov     rbp, rsp
			x86_inst.type = X86_MOVQ;
			x86_inst.as.move = X86_Mov_Inst{ Make_Register(RBP), Make_Register(RSP) };
			PUSH_VALUE(x86_inst);
		}
								   break;
		case LLR_InstType::Epilogue: {

			X86_Inst x86_inst = {};

			//push	rbp
			x86_inst.type = X86_POP;
			x86_inst.as.pop = X86_Pop_Inst{ Make_Register(RBP) };
			PUSH_VALUE(x86_inst);
			//ret
			x86_inst.type = X86_RET;
			PUSH_VALUE(x86_inst);
		}
								   break;
		case LLR_InstType::StackAlloc: {

			x86_inst.type = X86_ISUB;
			x86_inst.as.bin_op.destination = Make_Register(RSP);
			x86_inst.as.bin_op.value = Make_Constant(inst.as.s_alloc.amount);

			PUSH_VALUE(x86_inst);
		}
									 break;

		case LLR_InstType::StackFree: {

			x86_inst.type = X86_IADD;
			x86_inst.as.bin_op.destination = Make_Register(RSP);
			x86_inst.as.bin_op.value = Make_Constant(inst.as.s_alloc.amount);

			PUSH_VALUE(x86_inst);
		}
									break;
		case LLR_InstType::StackOffset: {

			x86_inst.type = X86_CONSTANT_OFFSET;

			x86_inst.as.constant_offset.from = Make_Register(RBP);
			x86_inst.as.constant_offset.offset = Make_Constant(inst.as.s_offset.offset);
			x86_inst.as.constant_offset.size = (X86_Word)inst.as.s_offset.load_type;

			PUSH_VALUE(x86_inst);
		}
									  break;

		case LLR_InstType::RegisterAlloc: {
			Allocate_Register(inst.as.reg_alloc.type, inst.as.reg_alloc.AllocationID);
		}
										break;

		case LLR_InstType::RegisterFree: {
			Free_Register(inst.as.reg_free.AllocationID);
		}
									   break;
		case LLR_InstType::RegisterAllocationID: {
			auto reg = *Make_Register(Get_Register(inst.as.reg_allocation_id.AllocationID));
			PUSH_VALUE(reg);
		}
											   break;
		case LLR_InstType::Move: {

			x86_inst.type = X86_MOVQ;

			std::vector <X86_Inst> args;

			Assemble(*inst.as.move.from, args);
			Assemble(*inst.as.move.to, args);

			x86_inst.as.move.source = new X86_Inst(args[0]);
			x86_inst.as.move.destination = new X86_Inst(args[1]);

			PUSH_VALUE(x86_inst);
		}
							   break;
		case LLR_InstType::SIAdd:
		case LLR_InstType::ZIAdd:
		{
			x86_inst.type = X86_IADD;

			std::vector <X86_Inst> args;

			Assemble(*inst.as.bin_op.destination, args);
			Assemble(*inst.as.bin_op.value, args);

			x86_inst.as.bin_op.destination = new X86_Inst(args[0]);
			x86_inst.as.bin_op.value = new X86_Inst(args[1]);

			PUSH_VALUE(x86_inst);
		}
		break;

		case LLR_InstType::Constant:
		{
			PUSH_VALUE(*Make_Constant(*(i64*)inst.as.constant.bytes));
		}
		break;

		default:
			GS_CORE_ASSERT(0, "Un-Implemented LLR instruction");
			break;
		}*/