#include "pch.h"

#include "BackEnd/MC_Gen.h"

#include "BackEnd/x64_Emit.h"

namespace Glass
{

#define X64_FP_REG_COUNT 6
#define X64_REG_COUNT 10
#define X64_TOTAL_REG_COUNT 32
#define REG_BUFS_SZ 65553

	const u8 x64_fp_registers[X64_FP_REG_COUNT] = { XMM0, XMM1, XMM2, XMM3, XMM4, XMM5 };
	const u8 x64_registers[X64_REG_COUNT] = { RAX, RBX, RCX, RDX, R8 ,R9, RSI, RDI };

	struct Reg_Allocation
	{
		u8 allocated_register = 0xff;
		u8 temp_register = 0xff;
		u8 temp_register2 = 0xff;
		bool spilled = false;
	};

	inline void MC_Proc_Reg_Lifetimes(MC_Gen& g, Il_Proc& proc, Il_IDX proc_idx, Array<i8>& lifetimes)
	{
		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX i = block.instructions[j];
				Il_Node node = proc.instruction_storage[i];

				GS_Type* node_type = get_type_at(node.type_idx);

				switch (node.node_type)
				{
				case Il_Struct_Initializer:
				{
					ASSERT(node.si.member_count < SI_SMALL_COUNT);

					for (size_t i = 0; i < node.si.member_count; i++)
					{
						lifetimes[node.si.members_value_nodes[i]]++;
					}
				}
				break;
				case Il_Cast:
				{
					lifetimes[node.cast.castee_node_idx]++;
				}
				break;
				case Il_StructElementPtr:
				{
					lifetimes[node.element_ptr.ptr_node_idx]++;
				}
				break;
				case Il_ArrayElementPtr:
				{
					lifetimes[node.aep.ptr_node_idx]++;
					lifetimes[node.aep.index_node_idx]++;
				}
				break;
				case Il_Add:
				case Il_Sub:
				case Il_Mul:
				case Il_Div:
					lifetimes[node.math_op.left_node_idx]++;
					lifetimes[node.math_op.right_node_idx]++;
					break;
				case Il_Ret:
					if (node_type != get_ts().void_Ty)
						lifetimes[node.ret.value_node_idx]++;
					break;
				case Il_Store:
					lifetimes[node.store.ptr_node_idx]++;
					lifetimes[node.store.value_node_idx]++;
					break;
				case Il_Load:
					lifetimes[node.load.ptr_node_idx]++;
					break;
				case Il_Call:
					for (size_t i = 0; i < node.call.argument_count; i++)
						lifetimes[node.call.arguments[i]]++;
					break;
				case Il_Call_Ptr:
				{
					lifetimes[node.call.proc_idx]++;

					for (size_t i = 0; i < node.call.argument_count; i++)
						lifetimes[node.call.arguments[i]]++;
				}
				break;
				case Il_Cond_Branch:
					lifetimes[node.c_branch.condition_node_idx]++;
					break;
				case Il_Value_Cmp:
					lifetimes[node.cmp_op.left_node_idx]++;
					lifetimes[node.cmp_op.right_node_idx]++;
					break;
				case Il_Param:
				case Il_Alloca:
				case Il_Const:
				case Il_Branch:
				case Il_String:
				case Il_ZI:
				case Il_Global_Address:
				case Il_Proc_Address:
					break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}
		}
	}

	u8 call_conv_parameter_registers[4] = { RCX,RDX,R8,R9 };
	u8 call_conv_parameter_fp_registers[4] = { XMM0, XMM1, XMM2, XMM3 };

	static i64 align_to(i64 unaligned_val, i64 alignment) {
		i64 mask = alignment - 1;
		i64 aligned_val = unaligned_val + (-unaligned_val & mask);
		return aligned_val;
	}

	Inst_Op register_values_buffer[1] = {};
	u8 register_flags_buffer[1] = {};
	Reg_Allocation register_allocations_buffer[1] = {};

	inline Inst_Op Make_Reg(u8 reg) {
		Inst_Op reg_op;
		reg_op.type = Op_Reg;
		reg_op.reg = reg;
		return reg_op;
	}

	inline Inst_Op Make_Reg_Disp(u8 reg, i32 displacement) {
		Inst_Op reg_op;
		reg_op.type = Op_Reg_Disp4;
		reg_op.reg_disp.r = reg;
		reg_op.reg_disp.disp = displacement;
		return reg_op;
	}

	inline Inst_Op Make_Disp(i32 byte_offset) {
		Inst_Op rel_disp4;
		rel_disp4.type = Op_Disp4;
		rel_disp4.disp_4 = byte_offset;
		return rel_disp4;
	}

	inline Inst_Op Make_Imm32(i32 value) {
		Inst_Op imm_op = {};
		imm_op.type = Op_Imm32;
		imm_op.imm32 = value;
		return imm_op;
	}

	inline Inst_Op Make_Imm8(i8 value) {
		Inst_Op imm_op = {};
		imm_op.type = Op_Imm8;
		imm_op.imm8 = value;
		return imm_op;
	}

	enum Value_Desc_Type
	{
		ValD_Mem,
		ValD_Reg,
		ValD_Imm,
		ValD_Addr_Relative,
		ValD_Spill,
	};

	struct Value_Desc {
		Value_Desc_Type desc_type;
		u8 reg_idx = 0xff;
		i32 offset = 0xcccccccc;
		i32 spillage_location = 0xcccccccc;
		i64 imm = 0xcccccccc;
		GS_Type* type = nullptr;
		u64 type_size = -1;
		u64 type_flags = -1;
	};

	Array<Value_Desc> register_values;
	Array<i8> register_use_counts = {};

	inline void MC_Gen_Proc_Codegen(MC_Gen& g, Il_Proc& proc, Il_IDX proc_idx)
	{
		bool caller_saved_registers[X64_TOTAL_REG_COUNT] = {};

		caller_saved_registers[RCX] = true;
		caller_saved_registers[RDX] = true;
		caller_saved_registers[R8] = true;
		caller_saved_registers[R9] = true;

		caller_saved_registers[XMM0] = true;
		caller_saved_registers[XMM1] = true;
		caller_saved_registers[XMM2] = true;
		caller_saved_registers[XMM3] = true;
		caller_saved_registers[XMM4] = true;
		caller_saved_registers[XMM5] = true;

		register_use_counts.count = proc.instruction_storage.count;

		memset(register_use_counts.data, 0, proc.instruction_storage.count);

		MC_Proc_Reg_Lifetimes(g, proc, proc_idx, register_use_counts);

		MC_Symbol& sym = g.symbols[g.proc_to_symbol[proc_idx]];
		sym.value = (u32)g.code.count;

		Emit_Push(g.code, Make_Reg(RBP));
		Emit_Mov(g.code, Make_Reg(RBP), Make_Reg(RSP), 64);
		u32 stack_sub_byte_offset = Emit_Sub(g.code, Make_Reg(RSP), Make_Imm32(0), 64);

		i32 stack_sz = 0;
		i32 stack_top_section_size = 0;

		struct Register_State
		{
			bool allocated;
			Il_IDX vreg_idx;
		};

		Register_State register_state[X64_TOTAL_REG_COUNT] = {};

		auto spill_gpr = [&](u8 reg) -> i32 {

			if (register_state[reg].allocated) {
				stack_sz += 8;
				Emit_Mov(g.code, Make_Reg_Disp(RBP, -stack_sz), Make_Reg(reg), 64);
				auto& vd = register_values[register_state[reg].vreg_idx];
				vd.desc_type = ValD_Spill;
				vd.offset = -stack_sz;

				register_state[reg].allocated = false;
				register_state[reg].vreg_idx = 0xcccccccc;

				return -stack_sz;
			}

			register_state[reg].allocated = false;
			register_state[reg].vreg_idx = 0xcccccccc;

			return -1;
		};

		auto spill_fp = [&](u8 reg) -> i32 {

			if (register_state[reg].allocated) {

				stack_sz += 8;

				auto& vd = register_values[register_state[reg].vreg_idx];

				if (vd.type_size == 4)
					Emit_MovSS(g.code, Make_Reg_Disp(RBP, -stack_sz), Make_Reg(reg));
				else if (vd.type_size == 8)
					Emit_MovSD(g.code, Make_Reg_Disp(RBP, -stack_sz), Make_Reg(reg));
				else GS_ASSERT_UNIMPL();

				vd.desc_type = ValD_Spill;
				vd.offset = -stack_sz;

				register_state[reg].allocated = false;
				register_state[reg].vreg_idx = 0xcccccccc;

				return -stack_sz;
			}

			register_state[reg].allocated = false;
			register_state[reg].vreg_idx = 0xcccccccc;

			return -1;
		};

		auto allocate_gpr = [&](Il_IDX vreg) {

			for (size_t i = 0; i < X64_REG_COUNT; i++)
			{
				u8 physical_gpr = x64_registers[i];

				if (register_state[physical_gpr].allocated) {
					continue;
				}

				register_state[physical_gpr].allocated = true;
				register_state[physical_gpr].vreg_idx = vreg;

				return physical_gpr;
			}

			//out of registers
			__debugbreak();
		};

		auto allocate_fp = [&](Il_IDX vreg) {

			for (size_t i = 0; i < X64_FP_REG_COUNT; i++)
			{
				u8 physical_fp = x64_fp_registers[i];

				if (register_state[physical_fp].allocated) {
					continue;
				}

				register_state[physical_fp].allocated = true;
				register_state[physical_fp].vreg_idx = vreg;

				return physical_fp;
			}

			u8 spilled_register = XMM5;

			spill_fp(spilled_register);

			register_state[spilled_register].allocated = true;
			register_state[spilled_register].vreg_idx = vreg;

			return spilled_register;
		};

		auto as_input = [&](Il_IDX vreg, u8 dest, u8 type_size) -> Inst_Op {

			auto& vd = register_values[vreg];

			switch (vd.desc_type)
			{
			case ValD_Imm:
				Emit_Mov(g.code, Make_Reg(dest), Make_Imm32(vd.imm), 64);
				break;
			case ValD_Addr_Relative:
			{
				Emit_Lea(g.code, Make_Reg(dest), Make_Reg_Disp(vd.reg_idx, vd.offset), 64);
				break;
			}
			break;
			case ValD_Spill:
			{
				if (vd.type_flags & TN_Float_Type) {
					if (vd.type_size == 8)
						Emit_MovSD(g.code, Make_Reg(dest), Make_Reg_Disp(RBP, vd.offset));
					else if (vd.type_size == 4)
						Emit_MovSS(g.code, Make_Reg(dest), Make_Reg_Disp(RBP, vd.offset));
					else GS_ASSERT_UNIMPL();
				}
				else {
					Emit_Mov(g.code, Make_Reg(dest), Make_Reg_Disp(RBP, vd.offset), 64);
				}
				break;
			}
			break;
			case ValD_Reg:
			{

				if (vd.reg_idx == dest) {
					break;
				}

				if (vd.type_flags & TN_Float_Type) {
					if (vd.type_size == 8)
						Emit_MovSD(g.code, Make_Reg(dest), Make_Reg(vd.reg_idx));
					else if (vd.type_size == 4)
						Emit_MovSS(g.code, Make_Reg(dest), Make_Reg(vd.reg_idx));
					else GS_ASSERT_UNIMPL();
				}
				else {
					Emit_Mov(g.code, Make_Reg(dest), Make_Reg(vd.reg_idx), 64);
				}

				break;
			}
			default:
				break;
			}

			return Make_Reg(dest);
		};

		auto free_reg = [&](u8 reg) {
			register_state[reg].allocated = false;
			register_state[reg].vreg_idx = 0xcccccccc;
		};

		auto use = [&](Il_IDX vreg) {

			register_use_counts[vreg]--;

			if (register_use_counts[vreg] <= 0) {

				auto vd = register_values[vreg];

				switch (vd.desc_type)
				{
				case ValD_Addr_Relative:
				case ValD_Reg:
					free_reg(vd.reg_idx);
					break;
				case ValD_Imm:
					break;
				default:
					break;
				}
			}
		};

		struct Block_Relocation
		{
			u32 block_start;
			i32 locations[8];
			i32 starts[8];
			u32 count = 0;
		};

		Block_Relocation block_reloactions[512] = {};

		bool callee_saved[X64_REG_COUNT] = {};

		callee_saved[RBX] = true;
		callee_saved[RDI] = true;
		callee_saved[RSI] = true;

		struct Saved_Register
		{
			u8 reg_idx;
			i32 stack_offset;
		};

		Array<Saved_Register> saved_registers;

		for (size_t i = 0; i < X64_REG_COUNT; i++)
		{
			u8 reg = x64_registers[i];
			if (callee_saved[reg]) {
				stack_sz += 8;
				Emit_Mov(g.code, Make_Reg_Disp(RBP, -stack_sz), Make_Reg(reg), 64);
				Array_Add(saved_registers, { reg,-stack_sz });
			}
		}

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];
			block_reloactions[block_idx].block_start = g.code.count;

			for (size_t i = 0; i < X64_REG_COUNT; i++)
			{
				u8 reg = x64_registers[i];

				if (register_state[reg].allocated) {
					__debugbreak();
				}
			}

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX idx = block.instructions[j];
				Il_Node node = proc.instruction_storage[idx];
				GS_Type* type = get_type_at(node.type_idx);
				u64 type_size = get_type_size(type);
				auto type_flags = get_type_flags(type);

				register_values[idx].type = type;
				register_values[idx].type_size = type_size;
				register_values[idx].type_flags = type_flags;

				switch (node.node_type)
				{
				case Il_Alloca:
				{
					GS_Type* alloca_type = get_type_at(node.aloca.type_idx);
					u64 alloca_type_size = get_type_size(alloca_type);

					stack_sz += alloca_type_size;
					register_values[idx].desc_type = ValD_Addr_Relative;
					register_values[idx].reg_idx = RBP;
					register_values[idx].offset = -stack_sz;
				}
				break;
				case Il_Const:
				{
					if (type_flags & TN_Float_Type) {

						auto val_fp = allocate_fp(idx);

						MC_Relocation relocation;

						u64 data_offset = g.rdata.count;

						if (type_size == 4) {
							float as_f32 = (float)node.constant.as.f64;
							Write_32(g.rdata, *(u64*)&as_f32);
							relocation.reloaction_offset = Emit_MovSS(g.code, Make_Reg(val_fp), Make_Disp(data_offset));
						}
						else if (type_size == 8) {
							Write_64(g.rdata, node.constant.as.us8);
							relocation.reloaction_offset = Emit_MovSD(g.code, Make_Reg(val_fp), Make_Disp(data_offset));
						}

						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = val_fp;

						relocation.symbol_idx = g.rdata_symbol_index;
						relocation.relocation_type = IMAGE_REL_AMD64_REL32;

						Array_Add(g.code_relocations_sections, relocation);
					}
					else {
						register_values[idx].desc_type = ValD_Imm;
						register_values[idx].imm = node.constant.as.s8;
					}
				}
				break;
				case Il_String:
				{
					u32 string_rdata_byte_offset = (u32)g.rdata.count;

					for (size_t i = 0; i < node.string.str.count; i++)
					{
						Array_Add(g.rdata, (u8)node.string.str[i]);
					}

					Array_Add(g.rdata, (u8)0);

					auto addr_gpr = allocate_gpr(idx);

					MC_Relocation relocation;
					relocation.reloaction_offset = Emit_Lea(g.code, Make_Reg(addr_gpr), Make_Disp(string_rdata_byte_offset), 64);
					relocation.symbol_idx = g.rdata_symbol_index;
					relocation.relocation_type = IMAGE_REL_AMD64_REL32;

					Array_Add(g.code_relocations_sections, relocation);

					register_values[idx].desc_type = ValD_Reg;
					register_values[idx].reg_idx = addr_gpr;
				}
				break;
				case Il_Struct_Initializer:
				{
					GS_Struct& _struct = get_struct(type);

					stack_sz += type_size;

					auto tmp_reg = allocate_gpr(idx);

					for (size_t i = 0; i < node.si.member_count; i++)
					{
						u64 offset = _struct.offsets[i];

						u64 member_type_size = get_type_size(_struct.members[i]);

						if (member_type_size > 8)
							GS_ASSERT_UNIMPL();

						as_input(node.si.members_value_nodes[i], tmp_reg, type_size);
						Emit_Mov(g.code, Make_Reg_Disp(RBP, (-stack_sz) + offset), Make_Reg(tmp_reg), member_type_size * 8);
						use(node.si.members_value_nodes[i]);
					}

					register_values[idx].desc_type = ValD_Addr_Relative;
					register_values[idx].reg_idx = RBP;
					register_values[idx].offset = -stack_sz;

					free_reg(tmp_reg);
				}
				break;
				case Il_ZI:
				{
					u64 aligned_8_size = align_to(type_size, 8);
					stack_sz += aligned_8_size;

					register_values[idx].desc_type = ValD_Addr_Relative;
					register_values[idx].reg_idx = RBP;
					register_values[idx].offset = -stack_sz;

					u64 pointer = -stack_sz;
					for (size_t i = 0; i < aligned_8_size / 8; i++)
					{
						Emit_Mov(g.code, Make_Reg_Disp(RBP, pointer), Make_Imm32(0), 64);
						pointer += 8;
					}
				}
				break;
				case Il_StructElementPtr:
				{
					GS_Struct& _struct = get_struct(type);
					u64 offset = _struct.offsets[node.element_ptr.element_idx];

					auto addr_gpr = allocate_gpr(idx);
					as_input(node.element_ptr.ptr_node_idx, addr_gpr, type_size);

					register_values[idx].desc_type = ValD_Addr_Relative;
					register_values[idx].reg_idx = addr_gpr;
					register_values[idx].offset = offset;

					use(node.element_ptr.ptr_node_idx);
				}
				break;
				case Il_ArrayElementPtr:
				{
					auto addr_gpr = allocate_gpr(idx);
					auto index_gpr = allocate_gpr(idx);
					as_input(node.aep.ptr_node_idx, addr_gpr, type_size);
					as_input(node.aep.index_node_idx, index_gpr, type_size);

					Emit_IMul3(g.code, Make_Reg(index_gpr), Make_Reg(index_gpr), Make_Imm32(type_size), 64);
					Emit_Add(g.code, Make_Reg(addr_gpr), Make_Reg(index_gpr), 64);

					register_values[idx].desc_type = ValD_Reg;
					register_values[idx].reg_idx = addr_gpr;

					free_reg(index_gpr);

					use(node.aep.ptr_node_idx);
					use(node.aep.index_node_idx);
				}
				break;
				case Il_Load: {

					if (type_flags & TN_Float_Type) {

						use(node.load.ptr_node_idx);

						//auto addr_gpr = allocate_gpr(idx);
						auto val_fp = allocate_fp(idx);

						auto pointer_op = Make_Reg_Disp(register_values[node.load.ptr_node_idx].reg_idx, register_values[node.load.ptr_node_idx].offset);

						if (type_size == 8)
							Emit_MovSD(g.code, Make_Reg(val_fp), pointer_op);
						if (type_size == 4)
							Emit_MovSS(g.code, Make_Reg(val_fp), pointer_op);

						use(node.load.ptr_node_idx);

						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = val_fp;

						break;
					}

					auto val_gpr = allocate_gpr(idx);

					register_values[idx].desc_type = ValD_Reg;

					auto pointer_op = Make_Reg_Disp(register_values[node.load.ptr_node_idx].reg_idx, register_values[node.load.ptr_node_idx].offset);

					if (type_size > 8) {
						Emit_Lea(g.code, Make_Reg(val_gpr), pointer_op, 64);
						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = val_gpr;
					}
					else {
						Emit_Mov(g.code, Make_Reg(val_gpr), pointer_op, type_size * 8);
						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = val_gpr;
					}

					use(node.load.ptr_node_idx);
				}
							break;
				case Il_Store:
				{
					if (type_flags & TN_Float_Type) {

						use(node.store.value_node_idx);
						auto val_fp = allocate_fp(idx);

						as_input(node.store.value_node_idx, val_fp, type_size);

						if (type_size == 8)
							Emit_MovSD(g.code, Make_Reg_Disp(register_values[node.store.ptr_node_idx].reg_idx, register_values[node.store.ptr_node_idx].offset), Make_Reg(val_fp));
						if (type_size == 4)
							Emit_MovSS(g.code, Make_Reg_Disp(register_values[node.store.ptr_node_idx].reg_idx, register_values[node.store.ptr_node_idx].offset), Make_Reg(val_fp));

						use(node.store.ptr_node_idx);

						free_reg(val_fp);

						break;
					}

					if (type_size > 8) {

						auto addr_gpr = allocate_gpr(idx);
						auto val_gpr = allocate_gpr(idx);

						as_input(node.store.value_node_idx, val_gpr, type_size);
						as_input(node.store.ptr_node_idx, addr_gpr, type_size);

						u64 remainder = type_size;

						auto tmp_gpr = allocate_gpr(idx);

						u64 pointer = 0;

						while (remainder > 0) {

							u8 moved_size = 0;

							if (remainder >= 8) {
								moved_size = 8;
							}
							else if (remainder >= 4) {
								moved_size = 4;
							}
							else if (remainder >= 2) {
								moved_size = 2;
							}
							else {
								moved_size = 1;
							}

							remainder -= moved_size;

							Emit_Mov(g.code, Make_Reg(tmp_gpr), Make_Reg_Disp(val_gpr, pointer), moved_size * 8);
							Emit_Mov(g.code, Make_Reg_Disp(addr_gpr, pointer), Make_Reg(tmp_gpr), moved_size * 8);

							pointer += moved_size;
						}

						free_reg(tmp_gpr);

						free_reg(addr_gpr);
						free_reg(val_gpr);
						use(node.store.value_node_idx);
						use(node.store.ptr_node_idx);
					}
					else
					{
						auto val_gpr = allocate_gpr(idx);

						as_input(node.store.value_node_idx, val_gpr, type_size);

						Emit_Mov(g.code, Make_Reg_Disp(register_values[node.store.ptr_node_idx].reg_idx, register_values[node.store.ptr_node_idx].offset), Make_Reg(val_gpr), type_size * 8);

						free_reg(val_gpr);
						use(node.store.value_node_idx);
						use(node.store.ptr_node_idx);
					}
				}
				break;
				case Il_Ret: {
					if (type != get_ts().void_Ty)
					{
						if (type_flags & TN_Float_Type) {
							auto val_fp = XMM0;
							as_input(node.ret.value_node_idx, val_fp, type_size);
						}
						else {
							auto val_gpr = RAX;
							as_input(node.ret.value_node_idx, val_gpr, type_size);
						}
					}
				}
						   break;
				case Il_Param: {

					if (type_flags & TN_Float_Type) {

						if (node.param.index >= 4) {

							auto val_fp = allocate_fp(idx);

							if (type_size == 4)
								Emit_MovSS(g.code, Make_Reg(val_fp), Make_Reg_Disp(RBP, 32 + 8 + (node.param.index - 3) * 8));
							else if (type_size == 8)
								Emit_MovSD(g.code, Make_Reg(val_fp), Make_Reg_Disp(RBP, 32 + 8 + (node.param.index - 3) * 8));

							register_values[idx].desc_type = ValD_Reg;
							register_values[idx].reg_idx = val_fp;
						}
						else {
							u8 needed_regiser = call_conv_parameter_fp_registers[node.param.index];

							register_state[needed_regiser].allocated = true;
							register_state[needed_regiser].vreg_idx = idx;

							register_values[idx].desc_type = ValD_Reg;
							register_values[idx].reg_idx = needed_regiser;
						}
					}
					else {

						if (node.param.index >= 4) {

							auto val_gpr = allocate_gpr(idx);

							Emit_Mov(g.code, Make_Reg(val_gpr), Make_Reg_Disp(RBP, 32 + 8 + (node.param.index - 3) * 8), type_size * 8);

							register_values[idx].desc_type = ValD_Reg;
							register_values[idx].reg_idx = val_gpr;
						}
						else {
							u8 needed_regiser = call_conv_parameter_registers[node.param.index];

							register_state[needed_regiser].allocated = true;
							register_state[needed_regiser].vreg_idx = idx;

							register_values[idx].desc_type = ValD_Reg;
							register_values[idx].reg_idx = needed_regiser;
						}
					}
				}
							 break;
				case Il_Call:
				{
					auto arguments_ptr = node.call.arguments;

					auto signature = get_type_at(node.call.signature);

					if (node.call.argument_count > SMALL_ARG_COUNT) {
						arguments_ptr = node.call.arguments_ptr;
					}

					i32 stack_top_pointer = 0;

					u8 tmp_gpr = 0xff;
					//u8 tmp_fp = 0xff;

					bool is_variadic = false;

					if (node.node_type == Il_Call)
					{
						is_variadic = g.prog->procedures[node.call.proc_idx].variadic;
					}

					bool call_used_registers[X64_TOTAL_REG_COUNT]{ };

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Il_IDX argument_idx = arguments_ptr[i];

						GS_Type* argument_type = get_type_at(proc.instruction_storage[argument_idx].type_idx);
						auto argument_type_flags = get_type_flags(argument_type);
						auto argument_type_size = get_type_flags(argument_type);

						if (argument_type_flags & TN_Float_Type) {

							if (i >= 4)
							{
								//use(argument_idx);
								u8 tmp_fp = allocate_fp(idx);
								as_input(argument_idx, tmp_fp, 0);

								if (argument_type_size == 8) {
									Emit_MovSD(g.code, Make_Reg_Disp(RSP, 32 + stack_top_pointer), Make_Reg(tmp_fp));
								}
								else {
									Emit_MovSS(g.code, Make_Reg_Disp(RSP, 32 + stack_top_pointer), Make_Reg(tmp_fp));
								}

								free_reg(tmp_fp);

								stack_top_pointer += 8;
							}
							else
							{
								u8 needed_register = call_conv_parameter_fp_registers[i];
								//use(arguments_ptr[i]);

								spill_fp(needed_register);

								register_state[needed_register].allocated = true;
								call_used_registers[needed_register] = true;

								as_input(argument_idx, needed_register, 0);

								if (is_variadic) {

									u8 needed_register_gpr = call_conv_parameter_registers[i];
									spill_gpr(needed_register_gpr);
									register_state[needed_register_gpr].allocated = true;
									call_used_registers[needed_register_gpr] = true;

									Emit_MovQ(g.code, Make_Reg(needed_register_gpr), Make_Reg(needed_register));
								}
							}
						}
						else {

							if (i >= 4)
							{
								if (tmp_gpr == 0xff) {
									tmp_gpr = allocate_gpr(idx);
								}

								//use(argument_idx);
								as_input(argument_idx, tmp_gpr, 0);
								Emit_Mov(g.code, Make_Reg_Disp(RSP, 32 + stack_top_pointer), Make_Reg(tmp_gpr), type_size * 8);

								stack_top_pointer += 8;
							}
							else {
								u8 needed_register = call_conv_parameter_registers[i];
								//use(argument_idx);
								spill_gpr(needed_register);
								register_state[needed_register].allocated = true;
								call_used_registers[needed_register] = true;

								as_input(argument_idx, needed_register, 8);
							}
						}
					}

					if (tmp_gpr != 0xff) {
						free_reg(tmp_gpr);
					}

					// 					if (tmp_fp != 0xff) {
					// 						free_reg(tmp_fp);
					// 					}

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Il_IDX argument_idx = arguments_ptr[i];
						use(argument_idx);
					}

					for (size_t i = 0; i < X64_REG_COUNT; i++)
					{
						u8 reg = x64_registers[i];

						if (call_used_registers[reg])
						{
							register_state[reg].allocated = false;
						}
					}

					for (size_t i = 0; i < X64_FP_REG_COUNT; i++)
					{
						u8 reg = x64_fp_registers[i];

						if (call_used_registers[reg])
						{
							register_state[reg].allocated = false;
						}
					}

					for (size_t i = 0; i < X64_REG_COUNT; i++)
					{
						u8 reg = x64_registers[i];

						if (caller_saved_registers[reg]) {
							if (register_state[reg].allocated) {
								spill_gpr(reg);
							}
						}
					}

					for (size_t i = 0; i < X64_FP_REG_COUNT; i++)
					{
						u8 fp_reg = x64_fp_registers[i];

						if (caller_saved_registers[fp_reg]) {
							if (register_state[fp_reg].allocated) {
								spill_fp(fp_reg);
							}
						}
					}

					bool is_float = type_flags & TN_Float_Type;

					if (is_float) {
						spill_fp(XMM0);
					}
					else {
						spill_gpr(RAX);
					}

					if (signature->proc.return_type == get_ts().void_Ty) {

					}
					else {
						if (is_float)
						{
							register_state[XMM0].allocated = true;
							register_state[XMM0].vreg_idx = idx;
						}
						else {
							register_state[RAX].allocated = true;
							register_state[RAX].vreg_idx = idx;
						}
					}

					MC_Relocation relocation;
					relocation.reloaction_offset = Emit_Call(g.code, 0);
					relocation.symbol_idx = g.proc_to_symbol[node.call.proc_idx];
					relocation.relocation_type = IMAGE_REL_AMD64_REL32;

					Array_Add(g.code_relocations, relocation);

					if (signature->proc.return_type == get_ts().void_Ty || register_use_counts[idx] <= 0) {
						if (is_float)
						{
							free_reg(XMM0);
						}
						else {
							free_reg(RAX);
						}
					}
					else {
						register_values[idx].desc_type = ValD_Reg;

						if (is_float)
						{
							register_values[idx].reg_idx = XMM0;
						}
						else {
							register_values[idx].reg_idx = RAX;
						}
					}

					stack_top_section_size = std::max(stack_top_pointer, stack_top_section_size);
				}
				break;
				case Il_Add:
				case Il_Sub:
				case Il_Mul:
				{

					if (type_flags & TN_Float_Type)
					{

						auto lhs_fp = allocate_fp(idx);
						auto rhs_fp = allocate_fp(idx);

						as_input(node.math_op.left_node_idx, lhs_fp, type_size);
						as_input(node.math_op.right_node_idx, rhs_fp, type_size);

						use(node.math_op.left_node_idx);
						use(node.math_op.right_node_idx);

						if (node.node_type == Il_Sub)
						{
							if (type_size == 4)
								Emit_SubSS(g.code, Make_Reg(lhs_fp), Make_Reg(rhs_fp));
							if (type_size == 8)
								Emit_SubSD(g.code, Make_Reg(lhs_fp), Make_Reg(rhs_fp));
						}

						if (node.node_type == Il_Add)
						{
							if (type_size == 4)
								Emit_AddSS(g.code, Make_Reg(lhs_fp), Make_Reg(rhs_fp));
							if (type_size == 8)
								Emit_AddSD(g.code, Make_Reg(lhs_fp), Make_Reg(rhs_fp));
						}

						if (node.node_type == Il_Mul)
						{
							if (type_size == 4)
								Emit_MulSS(g.code, Make_Reg(lhs_fp), Make_Reg(rhs_fp));
							if (type_size == 8)
								Emit_MulSD(g.code, Make_Reg(lhs_fp), Make_Reg(rhs_fp));
						}

						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = lhs_fp;

						free_reg(rhs_fp);
					}
					else
					{
						auto lhs_gpr = allocate_gpr(idx);
						auto rhs_gpr = allocate_gpr(idx);
						as_input(node.math_op.left_node_idx, lhs_gpr, type_size);
						as_input(node.math_op.right_node_idx, rhs_gpr, type_size);

						if (node.node_type == Il_Sub)
							Emit_Sub(g.code, Make_Reg(lhs_gpr), Make_Reg(rhs_gpr), type_size * 8);

						if (node.node_type == Il_Add)
							Emit_Add(g.code, Make_Reg(lhs_gpr), Make_Reg(rhs_gpr), type_size * 8);

						if (node.node_type == Il_Mul)
							Emit_IMul(g.code, Make_Reg(lhs_gpr), Make_Reg(rhs_gpr), type_size * 8);

						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = lhs_gpr;

						free_reg(rhs_gpr);

						use(node.math_op.left_node_idx);
						use(node.math_op.right_node_idx);
					}
				}
				break;
				case Il_Value_Cmp:
				{
					auto lhs_gpr = allocate_gpr(idx);
					auto rhs_gpr = allocate_gpr(idx);
					as_input(node.cmp_op.left_node_idx, lhs_gpr, type_size);
					as_input(node.cmp_op.right_node_idx, rhs_gpr, type_size);

					Emit_Cmp(g.code, Make_Reg(lhs_gpr), Make_Reg(rhs_gpr), type_size * 8);

					switch (node.cmp_op.compare_type)
					{
					case Il_Cmp_Lesser:
						Emit_SetL(g.code, Make_Reg(lhs_gpr));
						break;
					case Il_Cmp_Greater:
						Emit_SetG(g.code, Make_Reg(lhs_gpr));
						break;
					case Il_Cmp_Equal:
						Emit_SetE(g.code, Make_Reg(lhs_gpr));
						break;
					case Il_Cmp_NotEqual:
						Emit_SetNE(g.code, Make_Reg(lhs_gpr));
						break;
					default:
						GS_ASSERT_UNIMPL();
						break;
					}

					register_values[idx].desc_type = ValD_Reg;
					register_values[idx].reg_idx = lhs_gpr;

					free_reg(rhs_gpr);

					use(node.cmp_op.left_node_idx);
					use(node.cmp_op.right_node_idx);
				}
				break;
				case Il_Cond_Branch:
				{
					auto tmp_gpr = allocate_gpr(idx);
					as_input(node.c_branch.condition_node_idx, tmp_gpr, type_size);

					Emit_Cmp(g.code, Make_Reg(tmp_gpr), Make_Imm8(0), 8);

					auto target_idx = node.c_branch.true_case_block_idx;

					block_reloactions[target_idx].locations[block_reloactions[target_idx].count] = Emit_JNE(g.code, 0);
					block_reloactions[target_idx].starts[block_reloactions[target_idx].count] = g.code.count;
					block_reloactions[target_idx].count++;

					target_idx = node.c_branch.false_case_block_idx;

					block_reloactions[target_idx].locations[block_reloactions[target_idx].count] = Emit_JE(g.code, 0);
					block_reloactions[target_idx].starts[block_reloactions[target_idx].count] = g.code.count;
					block_reloactions[target_idx].count++;

					free_reg(tmp_gpr);

					use(node.c_branch.condition_node_idx);
				}
				break;
				case Il_Branch:
				{
					u32 displacement_location = Emit_Jmp(g.code, 0);
					u32 instruction_byte_offset = g.code.count;

					auto target_idx = node.br.block_idx;

					block_reloactions[target_idx].locations[block_reloactions[target_idx].count] = displacement_location;
					block_reloactions[target_idx].starts[block_reloactions[target_idx].count] = instruction_byte_offset;
					block_reloactions[target_idx].count++;
				}
				break;
				case Il_Cast:
				{
					switch (node.cast.cast_type)
					{
					case Il_Cast_Ptr:
					{
						auto val_gpr = allocate_gpr(idx);
						as_input(node.cast.castee_node_idx, val_gpr, type_size);

						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = val_gpr;
					}
					break;
					case Il_Cast_FloatExt:
					{
						auto val_gpr = allocate_fp(idx);
						as_input(node.cast.castee_node_idx, val_gpr, type_size);

						Emit_CVTSS2SD(g.code, Make_Reg(val_gpr), Make_Reg(val_gpr));

						register_values[idx].desc_type = ValD_Reg;
						register_values[idx].reg_idx = val_gpr;
					}
					break;
					default:
						GS_ASSERT_UNIMPL();
						break;
					}

					use(node.cast.castee_node_idx);
				}
				break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}
		}

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Block_Relocation block_relocation = block_reloactions[block_idx];

			for (size_t i = 0; i < block_relocation.count; i++)
			{
				*(u32*)&g.code[block_relocation.locations[i]] = (u32)((i32)block_relocation.block_start - block_relocation.starts[i]);
			}
		}

		for (size_t i = 0; i < saved_registers.count; i++)
		{
			Emit_Mov(g.code, Make_Reg(saved_registers[i].reg_idx), Make_Reg_Disp(RBP, saved_registers[i].stack_offset), 64);
		}

		i32 final_stack_size = align_to(stack_sz + 32 + 8 + stack_top_section_size, 16);

		*(i32*)&g.code[stack_sub_byte_offset] = final_stack_size;

		Emit_Add(g.code, Make_Reg(RSP), Make_Imm32(final_stack_size), 64);
		Emit_Pop(g.code, Make_Reg(RBP));
		Emit_Ret(g.code);
	}

	std::map<Il_IDX, std::string> register_names;

#define Phys_Reg(x) 0xffff - x

	inline MC_Inst* Put_Inst(MC_Ctx& ctx, MC_Inst inst)
	{
		Array_Add(ctx.instructions, inst);
		Il_IDX inserted_index = ctx.instructions.count - 1;
		MC_Inst* inserted_instruction = &ctx.instructions[inserted_index];

		if (ctx.inst_first == -1)
		{
			ctx.inst_first = inserted_index;
			ctx.inst_head = inserted_index;
			inserted_instruction->prev = -1;
		}
		else
		{
			MC_Inst* prev_instruction = &ctx.instructions[ctx.inst_head];
			prev_instruction->next = inserted_index;
			inserted_instruction->prev = ctx.inst_head;
			ctx.inst_head = inserted_index;
		}

		inserted_instruction->next = -1;

		return inserted_instruction;
	}

	inline MC_Inst* Inst_RM(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX dest, Il_IDX base, i32 disp, i8 bit_size)
	{
		MC_Inst rm = {};
		rm.op_code = op_code;
		rm.flags = Inst_Flag_Disp;
		rm.operands[0] = dest;
		rm.operands[1] = base;
		rm.disp = disp;
		rm.in_count = 2;
		rm.out_count = 1;
		rm.output = dest;
		rm.bit_size = bit_size;
		return Put_Inst(ctx, rm);
	}

	inline MC_Inst* Inst_MR(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX src, Il_IDX base, i32 disp, i8 bit_size)
	{
		MC_Inst rm = {};
		rm.op_code = op_code;
		rm.flags = Inst_Flag_Disp;
		rm.operands[0] = src;
		rm.operands[1] = base;
		rm.disp = disp;
		rm.in_count = 2;
		rm.bit_size = bit_size;
		rm.direction = true;
		return Put_Inst(ctx, rm);
	}

	inline MC_Inst* Inst_MI(MC_Ctx& ctx, MC_Inst_Type op_code, i32 imm, Il_IDX base, i32 disp, i8 bit_size)
	{
		MC_Inst mi = {};
		mi.op_code = op_code;
		mi.flags = Inst_Flag_Imm | Inst_Flag_Disp;
		mi.imm = imm;
		mi.operands[1] = base;
		mi.disp = disp;
		mi.bit_size = bit_size;
		mi.in_count = 2;
		mi.out_count = 1;
		return Put_Inst(ctx, mi);
	}

	inline MC_Inst* Inst_RI(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX dest, i32 imm, i8 bit_size)
	{
		MC_Inst ri = {};
		ri.op_code = op_code;
		ri.flags = Inst_Flag_Imm;
		ri.imm = imm;
		ri.operands[0] = dest;
		ri.bit_size = bit_size;
		ri.in_count = 1;
		ri.out_count = 1;
		ri.output = dest;
		return Put_Inst(ctx, ri);
	}

	inline MC_Inst* Inst_RR(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX dest, Il_IDX src, i8 bit_size, bool has_dest = true)
	{
		MC_Inst rr = {};
		rr.op_code = op_code;
		rr.operands[0] = dest;
		rr.operands[1] = src;
		rr.bit_size = bit_size;
		rr.in_count = 2;
		rr.out_count = has_dest;
		rr.output = dest;
		return Put_Inst(ctx, rr);
	}

	inline MC_Inst* Inst_RRI(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX dest, Il_IDX src, i32 imm, i8 bit_size, bool has_dest = true)
	{
		MC_Inst rri = {};
		rri.op_code = op_code;
		rri.operands[0] = dest;
		rri.operands[1] = src;
		rri.flags = Inst_Flag_Imm;
		rri.bit_size = bit_size;
		rri.in_count = 2;
		rri.out_count = has_dest;
		rri.output = dest;
		rri.imm = imm;
		return Put_Inst(ctx, rri);
	}

	inline MC_Inst* Inst_R(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX dest, i8 bit_size, bool has_dest = true)
	{
		MC_Inst rr = {};
		rr.op_code = op_code;
		rr.operands[0] = dest;
		rr.bit_size = bit_size;
		rr.in_count = 1;
		rr.out_count = has_dest;
		rr.output = dest;
		return Put_Inst(ctx, rr);
	}

	inline MC_Inst* Inst_(MC_Ctx& ctx, MC_Inst_Type op_code)
	{
		MC_Inst rr = {};
		rr.op_code = op_code;
		return Put_Inst(ctx, rr);
	}

	inline MC_Inst* Inst_RrD(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX dest, i32 disp, i8 bit_size)
	{
		MC_Inst Rdr = {};
		Rdr.op_code = op_code;
		Rdr.flags = Inst_Flag_R_Data;
		Rdr.operands[0] = dest;
		Rdr.disp = disp;
		Rdr.bit_size = bit_size;
		Rdr.in_count = 1;
		Rdr.out_count = 1;
		Rdr.output = dest;
		return Put_Inst(ctx, Rdr);
	}

	inline MC_Inst* Inst_Label(MC_Ctx& ctx, Il_IDX label_id)
	{
		MC_Inst label = {};
		label.op_code = Inst_LABEL;
		label.imm = label_id;
		return Put_Inst(ctx, label);
	}

	inline MC_Inst* Inst_CJMP(MC_Ctx& ctx, MC_Inst_Type op_code, Il_IDX label_id)
	{
		MC_Inst cjmp = {};
		cjmp.op_code = op_code;
		cjmp.operands[0] = label_id;
		return Put_Inst(ctx, cjmp);
	}

	inline MC_Inst* Inst_jmp(MC_Ctx& ctx, Il_IDX label_id)
	{
		MC_Inst cjmp = {};
		cjmp.op_code = Inst_JMP;
		cjmp.operands[0] = label_id;
		return Put_Inst(ctx, cjmp);
	}

	i32 Stack_Alloc(MC_Ctx& ctx, i32 size, Il_IDX node_idx)
	{
		ctx.stack_usage += size;
		return ctx.stack_usage;
	}

#define DBG(x) x

	int interval_compare(const void* a, const void* b)
	{
		MC_VReg* in_a = *(MC_VReg**)a;
		MC_VReg* in_b = *(MC_VReg**)b;

		if (in_a->range_start == in_b->range_start) return 0;
		else if (in_a->range_start < in_b->range_start) return -1;
		else return 1;
	}

	void MC_Proc_Codegen(MC_Gen& g, MC_Ctx& ctx, Il_Proc& proc, Il_IDX proc_idx)
	{
		Array<MC_VReg>& virtual_registers = g.virtual_registers_buffer;
		virtual_registers.count = 64;

		static std::unordered_map<Il_IDX, Il_IDX> virtual_registers_lookup;
		virtual_registers_lookup.clear();

		auto make_vreg = [&](Il_IDX node_idx = -1)
		{
			Il_IDX new_reg_idx = virtual_registers.count;

			//if (new_reg_idx == 88)__debugbreak();

			Array_Add(virtual_registers, {});

			if (node_idx != -1)
			{
				virtual_registers_lookup[node_idx] = new_reg_idx;
			}

			return new_reg_idx;
		};

		auto get_vreg = [&](Il_IDX node_idx) -> MC_VReg*
		{
			Il_IDX vreg = virtual_registers_lookup.at(node_idx);
			return &virtual_registers[vreg];
		};

		auto get_vreg_idx = [&](Il_IDX node_idx) -> Il_IDX
		{
			Il_IDX vreg = virtual_registers_lookup.at(node_idx);
			return vreg;
		};

		auto can_imm_32 = [&](Il_Node& node, i32 bit_size)
		{
			if (node.node_type != Il_Const) {
				return false;
			}

			auto value = node.constant.as.us8;

			if (bit_size > 32) {
				bool sign = (value >> 31ull) & 1;
				uint64_t top = value >> 32ull;

				if (top != (sign ? 0xFFFFFFFF : 0)) {
					return false;
				}
			}

			return true;
		};

		auto input_reg = [&](Il_Node* n, Il_IDX node_idx, i8 bit_size, Il_IDX reg = -1)
		{
			if (n->node_type == Il_Alloca)
			{
				if (reg == -1)
				{
					reg = make_vreg();
				}

				Inst_RM(ctx, Inst_LEA, reg, RBP, -get_vreg(node_idx)->stack_slot, bit_size);
			}
			else
			{
				if (reg != -1)
				{
					auto vreg = get_vreg_idx(node_idx);
					Inst_RR(ctx, Inst_MOV, reg, vreg, bit_size);
					return vreg;
				}
				else
				{
					reg = get_vreg_idx(node_idx);
				}
			}

			return reg;
		};

		auto select_addr = [&](Il_Node* n, Il_IDX n_idx, i8 bit_size, Il_IDX dest, Il_IDX src = -1, int store_op = -1)
		{
			Il_IDX base;
			i32 disp = 0;

			if (n->node_type == Il_Alloca)
			{
				base = RBP;
				disp += -get_vreg(n_idx)->stack_slot;
			}
			else
			{
				base = get_vreg_idx(n_idx);
			}

			if (store_op >= 0)
			{
				return Inst_MR(ctx, (MC_Inst_Type)store_op, src, base, disp, bit_size);
			}
			else
			{
				return Inst_RM(ctx, Inst_LEA, dest, base, disp, bit_size);
			}
		};

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			Inst_Label(ctx, block_idx);

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX idx = block.instructions[j];
				Il_Node node = proc.instruction_storage[idx];
				GS_Type* type = &ctx.ts->type_storage[node.type_idx];
				u64 type_size = TypeSystem_Get_Type_Size(*ctx.ts, type);
				u8 bit_size = type_size * 8;
				auto type_flags = TypeSystem_Get_Type_Flags(*ctx.ts, type);
				bool is_float = type_flags & TN_Float_Type;
				bool is_signed = !(type_flags & TN_Unsigned_Type);

				switch (node.node_type)
				{
				case Il_Const:
				{
					if (is_float)
					{
						auto vreg = make_vreg(idx);
						virtual_registers[vreg].fp = true;

						u64 data_offset = g.rdata.count;

						if (type_size == 4) {
							float as_f32 = (float)node.constant.as.f64;
							Write_32(g.rdata, *(u64*)&as_f32);
						}
						else if (type_size == 8) {
							Write_64(g.rdata, node.constant.as.us8);
						}

						Inst_RrD(ctx, Inst_MOVF, vreg, data_offset, bit_size);
					}
					else if (!can_imm_32(node, bit_size))
					{
						auto vreg = make_vreg(idx);
						Inst_RI(ctx, Inst_MOVABS, vreg, 0, bit_size)->abs = node.constant.as.s8;
					}
				}
				break;
				case Il_String:
				{
					u32 string_rdata_byte_offset = (u32)g.rdata.count;

					for (size_t i = 0; i < node.string.str.count; i++)
					{
						Array_Add(g.rdata, (u8)node.string.str[i]);
					}

					Array_Add(g.rdata, (u8)0);

					auto vreg = make_vreg(idx);
					Inst_RrD(ctx, Inst_LEA, vreg, string_rdata_byte_offset, 64);
				}
				break;
				case Il_Call:
				{
					auto arguments_ptr = node.call.arguments;

					auto signature = get_type_at(node.call.signature);

					if (node.call.argument_count > SMALL_ARG_COUNT) {
						arguments_ptr = node.call.arguments_ptr;
					}

					MC_Inst call_inst = {};
					call_inst.in_count = 2;

					const u8 num_caller_saved_gpr = 7;
					const u8 num_caller_saved_fp = 6;
					const u8 caller_saved_registers_gpr[num_caller_saved_gpr] = { RAX,RCX,RDX,R8,R9,R10,R11 };
					const u8 caller_saved_registers_fp[num_caller_saved_fp] = { XMM0,XMM1,XMM2,XMM3,XMM4,XMM5 };

					bool saved_registers[XMM7] = {};

					bool variadic = false;

					variadic = g.prog->procedures[node.call.proc_idx].variadic;

					for (size_t i = 0; i < num_caller_saved_gpr; i++)
						saved_registers[caller_saved_registers_gpr[i]] = true;

					for (size_t i = 0; i < num_caller_saved_fp; i++)
						saved_registers[caller_saved_registers_fp[i]] = true;

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Il_IDX argument_idx = arguments_ptr[i];

						GS_Type* argument_type = get_type_at(proc.instruction_storage[argument_idx].type_idx);
						auto argument_type_flags = get_type_flags(argument_type);
						auto argument_type_size = get_type_size(argument_type);
						auto arg_bit_size = argument_type_size * 8;

						bool arg_is_float = argument_type_flags & TN_Float_Type;

						Il_Node& arg_node = proc.instruction_storage[argument_idx];

						auto argument_conv_reg = call_conv_parameter_registers[i];

						if (arg_is_float)
						{
							argument_conv_reg = call_conv_parameter_fp_registers[i];
						}

						auto argument_vreg = make_vreg();
						virtual_registers[argument_vreg].reg = argument_conv_reg;
						saved_registers[argument_conv_reg] = false;

						if (arg_is_float)
						{
							virtual_registers[argument_vreg].fp = true;
							get_vreg(argument_idx)->hint = argument_conv_reg;
							Inst_RR(ctx, Inst_MOVF, argument_vreg, get_vreg_idx(argument_idx), arg_bit_size);

							if (variadic)
							{
								auto argument_gpr = call_conv_parameter_registers[i];
								auto argument_vreg_gpr = make_vreg();
								virtual_registers[argument_vreg_gpr].reg = argument_gpr;
								saved_registers[argument_gpr] = false;

								Inst_RR(ctx, Inst_MovF2G, argument_vreg_gpr, argument_vreg, arg_bit_size);
							}
						}
						else
						{
							if (can_imm_32(arg_node, arg_bit_size))
							{
								Inst_RI(ctx, Inst_MOV, argument_vreg, arg_node.constant.as.s4, arg_bit_size);
							}
							else {
								input_reg(&arg_node, argument_idx, arg_bit_size, argument_vreg);
								get_vreg(argument_idx)->hint = argument_conv_reg;
							}
						}

						call_inst.operands[call_inst.in_count] = argument_vreg;
						call_inst.in_count++;
					}

					call_inst.op_code = Inst_CALL;
					call_inst.operands[1] = node.call.proc_idx;

					if (type_size != 0)
					{
						u8 ret_reg = RAX;

						if (is_float) ret_reg = XMM0;

						saved_registers[ret_reg] = false;
						auto call_vreg = make_vreg(idx);
						virtual_registers[call_vreg].reg = ret_reg;
						virtual_registers[call_vreg].fp = is_float;

						call_inst.out_count = 1;
						call_inst.output = call_vreg;
					}

					for (u8 i = 0; i < XMM7; i++)
					{
						if (saved_registers[i])
						{
							auto tmp_reg = make_vreg();
							virtual_registers[tmp_reg].reg = i;
							virtual_registers[tmp_reg].fp = i >= XMM0;

							call_inst.tmps[call_inst.tmp_count] = tmp_reg;
							call_inst.tmp_count++;
						}
					}

					Put_Inst(ctx, call_inst);
				}
				break;
				case Il_Alloca:
				{
					Il_IDX vreg = make_vreg(idx);
					GS_Type* alloca_type = &ctx.ts->type_storage[node.aloca.type_idx];
					int alloca_size = TypeSystem_Get_Type_Size(*ctx.ts, alloca_type);
					virtual_registers[vreg].stack_slot = Stack_Alloc(ctx, alloca_size, idx);
				}
				break;
				case Il_Load:
				{
					Il_IDX vreg = make_vreg(idx);

					Il_Node& ptr_node = proc.instruction_storage[node.load.ptr_node_idx];

					u8 base = 0;
					i32 disp = 0;

					if (ptr_node.node_type == Il_Alloca)
					{
						auto stack_slot = get_vreg(node.load.ptr_node_idx)->stack_slot;
						disp = -stack_slot;
						base = RBP;
					}
					else {
						base = get_vreg_idx(node.load.ptr_node_idx);
					}

					if (is_float)
					{
						virtual_registers[vreg].fp = true;
						Inst_RM(ctx, Inst_MOVF, vreg, base, disp, bit_size);
					}
					else {
						Inst_RM(ctx, Inst_MOV, vreg, base, disp, bit_size);
					}
				}
				break;
				case Il_StructElementPtr:
				{
					auto vreg = make_vreg(idx);

					Il_Node& ptr_node = proc.instruction_storage[node.element_ptr.ptr_node_idx];

					Il_IDX base = 0;
					i32 disp = 0;

					if (ptr_node.node_type == Il_Alloca)
					{
						disp = -get_vreg(node.element_ptr.ptr_node_idx)->stack_slot;
						base = RBP;
					}
					else {
						base = get_vreg_idx(node.element_ptr.ptr_node_idx);
					}

					disp += node.element_ptr.offset;

					Inst_RM(ctx, Inst_LEA, vreg, base, disp, 64);
				}
				break;
				case Il_Store:
				{
					Il_Node& ptr_node = proc.instruction_storage[node.store.ptr_node_idx];
					Il_Node& val_node = proc.instruction_storage[node.store.value_node_idx];

					if (val_node.node_type == Il_ZI)
					{
						Il_IDX base;
						i32 disp = 0;

						if (ptr_node.node_type == Il_Alloca)
						{
							base = RBP;
							disp = -get_vreg(node.store.ptr_node_idx)->stack_slot;
						}

						else GS_ASSERT_UNIMPL();

						u64 remainder = type_size;
						u64 pointer = 0;

						while (remainder > 0) {

							u8 moved_size = 0;

							if (remainder >= 8) {
								moved_size = 8;
							}
							else if (remainder >= 4) {
								moved_size = 4;
							}
							else if (remainder >= 2) {
								moved_size = 2;
							}
							else {
								moved_size = 1;
							}

							remainder -= moved_size;

							Inst_MI(ctx, Inst_MOV, 0, base, disp + pointer, moved_size * 8);

							pointer += moved_size;
						}
					}
					else if (can_imm_32(val_node, bit_size))
					{
						//	Inst_MI(ctx, Inst_MOV, , base, disp, bit_size);
						MC_Inst* store_inst = select_addr(&ptr_node, node.store.ptr_node_idx, bit_size, -1, -1, Inst_MOV);
						store_inst->flags |= Inst_Flag_Imm;
						store_inst->imm = val_node.constant.as.s4;
						store_inst->direction = false;
					}
					else
					{
						int store_op = Inst_MOV;

						if (is_float)
						{
							store_op = Inst_MOVF;
						}

						auto src = input_reg(&val_node, node.store.value_node_idx, bit_size);

						select_addr(&ptr_node, node.store.ptr_node_idx, bit_size, -1, src, store_op);
					}
				}
				break;
				case Il_Add:
				case Il_Sub:
				case Il_Bit_And:
				case Il_Bit_Or:
				{

					Il_Node& left_node = proc.instruction_storage[node.math_op.left_node_idx];
					Il_Node& right_node = proc.instruction_storage[node.math_op.right_node_idx];

					auto vreg = make_vreg(idx);

					if (is_float)
					{
						virtual_registers[vreg].fp = true;

						MC_Inst_Type op;

						if (node.node_type == Il_Add)
							op = Inst_ADDF;
						else if (node.node_type == Il_Sub)
							op = Inst_SUBF;
						else ASSERT(nullptr);

						Inst_RR(ctx, Inst_MOVF, vreg, get_vreg_idx(node.math_op.left_node_idx), bit_size);
						Inst_RR(ctx, op, vreg, get_vreg_idx(node.math_op.right_node_idx), bit_size);
					}
					else
					{
						MC_Inst_Type op;

						if (node.node_type == Il_Add)
							op = Inst_ADD;
						else if (node.node_type == Il_Sub)
							op = Inst_SUB;
						else if (node.node_type == Il_Bit_Or)
							op = Inst_OR;
						else if (node.node_type == Il_Bit_And)
							op = Inst_AND;

						if (can_imm_32(left_node, bit_size))
						{
							Inst_RI(ctx, Inst_MOV, vreg, left_node.constant.as.s4, bit_size);
						}
						else {
							input_reg(&left_node, node.math_op.left_node_idx, bit_size, vreg);
						}

						if (can_imm_32(right_node, bit_size))
						{
							Inst_RI(ctx, op, vreg, right_node.constant.as.s4, bit_size);
						}
						else {
							auto lhs_reg = input_reg(&right_node, node.math_op.right_node_idx, bit_size);
							Inst_RR(ctx, op, vreg, lhs_reg, bit_size);
						}
					}
				}
				break;
				case Il_Mul:
				{
					Il_Node& left_node = proc.instruction_storage[node.math_op.left_node_idx];
					Il_Node& right_node = proc.instruction_storage[node.math_op.right_node_idx];

					auto vreg = make_vreg(idx);

					if (is_float)
					{
						virtual_registers[vreg].fp = true;

						Inst_RR(ctx, Inst_MOVF, vreg, get_vreg_idx(node.math_op.left_node_idx), bit_size);
						Inst_RR(ctx, Inst_MULF, vreg, get_vreg_idx(node.math_op.right_node_idx), bit_size);
					}
					else
					{
						if (bit_size < 16) {
							bit_size = 16;
						}

						if (can_imm_32(left_node, bit_size))
						{
							Inst_RI(ctx, Inst_MOV, vreg, left_node.constant.as.s4, bit_size);
						}
						else {
							input_reg(&left_node, node.math_op.left_node_idx, bit_size, vreg);
						}

						if (can_imm_32(right_node, bit_size))
						{
							Inst_RRI(ctx, Inst_IMUL, vreg, vreg, right_node.constant.as.s4, bit_size);
						}
						else {
							auto lhs_reg = input_reg(&right_node, node.math_op.right_node_idx, bit_size);
							Inst_RR(ctx, Inst_IMUL, vreg, lhs_reg, bit_size);
						}
					}
				}
				break;
				case Il_Div:
				{

					if (is_float)
					{
						auto vreg = make_vreg(idx);
						virtual_registers[vreg].fp = true;

						Inst_RR(ctx, Inst_MOVF, vreg, get_vreg_idx(node.math_op.left_node_idx), bit_size);
						Inst_RR(ctx, Inst_DIVF, vreg, get_vreg_idx(node.math_op.right_node_idx), bit_size);
					}
					else
					{
						Il_Node& left_node = proc.instruction_storage[node.math_op.left_node_idx];
						Il_Node& right_node = proc.instruction_storage[node.math_op.right_node_idx];

						MC_Inst_Type op = Inst_MOV;
						if (bit_size <= 8)       op = is_signed ? Inst_MOVSXB : Inst_MOVZXB;
						else if (bit_size <= 16) op = is_signed ? Inst_MOVSXW : Inst_MOVZXW;

						auto rax_vreg = make_vreg(idx);
						auto rdx_vreg = make_vreg();

						virtual_registers[rax_vreg].reg = RAX;
						virtual_registers[rdx_vreg].reg = RDX;

						if (!can_imm_32(left_node, bit_size))
						{
							if (bit_size < 32) bit_size = 32;

							Il_IDX lhs_vreg = input_reg(&left_node, node.math_op.left_node_idx, bit_size);
							Inst_RR(ctx, op, rax_vreg, lhs_vreg, bit_size);
						}
						else
						{
							Inst_RI(ctx, Inst_MOV, rax_vreg, left_node.constant.as.s4, bit_size);
						}

						MC_Inst_Type extend_op = bit_size > 32 ? Inst_CQO : Inst_CDQ;

						Il_IDX rhs = -1;

						if (!can_imm_32(right_node, bit_size) && bit_size < 32)
						{
							rhs = make_vreg();

							if (bit_size < 32) bit_size = 32;

							Il_IDX rhs_vreg = input_reg(&right_node, node.math_op.right_node_idx, bit_size);
							Inst_RR(ctx, op, rhs, rhs_vreg, bit_size);
						}
						else
						{
							rhs = make_vreg();
							Inst_RI(ctx, Inst_MOV, rhs, right_node.constant.as.s4, bit_size);
						}

						MC_Inst* extend_inst = Inst_(ctx, extend_op);

						extend_inst->output = rdx_vreg;
						extend_inst->out_count = 1;

						MC_Inst* div_inst = Inst_R(ctx, is_signed ? Inst_IDIV : Inst_DIV, rhs, bit_size, false);
						div_inst->in_count++;
						div_inst->operands[1] = rdx_vreg;
					}
				}
				break;
				case Il_Branch:
				{
					Inst_jmp(ctx, node.br.block_idx);
				}
				break;
				case Il_Value_Cmp:
				{
					Il_Node& left_node = proc.instruction_storage[node.cmp_op.left_node_idx];
					Il_Node& right_node = proc.instruction_storage[node.cmp_op.right_node_idx];

					auto vreg = make_vreg(idx);

					if (left_node.node_type == Il_Const)
					{
						Inst_RI(ctx, Inst_MOV, vreg, left_node.constant.as.s4, bit_size);
					}
					else {
						input_reg(&left_node, node.math_op.left_node_idx, bit_size, vreg);
					}

					if (right_node.node_type == Il_Const)
					{
						Inst_RI(ctx, Inst_CMP, vreg, right_node.constant.as.s4, bit_size);
					}
					else {
						auto rhs_reg = input_reg(&right_node, node.math_op.right_node_idx, bit_size);
						Inst_RR(ctx, Inst_CMP, vreg, rhs_reg, bit_size);
					}

					MC_Inst_Type op_code;

					switch (node.cmp_op.compare_type)
					{
					case Il_Cmp_Lesser: op_code = Inst_SETL; break;
					case Il_Cmp_Greater: op_code = Inst_SETG; break;
					case Il_Cmp_LesserEqual: op_code = Inst_SETLE; break;
					case Il_Cmp_GreaterEqual: op_code = Inst_SETGE; break;
					case Il_Cmp_Equal: op_code = Inst_SETE; break;
					case Il_Cmp_NotEqual: op_code = Inst_SETNE; break;
					default:
						GS_ASSERT_UNIMPL();
						break;
					}

					Inst_R(ctx, op_code, vreg, 8);
				}
				break;
				case Il_Cond_Branch:
				{
					Inst_RI(ctx, Inst_CMP, get_vreg_idx(node.c_branch.condition_node_idx), 0, bit_size);

					if (block_idx + 1 != node.c_branch.true_case_block_idx)
						Inst_CJMP(ctx, Inst_JNE, node.c_branch.true_case_block_idx);

					Inst_CJMP(ctx, Inst_JE, node.c_branch.false_case_block_idx);
				}
				break;
				case Il_Param:
				{
					auto argument_vreg = make_vreg(idx);

					if (is_float)
					{
						virtual_registers[argument_vreg].reg = call_conv_parameter_fp_registers[node.param.index];
					}
					else
					{
						virtual_registers[argument_vreg].reg = call_conv_parameter_registers[node.param.index];
					}
				}
				break;
				case Il_Cast:
				{
					Il_Node& castee_node = proc.instruction_storage[node.cast.castee_node_idx];

					auto src = input_reg(&castee_node, node.cast.castee_node_idx, bit_size);

					if (node.cast.cast_type == Il_Cast_Ptr)
					{
						virtual_registers_lookup[idx] = src;
					}
					else
					{
						auto src = get_vreg_idx(node.cast.castee_node_idx);
						if (node.cast.cast_type == Il_Cast_FloatExt)
						{
							auto vreg = make_vreg(idx);
							virtual_registers[vreg].fp = true;
							Inst_RR(ctx, Inst_FExt, vreg, src, 32);
						}
					}
				}
				break;
				case Il_Ret:
				{
					if (type_size > 0)
					{
						if (is_float)
						{
							Inst_RR(ctx, Inst_MOV, XMM0, get_vreg_idx(node.ret.value_node_idx), bit_size);
						}
						else
						{
							Il_Node& value_node = proc.instruction_storage[node.ret.value_node_idx];

							if (can_imm_32(value_node, bit_size))
							{
								Inst_RI(ctx, Inst_MOV, RAX, value_node.constant.as.s4, bit_size);
							}
							else
							{
								input_reg(&value_node, node.ret.value_node_idx, bit_size, RAX);
							}
						}
					}
				}
				break;
				default:
					break;
				}
			}
		}

		int t = 0;

		MC_Inst* inst = nullptr;
		for (Il_IDX idx = ctx.inst_first; idx != -1; idx = inst->next)
		{
			inst = &ctx.instructions[idx];

			inst->time = t;

			for (int i = 0; i < inst->in_count; i++)
			{
				Il_IDX operand = inst->operands[i];

				if (operand >= 64)
				{
					MC_VReg& vreg = virtual_registers[operand];
					vreg.uses[vreg.use_count] = t;
					vreg.use_count++;
					vreg.range_end = t - 1;
				}
			}

			for (int i = 0; i < inst->tmp_count; i++)
			{
				Il_IDX tmp = inst->tmps[i];
				MC_VReg& vreg = virtual_registers[tmp];

				vreg.range_start = t;

				vreg.uses[vreg.use_count] = t;
				vreg.use_count++;
				vreg.range_start = t - 1;
				vreg.range_end = t - 1;
			}

			if (inst->out_count)
			{
				Il_IDX out_operand = inst->output;
				MC_VReg& vreg = virtual_registers[out_operand];

				if (vreg.range_start == -1)
				{
					vreg.range_start = t;
					vreg.range_end = t;
				}
			}

			t += 4;
		}

#define phys_reg_count_gpr 7
#define phys_reg_count_fp 6
#define phys_reg_count_total phys_reg_count_gpr + phys_reg_count_fp

		u8 gpr_register_pool[phys_reg_count_gpr] = { RAX,RCX,RDX,R8,R9,R10,R11 };
		u8 fp_register_pool[phys_reg_count_fp] = { XMM0, XMM1, XMM2, XMM3, XMM4, XMM5 };
		bool register_state[XMM7] = { 0 };

		Il_IDX active_intervals[phys_reg_count_total] = {};
		int active_count = 0;

		Array<MC_VReg*> intervals;

		for (size_t i = 64; i < virtual_registers.count; i++)
		{
			Array_Add(intervals, &virtual_registers[i]);
		}

		auto sort_intervals = [&]()
		{
			qsort(intervals.data, intervals.count, 8, interval_compare);
		};

		auto expire_intervals = [&](MC_VReg& interval)
		{
			bool remove_interval[phys_reg_count_total] = {};

			for (size_t j = 0; j < active_count; j++)
			{
				auto active_interval = active_intervals[j];

				if (virtual_registers[active_interval].range_end < interval.range_start)
				{
					register_state[virtual_registers[active_interval].assigned_register] = false;
					remove_interval[j] = true;
				}
			}

			Il_IDX new_live_intervals[phys_reg_count_total] = {};
			int new_active_count = 0;

			for (size_t i = 0; i < active_count; i++)
			{
				if (!remove_interval[i])
				{
					new_live_intervals[new_active_count] = active_intervals[i];
					new_active_count++;
				}
				else {
					DBG(GS_CORE_TRACE("vreg {} was freed r{}", active_intervals[i], virtual_registers[active_intervals[i]].assigned_register));
				}
			}

			memcpy(active_intervals, new_live_intervals, phys_reg_count_total);
			active_count = new_active_count;

		};

		auto spill_interval = [&](Il_IDX interval_idx, Il_IDX current_interval_idx)
		{
			auto& interval = virtual_registers[interval_idx];
			auto& current_interval = virtual_registers[current_interval_idx];

			auto new_split_idx = make_vreg();
			auto& new_split = virtual_registers[new_split_idx];

			ctx.stack_usage += 8;
			interval.stack_slot = ctx.stack_usage;

			//if (interval.stack_slot == 48)__debugbreak();

			interval.split_child = new_split_idx;

			new_split.range_start = current_interval.range_end;
			new_split.range_end = interval.range_end;
			new_split.fp = interval.fp;

			for (size_t i = 0; i < interval.use_count; i++)
			{
				if (interval.uses[i] > current_interval.range_start)
				{
					new_split.uses[new_split.use_count] = interval.uses[i];
					new_split.use_count++;
				}
			}

			Array_Add(intervals, &new_split);

			interval.range_end = current_interval.range_start;

			{
				Il_IDX prev_idx = -1;
				MC_Inst* prev = nullptr;

				MC_Inst* inst = nullptr;
				for (Il_IDX idx = ctx.inst_first; idx != -1; idx = inst->next)
				{
					inst = &ctx.instructions[idx];

					auto t = inst->time;

					if (t >= interval.range_end) {
						break;
					}

					prev_idx = idx;
					prev = inst;
				}

				MC_Inst new_inst = {};
				new_inst.flags = Inst_Flag_Disp;
				new_inst.op_code = new_split.fp ? Inst_MOVF : Inst_MOV;
				new_inst.in_count = 2;
				new_inst.operands[0] = interval_idx;
				new_inst.operands[1] = RBP;
				new_inst.disp = -interval.stack_slot;
				new_inst.direction = true;
				new_inst.bit_size = 64;
				new_inst.time = prev->time + 1;

				Array_Add(ctx.instructions, new_inst);
				Il_IDX inserted_index = ctx.instructions.count - 1;
				MC_Inst* inserted_instruction = &ctx.instructions[inserted_index];

				inserted_instruction->prev = prev_idx;
				inserted_instruction->next = prev->next;

				prev->next = inserted_index;
			}
			if (new_split.use_count)
			{
				DBG(GS_CORE_TRACE("vreg {} was reloaded from RBP-{} into {}", interval_idx, interval.stack_slot, new_split_idx));

				int next_use = 0;

				for (size_t i = 0; i < new_split.use_count; i++)
				{
					int t = new_split.uses[i];
					next_use = t;

					if (t > new_split.range_start)
					{
						break;
					}
				}

				Il_IDX next_idx = -1;
				MC_Inst* next = nullptr;

				MC_Inst* inst = nullptr;
				for (Il_IDX idx = ctx.inst_first; idx != -1; idx = inst->next)
				{
					inst = &ctx.instructions[idx];

					auto t = inst->time;

					next_idx = idx;
					next = inst;

					if (t >= next_use) {
						break;
					}
				}

				new_split.range_start = next_use - 1;

				MC_Inst new_inst = {};
				new_inst.flags = Inst_Flag_Disp;
				new_inst.op_code = new_split.fp ? Inst_MOVF : Inst_MOV;
				new_inst.in_count = 2;
				new_inst.out_count = 1;
				new_inst.output = new_split_idx;
				new_inst.operands[0] = new_split_idx;
				new_inst.operands[1] = RBP;
				new_inst.disp = -interval.stack_slot;
				new_inst.direction = false;
				new_inst.bit_size = 64;
				new_inst.time = next->time - 1;

				Array_Add(ctx.instructions, new_inst);
				Il_IDX inserted_index = ctx.instructions.count - 1;
				MC_Inst* inserted_instruction = &ctx.instructions[inserted_index];
				MC_Inst* prev = &ctx.instructions[next->prev];

				inserted_instruction->prev = next->prev;
				inserted_instruction->next = next_idx;

				next->prev = inserted_index;
				prev->next = inserted_index;
			}

			sort_intervals();
		};

		sort_intervals();

		auto sort_active = [&]()
		{
			for (int i = 0; i < active_count - 1; i++) {
				for (int j = 0; j < active_count - i - 1; j++) {

					Il_IDX ai = active_intervals[j];
					Il_IDX nai = active_intervals[j + 1];

					if (virtual_registers[ai].range_end > virtual_registers[nai].range_start) {
						int temp = active_intervals[j];
						active_intervals[j] = active_intervals[j + 1];
						active_intervals[j + 1] = temp;
					}
				}
			}
		};

		while (intervals.count)
		{
			auto& interval = *intervals[0];
			intervals.data++;
			intervals.count--;

			if (interval.stack_slot) continue;

			Il_IDX current_index = (Il_IDX)(&interval - virtual_registers.data);

			expire_intervals(interval);

			bool found_free = false;

			if (interval.reg >= 0)
			{
				u8 reg = interval.reg;

				if (register_state[reg]) {

					Il_IDX blocked_interval = -1;

					for (size_t i = 0; i < active_count; i++)
					{
						if (virtual_registers[active_intervals[i]].assigned_register == reg)
						{
							blocked_interval = active_intervals[i];
							active_intervals[i] = current_index;
							break;
						}
					}

					DBG(GS_CORE_TRACE("vreg {} was spilled", blocked_interval));

					spill_interval(blocked_interval, current_index);

					found_free = true;
				}
				else {
					active_intervals[active_count] = current_index;
					active_count++;
				}

				DBG(GS_CORE_TRACE("vreg {} was allocated r{}", current_index, reg));

				register_state[reg] = true;
				virtual_registers[current_index].assigned_register = reg;

				found_free = true;
			}
			else if (!found_free) {

				if (interval.hint >= 0)
				{
					Il_IDX hinted_reg = interval.hint;

					if (hinted_reg >= 64)
					{
						if (virtual_registers[hinted_reg].fp != interval.fp) {
							hinted_reg = 0;
						}
						else
						{
							hinted_reg = virtual_registers[hinted_reg].assigned_register;
						}
					}

					if (!register_state[hinted_reg])
					{
						found_free = true;
						virtual_registers[current_index].assigned_register = hinted_reg;
						register_state[hinted_reg] = true;
					}
				}

				if (!found_free)
				{
					u8* register_pool = interval.fp ? fp_register_pool : gpr_register_pool;
					auto register_pool_count = interval.fp ? phys_reg_count_fp : phys_reg_count_gpr;

					for (size_t j = 0; j < register_pool_count; j++)
					{
						u8 reg = register_pool[j];

						if (!register_state[reg]) {

							register_state[reg] = true;
							virtual_registers[current_index].assigned_register = reg;

							DBG(GS_CORE_TRACE("vreg {} was allocated r{}", current_index, reg));

							found_free = true;
							break;
						}
					}
				}

				if (!found_free)
				{
					break;
				}

				active_intervals[active_count] = current_index;
				active_count++;
			}

			sort_active();
		}

		auto split_interval_at = [&](MC_VReg* interval, int pos) {
			while (interval->split_child >= 0 && pos > interval->range_end) {
				interval = &virtual_registers[interval->split_child];
			}

			return interval;
		};

		for (Il_IDX idx = ctx.inst_first; idx != -1; idx = inst->next)
		{
			inst = &ctx.instructions[idx];

			for (size_t i = 0; i < inst->in_count; i++)
			{
				if (inst->operands[i] >= 64)
				{
					auto interval = split_interval_at(&virtual_registers[inst->operands[i]], inst->time);
					inst->operands[i] = interval->assigned_register;
				}
			}
		}

		int x = 30;
	}

	void MC_Gen_Proc_Codegen3(MC_Gen& g, Il_Proc& proc, Il_IDX proc_idx)
	{
		MC_Ctx ctx;

		ctx.proc = &proc;

		ctx.instructions = g.instruction_buffer;
		ctx.instructions.count = 0;

		MC_Proc_Codegen(g, ctx, proc, proc_idx);

		MC_Symbol& sym = g.symbols[g.proc_to_symbol[proc_idx]];
		sym.value = (u32)g.code.count;

		ctx.stack_usage += 32;

		Array<MC_Label>& labels = g.label_buffer;
		labels.count = 0xffff;

		Array<MC_JumpPatch>& jump_patches = g.jump_patches_buffer;
		jump_patches.count = 0;

		if (ctx.stack_usage)
		{
			ctx.stack_usage = align_to(ctx.stack_usage, 16);

			Emit_Push(g.code, Make_Reg(RBP));
			Emit_Mov(g.code, Make_Reg(RBP), Make_Reg(RSP), 64);
			Emit_Sub(g.code, Make_Reg(RSP), Make_Imm32(ctx.stack_usage), 64);
		}

		MC_Inst* inst = nullptr;
		for (Il_IDX idx = ctx.inst_first; idx != -1; idx = inst->next)
		{
			inst = &ctx.instructions[idx];

			if ((inst->op_code == Inst_MOV || inst->op_code == Inst_MOVF) && !(inst->flags & Inst_Flag_Disp) && !(inst->flags & Inst_Flag_Imm) && !(inst->flags & Inst_Flag_R_Data))
			{
				if (inst->operands[0] == inst->operands[1])
					continue;
			}

			if (inst->op_code == Inst_LABEL)
			{
				labels[inst->imm].offset = (u32)g.code.count;
			}
			else if (inst->op_code == Inst_JMP)
			{
				MC_JumpPatch patch;
				patch.label_idx = inst->operands[0];
				patch.offset = Emit_Jmp(g.code, 0);

				Array_Add(jump_patches, patch);
			}
			else if (inst->op_code > Inst_JMP && inst->op_code < Inst_JMP_MAX)
			{
				u32 offset = 0;

				switch (inst->op_code)
				{
				case Inst_JE: offset = Emit_JE(g.code, 0); break;
				case Inst_JNE: offset = Emit_JNE(g.code, 0); break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}

				MC_JumpPatch patch;
				patch.label_idx = inst->operands[0];
				patch.offset = offset;

				Array_Add(jump_patches, patch);
			}
			else if (inst->op_code > Inst_SETO && inst->op_code < Inst_SET_MAX)
			{
				Inst_Op op = Make_Reg(inst->operands[0]);

				switch (inst->op_code)
				{
				case Inst_SETE: Emit_SetE(g.code, op); break;
				case Inst_SETNE: Emit_SetNE(g.code, op); break;
				case Inst_SETL: Emit_SetL(g.code, op); break;
				case Inst_SETG: Emit_SetG(g.code, op); break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}
			else if (inst->op_code == Inst_CALL)
			{
				Il_IDX proc_idx = inst->operands[1];

				MC_Relocation relocation;
				relocation.reloaction_offset = Emit_Call(g.code, 0);
				relocation.symbol_idx = g.proc_to_symbol[proc_idx];
				relocation.relocation_type = IMAGE_REL_AMD64_REL32;

				Array_Add(g.code_relocations, relocation);
			}
			else if (inst->op_code == Inst_MOVABS)
			{
				Emit_MovAbs(g.code, inst->operands[0], inst->abs);
			}
			else if (inst->op_code == Inst_IMUL)
			{
				if (inst->flags & Inst_Flag_Imm)
				{
					Inst_Op imm = Make_Imm32(inst->imm);

					if (inst->bit_size == 8) imm.type = Op_Imm8;
					if (inst->bit_size == 16) imm.type = Op_Imm16;

					Emit_IMul3(g.code, Make_Reg(inst->operands[0]), Make_Reg(inst->operands[1]), imm, inst->bit_size);
				}
				else
				{
					Emit_IMul(g.code, Make_Reg(inst->operands[0]), Make_Reg(inst->operands[1]), inst->bit_size);
				}
			}
			else if (inst->op_code == Inst_DIV || inst->op_code == Inst_IDIV)
			{
				Inst_Op op1 = Make_Reg(inst->operands[0]);

				switch (inst->op_code)
				{
				case Inst_IDIV:
					Emit_IDiv(g.code, op1, inst->bit_size);
					break;
				case Inst_DIV:
					Emit_Div(g.code, op1, inst->bit_size);
					break;
				default:
					break;
				}

			}
			else if (inst->op_code == Inst_CQO || inst->op_code == Inst_CDQ)
			{
				switch (inst->op_code)
				{
				case Inst_CQO:
					Emit_CQO(g.code);
					break;
				case Inst_CDQ:
					Emit_CDQ(g.code);
					break;
				default:
					break;
				}

			}
			else
			{
				Inst_Op op1;
				Inst_Op op2;

				if (inst->flags & Inst_Flag_Disp)
				{
					if (inst->flags & Inst_Flag_Imm)
					{
						op2 = Make_Imm32(inst->imm);
						op1 = Make_Reg_Disp(inst->operands[1], inst->disp);
					}
					else
					{
						op1 = Make_Reg(inst->operands[0]);
						op2 = Make_Reg_Disp(inst->operands[1], inst->disp);
					}
				}
				else if (inst->flags & Inst_Flag_R_Data)
				{
					op1 = Make_Reg(inst->operands[0]);
					op2 = Make_Disp(inst->disp);
				}
				else
				{
					if (inst->flags & Inst_Flag_Imm)
					{
						op2 = Make_Imm32(inst->imm);
						op1 = Make_Reg(inst->operands[0]);
					}
					else {
						op2 = Make_Reg(inst->operands[1]);
						op1 = Make_Reg(inst->operands[0]);
					}
				}

				if (inst->direction)
				{
					auto tmp = op1;
					op1 = op2;
					op2 = tmp;
				}

				if (op2.type == Op_Imm32)
				{
					if (inst->bit_size == 8) op2.type = Op_Imm8;
					if (inst->bit_size == 16) op2.type = Op_Imm16;
				}

				u32 rip_relocation_offset = -1;

				switch (inst->op_code)
				{
				case Inst_MOV:
					Emit_Mov(g.code, op1, op2, inst->bit_size);
					break;
				case Inst_LEA:
					rip_relocation_offset = Emit_Lea(g.code, op1, op2, inst->bit_size);
					break;
				case Inst_ADD:
					Emit_Add(g.code, op1, op2, inst->bit_size);
					break;
				case Inst_SUB:
					Emit_Sub(g.code, op1, op2, inst->bit_size);
					break;
				case Inst_AND:
					Emit_And(g.code, op1, op2, inst->bit_size);
					break;
				case Inst_OR:
					Emit_Or(g.code, op1, op2, inst->bit_size);
					break;
				case Inst_CMP:
					Emit_Cmp(g.code, op1, op2, inst->bit_size);
					break;
				case Inst_MOVF:
				{
					if (inst->bit_size == 64)
					{
						rip_relocation_offset = Emit_MovSD(g.code, op1, op2);
					}
					else if (inst->bit_size == 32) {
						rip_relocation_offset = Emit_MovSS(g.code, op1, op2);
					}
					else
					{
						GS_ASSERT_UNIMPL();
					}
				}
				break;
				case Inst_ADDF:
				{
					if (inst->bit_size == 64)
					{
						Emit_AddSD(g.code, op1, op2);
					}
					else if (inst->bit_size == 32) {
						Emit_AddSS(g.code, op1, op2);
					}
					else
					{
						GS_ASSERT_UNIMPL();
					}
				}
				break;
				case Inst_SUBF:
				{
					if (inst->bit_size == 64)
					{
						Emit_SubSD(g.code, op1, op2);
					}
					else if (inst->bit_size == 32) {
						Emit_SubSS(g.code, op1, op2);
					}
					else
					{
						GS_ASSERT_UNIMPL();
					}
				}
				break;
				case Inst_MULF:
				{
					if (inst->bit_size == 64)
					{
						Emit_MulSD(g.code, op1, op2);
					}
					else if (inst->bit_size == 32) {
						Emit_MulSS(g.code, op1, op2);
					}
					else
					{
						GS_ASSERT_UNIMPL();
					}
				}
				break;
				case Inst_DIVF:
				{
					if (inst->bit_size == 64)
					{
						Emit_DivSD(g.code, op1, op2);
					}
					else if (inst->bit_size == 32) {
						Emit_DivSS(g.code, op1, op2);
					}
					else
					{
						GS_ASSERT_UNIMPL();
					}
				}
				break;
				case Inst_FExt:
				{
					Emit_CVTSS2SD(g.code, op1, op2);
				}
				break;
				case Inst_MovF2G:
				{
					Emit_MovQ(g.code, op1, op2);
				}
				break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}

				if (inst->flags & Inst_Flag_R_Data)
				{
					ASSERT(rip_relocation_offset != -1);

					MC_Relocation relocation;
					relocation.reloaction_offset = rip_relocation_offset;
					relocation.symbol_idx = g.rdata_symbol_index;
					relocation.relocation_type = IMAGE_REL_AMD64_REL32;

					Array_Add(g.code_relocations_sections, relocation);
				}
			}
		}

		if (ctx.stack_usage)
		{
			Emit_Add(g.code, Make_Reg(RSP), Make_Imm32(ctx.stack_usage), 64);
			Emit_Pop(g.code, Make_Reg(RBP));
		}

		for (size_t i = 0; i < jump_patches.count; i++)
		{
			MC_JumpPatch& patch = jump_patches[i];
			MC_Label& label = labels[patch.label_idx];
			u32* as_pointer = (u32*)&g.code[patch.offset];
			*as_pointer = label.offset - (patch.offset + 4);
		}

		Emit_Ret(g.code);
	}

	inline void MC_Gen_Decl_Proc_Symbols(MC_Gen& g)
	{
		g.proc_to_symbol = Array_Reserved<u32>(g.prog->procedures.count);
		g.proc_to_symbol.count = g.prog->procedures.count;

		for (size_t i = 0; i < g.prog->procedures.count; i++)
		{
			Il_Proc& proc = g.prog->procedures[i];
			g.proc_to_symbol[i] = (u32)g.symbols.count;

			MC_Symbol symbol = { 0 };
			symbol.symbol_name = String_Copy(proc.proc_name);
			symbol.value = 0;

			if (proc.external) {
				symbol.section_value = 0;
			}
			else {
				symbol.section_value = g.text_section_index;
			}

			Array_Add(g.symbols, symbol);
		}
	}

	Const_Union MC_Gen_Constant_Evaluate(MC_Gen& g, Array<Il_Node>& nodes, Il_IDX node_idx)
	{
		Il_Node node = nodes[node_idx];

		switch (node.node_type)
		{
		case Il_Const:
		{
			return node.constant.as;
		}
		default:
			GS_ASSERT_UNIMPL();
			break;
		}
	}

	void MC_Gen_Globals_Codegen(MC_Gen& g)
	{
		for (size_t i = 0; i < g.prog->globals.count; i++)
		{
			Il_Global& global = g.prog->globals[i];

			MC_Symbol symbol = { 0 };
			symbol.symbol_name = String_Copy(global.name);
			symbol.value = 0;

			if (global.external) {
				symbol.section_value = 0;
			}
			else {

				auto type_size = get_type_size(global.type);

				symbol.value = (u32)g.rdata.count;

				Array<u8>* storage = &g.data;
				Array_Add(g.global_section_offsets, (u32)g.data.count);

				if (global.read_only) {
					storage = &g.rdata;
					Array_Add(g.global_section_offsets, (u32)g.rdata.count);
				}

				if (global.initializer != (Il_IDX)-1) {

					auto result = MC_Gen_Constant_Evaluate(g, global.initializer_storage, global.initializer);

					u8* init_data_ptr = (u8*)&result;

					if (init_data_ptr) {
						u8* init_data_ptr = (u8*)result.ptr;
					}

					for (size_t i = 0; i < type_size; i++)
					{
						Array_Add(*storage, init_data_ptr[i]);
					}
				}
				else {
					for (size_t i = 0; i < type_size; i++)
					{
						Array_Add(g.data, (u8)0);
					}
				}

				symbol.section_value = g.data_section_index;
			}

			Array_Add(g.global_to_symbol, (u32)g.symbols.count);

			Array_Add(g.symbols, symbol);
		}
	}

	void MC_Gen_Program_Codegen(MC_Gen& g)
	{
		MC_Gen_Globals_Codegen(g);

		MC_Gen_Decl_Proc_Symbols(g);

		for (size_t i = 0; i < g.prog->procedures.count; i++)
		{
			Il_Proc& proc = g.prog->procedures[i];
			if (!proc.external)
			{
				//MC_Gen_Proc_Codegen(g, proc, (Il_IDX)i);
				MC_Gen_Proc_Codegen3(g, proc, (Il_IDX)i);
			}
		}
	}

	Il_Program test_program;

	bool MC_Gen_Run(MC_Gen& g)
	{
		register_names[Phys_Reg(RAX)] = "rax";
		register_names[Phys_Reg(RBX)] = "rbx";
		register_names[Phys_Reg(RCX)] = "rcx";
		register_names[Phys_Reg(RDX)] = "rdx";
		register_names[Phys_Reg(RBP)] = "rbp";

		auto begin = std::chrono::high_resolution_clock::now();

		bool use_test_program = false;

		g.jump_patches_buffer = Array_Reserved<MC_JumpPatch>(0xffff);
		g.instruction_buffer = Array_Reserved<MC_Inst>(0xffff);
		g.label_buffer = Array_Reserved<MC_Label>(0xffff);
		g.virtual_registers_buffer = Array_Reserved<MC_VReg>(0xffff);

		if (use_test_program) {

			// 			Il_Program_Init(test_program, g.prog->type_system);
			// 			g.prog = &test_program;
			// 			g.type_system = g.prog->type_system;
			// 
			// 			auto int_ptr = TypeSystem_Get_Pointer_Type(*g.type_system, g.type_system->int_Ty, 1);
			// 
			// 			Il_IDX test_proc_idx = Il_Insert_Proc(test_program, String_Make("main"), TypeSystem_Get_Proc_Type(*g.type_system, g.type_system->void_Ty, {}));
			// 			Il_Proc& test_proc = test_program.procedures[test_proc_idx];
			// 
			// 			Il_IDX test_var = Il_Insert_Alloca(test_proc, g.type_system->int_Ty);
			// 			Il_IDX test_var2 = Il_Insert_Alloca(test_proc, int_ptr);
			// 			Il_IDX test_constant = Il_Insert_Constant(test_proc, (void*)0xffff, g.type_system->int_Ty);
			// 
			// 			Il_IDX add_res = Il_Insert_Math_Op(test_proc, g.type_system->int_Ty, Il_Add, test_constant, test_constant);
			// 			Il_IDX add_res2 = Il_Insert_Math_Op(test_proc, g.type_system->int_Ty, Il_Add, add_res, test_constant);
			// 
			// 			Il_Insert_Store(test_proc, g.type_system->int_Ty, test_var, add_res);
			// 			Il_Insert_Store(test_proc, g.type_system->int_Ty, test_var, add_res);
			// 			Il_Insert_Store(test_proc, g.type_system->int_Ty, test_var, add_res);
			// 			Il_Insert_Store(test_proc, g.type_system->int_Ty, test_var, add_res2);
			// 
			// 			//Il_IDX loaded_var = Il_Insert_Load(test_proc, g.type_system->int_Ty, test_var);
			// 			Il_Insert_Store(test_proc, int_ptr, test_var2, test_var);
			// 
			// 			GS_CORE_TRACE("test program: {}", Il_Print_Proc(test_proc));
		}
		else
		{
		}

		register_values = Array_Reserved<Value_Desc>(65553);
		register_values.count = 65553;

		register_use_counts = Array_Reserved<i8>(65553);

		{
			GS_PROFILE_SCOPE("il emit x64");
			MC_Gen_Program_Codegen(g);
		}

		auto output_begin = std::chrono::high_resolution_clock::now();
		{
			GS_PROFILE_SCOPE("il emit object");
			MC_Gen_Output(g);
		}
		auto output_end = std::chrono::high_resolution_clock::now();

		auto end = std::chrono::high_resolution_clock::now();

		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
		auto output_micros = std::chrono::duration_cast<std::chrono::microseconds>(output_end - output_begin).count();

		double back_time_f = microseconds;
		back_time_f /= 1000000.0;

		double output_f = output_micros;
		output_f /= 1000000.0;

		GS_CORE_INFO("backend took total: {} s", back_time_f);

		return true;
	}

	void MC_Gen_Output(MC_Gen& g)
	{
#define COFF_MACHINE (u16)IMAGE_FILE_MACHINE_AMD64

		g.coff_obj = Array_Reserved<u8>(1024 * 1024);

		Array<u8> str_table_data;

		COFF_File_Header header = { 0 };
		header.machine = COFF_MACHINE;
		header.time_date_stamp = (u32)time(nullptr);

		COFF_File_Header* header_ptr = (COFF_File_Header*)Write_Bytes(g.coff_obj, (u8*)&header, IMAGE_SIZEOF_FILE_HEADER);

		u32 num_section_symbols = 0;

		COFF_Section_Header* text_header_ptr = nullptr;

		{
			COFF_Section_Header section_header = { 0 };
			strcpy((char*)section_header.name, ".text");
			section_header.characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_ALIGN_16BYTES;
			text_header_ptr = (COFF_Section_Header*)Write_Bytes(g.coff_obj, (u8*)&section_header, IMAGE_SIZEOF_SECTION_HEADER);
			header_ptr->number_of_sections++;

			num_section_symbols++;
		}

		const int text_section_idx = header_ptr->number_of_sections;

		COFF_Section_Header* rdata_header_ptr = nullptr;

		{
			COFF_Section_Header section_header = { 0 };
			strcpy((char*)section_header.name, ".rdata");
			section_header.characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_ALIGN_16BYTES;

			rdata_header_ptr = (COFF_Section_Header*)Write_Bytes(g.coff_obj, (u8*)&section_header, IMAGE_SIZEOF_SECTION_HEADER);
			header_ptr->number_of_sections++;
			num_section_symbols++;
		}

		const int rdata_section_idx = header_ptr->number_of_sections;

		COFF_Section_Header* data_header_ptr = nullptr;

		{
			COFF_Section_Header section_header = { 0 };
			strcpy((char*)section_header.name, ".data");
			section_header.characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_16BYTES;

			data_header_ptr = (COFF_Section_Header*)Write_Bytes(g.coff_obj, (u8*)&section_header, IMAGE_SIZEOF_SECTION_HEADER);
			header_ptr->number_of_sections++;
			num_section_symbols++;
		}

		const int data_section_idx = header_ptr->number_of_sections;

		text_header_ptr->pointer_to_raw_data = (u32)g.coff_obj.count;

		for (size_t i = 0; i < g.code.count; i++)
		{
			Write_8(g.coff_obj, g.code[i]);
		}

		text_header_ptr->size_of_raw_data = (u32)((u32)g.coff_obj.count - (u32)text_header_ptr->pointer_to_raw_data);
		text_header_ptr->pointer_to_relocs = (u32)g.coff_obj.count;

		u32 symbol_base_count = 2 * num_section_symbols;

		for (size_t i = 0; i < g.code_relocations.count; i++)
		{
			COFF_Relocation relocation;
			relocation.address = g.code_relocations[i].reloaction_offset;
			relocation.symbol_table_idx = g.code_relocations[i].symbol_idx + symbol_base_count;
			relocation.type = g.code_relocations[i].relocation_type;

			Write_Bytes(g.coff_obj, (u8*)&relocation, 10);
		}

		for (size_t i = 0; i < g.code_relocations_sections.count; i++)
		{
			COFF_Relocation relocation;
			relocation.address = g.code_relocations_sections[i].reloaction_offset;
			relocation.symbol_table_idx = g.code_relocations_sections[i].symbol_idx;
			relocation.type = g.code_relocations_sections[i].relocation_type;

			Write_Bytes(g.coff_obj, (u8*)&relocation, 10);
		}

		text_header_ptr->relocation_count = (u32)(g.code_relocations.count + g.code_relocations_sections.count);

		if (g.rdata.count) {

			rdata_header_ptr->pointer_to_raw_data = (u32)g.coff_obj.count;

			for (size_t i = 0; i < g.rdata.count; i++)
			{
				Write_8(g.coff_obj, g.rdata[i]);
			}

			rdata_header_ptr->size_of_raw_data = (u32)((u32)g.coff_obj.count - (u32)rdata_header_ptr->pointer_to_raw_data);
		}

		if (g.data.count) {

			data_header_ptr->pointer_to_raw_data = (u32)g.coff_obj.count;

			for (size_t i = 0; i < g.data.count; i++)
			{
				Write_8(g.coff_obj, g.data[i]);
			}

			data_header_ptr->size_of_raw_data = (u32)((u32)g.coff_obj.count - (u32)data_header_ptr->pointer_to_raw_data);
		}

		header_ptr->pointer_to_symbol_table = (u32)g.coff_obj.count;

		{

			COFF_Symbol symbol_text = { 0 };
			strcpy((char*)symbol_text.short_name, ".text");
			symbol_text.storage_class = IMAGE_SYM_CLASS_STATIC;
			symbol_text.type = IMAGE_SYM_TYPE_NULL;
			symbol_text.value = 0;
			symbol_text.aux_symbols_count = 1;
			symbol_text.section_number = text_section_idx;

			header_ptr->number_of_symbols++;
			Write_Bytes(g.coff_obj, (u8*)&symbol_text, IMAGE_SIZEOF_SYMBOL);

			COFF_AuxSectionSymbol sym_text_aux = { 0 };
			sym_text_aux.length = text_header_ptr->size_of_raw_data;
			sym_text_aux.checksum = 0xffffffff;
			sym_text_aux.number = text_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&sym_text_aux, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;
		}

		{
			COFF_Symbol symbol_rdata = { 0 };
			strcpy((char*)symbol_rdata.short_name, ".rdata");
			symbol_rdata.storage_class = IMAGE_SYM_CLASS_STATIC;
			symbol_rdata.type = IMAGE_SYM_TYPE_NULL;
			symbol_rdata.value = 0;
			symbol_rdata.aux_symbols_count = 1;
			symbol_rdata.section_number = rdata_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&symbol_rdata, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;

			COFF_AuxSectionSymbol sym_rdata_aux = { 0 };
			sym_rdata_aux.length = rdata_header_ptr->size_of_raw_data;
			sym_rdata_aux.checksum = 0xffffffff;
			sym_rdata_aux.number = rdata_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&sym_rdata_aux, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;
		}

		{
			COFF_Symbol symbol_data = { 0 };
			strcpy((char*)symbol_data.short_name, ".data");
			symbol_data.storage_class = IMAGE_SYM_CLASS_STATIC;
			symbol_data.type = IMAGE_SYM_TYPE_NULL;
			symbol_data.value = 0;
			symbol_data.aux_symbols_count = 1;
			symbol_data.section_number = data_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&symbol_data, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;

			COFF_AuxSectionSymbol sym_data_aux = { 0 };
			sym_data_aux.length = data_header_ptr->size_of_raw_data;
			sym_data_aux.checksum = 0xffffffff;
			sym_data_aux.number = data_section_idx;

			Write_Bytes(g.coff_obj, (u8*)&sym_data_aux, IMAGE_SIZEOF_SYMBOL);
			header_ptr->number_of_symbols++;
		}

		for (size_t i = 0; i < g.symbols.count; i++)
		{
			MC_Symbol mc_symbol = g.symbols[i];

			header_ptr->number_of_symbols++;

			COFF_Symbol symbol = { 0 };

			if (mc_symbol.symbol_name.count > 8) {
				symbol.long_name[0] = 0;
				symbol.long_name[1] = str_table_data.count + 4;

				for (size_t i = 0; i < mc_symbol.symbol_name.count; i++)
				{
					Array_Add(str_table_data, (u8)mc_symbol.symbol_name[i]);
				}

				Array_Add(str_table_data, (u8)0);
			}
			else {
				strcpy((char*)symbol.short_name, mc_symbol.symbol_name.data);
			}

			symbol.storage_class = IMAGE_SYM_CLASS_EXTERNAL;
			symbol.type = IMAGE_SYM_TYPE_NULL + (IMAGE_SYM_DTYPE_FUNCTION << 4);
			symbol.value = mc_symbol.value;
			symbol.aux_symbols_count = 0;
			symbol.section_number = mc_symbol.section_value;

			Write_Bytes(g.coff_obj, (u8*)&symbol, IMAGE_SIZEOF_SYMBOL);
		}

		u32* string_table_length = Write_32_Ptr(g.coff_obj);
		u8* string_table_begin = (u8*)string_table_length + 4;

		for (size_t i = 0; i < str_table_data.count; i++)
		{
			Array_Add(g.coff_obj, str_table_data[i]);
		}

		*string_table_length = (u32)((u64)(g.coff_obj.data + g.coff_obj.count) - (u64)string_table_begin) + 4;

		std::string output_path = std::string(g.output_path.data);

		fs_path obj_path = fs_path(output_path);
		fs_path obj_directory_path = fs_path(output_path);
		obj_directory_path.remove_filename();
		obj_path.replace_extension(".obj");

		std::filesystem::create_directories(obj_directory_path);

		{
			std::ofstream file(obj_path.string(), std::ios::binary);
			file.write((const char*)g.coff_obj.data, g.coff_obj.count);
			file.close();
		}
	}

	MC_Gen MC_Gen_Make(MC_Gen_Spec spec, Il_Program* program)
	{
		MC_Gen g = { 0 };
		g.output_path = String_Copy(spec.output_path);
		g.prog = program;

		ASSERT(g.prog);
		ASSERT(g.ts);

		return g;
	}
}
