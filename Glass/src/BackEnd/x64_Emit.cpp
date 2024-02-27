#include "pch.h"

#include "BackEnd/x64_Emit.h"

namespace Glass
{
	void Emit_Push(Array<u8>& bytes, Inst_Op op)
	{
		u8 push_op_code = 0;

		if (op.type == Op_Reg) {
			push_op_code = 0x50;
			push_op_code += op.reg;

			if (op.reg > RDI) {
				push_op_code -= R8;
				Emit_Rex(bytes, 0, 0, 0, 1);
			}

			Write_8(bytes, push_op_code);
		}

		if (op.type == Op_Imm8) {
			push_op_code = 0x6a;
			Write_8(bytes, push_op_code);
			Write_8(bytes, op.imm8);
		}

		if (op.type == Op_Imm16 || op.type == Op_Imm32) {
			push_op_code = 0x68;
			Write_8(bytes, push_op_code);
			Write_32(bytes, op.imm32);
		}

		if (op.type == Op_Reg_Disp4 || op.type == Op_Reg_Disp1) {
			push_op_code = 0xff;

			if (op.reg > RDI) {
				Emit_Rex(bytes, 0, 0, 0, 1);
				op.reg_disp.r -= R8;
			}

			Write_8(bytes, push_op_code);
			Emit_Mod_RM(bytes, Mod_Disp4, 0b110, op.reg_disp.r);
			Write_32(bytes, op.reg_disp.disp);
		}
	}

	void Emit_Pop(Array<u8>& bytes, Inst_Op op)
	{
		if (op.type == Op_Imm8 || op.type == Op_Imm16 || op.type == Op_Imm32) ASSERT(nullptr, "invalid operand");

		u8 pop_op_code = 0;

		if (op.type == Op_Reg) {
			pop_op_code = 0x58;
			pop_op_code += op.reg;

			if (op.reg > RDI) {
				pop_op_code -= R8;
				Emit_Rex(bytes, 0, 0, 0, 1);
			}

			Write_8(bytes, pop_op_code);
		}

		if (op.type == Op_Reg_Disp4 || op.type == Op_Reg_Disp1)
		{
			pop_op_code = 0x8f;

			if (op.reg > RDI) {
				Emit_Rex(bytes, 0, 0, 0, 1);
				op.reg_disp.r -= R8;
			}

			Write_8(bytes, pop_op_code);
			Emit_Mod_RM(bytes, Mod_Disp4, 0b000, op.reg_disp.r);
			Write_32(bytes, op.reg_disp.disp);
		}
	}

	void Emit_Mov(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		if (op1.type == Op_Imm8 || op1.type == Op_Imm16 || op1.type == Op_Imm32) ASSERT(nullptr, "invalid operand");
		if (op1.type == Op_Reg_Disp4 && op2.type == Op_Reg_Disp4) ASSERT(nullptr, "invalid operand");

		u8 mov_op_code = -1;

		u8 immx = 0;
		i32 imm = -2;

		bool w = 0;
		bool r = 0;
		bool b = 0;

		u8 reg_opcode = 0;
		u8 rm = 0;
		u8 mod = 0;

		bool displace = 0;
		i32 disp = 0;

		bool rsp_sib = 0;

		if ((op1.type == Op_Reg || op1.type == Op_Reg_Disp4) && (op2.type == Op_Imm8 || op2.type == Op_Imm16 || op2.type == Op_Imm32))
		{
			mov_op_code = 0xc6 + (op_size > 8);

			if (op_size > 32) w = 1;
			if (op1.reg >= R8) { op1.reg -= R8; b = 1; }

			reg_opcode = 0b000;
			rm = op1.reg;
			mod = op1.type == Op_Reg ? Mod_Reg : Mod_Disp4;

			if (op1.type == Op_Reg_Disp4) {
				displace = 1;
				disp = op1.reg_disp.disp;
				if (!r && op1.reg == RSP) { rsp_sib = true; rm = 0b100; }
			}

			if (op2.type == Op_Imm8) immx = 8;
			if (op2.type == Op_Imm16) immx = 16;
			if (op2.type == Op_Imm32) immx = 32;

			imm = op2.imm32;
		}

		if ((op1.type == Op_Reg || op1.type == Op_Reg_Disp4) && op2.type == Op_Reg)
		{
			mov_op_code = 0x88 + (op_size > 8);

			if (op_size > 32) w = 1;
			if (op1.reg >= R8) { op1.reg -= R8; b = 1; }
			if (op2.reg >= R8) { op2.reg -= R8; r = 1; }

			reg_opcode = op2.reg;
			rm = op1.reg;
			mod = op1.type == Op_Reg ? Mod_Reg : Mod_Disp4;

			if (op1.type == Op_Reg_Disp4) {
				displace = 1;
				disp = op1.reg_disp.disp;
				if (!b && op1.reg == RSP) { rsp_sib = true; rm = 0b100; }
			}
		}

		if (op1.type == Op_Reg && op2.type == Op_Reg_Disp4)
		{
			mov_op_code = 0x8a + (op_size > 8);

			if (op_size > 32) w = 1;
			if (op1.reg >= R8) { op1.reg -= R8; r = 1; }
			if (op2.reg >= R8) { op2.reg -= R8; b = 1; }

			reg_opcode = op1.reg;
			rm = op2.reg;
			mod = Mod_Disp4;

			if (op2.reg == RSP) { rsp_sib = true; rm = 0b100; }

			if (op2.type == Op_Reg_Disp4) {
				displace = 1;
				disp = op2.reg_disp.disp;
			}
		}

		if (op_size == 16) Write_8(bytes, 0x66);

		if (w || r || b) {
			Emit_Rex(bytes, w, r, 0, b);
		}

		Write_8(bytes, mov_op_code);
		Emit_Mod_RM(bytes, mod, reg_opcode, rm);
		if (rsp_sib) Emit_SIB(bytes, 00, RSP, RSP);

		if (displace) Write_32(bytes, (u32)disp);

		if (immx == 8) Write_8(bytes, (u8)imm);
		if (immx == 16) Write_16(bytes, (u16)imm);
		if (immx == 32) Write_32(bytes, (u32)imm);
	}

	void Emit_Ret(Array<u8>& bytes)
	{
		Write_8(bytes, 0xc3);
	}

	u32 Emit_Call(Array<u8>& bytes, u32 displacement)
	{
		Write_8(bytes, 0xe8);
		Write_32(bytes, displacement);
		return u32(bytes.count - 4);
	}

	void Emit_Call(Array<u8>& bytes, Inst_Op op)
	{
		ASSERT(op.type == Op_Reg || op.type == Op_Reg_Disp4, "invalid operand");

		u8 mod = Mod_Reg;
		u8 reg_op_code = 2;
		u8 rm = op.reg;

		bool b = 0;

		if (op.reg >= R8) {
			b = 1;
			op.reg -= R8;
		}

		if (op.type == Op_Reg_Disp4) {
			mod = Mod_Disp4;
		}

		if (b)
			Emit_Rex(bytes, 0, 0, 0, 1);

		Write_8(bytes, 0xff);
		Emit_Mod_RM(bytes, mod, reg_op_code, rm);

		if (op.type == Op_Reg_Disp4) {
			Write_32(bytes, op.reg_disp.disp);
		}
	}

	void Emit_MovAbs(Array<u8>& bytes, u8 reg, i64 imm64)
	{
		Emit_Rex(bytes, true, false, false, false);
		Write_8(bytes, 0xb8 + reg);
		Write_64(bytes, *(u64*)&imm64);
	}

	u32 Emit_Jmp(Array<u8>& bytes, u32 displacement)
	{
		Write_8(bytes, 0xe9);
		u32 jmp_word_ptr = (u32)bytes.count;
		Write_32(bytes, displacement);
		return jmp_word_ptr;
	}

	u32 Emit_JE(Array<u8>& bytes, u32 displacement)
	{
		Write_8(bytes, 0x0f);
		Write_8(bytes, 0x84);
		u32 jmp_word_ptr = (u32)bytes.count;
		Write_32(bytes, displacement);
		return jmp_word_ptr;
	}

	u32 Emit_JNE(Array<u8>& bytes, u32 displacement)
	{
		Write_8(bytes, 0x0f);
		Write_8(bytes, 0x85);
		u32 jmp_word_ptr = (u32)bytes.count;
		Write_32(bytes, displacement);
		return jmp_word_ptr;
	}

	u32 Emit_Add(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		if (op1.type == Op_Imm8 || op1.type == Op_Imm16 || op1.type == Op_Imm32) ASSERT(nullptr, "invalid operand");
		if (op1.type == Op_Reg_Disp4 && op2.type == Op_Reg_Disp4) ASSERT(nullptr, "invalid operand");

		u8 add_mnemonic = 0;
		u8 reg_op_code = 0;
		u8 rm = 0;
		u8 mod = 0;

		bool w = 0;
		bool r = 0;
		bool x = 0;
		bool b = 0;
		bool disp = 0;
		u32 displacement = 0;

		int immx = 0;
		i32 imm = 0;

		if ((op1.type == Op_Reg || op1.type == Op_Reg_Disp4) && op2.type == Op_Reg) {

			add_mnemonic = 0x00 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { r = 1; op2.reg -= R8; }

			reg_op_code = op2.reg;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op1.type == Op_Reg && op2.type == Op_Reg_Disp4) {

			add_mnemonic = 0x02 + (op_size > 8);

			if (op1.reg >= R8) { r = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { b = 1; op2.reg -= R8; }

			reg_op_code = op1.reg;
			rm = op2.reg;
			mod = Mod_Disp4;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op2.reg_disp.disp;
		}

		if ((op1.type == Op_Reg_Disp4 || op1.type == Op_Reg) && (op2.type == Op_Imm8 || op2.type == Op_Imm16 || op2.type == Op_Imm32)) {
			add_mnemonic = 0x80 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }

			reg_op_code = 0b000;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (op2.type == Op_Imm8) immx = 8;
			if (op2.type == Op_Imm16) immx = 16;
			if (op2.type == Op_Imm32) immx = 32;

			if (immx == 8 && op_size != 8)add_mnemonic += 0x2;

			if (op_size != 64) ASSERT(immx <= op_size);

			imm = op2.imm32;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op_size > 32) {
			w = 1;
		}

		if (op_size == 16) Write_8(bytes, 0x66);

		if (w || r || x || b) {
			Emit_Rex(bytes, w, r, x, b);
		}

		Write_8(bytes, add_mnemonic);
		Emit_Mod_RM(bytes, mod, reg_op_code, rm);

		if (displacement) Write_32(bytes, displacement);

		u32 imm_byte_index = (u32)-1;

		if (immx > 0) {
			imm_byte_index = (u32)bytes.count;
		}

		if (immx == 8) Write_8(bytes, (i8)imm);
		if (immx == 16) Write_16(bytes, (i16)imm);
		if (immx == 32) Write_32(bytes, (i32)imm);

		return imm_byte_index;
	}

	u32 Emit_Sub(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		u8 sub_mnemonic = 0;
		u8 reg_op_code = 0;
		u8 rm = 0;
		u8 mod = 0;

		bool w = 0;
		bool r = 0;
		bool x = 0;
		bool b = 0;
		bool disp = 0;
		u32 displacement = 0;

		int immx = 0;
		i32 imm = 0;

		if ((op1.type == Op_Reg || op1.type == Op_Reg_Disp4) && op2.type == Op_Reg) {

			sub_mnemonic = 0x28 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { r = 1; op2.reg -= R8; }

			reg_op_code = op2.reg;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op1.type == Op_Reg && op2.type == Op_Reg_Disp4) {

			sub_mnemonic = 0x2A + (op_size > 8);

			if (op1.reg >= R8) { r = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { b = 1; op2.reg -= R8; }

			reg_op_code = op1.reg;
			rm = op2.reg;
			mod = Mod_Disp4;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op2.reg_disp.disp;
		}

		if ((op1.type == Op_Reg_Disp4 || op1.type == Op_Reg) && (op2.type == Op_Imm8 || op2.type == Op_Imm16 || op2.type == Op_Imm32)) {
			sub_mnemonic = 0x80 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }

			reg_op_code = 0b101;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (op2.type == Op_Imm8) immx = 8;
			if (op2.type == Op_Imm16) immx = 16;
			if (op2.type == Op_Imm32) immx = 32;

			if (immx == 8 && op_size != 8) sub_mnemonic += 0x2;

			if (op_size != 64) ASSERT(immx <= op_size);

			imm = op2.imm32;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op_size > 32) {
			w = 1;
		}

		if (op_size == 16) Write_8(bytes, 0x66);

		if (w || r || x || b) {
			Emit_Rex(bytes, w, r, x, b);
		}

		Write_8(bytes, sub_mnemonic);
		Emit_Mod_RM(bytes, mod, reg_op_code, rm);

		if (displacement) Write_32(bytes, displacement);

		u32 imm_byte_index = (u32)-1;

		if (immx > 0) {
			imm_byte_index = (u32)bytes.count;
		}

		if (immx == 8) Write_8(bytes, (i8)imm);
		if (immx == 16) Write_16(bytes, (i16)imm);
		if (immx == 32) Write_32(bytes, (i32)imm);

		return imm_byte_index;
	}

	void Emit_And(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		u8 and_mnemonic = 0;
		u8 reg_op_code = 0;
		u8 rm = 0;
		u8 mod = 0;

		bool w = 0;
		bool r = 0;
		bool x = 0;
		bool b = 0;
		bool disp = 0;
		u32 displacement = 0;

		int immx = 0;
		i32 imm = 0;

		if ((op1.type == Op_Reg || op1.type == Op_Reg_Disp4) && op2.type == Op_Reg) {

			and_mnemonic = 0x20 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { r = 1; op2.reg -= R8; }

			reg_op_code = op2.reg;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op1.type == Op_Reg && op2.type == Op_Reg_Disp4) {

			and_mnemonic = 0x22 + (op_size > 8);

			if (op1.reg >= R8) { r = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { b = 1; op2.reg -= R8; }

			reg_op_code = op1.reg;
			rm = op2.reg;
			mod = Mod_Disp4;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op2.reg_disp.disp;
		}

		if ((op1.type == Op_Reg_Disp4 || op1.type == Op_Reg) && (op2.type == Op_Imm8 || op2.type == Op_Imm16 || op2.type == Op_Imm32)) {
			and_mnemonic = 0x80 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }

			reg_op_code = 0b100;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (op2.type == Op_Imm8) immx = 8;
			if (op2.type == Op_Imm16) immx = 16;
			if (op2.type == Op_Imm32) immx = 32;

			if (immx == 8 && op_size != 8) and_mnemonic += 0x2;

			if (op_size != 64) ASSERT(immx <= op_size);

			imm = op2.imm32;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op_size > 32) {
			w = 1;
		}

		if (op_size == 16) Write_8(bytes, 0x66);

		if (w || r || x || b) {
			Emit_Rex(bytes, w, r, x, b);
		}

		Write_8(bytes, and_mnemonic);
		Emit_Mod_RM(bytes, mod, reg_op_code, rm);

		if (displacement) Write_32(bytes, displacement);

		if (immx == 8) Write_8(bytes, (i8)imm);
		if (immx == 16) Write_16(bytes, (i16)imm);
		if (immx == 32) Write_32(bytes, (i32)imm);

	}

	void Emit_Or(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		u8 or_mnemonic = 0;
		u8 reg_op_code = 0;
		u8 rm = 0;
		u8 mod = 0;

		bool w = 0;
		bool r = 0;
		bool x = 0;
		bool b = 0;
		bool disp = 0;
		u32 displacement = 0;

		int immx = 0;
		i32 imm = 0;

		if ((op1.type == Op_Reg || op1.type == Op_Reg_Disp4) && op2.type == Op_Reg) {

			or_mnemonic = 0x08 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { r = 1; op2.reg -= R8; }

			reg_op_code = op2.reg;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op1.type == Op_Reg && op2.type == Op_Reg_Disp4) {

			or_mnemonic = 0x0a + (op_size > 8);

			if (op1.reg >= R8) { r = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { b = 1; op2.reg -= R8; }

			reg_op_code = op1.reg;
			rm = op2.reg;
			mod = Mod_Disp4;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op2.reg_disp.disp;
		}

		if ((op1.type == Op_Reg_Disp4 || op1.type == Op_Reg) && (op2.type == Op_Imm8 || op2.type == Op_Imm16 || op2.type == Op_Imm32)) {
			or_mnemonic = 0x80 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }

			reg_op_code = 0b001;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (op2.type == Op_Imm8) immx = 8;
			if (op2.type == Op_Imm16) immx = 16;
			if (op2.type == Op_Imm32) immx = 32;

			if (immx == 8 && op_size != 8) or_mnemonic += 0x2;

			if (op_size != 64) ASSERT(immx <= op_size);

			imm = op2.imm32;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op_size > 32) {
			w = 1;
		}

		if (op_size == 16) Write_8(bytes, 0x66);

		if (w || r || x || b) {
			Emit_Rex(bytes, w, r, x, b);
		}

		Write_8(bytes, or_mnemonic);
		Emit_Mod_RM(bytes, mod, reg_op_code, rm);

		if (displacement) Write_32(bytes, displacement);

		if (immx == 8) Write_8(bytes, (i8)imm);
		if (immx == 16) Write_16(bytes, (i16)imm);
		if (immx == 32) Write_32(bytes, (i32)imm);
	}

	void Emit_IMul(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		ASSERT(op1.type == Op_Reg, "invalid operand");
		ASSERT(op2.type == Op_Reg || op2.type == Op_Reg_Disp4, "invalid operand");
		ASSERT(op_size != 8, "invalid size");

		u8 mod = 0;
		u8 reg = 0;
		u8 rm = 0;

		bool w = 0;
		bool r = 0;
		bool b = 0;

		if (op1.reg >= R8) { op1.reg -= R8; r = 1; };
		if (op2.reg >= R8) { op2.reg -= R8; b = 1; }

		mod = op2.type == Op_Reg ? Mod_Reg : Mod_Disp4;
		reg = op1.reg;
		rm = op2.reg;

		if (op_size == 16) Write_8(bytes, 0x66);
		if (op_size == 64) w = 1;

		if (w || r || b) Emit_Rex(bytes, w, r, 0, b);

		Write_8(bytes, 0x0f);
		Write_8(bytes, 0xaf);
		Emit_Mod_RM(bytes, mod, reg, rm);

		if (op2.type == Op_Reg_Disp4) Write_32(bytes, op2.reg_disp.disp);
	}

	void Emit_IMul3(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, Inst_Op op3, u8 op_size)
	{
		ASSERT(op1.type == Op_Reg, "invalid operand");
		ASSERT(op2.type == Op_Reg || op2.type == Op_Reg_Disp4, "invalid operand");
		ASSERT(op3.type == Op_Imm8 || op3.type == Op_Imm16 || op3.type == Op_Imm32, "invalid operand");
		ASSERT(op_size != 8, "invalid size");

		u8 mod = 0;
		u8 reg = 0;
		u8 rm = 0;

		bool w = 0;
		bool r = 0;
		bool b = 0;

		if (op1.reg >= R8) { op1.reg -= R8; r = 1; };
		if (op2.reg >= R8) { op2.reg -= R8; b = 1; }

		mod = op2.type == Op_Reg ? Mod_Reg : Mod_Disp4;
		reg = op1.reg;
		rm = op2.reg;

		if (op_size == 16) Write_8(bytes, 0x66);
		if (op_size == 64) w = 1;

		if (w || r || b) Emit_Rex(bytes, w, r, 0, b);

		Write_8(bytes, 0x69);
		Emit_Mod_RM(bytes, mod, reg, rm);

		if (op2.type == Op_Reg_Disp4) Write_32(bytes, op2.reg_disp.disp);

		if (op3.type == Op_Imm16) Write_16(bytes, op3.imm16);
		if (op3.type == Op_Imm32) Write_32(bytes, op3.imm32);
	}

	void Emit_IDiv(Array<u8>& bytes, Inst_Op op1, u8 op_size)
	{
		ASSERT(op1.type != Op_Imm8 && op1.type != Op_Imm16 && op1.type != Op_Imm32, "invalid operand");

		u8 mod = 0;
		u8 reg = 0;
		u8 rm = 0;

		bool w = 0;
		bool b = 0;

		u8 op_code = 0xf6 + (op_size > 8);

		if (op1.reg >= R8) { op1.reg -= R8; b = 1; };

		mod = op1.type == Op_Reg ? Mod_Reg : Mod_Disp4;
		reg = 0b111;
		rm = op1.reg;

		if (op_size == 16) Write_8(bytes, 0x66);
		if (op_size == 64) w = 1;

		if (w || b) Emit_Rex(bytes, w, 0, 0, b);

		Write_8(bytes, op_code);
		Emit_Mod_RM(bytes, mod, reg, rm);

		if (op1.type == Op_Reg_Disp4) Write_32(bytes, op1.reg_disp.disp);
	}

	void Emit_Div(Array<u8>& bytes, Inst_Op op1, u8 op_size)
	{
		ASSERT(op1.type != Op_Imm8 && op1.type != Op_Imm16 && op1.type != Op_Imm32, "invalid operand");

		u8 mod = 0;
		u8 reg = 0;
		u8 rm = 0;

		bool w = 0;
		bool b = 0;

		u8 op_code = 0xf6 + (op_size > 8);

		if (op1.reg >= R8) { op1.reg -= R8; b = 1; };

		mod = op1.type == Op_Reg ? Mod_Reg : Mod_Disp4;
		reg = 0b110;
		rm = op1.reg;

		if (op_size == 16) Write_8(bytes, 0x66);
		if (op_size == 64) w = 1;

		if (w || b) Emit_Rex(bytes, w, 0, 0, b);

		Write_8(bytes, op_code);
		Emit_Mod_RM(bytes, mod, reg, rm);

		if (op1.type == Op_Reg_Disp4) Write_32(bytes, op1.reg_disp.disp);
	}

	void Emit_CWD(Array<u8>& bytes)
	{
		Write_8(bytes, 0x66);
		Write_8(bytes, 0x99);
	}

	void Emit_CDQ(Array<u8>& bytes)
	{
		Write_8(bytes, 0x99);
	}

	void Emit_CQO(Array<u8>& bytes)
	{
		Emit_Rex(bytes, 1, 0, 0, 0);
		Write_8(bytes, 0x99);
	}

	void Emit_Cmp(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		if (op1.type == Op_Imm8 || op1.type == Op_Imm16 || op1.type == Op_Imm32) ASSERT(nullptr, "invalid operand");
		if (op1.type == Op_Reg_Disp4 && op2.type == Op_Reg_Disp4) ASSERT(nullptr, "invalid operand");

		u8 cmp_mnemonic = 0;
		u8 reg_op_code = 0;
		u8 rm = 0;
		u8 mod = 0;

		bool w = 0;
		bool r = 0;
		bool x = 0;
		bool b = 0;
		bool disp = 0;
		u32 displacement = 0;

		int immx = 0;
		i32 imm = 0;

		if ((op1.type == Op_Reg || op1.type == Op_Reg_Disp4) && op2.type == Op_Reg) {

			cmp_mnemonic = 0x38 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { r = 1; op2.reg -= R8; }

			reg_op_code = op2.reg;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op1.type == Op_Reg && op2.type == Op_Reg_Disp4) {

			cmp_mnemonic = 0x3a + (op_size > 8);

			if (op1.reg >= R8) { r = 1; op1.reg -= R8; }
			if (op2.reg >= R8) { b = 1; op2.reg -= R8; }

			reg_op_code = op1.reg;
			rm = op2.reg;
			mod = Mod_Disp4;
			disp = mod == Op_Reg_Disp4;

			if (disp) displacement = op2.reg_disp.disp;
		}

		if ((op1.type == Op_Reg_Disp4 || op1.type == Op_Reg) && (op2.type == Op_Imm8 || op2.type == Op_Imm16 || op2.type == Op_Imm32)) {

			cmp_mnemonic = 0x80 + (op_size > 8);

			if (op1.reg >= R8) { b = 1; op1.reg -= R8; }

			reg_op_code = 0b111;
			rm = op1.reg;
			mod = op1.type == Op_Reg_Disp4 ? Mod_Disp4 : Mod_Reg;
			disp = mod == Op_Reg_Disp4;

			if (op2.type == Op_Imm8) immx = 8;
			if (op2.type == Op_Imm16) immx = 16;
			if (op2.type == Op_Imm32) immx = 32;

			if (immx == 8 && op_size != 8) cmp_mnemonic += 0x2;

			if (op_size != 64) ASSERT(immx <= op_size);

			imm = op2.imm32;

			if (disp) displacement = op1.reg_disp.disp;
		}

		if (op_size > 32) {
			w = 1;
		}

		if (op_size == 16) Write_8(bytes, 0x66);

		if (w || r || x || b) {
			Emit_Rex(bytes, w, r, x, b);
		}

		Write_8(bytes, cmp_mnemonic);
		Emit_Mod_RM(bytes, mod, reg_op_code, rm);

		if (displacement) Write_32(bytes, displacement);

		if (immx == 8) Write_8(bytes, (i8)imm);
		if (immx == 16) Write_16(bytes, (i16)imm);
		if (immx == 32) Write_32(bytes, (i32)imm);
	}

	void Emit_SetG(Array<u8>& bytes, Inst_Op op1)
	{
		Write_SetX(bytes, 0x9f, op1);
	}

	void Emit_SetL(Array<u8>& bytes, Inst_Op op1)
	{
		Write_SetX(bytes, 0x9c, op1);
	}

	void Emit_SetE(Array<u8>& bytes, Inst_Op op1)
	{
		Write_SetX(bytes, 0x94, op1);
	}

	void Emit_SetNE(Array<u8>& bytes, Inst_Op op1)
	{
		Write_SetX(bytes, 0x95, op1);
	}

	u32 Emit_Lea(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
		if (op_size == 8) ASSERT(nullptr, "invalid operand");
		if (op1.type == Op_Imm8 || op1.type == Op_Imm16 || op1.type == Op_Imm32) ASSERT(nullptr, "invalid operand");
		if (op2.type == Op_Imm8 || op2.type == Op_Imm16 || op2.type == Op_Imm32) ASSERT(nullptr, "invalid operand");
		if (op1.type == Op_Reg_Disp4) ASSERT(nullptr, "invalid operand");
		if (op2.type == Op_Reg) ASSERT(nullptr, "invalid operand");

		bool w = 0;
		bool r = 0;
		bool b = 0;

		if (op_size == 64) w = 1;
		if (op_size == 16) Write_8(bytes, 0x66);

		if (op2.type == Op_Reg_Disp4 && op2.reg_disp.r >= R8) {
			op2.reg_disp.r -= R8;
			b = true;
		}

		if (op1.reg >= R8) {
			op1.reg -= R8;
			r = true;
		}

		u8 mod = 0xff;
		u8 reg = op1.reg;
		u8 rm = 0;

		if (w || b || r) {
			Emit_Rex(bytes, w, r, 0, b);
		}

		bool disp = 0;
		u32 displacement = 0xffffffff;

		if (op2.type == Op_Disp4) {
			mod = Mod_Ind;
			rm = 0b101;
			disp = 1;
			displacement = (i32)op2.disp_4;
		}

		if (op2.type == Op_Reg_Disp4) {
			mod = Mod_Disp4;
			rm = op2.reg_disp.r;
			disp = 1;
			displacement = op2.reg_disp.disp;
		}

		Write_8(bytes, 0x8d);

		Emit_Mod_RM(bytes, mod, reg, rm);

		u32 displacement_byte_idx = 0xffffffff;

		if (disp)
		{
			displacement_byte_idx = (u32)bytes.count;
			Write_32(bytes, displacement);
		}

		return displacement_byte_idx;
	}

	void Write_SetX(Array<u8>& bytes, u8 second_op_code, Inst_Op op1)
	{
		if (op1.reg >= R8) { op1.reg -= R8; Write_Rex(bytes, 0, 0, 0, 0); }

		Write_8(bytes, 0x0f);
		Write_8(bytes, second_op_code);

		u8 mod = op1.type == Op_Reg ? Mod_Reg : Mod_Disp4;
		u8 reg = 0b000;
		u8 rm = op1.reg;

		Write_Mod_RM(bytes, mod, reg, rm);

		if (op1.type == Op_Reg_Disp4) {
			Write_32(bytes, op1.reg_disp.disp);
		}
	}

	u32 Emit_SSE(Array<u8>& bytes, Instruction_SSE instruction, Inst_Op op1, Inst_Op op2)
	{
		u8 direction = instruction.has_direction ? op1.type != Op_Reg : 0;

		if (instruction.has_direction) {
			if (op1.type != Op_Reg) {
				auto tmp_op = op1;
				op1 = op2;
				op2 = tmp_op;
			}
		}

		u8 mod = op2.type == Op_Reg ? Mod_Reg : Mod_Disp4;
		u8 reg = op1.reg;
		u8 rm = op2.reg;

		bool r = 0;
		bool b = 0;

		if (op2.type == Op_Disp4) {
			mod = Mod_Ind;
			rm = 0b101;
		}

		bool rsp_sib = false;

		if (rm == RSP) {
			rsp_sib = true;
			rm = 0b100;
		}

		if (reg >= XMM0)
			reg -= XMM0;

		if (rm >= XMM0)
			rm -= XMM0;

		if (reg >= R8 && reg < XMM0) {
			reg -= R8;
			r = true;
		}

		if (rm >= R8 && rm < XMM0) {
			rm -= R8;
			b = true;
		}

		if (instruction.op_code == 0x6e || instruction.op_code == 0x7e) {
			Write_8(bytes, 0x66);
		}
		else {
			Write_8(bytes, instruction.is_double ? 0xF2 : 0xF3);
		}

		if (instruction.rex_w || r || b) {
			Emit_Rex(bytes, instruction.rex_w, r, 0, b);
		}

		Write_8(bytes, 0x0f);
		Write_8(bytes, instruction.op_code + direction);
		Write_Mod_RM(bytes, mod, reg, rm);

		if (rsp_sib) Emit_SIB(bytes, 0, RSP, RSP);

		u32 disp_byte_offset = -1;

		if (op2.type == Op_Reg_Disp4) {
			disp_byte_offset = (u32)bytes.count;
			Write_32(bytes, op2.reg_disp.disp);
		}

		if (op2.type == Op_Disp4) {
			disp_byte_offset = (u32)bytes.count;
			Write_32(bytes, op2.disp_4);
		}

		return disp_byte_offset;
	}

	void Emit_MovQ(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x7e;
		inst.is_double = false;
		inst.rex_w = true;

		auto tmp = op1;
		op1 = op2;
		op2 = tmp;

		Emit_SSE(bytes, inst, op1, op2);
	}

	u32 Emit_MovSS(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = true;
		inst.op_code = 0x10;
		inst.is_double = false;

		return Emit_SSE(bytes, inst, op1, op2);
	}

	u32 Emit_MovSD(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = true;
		inst.op_code = 0x10;
		inst.is_double = true;

		return Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_MulSS(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x59;
		inst.is_double = false;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_MulSD(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x59;
		inst.is_double = true;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_AddSS(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x58;
		inst.is_double = false;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_AddSD(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x58;
		inst.is_double = true;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_SubSS(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x5c;
		inst.is_double = false;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_SubSD(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x5c;
		inst.is_double = true;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_DivSS(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x5e;
		inst.is_double = false;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_DivSD(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x5e;
		inst.is_double = true;

		Emit_SSE(bytes, inst, op1, op2);
	}

	void Emit_CVTSS2SD(Array<u8>& bytes, Inst_Op op1, Inst_Op op2)
	{
		Instruction_SSE inst;
		inst.has_direction = false;
		inst.op_code = 0x5a;
		inst.is_double = false;

		Emit_SSE(bytes, inst, op1, op2);
	}
}
