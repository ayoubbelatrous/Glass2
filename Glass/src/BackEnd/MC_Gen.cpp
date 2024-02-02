#include "pch.h"

#include "BackEnd/MC_Gen.h"

#include "BackEnd/x64_Emit.h"

namespace Glass
{

#define X64_REG_COUNT 10
#define REG_BUFS_SZ 20000

	const u8 x64_registers[X64_REG_COUNT] = { RAX, RBX, RCX, RDX, R8 ,R9, RSI, RDI, RSP, RBP };

	struct Reg_Allocation
	{
		u8 allocated_register = 0xff;
		bool spilled = false;
	};

	inline void MC_Proc_Reg_Lifetimes(MC_Gen& g, Il_Proc& proc, Il_IDX proc_idx, Array<u8>& lifetimes)
	{
		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX i = block.instructions[j];
				Il_Node node = proc.instruction_storage[i];

				GS_Type* node_type = &g.prog->type_system->type_storage[node.type_idx];

				if (node.node_type == Il_Cast)
				{
					lifetimes[node.cast.castee_node_idx]++;
				}

				if (node.node_type == Il_Add || node.node_type == Il_Sub || node.node_type == Il_Mul || node.node_type == Il_Div) {
					lifetimes[node.math_op.left_node_idx]++;
					lifetimes[node.math_op.right_node_idx]++;
				}

				if (node.node_type == Il_Ret) {
					lifetimes[node.ret.value_node_idx]++;
				}

				if (node.node_type == Il_Store) {
					lifetimes[node.store.ptr_node_idx]++;
					lifetimes[node.store.value_node_idx]++;
				}

				if (node.node_type == Il_Load) {
					lifetimes[node.load.ptr_node_idx]++;
				}

				if (node.node_type == Il_Call) {
					for (size_t i = 0; i < node.call.argument_count; i++)
						lifetimes[node.call.arguments[i]]++;
				}
			}
		}
	}

	u8 call_conv_parameter_registers[4] = { RCX,RDX,R8,R9 };

	inline void MC_Reg_Alloc_Proc(MC_Gen& g, Il_Proc& proc, Il_IDX proc_idx, Array<Reg_Allocation>& allocations)
	{
		u8 allocated[X64_REG_COUNT] = { };
		Il_IDX reg_to_v_reg[X64_REG_COUNT] = { };

		u8 vreg_lifetimes[REG_BUFS_SZ] = { 0 };

		Array<u8> vreg_lifetimes_view;
		vreg_lifetimes_view.data = vreg_lifetimes;
		vreg_lifetimes_view.capacity = REG_BUFS_SZ;
		vreg_lifetimes_view.count = REG_BUFS_SZ;

		MC_Proc_Reg_Lifetimes(g, proc, proc_idx, vreg_lifetimes_view);

		auto use_register = [&](Il_IDX vreg_idx) {

			vreg_lifetimes[vreg_idx]--;

			if (vreg_lifetimes[vreg_idx] == 0) {
				if (allocations[vreg_idx].allocated_register != 0xff) {
					u8 allocated_register = allocations[vreg_idx].allocated_register;
					allocated[allocated_register] = 0;
					reg_to_v_reg[allocated_register] = (Il_IDX)-1;
				}
			}
		};

		auto allocate_register = [&](Il_IDX vreg_idx) -> u8 {

			u8 allocated_register = 0xff;

			for (u8 i = 0; i < X64_REG_COUNT; i++)
			{
				auto phys_reg = x64_registers[i];

				if (phys_reg == RBP) __debugbreak();

				if (allocated[phys_reg] == 0) {
					allocated_register = phys_reg;
					allocated[phys_reg] = 1;
					reg_to_v_reg[phys_reg] = vreg_idx;
					break;
				}
			}

			if (allocated_register == 0xff) {
				allocations[reg_to_v_reg[RAX]].spilled = true;
				allocated[reg_to_v_reg[RAX]] = 1;
				reg_to_v_reg[RAX] = vreg_idx;
				allocated_register = RAX;
			}

			return allocated_register;
		};

		auto temp_allocate_register = [&](Il_IDX vreg_idx) -> u8 {

			u8 allocated_register = 0xff;

			for (u8 i = 0; i < X64_REG_COUNT; i++)
			{
				auto phys_reg = x64_registers[i];

				if (phys_reg == RBP) __debugbreak();

				if (allocated[phys_reg] == 0) {
					allocated_register = phys_reg;
					allocated[phys_reg] = 1;
					reg_to_v_reg[phys_reg] = vreg_idx;
					break;
				}
			}

			if (allocated_register == 0xff) {
				allocations[reg_to_v_reg[RAX]].spilled = true;
				allocated[reg_to_v_reg[RAX]] = 1;
				reg_to_v_reg[RAX] = vreg_idx;
				allocated_register = RAX;
			}

			allocated[allocated_register] = 0;
			reg_to_v_reg[allocated_register] = -1;

			return allocated_register;
		};

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX idx = block.instructions[j];
				Il_Node node = proc.instruction_storage[idx];

				GS_Type* node_type = &g.prog->type_system->type_storage[node.type_idx];

				if (node.node_type == Il_Cast)
				{
					use_register(node.cast.castee_node_idx);
					allocations[idx].allocated_register = allocate_register(idx);
				}

				if (node.node_type == Il_Alloca)
				{
					allocations[idx].allocated_register = 0xff;
				}

				if (node.node_type == Il_Ret)
				{
					allocations[idx].allocated_register = 0xff;
					use_register(node.ret.value_node_idx);
				}

				if (node.node_type == Il_Store)
				{
					u8 allocated_register = temp_allocate_register(idx);

					allocations[idx].allocated_register = allocated_register;

					use_register(node.store.ptr_node_idx);
					use_register(node.store.value_node_idx);
				}

				if (node.node_type == Il_Load)
				{
					allocations[idx].allocated_register = 0xff;
					use_register(node.load.ptr_node_idx);
				}

				if (node.node_type == Il_Param)
				{
					allocations[idx].allocated_register = call_conv_parameter_registers[node.param.index];
				}

				if (node.node_type == Il_Cond_Branch)
				{
					use_register(node.c_branch.condition_node_idx);
				}

				if (node.node_type == Il_Div) {

					if (allocated[RAX] == 1) {
						allocations[reg_to_v_reg[RAX]].spilled = true;
					}

					allocated[RAX] = 1;
					reg_to_v_reg[RAX] = idx;

					if (allocated[RDX] == 1) {
						allocations[reg_to_v_reg[RDX]].spilled = true;
					}

					allocated[RDX] = 1;
					reg_to_v_reg[RDX] = (Il_IDX)-1;

					if (proc.instruction_storage[node.math_op.right_node_idx].node_type == Il_Const)
					{
						u8 allocated_register = allocate_register(idx);

						allocations[idx].allocated_register = allocated_register;

						allocated[RAX] = 0;
						reg_to_v_reg[RAX] = (Il_IDX)-1;
					}

					allocated[RDX] = 0;

					use_register(node.math_op.left_node_idx);
					use_register(node.math_op.right_node_idx);
				}

				if (node.node_type == Il_Add || node.node_type == Il_Sub || node.node_type == Il_Mul) {

					u8 allocated_register = allocate_register(idx);

					allocations[idx].allocated_register = allocated_register;

					use_register(node.math_op.left_node_idx);
					use_register(node.math_op.right_node_idx);
				}

				if (node.node_type == Il_Value_Cmp) {

					u8 tmp_register = RAX;

					if (node.cmp_op.compare_type == Il_Cmp_And || node.cmp_op.compare_type == Il_Cmp_Or) {

						if (allocated[tmp_register] == 1) {
							allocations[reg_to_v_reg[tmp_register]].spilled = true;
						}

						reg_to_v_reg[tmp_register] = (Il_IDX)-1;

						allocated[tmp_register] = 1;
					}

					u8 allocated_register = allocate_register(idx);

					allocations[idx].allocated_register = allocated_register;

					if (node.cmp_op.compare_type == Il_Cmp_And || node.cmp_op.compare_type == Il_Cmp_Or)
						allocated[tmp_register] = 1;

					use_register(node.cmp_op.left_node_idx);
					use_register(node.cmp_op.right_node_idx);
				}

				if (node.node_type == Il_String) {

					u8 allocated_register = allocate_register(idx);
					allocations[idx].allocated_register = allocated_register;
				}

				if (node.node_type == Il_Call) {

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Il_IDX argument_node_idx = node.call.arguments[i];

						u8 needed_register = call_conv_parameter_registers[i];

						if (allocated[needed_register] == 1) {
							allocations[reg_to_v_reg[needed_register]].spilled = true;
							allocated[needed_register] = 0;
						}
					}

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Il_IDX argument_node_idx = node.call.arguments[i];
						use_register(argument_node_idx);
					}

					u8 needed_register = RAX;

					if (allocated[needed_register] == 1) {
						allocations[reg_to_v_reg[needed_register]].spilled = true;
					}

					reg_to_v_reg[needed_register] = idx;

					allocations[idx].allocated_register = needed_register;

					if (g.prog->procedures[node.call.proc_idx].signature->proc.return_type != g.prog->type_system->void_Ty)
					{
						allocated[needed_register] = 1;
					}
					else {
						allocated[needed_register] = 0;
					}

				}
			}
		}
	}

	static i64 align_to(i64 unaligned_val, i64 alignment) {
		i64 mask = alignment - 1;
		i64 aligned_val = unaligned_val + (-unaligned_val & mask);
		return aligned_val;
	}

	inline void MC_Gen_Proc_Codegen(MC_Gen& g, Il_Proc& proc, Il_IDX proc_idx)
	{
		MC_Symbol& sym = g.symbols[g.proc_to_symbol[proc_idx]];
		sym.value = (u32)g.code.count;

		Inst_Op register_values[REG_BUFS_SZ] = {};
		Reg_Allocation register_allocations[REG_BUFS_SZ] = {};

		Array<Reg_Allocation> reg_allocations_array_view;
		reg_allocations_array_view.data = register_allocations;
		reg_allocations_array_view.capacity = REG_BUFS_SZ;
		reg_allocations_array_view.count = REG_BUFS_SZ;

		MC_Reg_Alloc_Proc(g, proc, proc_idx, reg_allocations_array_view);

		struct Block_Relocation
		{
			u32 block_start;
			i32 locations[8];
			i32 starts[8];
			u32 count = 0;
		};

		Block_Relocation block_reloactions[512] = {};
		Block_Relocation return_relocations = {};

		u64 stack_pointer = 0;
		u64 stack_top_section_size = 0;

		Inst_Op reg_rbp;
		reg_rbp.type = Op_Reg;
		reg_rbp.reg = RBP;

		Inst_Op reg_rsp;
		reg_rsp.type = Op_Reg;
		reg_rsp.reg = RSP;

		Inst_Op stack_sz;
		stack_sz.type = Op_Imm32;
		stack_sz.imm32 = 0;

		Emit_Push(g.code, reg_rbp);
		Emit_Mov(g.code, reg_rbp, reg_rsp, 64);
		u32 stack_allocation_byte_idx = Emit_Sub(g.code, reg_rsp, stack_sz, 64);

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			Block_Relocation& block_relocation = block_reloactions[block_idx];
			block_relocation.block_start = (u32)g.code.count;

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX i = block.instructions[j];
				Il_Node node = proc.instruction_storage[i];

				GS_Type* node_type = &g.prog->type_system->type_storage[node.type_idx];
				auto node_type_size = TypeSystem_Get_Type_Size(*g.prog->type_system, node_type);

				switch (node.node_type)
				{
				case Il_Cast:
				{
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op result_op;
					result_op.type = Op_Reg;
					result_op.reg = allocation.allocated_register;

					Emit_Mov(g.code, result_op, register_values[node.cast.castee_node_idx], 64);

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += node_type_size;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, register_values[i], (u8)(node_type_size * 8));

						register_values[i] = spillage_value;
					}
				}
				break;
				case Il_String:
				{
					Reg_Allocation allocation = register_allocations[i];

					u32 string_rdata_byte_offset = (u32)g.rdata.count;

					for (size_t i = 0; i < node.string.str.count; i++)
					{
						Array_Add(g.rdata, (u8)node.string.str[i]);
					}

					Array_Add(g.rdata, (u8)0);

					Inst_Op result_op;
					result_op.type = Op_Reg;
					result_op.reg = allocation.allocated_register;

					Inst_Op rel_disp4;
					rel_disp4.type = Op_Disp4;
					rel_disp4.disp_4 = string_rdata_byte_offset;

					MC_Relocation relocation;
					relocation.reloaction_offset = Emit_Lea(g.code, result_op, rel_disp4, 64);
					relocation.symbol_idx = g.rdata_symbol_index;
					relocation.relocation_type = IMAGE_REL_AMD64_REL32;

					Array_Add(g.code_relocations_sections, relocation);

					register_values[i] = result_op;

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += node_type_size;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, register_values[i], (u8)(node_type_size * 8));

						register_values[i] = spillage_value;
					}

				}
				break;
				case Il_Call:
				{
					Inst_Op stack_top_offset;
					stack_top_offset.type = Op_Reg_Disp4;
					stack_top_offset.reg_disp.r = RSP;
					stack_top_offset.reg_disp.disp = 32;

					u64 stack_top_pointer = 0;

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Il_IDX argument_node_idx = node.call.arguments[i];
						GS_Type* argument_type = &g.prog->type_system->type_storage[proc.instruction_storage[argument_node_idx].type_idx];

						auto argument_type_size = TypeSystem_Get_Type_Size(*g.prog->type_system, argument_type);

						Inst_Op argument_register;
						argument_register.type = Op_Reg;
						argument_register.reg = call_conv_parameter_registers[i];

						if (i < 4) {
							Emit_Mov(g.code, argument_register, register_values[argument_node_idx], (u32)(argument_type_size * 8));
						}
						else {
							stack_top_pointer += 8;

							Inst_Op argument_op = register_values[argument_node_idx];

							if (argument_op.type != Op_Reg && argument_op.type != Op_Imm8) {

								Inst_Op tmp_register;
								tmp_register.type = Op_Reg;
								tmp_register.reg = RAX;

								Emit_Mov(g.code, tmp_register, argument_op, (u32)(argument_type_size * 8));

								argument_op = tmp_register;
							}

							Emit_Mov(g.code, stack_top_offset, argument_op, (u32)(argument_type_size * 8));
							stack_top_offset.reg_disp.disp += 8;
						}
					}

					stack_top_section_size = std::max(stack_top_pointer, stack_top_section_size);

					MC_Relocation relocation;
					relocation.reloaction_offset = Emit_Call(g.code, 0);
					relocation.symbol_idx = g.proc_to_symbol[node.call.proc_idx];
					relocation.relocation_type = IMAGE_REL_AMD64_REL32;

					Array_Add(g.code_relocations, relocation);

					Reg_Allocation allocation = register_allocations[i];

					Inst_Op result_register;
					result_register.type = Op_Reg;
					result_register.reg = allocation.allocated_register;

					register_values[i] = result_register;

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += node_type_size;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, register_values[i], (u8)(node_type_size * 8));

						register_values[i] = spillage_value;
					}
				}
				break;
				case Il_Param:
				{
					u8 allocated_register = register_allocations[i].allocated_register;

					if (node.param.index < 4) {
						Inst_Op result_register;
						result_register.type = Op_Reg;
						result_register.reg = allocated_register;

						register_values[i] = result_register;
					}
					else {
						Inst_Op stack_param_location;
						stack_param_location.type = Op_Reg_Disp4;
						stack_param_location.reg_disp.r = RBP;
						stack_param_location.reg_disp.disp = 32 + 8 + (node.param.index - 3) * 8;
						register_values[i] = stack_param_location;
					}
				}
				break;
				case Il_Alloca:
				{
					Inst_Op op;
					op.type = Op_Reg_Disp4;
					op.reg_disp.r = RBP;

					stack_pointer += node_type_size;
					op.reg_disp.disp = -(i32)stack_pointer;

					register_values[i] = op;
				}
				break;
				case Il_ZI:
					break;
				case Il_Store:
				{
					Reg_Allocation allocation = register_allocations[i];
					Inst_Op value_op = register_values[node.store.value_node_idx];

					if (proc.instruction_storage[node.store.value_node_idx].node_type == Il_ZI) {

						u64 remainder = node_type_size;

						Inst_Op pointer_op = register_values[node.store.ptr_node_idx];

						while (remainder > 0) {

							Inst_Op zero;
							zero.imm32 = 0;
							zero.type = Op_Imm8;

							u8 zeroed_size = 0;

							if (remainder >= 8) {
								zeroed_size = 8;
								remainder -= zeroed_size;
								zero.type = Op_Imm32;
							}
							else if (remainder >= 4) {
								zeroed_size = 4;
								remainder -= zeroed_size;
								zero.type = Op_Imm32;
							}
							else if (remainder >= 2) {
								zeroed_size = 2;
								remainder -= zeroed_size;
								zero.type = Op_Imm16;
							}
							else {
								zeroed_size = 1;
								remainder -= zeroed_size;
								zero.type = Op_Imm8;
							}

							Emit_Mov(g.code, pointer_op, zero, zeroed_size * 8);
							pointer_op.reg_disp.disp += zeroed_size;
						}
					}
					else {

						if (node_type_size <= 8) {
							if (value_op.type == Op_Reg_Disp4 || value_op.type == Op_Reg_Disp1) {

								Inst_Op tmp_op;
								tmp_op.type = Op_Reg;
								tmp_op.reg = allocation.allocated_register;

								Emit_Mov(g.code, tmp_op, value_op, (u32)(node_type_size * 8));

								value_op = tmp_op;
							}

							Emit_Mov(g.code, register_values[node.store.ptr_node_idx], value_op, (u32)(node_type_size * 8));
						}
						else {

							u64 remainder = node_type_size;

							Inst_Op pointer_op = register_values[node.store.ptr_node_idx];

							Inst_Op tmp_op;
							tmp_op.type = Op_Reg;
							tmp_op.reg = allocation.allocated_register;

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

								Emit_Mov(g.code, tmp_op, value_op, moved_size * 8);
								Emit_Mov(g.code, pointer_op, tmp_op, moved_size * 8);

								pointer_op.reg_disp.disp += moved_size;
								value_op.reg_disp.disp += moved_size;
							}
						}
					}
				}
				break;
				case Il_Add:
				case Il_Sub:
				case Il_Mul:
				{
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op result_register;
					result_register.type = Op_Reg;
					result_register.reg = allocation.allocated_register;

					Inst_Op left_op = register_values[node.math_op.left_node_idx];
					Inst_Op right_op = register_values[node.math_op.right_node_idx];

					if (left_op.type == Op_Reg) {
						if (left_op.reg != result_register.reg)
							Emit_Mov(g.code, result_register, left_op, (u32)(node_type_size * 8));
					}
					else {
						Emit_Mov(g.code, result_register, left_op, (u32)(node_type_size * 8));
					}

					if (node.node_type == Il_Add) {
						Emit_Add(g.code, result_register, right_op, (u32)(node_type_size * 8));
					}

					if (node.node_type == Il_Sub) {
						Emit_Sub(g.code, result_register, right_op, (u32)(node_type_size * 8));
					}

					if (node.node_type == Il_Mul) {
						if (right_op.type == Op_Imm8 || right_op.type == Op_Imm16 || right_op.type == Op_Imm32) {
							Emit_IMul3(g.code, result_register, result_register, right_op, (u32)(node_type_size * 8));
						}
						else {
							Emit_IMul(g.code, result_register, right_op, (u32)(node_type_size * 8));
						}
					}

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += node_type_size;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, result_register, (u32)(node_type_size * 8));

						register_values[i] = spillage_value;
					}
					else {
						register_values[i] = result_register;
					}
				}
				break;
				case Il_Div:
				{
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op result_register;
					result_register.type = Op_Reg;
					result_register.reg = RAX;

					Inst_Op remainder_register;
					remainder_register.type = Op_Reg;
					remainder_register.reg = RDX;

					Inst_Op left_op = register_values[node.math_op.left_node_idx];
					Inst_Op right_op = register_values[node.math_op.right_node_idx];

					if (left_op.type == Op_Reg) {
						if (left_op.reg != result_register.reg)
							Emit_Mov(g.code, result_register, left_op, (u32)(node_type_size * 8));
					}
					else {
						Emit_Mov(g.code, result_register, left_op, (u32)(node_type_size * 8));
					}

					if (node_type_size == 2) Emit_CWD(g.code);
					if (node_type_size == 4) Emit_CDQ(g.code);
					if (node_type_size == 8) Emit_CQO(g.code);

					Inst_Op divisor_op = right_op;

					if (right_op.type == Op_Imm8 || right_op.type == Op_Imm16 || right_op.type == Op_Imm32) {

						divisor_op.type = Op_Reg;
						divisor_op.reg = allocation.allocated_register;

						Emit_Mov(g.code, divisor_op, right_op, (u8)(node_type_size * 8));
					}

					Emit_IDiv(g.code, divisor_op, (u8)(node_type_size * 8));

					if (allocation.allocated_register != 0xff) {
						Emit_Mov(g.code, divisor_op, result_register, (u8)(node_type_size * 8));
						register_values[i] = divisor_op;
					}
					else {
						register_values[i] = result_register;
					}

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += node_type_size;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, register_values[i], (u8)(node_type_size * 8));

						register_values[i] = spillage_value;
					}
				}
				break;
				case Il_Load:
				{
					register_values[i] = register_values[node.load.ptr_node_idx];
				}
				break;
				case Il_Const:
				{
					Inst_Op op;

					op.imm32 = node.constant.as.s4;

					if (node_type_size == 4) op.type = Op_Imm32;
					if (node_type_size == 2) op.type = Op_Imm16;
					if (node_type_size == 1) op.type = Op_Imm8;
					if (node_type_size > 4) op.type = Op_Imm32;

					register_values[i] = op;
				}
				break;
				case Il_Ret:
				{
					if (node.ret.value_node_idx != -1) {
						Inst_Op rax;
						rax.type = Op_Reg;
						rax.reg = RAX;

						Inst_Op returned_value_op = register_values[node.ret.value_node_idx];

						if (returned_value_op.type == Op_Reg) {
							if (returned_value_op.reg != rax.reg)
								Emit_Mov(g.code, rax, returned_value_op, (u32)(node_type_size * 8));
						}
						else {
							Emit_Mov(g.code, rax, returned_value_op, (u32)(node_type_size * 8));
						}

						u32 displacement_location = Emit_Jmp(g.code, 0xffffffff);
						u32 instruction_byte_offset = g.code.count;

						return_relocations.locations[return_relocations.count] = displacement_location;
						return_relocations.starts[return_relocations.count] = instruction_byte_offset;

						return_relocations.count++;

					}
				}
				break;
				case Il_Value_Cmp:
				{
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op left_op = register_values[node.cmp_op.left_node_idx];
					Inst_Op right_op = register_values[node.cmp_op.right_node_idx];

					Inst_Op result_register;
					result_register.type = Op_Reg;
					result_register.reg = allocation.allocated_register;

					if (node.cmp_op.compare_type == Il_Cmp_And || node.cmp_op.compare_type == Il_Cmp_Or)
					{
						Inst_Op tmp_register;
						tmp_register.type = Op_Reg;
						tmp_register.reg = RAX;

						Inst_Op zero;
						zero.type = Op_Imm32;
						zero.imm32 = 0;

						u8 bit_size = (u8)(node_type_size * 8);

						if (bit_size >= 32) zero.type = Op_Imm32;
						if (bit_size == 16) zero.type = Op_Imm16;
						if (bit_size == 8) zero.type = Op_Imm8;

						Emit_Mov(g.code, result_register, left_op, bit_size);
						Emit_Cmp(g.code, result_register, zero, bit_size);
						Emit_SetNE(g.code, result_register);

						Emit_Mov(g.code, tmp_register, right_op, bit_size);
						Emit_Cmp(g.code, tmp_register, zero, bit_size);
						Emit_SetNE(g.code, tmp_register);

						if (node.cmp_op.compare_type == Il_Cmp_And) {
							Emit_And(g.code, result_register, tmp_register, bit_size);
						}
						else {
							Emit_Or(g.code, result_register, tmp_register, bit_size);
						}
					}
					else
					{
						Emit_Mov(g.code, result_register, left_op, (u8)(node_type_size * 8));
						Emit_Cmp(g.code, result_register, right_op, (u8)(node_type_size * 8));

						if (node.cmp_op.compare_type == Il_Cmp_Greater) Emit_SetG(g.code, result_register);
						else if (node.cmp_op.compare_type == Il_Cmp_Lesser) Emit_SetL(g.code, result_register);
						else if (node.cmp_op.compare_type == Il_Cmp_Equal) Emit_SetE(g.code, result_register);
						else if (node.cmp_op.compare_type == Il_Cmp_NotEqual) Emit_SetNE(g.code, result_register);
						else GS_ASSERT_UNIMPL();
					}

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += node_type_size;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, result_register, (u8)(node_type_size * 8));

						register_values[i] = spillage_value;
					}
					else {
						register_values[i] = result_register;
					}
				}
				break;
				case Il_Cond_Branch:
				{
					Inst_Op condition_op = register_values[node.c_branch.condition_node_idx];

					Inst_Op zero;
					zero.type = Op_Imm8;
					zero.imm32 = 0;

					Emit_Cmp(g.code, condition_op, zero, 8);

					u32 displacement_location_a = Emit_JNE(g.code, 0xffffffff);
					u32 instruction_byte_offset_a = g.code.count;

					block_reloactions[node.c_branch.true_case_block_idx].locations[block_reloactions[node.c_branch.true_case_block_idx].count] = displacement_location_a;
					block_reloactions[node.c_branch.true_case_block_idx].starts[block_reloactions[node.c_branch.true_case_block_idx].count] = instruction_byte_offset_a;
					block_reloactions[node.c_branch.true_case_block_idx].count++;

					u32 displacement_location_b = Emit_JE(g.code, 0xffffffff);
					u32 instruction_byte_offset_b = g.code.count;

					block_reloactions[node.c_branch.false_case_block_idx].locations[block_reloactions[node.c_branch.false_case_block_idx].count] = displacement_location_b;
					block_reloactions[node.c_branch.false_case_block_idx].starts[block_reloactions[node.c_branch.false_case_block_idx].count] = instruction_byte_offset_b;
					block_reloactions[node.c_branch.false_case_block_idx].count++;
				}
				break;
				case Il_Branch:
				{
					u32 displacement_location = Emit_Jmp(g.code, 0xffffffff);
					u32 instruction_byte_offset = g.code.count;

					block_reloactions[node.br.block_idx].locations[block_reloactions[node.br.block_idx].count] = displacement_location;
					block_reloactions[node.br.block_idx].starts[block_reloactions[node.br.block_idx].count] = instruction_byte_offset;
					block_reloactions[node.br.block_idx].count++;
				}
				break;
				default:
					ASSERT(nullptr);
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

		u64 stack_size = stack_top_section_size + 32 + 8 + stack_pointer;

		stack_sz.imm32 = (i32)align_to((i64)stack_size, 16);

		*((i32*)&g.code[stack_allocation_byte_idx]) = stack_sz.imm32;

		for (size_t i = 0; i < return_relocations.count; i++)
		{
			*(u32*)&g.code[return_relocations.locations[i]] = (u32)((i32)g.code.count - return_relocations.starts[i]);
		}

		Emit_Add(g.code, reg_rsp, stack_sz, 64);
		Emit_Pop(g.code, reg_rbp);
		Emit_Ret(g.code);

		Inst_Op rsp_displacement;
		rsp_displacement.type = Op_Reg_Disp4;
		rsp_displacement.reg_disp.r = RSP;
		rsp_displacement.reg_disp.disp = 12;

		Inst_Op rax_reg;
		rax_reg.type = Op_Reg;
		rax_reg.reg = R11;
	}

	inline void MC_Gen_Decl_Proc_Symbols(MC_Gen& g)
	{
		g.proc_to_symbol = Array_Reserved<u32>(g.prog->procedures.count);

		for (size_t i = 0; i < g.prog->procedures.count; i++)
		{
			Il_Proc& proc = g.prog->procedures[i];
			Array_Add(g.proc_to_symbol, (u32)g.symbols.count);

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

	inline void MC_Gen_Program_Codegen(MC_Gen& g)
	{
		MC_Gen_Decl_Proc_Symbols(g);

		for (size_t i = 0; i < g.prog->procedures.count; i++)
		{
			Il_Proc& proc = g.prog->procedures[i];
			if (!proc.external)
				MC_Gen_Proc_Codegen(g, proc, (Il_IDX)i);
		}

		int x = 0;
	}

	bool MC_Gen_Run(MC_Gen& g)
	{
		auto begin = std::chrono::high_resolution_clock::now();

		MC_Gen_Program_Codegen(g);
		MC_Gen_Output(g);

		auto end = std::chrono::high_resolution_clock::now();

		GS_CORE_INFO("backend took: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

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

		fs_path obj_path = g.output_path.data;
		fs_path obj_directory_path = g.output_path.data;
		obj_directory_path.remove_filename();
		obj_path.replace_extension(".obj");

		std::filesystem::create_directories(obj_directory_path);

		{
			std::ofstream file(obj_path.string(), std::ios::binary);
			file.write((const char*)g.coff_obj.data, g.coff_obj.count);
		}
	}

	MC_Gen MC_Gen_Make(MC_Gen_Spec spec, Il_Program* program)
	{
		MC_Gen g = { 0 };
		g.output_path = String_Copy(spec.output_path);
		g.prog = program;
		g.ts = program->type_system;

		ASSERT(g.prog);
		ASSERT(g.ts);

		return g;
	}
}