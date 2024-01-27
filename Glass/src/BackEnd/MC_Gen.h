#pragma once

#include "BackEnd/Il.h"
#include "BackEnd/COFF.h"

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
		i32 disp;
		u8 r;
	};

	enum Inst_Op_Type
	{
		Op_Reg,
		Op_Reg_Disp1,
		Op_Reg_Disp4,

		Op_Imm8,
		Op_Imm16,
		Op_Imm32,
	};

	struct Inst_Op
	{
		union
		{
			u8 reg;
			i8 imm8;
			i16 imm16;
			i32 imm32;

			Inst_Op_Reg_Disp reg_disp;
		};

		Inst_Op_Type type;
	};

	struct MC_Gen_Spec
	{
		String output_path;
	};

	struct MC_Gen
	{
		String output_path;
		Il_Program* prog;
		Type_System* ts;

		Array<u8> code;
		Array<u8> coff_obj;
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

	inline void Write_Nop(MC_Gen& g) {
		Write_8(g.code, 0x90);
	}

	inline void MC_Gen_Output_Raw(MC_Gen& g) {

		fs_path raw_path = g.output_path.data;
		fs_path raw_directory_path = g.output_path.data;
		raw_directory_path.remove_filename();
		raw_path.replace_extension(".bin");

		std::filesystem::create_directories(raw_directory_path);

		{
			std::ofstream file(raw_path.string(), std::ios::binary);
			file.write((const char*)g.code.data, g.code.count);
		}
	}

	void Emit_Push(MC_Gen& g, Inst_Op op);
	void Emit_Add(MC_Gen& g, Inst_Op op1, Inst_Op op2, u8 op_size);
	void Emit_Sub(MC_Gen& g, Inst_Op op1, Inst_Op op2, u8 op_size);
	void Emit_Ret(MC_Gen& g);

	void MC_Gen_Output(MC_Gen& g);

	bool MC_Gen_Run(MC_Gen& g);

	MC_Gen MC_Gen_Make(MC_Gen_Spec spec, Il_Program* program);

	inline void Emit_Rex(MC_Gen& g, bool w, bool r, bool x, bool b) {

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

		Write_8(g.code, rex);
	}

	inline void Emit_Mod_RM(MC_Gen& g, u8 mod, u8 reg_op_code, u8 rm) {

		mod = mod << (7 - 1);
		reg_op_code = reg_op_code << (7 - 4);

		u8 mod_rm = 0;
		mod_rm |= mod;
		mod_rm |= reg_op_code;
		mod_rm |= rm;

		Write_8(g.code, mod_rm);
	}

	inline void Emit_Push(MC_Gen& g, Inst_Op op) {

		u8 push_op_code = 0;

		if (op.type == Op_Reg) {
			push_op_code = 0x50;
			push_op_code += op.reg;

			if (op.reg > RDI) {
				push_op_code -= R8;
				Emit_Rex(g, 0, 0, 0, 1);
			}

			Write_8(g.code, push_op_code);
		}

		if (op.type == Op_Imm8) {
			push_op_code = 0x6a;
			Write_8(g.code, push_op_code);
			Write_8(g.code, op.imm8);
		}

		if (op.type == Op_Imm16 || op.type == Op_Imm32) {
			push_op_code = 0x68;
			Write_8(g.code, push_op_code);
			Write_32(g.code, op.imm32);
		}

		if (op.type == Op_Reg_Disp4 || op.type == Op_Reg_Disp1) {
			push_op_code = 0xff;

			if (op.reg > RDI) {
				Emit_Rex(g, 0, 0, 0, 1);
				op.reg_disp.r -= R8;
			}

			Write_8(g.code, push_op_code);
			Emit_Mod_RM(g, Mod_Disp4, 0b110, op.reg_disp.r);
			Write_32(g.code, op.reg_disp.disp);
		}
	}

	inline void Emit_Add(MC_Gen& g, Inst_Op op1, Inst_Op op2, u8 op_size)
	{
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

		if (op_size == 16) Write_8(g.code, 0x66);

		if (w || r || x || b) {
			Emit_Rex(g, w, r, x, b);
		}

		Write_8(g.code, add_mnemonic);
		Emit_Mod_RM(g, mod, reg_op_code, rm);

		if (displacement) Write_32(g.code, displacement);

		if (immx == 8) Write_8(g.code, (i8)imm);
		if (immx == 16) Write_16(g.code, (i8)imm);
		if (immx == 32) Write_32(g.code, (i8)imm);
	}
	inline void Emit_Sub(MC_Gen& g, Inst_Op op1, Inst_Op op2, u8 op_size)
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

		if (op_size == 16) Write_8(g.code, 0x66);

		if (w || r || x || b) {
			Emit_Rex(g, w, r, x, b);
		}

		Write_8(g.code, sub_mnemonic);
		Emit_Mod_RM(g, mod, reg_op_code, rm);

		if (displacement) Write_32(g.code, displacement);

		if (immx == 8) Write_8(g.code, (i8)imm);
		if (immx == 16) Write_16(g.code, (i8)imm);
		if (immx == 32) Write_32(g.code, (i8)imm);
	}

	inline void test() {
		/*
		for (size_t i = 0; i < 100000; i++)
		{
			Inst_Op add_op1;
			add_op1.type = Op_Reg;
			add_op1.reg = RAX;

			Inst_Op add_op2;
			add_op2.type = Op_Reg;
			add_op2.reg = RAX;

			Emit_Add(g, add_op1, add_op2, 32);
			Emit_Add(g, add_op1, add_op2, 64);

			Emit_Sub(g, add_op1, add_op2, 32);
			Emit_Sub(g, add_op1, add_op2, 64);

			add_op1.reg = R15;
			add_op2.reg = RAX;
			Emit_Add(g, add_op1, add_op2, 32);
			Emit_Add(g, add_op1, add_op2, 64);

			Emit_Sub(g, add_op1, add_op2, 32);
			Emit_Sub(g, add_op1, add_op2, 64);

			add_op1.reg = R15;
			add_op2.reg = R14;
			Emit_Add(g, add_op1, add_op2, 32);
			Emit_Add(g, add_op1, add_op2, 64);

			Emit_Sub(g, add_op1, add_op2, 32);
			Emit_Sub(g, add_op1, add_op2, 64);

			add_op1.reg = RCX;
			add_op2.reg = R14;
			Emit_Add(g, add_op1, add_op2, 16);
			Emit_Sub(g, add_op1, add_op2, 16);

			add_op1.type = Op_Reg_Disp4;
			add_op1.reg_disp.disp = 0x64;

			add_op1.reg = R14;
			add_op2.reg = R15;
			Emit_Add(g, add_op1, add_op2, 8);
			Emit_Add(g, add_op2, add_op1, 8);

			Emit_Sub(g, add_op1, add_op2, 8);
			Emit_Sub(g, add_op2, add_op1, 8);

			add_op2.type = Op_Imm8;
			add_op2.imm32 = 0x6969;
			add_op1.type = Op_Reg;
			add_op1.reg = R14;
			Emit_Add(g, add_op1, add_op2, 64);
			Emit_Sub(g, add_op1, add_op2, 64);
		}

		// 		Emit_Add(g, add_op1, add_op2, 64);
		// 		Emit_Add(g, add_op1, add_op2, 16);
		// 		Emit_Add(g, add_op1, add_op2, 8);
		*/
		/*

						push_op.reg = RAX;
						Emit_Push(g, push_op);
						push_op.reg = RBX;
						Emit_Push(g, push_op);
						push_op.reg = RCX;
						Emit_Push(g, push_op);
						push_op.reg = RDX;
						Emit_Push(g, push_op);
						push_op.reg = RSP;
						Emit_Push(g, push_op);
						push_op.reg = RBP;
						Emit_Push(g, push_op);
						push_op.reg = RSI;
						Emit_Push(g, push_op);
						push_op.reg = RDI;
						Emit_Push(g, push_op);
						push_op.reg = R8;
						Emit_Push(g, push_op);
						push_op.reg = R9;
						Emit_Push(g, push_op);
						push_op.reg = R10;
						Emit_Push(g, push_op);
						push_op.reg = R11;
						Emit_Push(g, push_op);
						push_op.reg = R12;
						Emit_Push(g, push_op);
						push_op.reg = R13;
						Emit_Push(g, push_op);
						push_op.reg = R14;
						Emit_Push(g, push_op);
						push_op.reg = R15;
						Emit_Push(g, push_op);

						push_op.type = Op_Imm8;
						push_op.imm8 = 0x6f;
						Emit_Push(g, push_op);

						push_op.type = Op_Imm16;
						push_op.imm16 = 0x6f6f;
						Emit_Push(g, push_op);

						push_op.type = Op_Imm32;
						push_op.imm32 = 0x6f6f6f6f;
						Emit_Push(g, push_op);

						push_op.type = Op_Reg_Disp4;
						push_op.reg_disp.r = RBP;
						push_op.reg_disp.disp = 4;
						Emit_Push(g, push_op);

						push_op.type = Op_Reg_Disp4;
						push_op.reg_disp.r = R15;
						push_op.reg_disp.disp = 500;
						Emit_Push(g, push_op);
		*/
	}

}