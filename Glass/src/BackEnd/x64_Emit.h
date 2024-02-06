#pragma once

#include "Base/Types.h"
#include "Base/Array.h"

namespace Glass
{
	const u8 RAX = 0b0000;  // 0
	const u8 RBX = 0b0011;  // 3
	const u8 RCX = 0b0001;  // 1
	const u8 RDX = 0b0010;  // 2
	const u8 RSI = 0b0110;  // 6
	const u8 RDI = 0b0111;  // 7
	const u8 RBP = 0b0101;  // 5
	const u8 RSP = 0b0100;	// 4
	const u8 R8 = 0b1000;	// 8
	const u8 R9 = 0b1001;	// 9
	const u8 R10 = 0b1010;	// 10
	const u8 R11 = 0b1011;	// 11
	const u8 R12 = 0b1100;	// 12
	const u8 R13 = 0b1101;	// 13
	const u8 R14 = 0b1110;	// 14
	const u8 R15 = 0b1111;	// 15

	const u8 mod_rm_b = 0b00011000;

	const u8 Mod_Ind = 0b00000000;
	const u8 Mod_Disp1 = 0b00000001;
	const u8 Mod_Disp4 = 0b00000010;
	const u8 Mod_Reg = 0b00000011;

	struct Inst_Op_Reg_Disp
	{
		u8 r;
		i32 disp;
	};

	enum Inst_Op_Type
	{
		Op_Reg,
		Op_Reg_Disp1,
		Op_Reg_Disp4,
		Op_Disp4,

		Op_Imm8,
		Op_Imm16,
		Op_Imm32,
	};

	struct Inst_Op
	{
		Inst_Op_Type type;

		union
		{
			u8 reg;
			i8 imm8;
			i16 imm16;
			i32 imm32;
			u32 disp_4;

			Inst_Op_Reg_Disp reg_disp;
		};
	};

	inline void Write_8(Array<u8>& code, u8 v) {
		Array_Add(code, v);
	}

	inline void Write_16(Array<u8>& code, u16 v) {
		u8* as_byte = (u8*)&v;
		Array_Add(code, as_byte[0]);
		Array_Add(code, as_byte[1]);
	}

	inline void Write_32(Array<u8>& code, u32 v) {

		u8* as_byte = (u8*)&v;

		Array_Add(code, as_byte[0]);
		Array_Add(code, as_byte[1]);
		Array_Add(code, as_byte[2]);
		Array_Add(code, as_byte[3]);
	}

	inline u32* Write_32_Ptr(Array<u8>& code) {

		u8* ptr = code.data + code.count;

		Array_Add(code, (u8)0);
		Array_Add(code, (u8)0);
		Array_Add(code, (u8)0);
		Array_Add(code, (u8)0);

		return (u32*)ptr;
	}

	inline u8* Write_Bytes(Array<u8>& code, u8* bytes, u64 count) {

		u8* ptr = code.data + code.count;

		for (size_t i = 0; i < count; i++)
			Array_Add(code, bytes[i]);

		return ptr;
	}

	inline void Write_Nop(Array<u8>& bytes) {
		Write_8(bytes, 0x90);
	}

	inline void Emit_Rex(Array<u8>& bytes, bool w, bool r, bool x, bool b) {

		u8 rex = 0b01000000;
		// 	           wrxb
		if (w)
			rex = rex | ((u8)1 << 3);
		if (r)
			rex = rex | ((u8)1 << 2);
		if (x)
			rex = rex | ((u8)1 << 1);
		if (b)
			rex = rex | ((u8)1 << 0);

		Write_8(bytes, rex);
	}

	inline void Write_Rex(Array<u8>& bytes, bool w, bool r, bool x, bool b) {

		u8 rex = 0b01000000;
		// 	           wrxb
		if (w)
			rex = rex | ((u8)1 << 3);
		if (r)
			rex = rex | ((u8)1 << 2);
		if (x)
			rex = rex | ((u8)1 << 1);
		if (b)
			rex = rex | ((u8)1 << 0);

		Write_8(bytes, rex);
	}
	inline void Emit_Mod_RM(Array<u8>& bytes, u8 mod, u8 reg_op_code, u8 rm) {

		mod = mod << (7 - 1);
		reg_op_code = reg_op_code << (7 - 4);

		u8 mod_rm = 0;
		mod_rm |= mod;
		mod_rm |= reg_op_code;
		mod_rm |= rm;

		Write_8(bytes, mod_rm);
	}
	inline void Write_Mod_RM(Array<u8>& bytes, u8 mod, u8 reg_op_code, u8 rm) {

		mod = mod << (7 - 1);
		reg_op_code = reg_op_code << (7 - 4);

		u8 mod_rm = 0;
		mod_rm |= mod;
		mod_rm |= reg_op_code;
		mod_rm |= rm;

		Write_8(bytes, mod_rm);
	}


	inline void Emit_SIB(Array<u8>& bytes, u8 ss, u8 index, u8 base) {

		ss = ss << (7 - 1);
		index = index << (7 - 4);

		u8 sib = 0;
		sib |= ss;
		sib |= index;
		sib |= base;

		Write_8(bytes, sib);
	}

	void Emit_Push(Array<u8>& bytes, Inst_Op op);
	void Emit_Pop(Array<u8>& bytes, Inst_Op op);

	void Emit_Mov(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);
	void Emit_Ret(Array<u8>& bytes);
	u32 Emit_Call(Array<u8>& bytes, u32 displacement);
	void Emit_Call(Array<u8>& bytes, Inst_Op op);

	u32 Emit_Jmp(Array<u8>& bytes, u32 displacement);

	u32 Emit_JE(Array<u8>& bytes, u32 displacement);
	u32 Emit_JNE(Array<u8>& bytes, u32 displacement);

	u32 Emit_Add(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);
	u32 Emit_Sub(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);
	void Emit_And(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);
	void Emit_Or(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);

	void Emit_IMul(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);
	void Emit_IMul3(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, Inst_Op op3, u8 op_size);
	void Emit_IDiv(Array<u8>& bytes, Inst_Op op1, u8 op_size);

	void Emit_CWD(Array<u8>& bytes);
	void Emit_CDQ(Array<u8>& bytes);
	void Emit_CQO(Array<u8>& bytes);

	void Emit_Cmp(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);

	void Emit_SetG(Array<u8>& bytes, Inst_Op op1);
	void Emit_SetL(Array<u8>& bytes, Inst_Op op1);
	void Emit_SetE(Array<u8>& bytes, Inst_Op op1);
	void Emit_SetNE(Array<u8>& bytes, Inst_Op op1);

	u32 Emit_Lea(Array<u8>& bytes, Inst_Op op1, Inst_Op op2, u8 op_size);

	void Write_SetX(Array<u8>& bytes, u8 second_op_code, Inst_Op op1);
}