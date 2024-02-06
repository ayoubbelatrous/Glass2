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
		u8 temp_register = 0xff;
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

			if (vreg_lifetimes[vreg_idx] <= 0) {
				if (allocations[vreg_idx].allocated_register != 0xff) {
					u8 allocated_register = allocations[vreg_idx].allocated_register;
					allocated[allocated_register] = 0;
					reg_to_v_reg[allocated_register] = (Il_IDX)-1;
				}

				if (allocations[vreg_idx].temp_register != 0xff) {
					u8 temp_register = allocations[vreg_idx].temp_register;
					allocated[temp_register] = 0;
					reg_to_v_reg[temp_register] = (Il_IDX)-1;
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

				if (node.node_type == Il_ArrayElementPtr)
				{
					allocations[idx].allocated_register = allocate_register(idx);
					allocations[idx].temp_register = temp_allocate_register(idx);
					use_register(node.aep.index_node_idx);
					use_register(node.aep.ptr_node_idx);
				}

				if (node.node_type == Il_StructElementPtr)
				{
					use_register(node.element_ptr.ptr_node_idx);
					allocations[idx].allocated_register = temp_allocate_register(idx);
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

				if (node.node_type == Il_Struct_Initializer)
				{
					u8 allocated_register = temp_allocate_register(idx);
					allocations[idx].allocated_register = allocated_register;

					ASSERT(node.si.member_count < SI_SMALL_COUNT);

					for (size_t i = 0; i < node.si.member_count; i++)
					{
						use_register(node.si.members_value_nodes[i]);
					}
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

					if (vreg_lifetimes[node.load.ptr_node_idx] - 1 <= 0) {
						allocations[idx].allocated_register = allocate_register(idx);
						reg_to_v_reg[allocations[node.load.ptr_node_idx].allocated_register] = idx;
					}
					else {
						use_register(node.load.ptr_node_idx);
					}
				}

				if (node.node_type == Il_Param)
				{
					allocations[idx].allocated_register = call_conv_parameter_registers[node.param.index];
				}

				if (node.node_type == Il_Cond_Branch)
				{
					allocations[idx].allocated_register = temp_allocate_register(idx);
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
						allocated[tmp_register] = 0;

					use_register(node.cmp_op.left_node_idx);
					use_register(node.cmp_op.right_node_idx);
				}

				if (node.node_type == Il_Global_Address || node.node_type == Il_Proc_Address || node.node_type == Il_String) {
					u8 allocated_register = allocate_register(idx);
					allocations[idx].allocated_register = allocated_register;
				}

				if (node.node_type == Il_Call || node.node_type == Il_Call_Ptr) {

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

					GS_Type* signature = nullptr;

					if (node.node_type == Il_Call) {
						signature = g.prog->procedures[node.call.proc_idx].signature;
					}
					else {
						signature = &g.prog->type_system->type_storage[node.call.signature];
						use_register(node.call.proc_idx);
					}

					if (signature->proc.return_type != g.prog->type_system->void_Ty)
					{
						allocated[needed_register] = 1;
					}
					else {
						allocated[needed_register] = 0;
					}
				}

				if (node.node_type == Il_Cond_Branch) {
					use_register(node.c_branch.condition_node_idx);
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

		enum Value_Flag
		{
			VF_Address = BIT(1),
			VF_Stack = BIT(2),
		};

		Inst_Op register_values[REG_BUFS_SZ] = {};
		u8 register_flags[REG_BUFS_SZ] = {};

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
				case Il_Proc_Address:
				{
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op result_op;
					result_op.type = Op_Reg;
					result_op.reg = allocation.allocated_register;

					Inst_Op empty_disp_op;
					empty_disp_op.type = Op_Disp4;
					empty_disp_op.disp_4 = 0;

					MC_Relocation relocation;
					relocation.reloaction_offset = Emit_Lea(g.code, result_op, empty_disp_op, 64);
					relocation.symbol_idx = g.proc_to_symbol[node.proc_address.proc_idx];
					relocation.relocation_type = IMAGE_REL_AMD64_REL32;

					Array_Add(g.code_relocations, relocation);

					register_values[i] = result_op;

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += 8;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, result_op, 8 * 8);

						register_values[i] = spillage_value;
					}
				}
				break;
				case Il_Global_Address:
				{
					Reg_Allocation allocation = register_allocations[i];

					Il_Global& global = g.prog->globals[node.global_address.global_idx];

					auto global_idx = node.global_address.global_idx;

					Inst_Op result_op;
					result_op.type = Op_Reg;
					result_op.reg = allocation.allocated_register;

					Inst_Op rel_disp4;
					rel_disp4.type = Op_Disp4;
					rel_disp4.disp_4 = g.global_section_offsets[global_idx];

					MC_Relocation relocation;
					relocation.reloaction_offset = Emit_Lea(g.code, result_op, rel_disp4, 64);
					relocation.relocation_type = IMAGE_REL_AMD64_REL32;

					if (global.read_only) {
						relocation.symbol_idx = g.rdata_symbol_index;
					}
					else {
						relocation.symbol_idx = g.data_symbol_index;
					}

					Array_Add(g.code_relocations_sections, relocation);

					Inst_Op register_value_op;
					register_value_op.type = Op_Reg_Disp4;
					register_value_op.reg_disp.r = allocation.allocated_register;
					register_value_op.reg_disp.disp = 0;

					register_values[i] = register_value_op;

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += 8;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, result_op, 8 * 8);

						register_values[i] = spillage_value;
					}
					else {
						register_flags[i] = VF_Address;
					}
				}
				break;
				case Il_Call:
				case Il_Call_Ptr:
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
							if (register_flags[argument_node_idx] & VF_Address && register_values[argument_node_idx].type == Op_Reg_Disp4) {
								Emit_Lea(g.code, argument_register, register_values[argument_node_idx], 64);
							}
							else {
								Emit_Mov(g.code, argument_register, register_values[argument_node_idx], (u32)(argument_type_size * 8));
							}
						}
						else {

							Inst_Op tmp_register;
							tmp_register.type = Op_Reg;
							tmp_register.reg = RAX;

							stack_top_pointer += 8;

							Inst_Op argument_op = register_values[argument_node_idx];

							if (argument_op.type != Op_Reg && (argument_op.type != Op_Imm8 && argument_op.type != Op_Imm16 && argument_op.type != Op_Imm32)) {
								Emit_Mov(g.code, tmp_register, argument_op, (u32)(argument_type_size * 8));
								argument_op = tmp_register;
							}

							Emit_Mov(g.code, stack_top_offset, argument_op, (u32)(argument_type_size * 8));
							stack_top_offset.reg_disp.disp += 8;
						}
					}

					stack_top_section_size = std::max(stack_top_pointer, stack_top_section_size);

					if (node.node_type == Il_Call)
					{
						MC_Relocation relocation;
						relocation.reloaction_offset = Emit_Call(g.code, 0);
						relocation.symbol_idx = g.proc_to_symbol[node.call.proc_idx];
						relocation.relocation_type = IMAGE_REL_AMD64_REL32;

						Array_Add(g.code_relocations, relocation);
					}
					else {
						Emit_Call(g.code, register_values[node.call.proc_idx]);
					}

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
					register_flags[i] = VF_Address | VF_Stack;
				}
				break;
				case Il_ZI:
					break;
				case Il_Struct_Initializer:
				{
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op result_op;
					result_op.type = Op_Reg_Disp4;
					result_op.reg_disp.r = RBP;

					stack_pointer += node_type_size;
					result_op.reg_disp.disp = -(i32)stack_pointer;

					register_values[i] = result_op;

					ASSERT(node.si.member_count < SI_SMALL_COUNT);

					auto type_system = g.prog->type_system;

					GS_Struct& strct = type_system->struct_storage[type_system->type_name_storage[node_type->basic.type_name_id].struct_id];

					Inst_Op tmp_reg;
					tmp_reg.type = Op_Reg;
					tmp_reg.reg = allocation.allocated_register;

					for (size_t i = 0; i < node.si.member_count; i++)
					{
						GS_Type* member_type = strct.members[i];
						auto member_offset = strct.offsets[i];

						auto member_type_size = TypeSystem_Get_Type_Size(*type_system, member_type);

						auto member_type_size_bits = member_type_size * 8;

						Inst_Op member_value = register_values[node.si.members_value_nodes[i]];

						if (member_type_size <= 8) {

							Inst_Op member_op = result_op;
							member_op.reg_disp.disp += member_offset;

							if (member_value.type == Op_Reg_Disp4) {
								Emit_Mov(g.code, tmp_reg, member_value, member_type_size_bits);
								member_value = tmp_reg;
							}

							Emit_Mov(g.code, member_op, member_value, member_type_size_bits);
						}
						else {
							GS_ASSERT_UNIMPL();
						}
					}
				}
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

						Inst_Op tmp_op;
						tmp_op.type = Op_Reg;
						tmp_op.reg = allocation.allocated_register;

						if (node_type_size <= 8) {

							if (register_flags[node.store.value_node_idx] & VF_Address && value_op.type == Op_Reg_Disp4) {
								Emit_Lea(g.code, tmp_op, value_op, 64);
								value_op = tmp_op;
							}

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
					Reg_Allocation allocation = register_allocations[i];

					if (allocation.spilled) {

						Inst_Op pointer_op = register_values[node.load.ptr_node_idx];

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						Inst_Op tmp_reg;
						tmp_reg.type = Op_Reg;
						tmp_reg.reg = allocation.allocated_register;

						stack_pointer += 8;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						if (node_type_size > 8)
						{
							if (register_flags[node.load.ptr_node_idx] & VF_Address) {
								Emit_Lea(g.code, tmp_reg, pointer_op, 64);
							}
							else {
								Emit_Mov(g.code, tmp_reg, pointer_op, node_type_size * 8);
							}

							Emit_Mov(g.code, spillage_value, tmp_reg, node_type_size * 64);
						}
						else {
							Emit_Mov(g.code, tmp_reg, pointer_op, node_type_size * 8);
							Emit_Mov(g.code, spillage_value, tmp_reg, node_type_size * 8);
						}

						register_values[i] = spillage_value;
					}
					else {
						Inst_Op pointer_op = register_values[node.load.ptr_node_idx];

						if (pointer_op.type == Op_Reg) {

							Inst_Op new_pointer_op;
							new_pointer_op.type = Op_Reg_Disp4;
							new_pointer_op.reg_disp.r = pointer_op.reg;
							new_pointer_op.reg_disp.disp = 0;

							pointer_op = new_pointer_op;
						}

						register_values[i] = pointer_op;
					}
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
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op condition_op = register_values[node.c_branch.condition_node_idx];

					Inst_Op zero;
					zero.type = Op_Imm8;
					zero.imm32 = 0;

					if (condition_op.type == Op_Imm8 || condition_op.type == Op_Imm16 || condition_op.type == Op_Imm32)
					{
						Inst_Op tmp_reg;
						tmp_reg.type = Op_Reg;
						tmp_reg.reg = allocation.allocated_register;

						Emit_Mov(g.code, tmp_reg, condition_op, 8);
						condition_op = tmp_reg;
					}

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
				case Il_StructElementPtr:
				{
					Inst_Op pointer_op = register_values[node.element_ptr.ptr_node_idx];

					auto type_system = g.prog->type_system;

					GS_Struct& strct = type_system->struct_storage[type_system->type_name_storage[node_type->basic.type_name_id].struct_id];

					pointer_op.reg_disp.disp += strct.offsets[node.element_ptr.element_idx];

					register_values[i] = pointer_op;
					register_flags[i] = VF_Address;
				}
				break;
				case Il_ArrayElementPtr:
				{
					Reg_Allocation allocation = register_allocations[i];

					Inst_Op pointer_op = register_values[node.aep.ptr_node_idx];

					Inst_Op result;
					result.type = Op_Reg;
					result.reg = allocation.allocated_register;

					u8 bit_size = (u8)node_type_size * 8;

					GS_Type* index_type = &g.prog->type_system->type_storage[proc.instruction_storage[node.aep.index_node_idx].type_idx];
					auto index_type_size = TypeSystem_Get_Type_Size(*g.prog->type_system, index_type);

					if (register_flags[node.aep.ptr_node_idx] & VF_Address)
						Emit_Lea(g.code, result, pointer_op, 64);
					else
						Emit_Mov(g.code, result, pointer_op, 64);

					Inst_Op element_size_op;
					element_size_op.type = Op_Imm32;
					element_size_op.imm32 = node_type_size;

					Inst_Op tmp_reg;
					tmp_reg.type = Op_Reg;
					tmp_reg.reg = allocation.temp_register;

					u8 index_type_bit_size = (u8)index_type_size * 8;

					if (register_values[node.aep.index_node_idx].type != Op_Imm8 && register_values[node.aep.index_node_idx].type != Op_Imm16 && register_values[node.aep.index_node_idx].type != Op_Imm32) {
						Emit_IMul3(g.code, tmp_reg, register_values[node.aep.index_node_idx], element_size_op, index_type_bit_size);
						Emit_Add(g.code, result, tmp_reg, 64);
					}
					else {
						element_size_op.imm32 *= register_values[node.aep.index_node_idx].imm32;
						Emit_Add(g.code, result, element_size_op, 64);
					}

					Inst_Op register_value;
					register_value.type = Op_Reg_Disp4;
					register_value.reg_disp.r = allocation.allocated_register;
					register_value.reg_disp.disp = 0;

					register_values[i] = register_value;

					if (allocation.spilled) {

						Inst_Op spillage_value;
						spillage_value.type = Op_Reg_Disp4;
						spillage_value.reg_disp.r = RBP;

						stack_pointer += 8;
						spillage_value.reg_disp.disp = -(i32)stack_pointer;

						Emit_Mov(g.code, spillage_value, result, 8 * 8);

						register_values[i] = spillage_value;
					}
					else {
						register_flags[i] = VF_Address;
					}
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

				auto type_size = TypeSystem_Get_Type_Size(*g.prog->type_system, global.type);

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
				MC_Gen_Proc_Codegen(g, proc, (Il_IDX)i);
		}

		int x = 0;
	}

	bool MC_Gen_Run(MC_Gen& g)
	{
		auto begin = std::chrono::high_resolution_clock::now();
		MC_Gen_Program_Codegen(g);
		auto end = std::chrono::high_resolution_clock::now();

		MC_Gen_Output(g);

		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

		double back_time_f = microseconds;
		back_time_f /= 1000000.0;

		GS_CORE_INFO("backend took: {} us", microseconds);
		GS_CORE_INFO("backend took: {} s", back_time_f);

		return true;
	}

	void MC_Gen_Output(MC_Gen& g)
	{
#define COFF_MACHINE (u16)IMAGE_FILE_MACHINE_AMD64

		g.coff_obj = Array_Reserved<u8>(1024 * 1024 * 128);

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