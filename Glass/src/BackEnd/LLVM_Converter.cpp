#include "pch.h"

#include "LLVM_Converter.h"

namespace Glass
{
	LLVM_Converter LLVM_Converter_Make(LLVM_Converter_Spec spec, Il_Program* program)
	{
		LLVM_Converter converter;
		converter.prog = program;
		converter.spec = spec;
		return converter;
	}

	llvm::Type* to_llvm(LLVM_Converter& lc, GS_Type* ty) {

		auto it = lc.type_to_llvm.find(ty);

		if (it != lc.type_to_llvm.end()) {
			return it->second;
		}

		switch (ty->kind)
		{
		case Type_Basic:
		{
			auto type_size = get_type_size(ty);
			auto type_flags = get_type_flags(ty);

			if (!(type_flags & TN_Struct_Type)) {
				switch (type_size)
				{
				case 1:
					lc.type_to_llvm[ty] = lc.llvm_i8;
					break;
				case 2:
					lc.type_to_llvm[ty] = lc.llvm_i16;
					break;
				case 4:
					lc.type_to_llvm[ty] = lc.llvm_i32;
					break;
				case 8:
					lc.type_to_llvm[ty] = lc.llvm_i64;
					break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}

				return lc.type_to_llvm[ty];
			}
			else {

				Array<llvm::Type*> members;

				auto& type_name = get_ts().type_name_storage[ty->basic.type_name_id];
				GS_Struct& strct = get_ts().struct_storage[type_name.struct_id];

				for (u64 i = 0; i < strct.members.count; i++) {
					Array_Add(members, to_llvm(lc, strct.members[i]));
				}

				lc.type_to_llvm[ty] = llvm::StructType::create(llvm::ArrayRef(members.data, members.count), type_name.name.data);

				return lc.type_to_llvm[ty];
			}
		}
		break;
		case Type_Pointer:
		{
			lc.type_to_llvm[ty] = lc.llvm_ptr;
			return lc.type_to_llvm[ty];
		}
		break;
		case Type_Proc:
		{
			lc.type_to_llvm[ty] = lc.llvm_ptr;

			return lc.type_to_llvm[ty];
		}
		case Type_Array:
		{
			lc.type_to_llvm[ty] = llvm::ArrayType::get(to_llvm(lc, ty->array.element_type), ty->array.size);
			return lc.type_to_llvm[ty];
		}
		case Type_Dyn_Array:
		{
			lc.type_to_llvm[ty] = lc.llvm_Array;
			return lc.llvm_Array;
		}
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return nullptr;
	}

	llvm::FunctionType* LLVMC_Get_Func_Type(LLVM_Converter& lc, GS_Type* signature, bool variadic) {

		Array<llvm::Type*> paramters;
		for (u64 i = 0; i < signature->proc.params.count; i++) {

			auto param_ty = signature->proc.params[i];

			auto param_ty_size = get_type_size(param_ty);
			auto param_ty_flags = get_type_flags(param_ty);

			if (param_ty_flags & TN_Struct_Type || param_ty->kind == Type_Array) {
				if (param_ty_size == 2)
					Array_Add(paramters, lc.llvm_i16);
				else if (param_ty_size == 4)
					Array_Add(paramters, lc.llvm_i32);
				else if (param_ty_size == 8)
					Array_Add(paramters, lc.llvm_i64);
				else
					Array_Add(paramters, lc.llvm_ptr);
			}
			else {
				Array_Add(paramters, to_llvm(lc, param_ty));
			}
		}

		auto return_type = to_llvm(lc, signature->proc.return_type);

		return llvm::FunctionType::get(return_type, llvm::ArrayRef(paramters.data, paramters.count), variadic);
	}

	llvm::AllocaInst* LLVMC_CreateAlloca(LLVM_Converter& lc, llvm::Type* ty)
	{
		llvm::Function* function = lc.llvm_builder->GetInsertBlock()->getParent();

		llvm::IRBuilder<> TmpB(&function->getEntryBlock(),
			function->getEntryBlock().begin());
		auto alloca = TmpB.CreateAlloca(ty);
		return alloca;
	}

#define REG_BUFS_SZ 65553
	llvm::Value* register_values_buffer[REG_BUFS_SZ] = {};
	GS_Type* register_types_buffer[REG_BUFS_SZ] = {};

	void LLVMC_Proc_Codegen(LLVM_Converter& lc, Il_Proc& proc, Il_IDX proc_idx)
	{
		memset(register_values_buffer, 0, sizeof(register_values_buffer));
		memset(register_types_buffer, 0, sizeof(register_types_buffer));

		auto regv = register_values_buffer;
		auto regt = register_types_buffer;

		llvm::Function* func = lc.proc_to_llvm[proc_idx];

		llvm::BasicBlock* llvm_blocks[1024] = { 0 };

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			llvm::BasicBlock* bb = llvm::BasicBlock::Create(*lc.llvm_ctx, block.name.data, func);
			lc.llvm_builder->SetInsertPoint(bb);
			llvm_blocks[block_idx] = bb;
		}

		for (size_t block_idx = 0; block_idx < proc.blocks.count; block_idx++)
		{
			Il_Block& block = proc.blocks[block_idx];

			llvm::BasicBlock* bb = llvm_blocks[block_idx];
			lc.llvm_builder->SetInsertPoint(bb);

			bool terminator_encountered = false;

			for (size_t j = 0; j < block.instructions.count; j++)
			{
				Il_IDX idx = block.instructions[j];
				Il_Node node = proc.instruction_storage[idx];

				GS_Type* type = get_type(node.type_idx);
				auto type_size = get_type_size(type);
				auto type_flags = get_type_flags(type);

				regt[idx] = type;

				// 				if (idx == 19) __debugbreak();

				if (terminator_encountered)
				{
					break;
				}

				switch (node.node_type)
				{
				case Il_Param:
				{
					regv[idx] = func->getArg(node.param.index);
				}
				break;
				case Il_Alloca:
				{
					auto alloca_type = get_type_at(node.aloca.type_idx);
					regv[idx] = lc.llvm_builder->CreateAlloca(to_llvm(lc, alloca_type));
				}
				break;
				case Il_Struct_Initializer:
				{
					auto llvm_Ty = to_llvm(lc, type);

					llvm::Value* alloca = LLVMC_CreateAlloca(lc, llvm_Ty);

					auto members_value_nodes_ptr = node.si.members_value_nodes;

					if (node.si.member_count > SI_SMALL_COUNT) {
						members_value_nodes_ptr = node.si.members_value_nodes_ptr;
					}

					for (size_t i = 0; i < node.si.member_count; i++)
					{
						auto member = lc.llvm_builder->CreateStructGEP(llvm_Ty, alloca, i);
						lc.llvm_builder->CreateStore(regv[members_value_nodes_ptr[i]], member);
					}

					regv[idx] = lc.llvm_builder->CreateLoad(llvm_Ty, alloca);
				}
				break;
				case Il_Load:
				{
					regv[idx] = lc.llvm_builder->CreateLoad(to_llvm(lc, type), regv[node.load.ptr_node_idx]);
				}
				break;
				case Il_Store:
				{
					ASSERT(regv[node.store.ptr_node_idx]);
					ASSERT(regv[node.store.value_node_idx]);
					regv[idx] = lc.llvm_builder->CreateStore(regv[node.store.value_node_idx], regv[node.store.ptr_node_idx]);
				}
				break;
				case Il_StructElementPtr:
				{
					regv[idx] = lc.llvm_builder->CreateStructGEP(to_llvm(lc, type), regv[node.element_ptr.ptr_node_idx], node.element_ptr.element_idx);

					ASSERT(type->kind == Type_Basic);

					GS_Struct& strct = get_struct(type);

					regt[idx] = get_pointer_type(strct.members[node.element_ptr.element_idx], 1);
				}
				break;
				case Il_ArrayElementPtr:
				{
					regv[idx] = lc.llvm_builder->CreateGEP(to_llvm(lc, type), regv[node.aep.ptr_node_idx], regv[node.aep.index_node_idx]);
					regt[idx] = get_pointer_type(type, 1);
				}
				break;
				case Il_Call:
				case Il_Call_Ptr:
				{
					Array<llvm::Value*> arguments;

					bool variadic = false;

					if (node.node_type == Il_Call) {
						if (lc.prog->procedures[node.call.proc_idx].variadic) {
							variadic = true;
						}
					}

					Il_IDX* arguments_ptr = node.call.arguments;

					if (node.call.argument_count > SMALL_ARG_COUNT) {
						arguments_ptr = node.call.arguments_ptr;
					}

					for (size_t i = 0; i < node.call.argument_count; i++)
					{
						Il_IDX argument_node_idx = arguments_ptr[i];
						GS_Type* argument_type = regt[argument_node_idx];

						auto argument_size = get_type_size(argument_type);
						auto argument_flags = get_type_flags(argument_type);

						if ((argument_flags & TN_Struct_Type || argument_type->kind == Type_Array) && argument_size <= 8) {

							llvm::Type* adjusted_type = nullptr;

							if (argument_size == 8)
								adjusted_type = lc.llvm_i64;
							if (argument_size == 4)
								adjusted_type = lc.llvm_i32;
							if (argument_size == 2)
								adjusted_type = lc.llvm_i16;

							auto tmp_alloca = LLVMC_CreateAlloca(lc, adjusted_type);

							lc.llvm_builder->CreateStore(regv[argument_node_idx], tmp_alloca);

							Array_Add(arguments, (llvm::Value*)lc.llvm_builder->CreateLoad(adjusted_type, tmp_alloca));
						}
						else {
							if (argument_size == 4 && argument_flags & TN_Float_Type && variadic) {
								Array_Add(arguments, lc.llvm_builder->CreateFPExt(regv[argument_node_idx], lc.llvm_double));
							}
							else {
								Array_Add(arguments, regv[argument_node_idx]);
							}
						}
					}
					if (node.node_type == Il_Call)
						regv[idx] = lc.llvm_builder->CreateCall(lc.proc_to_llvm[node.call.proc_idx], llvm::ArrayRef(arguments.data, arguments.count));
					else
						regv[idx] = lc.llvm_builder->CreateCall(LLVMC_Get_Func_Type(lc, get_type_at(node.call.signature), false), regv[node.call.proc_idx], llvm::ArrayRef(arguments.data, arguments.count));
				}
				break;
				case Il_Sub:
				{
					if (type_flags & TN_Float_Type)
						regv[idx] = lc.llvm_builder->CreateFSub(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
					else
						regv[idx] = lc.llvm_builder->CreateSub(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
				}
				break;
				case Il_Add:
				{
					if (type_flags & TN_Float_Type)
						regv[idx] = lc.llvm_builder->CreateFAdd(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
					else
						regv[idx] = lc.llvm_builder->CreateAdd(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
				}
				break;
				case Il_Mul:
				{
					if (type_flags & TN_Float_Type)
						regv[idx] = lc.llvm_builder->CreateFMul(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
					else
						regv[idx] = lc.llvm_builder->CreateMul(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
				}
				break;
				case Il_Div:
				{
					if (type_flags & TN_Float_Type)
						regv[idx] = lc.llvm_builder->CreateFDiv(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
					else
						if (type_flags & TN_Unsigned_Type)
							regv[idx] = lc.llvm_builder->CreateUDiv(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
						else
							regv[idx] = lc.llvm_builder->CreateSDiv(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
				}
				break;
				case Il_Bit_And:
					regv[idx] = lc.llvm_builder->CreateAnd(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
					break;
				case Il_Bit_Or:
					regv[idx] = lc.llvm_builder->CreateOr(regv[node.math_op.left_node_idx], regv[node.math_op.right_node_idx]);
					break;
				case Il_Value_Cmp:
				{
					bool un_signed_type = type_flags & TN_Unsigned_Type;

					llvm::Value* result = nullptr;

					auto lhs = regv[node.cmp_op.left_node_idx];
					auto rhs = regv[node.cmp_op.right_node_idx];

					if (node.cmp_op.compare_type == Il_Cmp_Or || node.cmp_op.compare_type == Il_Cmp_And) {
						lhs = lc.llvm_builder->CreateICmpUGT(lhs, llvm::ConstantInt::get(lhs->getType(), 0));
						rhs = lc.llvm_builder->CreateICmpUGT(rhs, llvm::ConstantInt::get(rhs->getType(), 0));

						if (node.cmp_op.compare_type == Il_Cmp_Or)
							result = lc.llvm_builder->CreateOr(lhs, rhs);
						if (node.cmp_op.compare_type == Il_Cmp_And)
							result = lc.llvm_builder->CreateAnd(lhs, rhs);
					}
					else {

						llvm::CmpInst::Predicate cmp_inst;

						if (node.cmp_op.compare_type == Il_Cmp_Equal)
							cmp_inst = llvm::CmpInst::ICMP_EQ;
						else if (node.cmp_op.compare_type == Il_Cmp_NotEqual)
							cmp_inst = llvm::CmpInst::ICMP_NE;
						else if (node.cmp_op.compare_type == Il_Cmp_Greater)
							if (un_signed_type)
								cmp_inst = llvm::CmpInst::ICMP_UGT;
							else
								cmp_inst = llvm::CmpInst::ICMP_SGT;
						else if (node.cmp_op.compare_type == Il_Cmp_Lesser)
							if (un_signed_type)
								cmp_inst = llvm::CmpInst::ICMP_ULT;
							else
								cmp_inst = llvm::CmpInst::ICMP_SLT;
						else {
							GS_ASSERT_UNIMPL();
						}

						result = lc.llvm_builder->CreateCmp(cmp_inst, lhs, rhs);
					}

					regv[idx] = lc.llvm_builder->CreateZExt(result, lc.llvm_i8);
				}
				break;
				case Il_Cond_Branch:
				{
					auto compare = lc.llvm_builder->CreateCmp(llvm::CmpInst::ICMP_NE, regv[node.c_branch.condition_node_idx], llvm::ConstantInt::get(lc.llvm_i8, 0));
					lc.llvm_builder->CreateCondBr(compare, llvm_blocks[node.c_branch.true_case_block_idx], llvm_blocks[node.c_branch.false_case_block_idx]);
					terminator_encountered = true;
				}
				break;
				case Il_Branch:
				{
					lc.llvm_builder->CreateBr(llvm_blocks[node.br.block_idx]);
					terminator_encountered = true;
				}
				break;
				case Il_Const:
				{
					if (type_flags & TN_Float_Type) {
						regv[idx] = llvm::ConstantFP::get(to_llvm(lc, type), node.constant.as.f64);
					}
					else {
						if (type_flags & TN_Pointer_Type) {
							if (node.constant.as.ptr == 0)
							{
								regv[idx] = llvm::ConstantPointerNull::get((llvm::PointerType*)to_llvm(lc, type));
							}
							else {
								regv[idx] = lc.llvm_builder->CreateIntToPtr(llvm::ConstantInt::get(lc.llvm_i64, node.constant.as.us8), lc.llvm_ptr);
							}
						}
						else {
							regv[idx] = llvm::ConstantInt::get(to_llvm(lc, type), node.constant.as.us8);
						}
					}
				}
				break;
				case Il_ZI:
				{
					regv[idx] = llvm::ConstantAggregateZero::get(to_llvm(lc, type));
				}
				break;
				case Il_String:
				{
					ASSERT(node.string.str.data[node.string.str.count] == 0);
					regv[idx] = lc.llvm_builder->CreateGlobalStringPtr(llvm::StringRef(node.string.str.data, node.string.str.count + 1));
				}
				break;
				case Il_Ret:
					if (get_ts().void_Ty == type) {
						lc.llvm_builder->CreateRetVoid();
					}
					else {
						lc.llvm_builder->CreateRet(register_values_buffer[node.ret.value_node_idx]);
					}
					terminator_encountered = true;
					break;
				case Il_Proc_Address:
					regv[idx] = lc.proc_to_llvm[node.proc_address.proc_idx];
					break;
				case Il_Global_Address:
					regv[idx] = lc.global_to_llvm[node.global_address.global_idx];
					break;
				case Il_Cast:
					if (node.cast.cast_type == Il_Cast_Ptr) {
						regv[idx] = regv[node.cast.castee_node_idx];
					}
					else {
						auto cast_from_type = get_type_at(node.cast.from_type_idx);
						auto cast_from_type_llvm = to_llvm(lc, cast_from_type);

						auto cast_to_type_flags = get_type_flags(cast_from_type);

						auto llvm_Ty = to_llvm(lc, type);

						llvm::Instruction::CastOps cast_ops;

						if (node.cast.cast_type == Il_Cast_Int2Float) {
							if (cast_to_type_flags & TN_Unsigned_Type)
							{
								regv[idx] = lc.llvm_builder->CreateUIToFP(regv[node.cast.castee_node_idx], llvm_Ty);
								cast_ops = llvm::Instruction::CastOps::UIToFP;
							}
							else
							{
								regv[idx] = lc.llvm_builder->CreateSIToFP(regv[node.cast.castee_node_idx], llvm_Ty);
								cast_ops = llvm::Instruction::CastOps::SIToFP;
							}
						}
						else if (node.cast.cast_type == Il_Cast_FloatExt) {
							regv[idx] = lc.llvm_builder->CreateFPExt(regv[node.cast.castee_node_idx], llvm_Ty);
							cast_ops = llvm::Instruction::CastOps::FPExt;
						}
						else if (node.cast.cast_type == Il_Cast_FloatTrunc) {
							regv[idx] = lc.llvm_builder->CreateFPTrunc(regv[node.cast.castee_node_idx], llvm_Ty);
							cast_ops = llvm::Instruction::CastOps::FPTrunc;
						}
						else if (node.cast.cast_type == Il_Cast_IntSExt) {
							regv[idx] = lc.llvm_builder->CreateSExt(regv[node.cast.castee_node_idx], llvm_Ty);
							cast_ops = llvm::Instruction::CastOps::SExt;
						}
						else if (node.cast.cast_type == Il_Cast_IntZExt) {
							regv[idx] = lc.llvm_builder->CreateZExt(regv[node.cast.castee_node_idx], llvm_Ty);
							cast_ops = llvm::Instruction::CastOps::ZExt;
						}
						else if (node.cast.cast_type == Il_Cast_IntTrunc) {
							regv[idx] = lc.llvm_builder->CreateTrunc(regv[node.cast.castee_node_idx], llvm_Ty);
							cast_ops = llvm::Instruction::CastOps::Trunc;
						}
						else {
							GS_ASSERT_UNIMPL();
						}
					}
					break;
				default:
					GS_ASSERT_UNIMPL();
					break;
				}
			}

			if (!terminator_encountered) {
				lc.llvm_builder->CreateBr(llvm_blocks[block_idx + 1]);
			}
		}

	}

	llvm::Value* LLVMC_Initializer_Codegen(LLVM_Converter& lc, Array<Il_Node> nodes, Il_IDX node_idx)
	{
		Il_Node node = nodes[node_idx];

		GS_Type* type = get_type_at(node.type_idx);
		auto type_flags = get_type_flags(type);

		switch (node.node_type)
		{
		case Il_Const:
			if (type_flags & TN_Float_Type) {
				return llvm::ConstantFP::get(to_llvm(lc, type), node.constant.as.f64);
			}
			if (type_flags & TN_Struct_Type) {
				GS_ASSERT_UNIMPL();
			}
			else {
				if (node.constant.as.ptr == 0 && type_flags & TN_Pointer_Type) {
					return llvm::ConstantPointerNull::get((llvm::PointerType*)to_llvm(lc, type));
				}
				else {
					return llvm::ConstantInt::get(to_llvm(lc, type), node.constant.as.us8);
				}
			}
			break;
		case Il_Global_Address:
			return lc.global_to_llvm[node.global_address.global_idx];
			break;
		case Il_ArrayElementPtr:
		{
			auto ptr = (llvm::Constant*)LLVMC_Initializer_Codegen(lc, nodes, node.aep.ptr_node_idx);
			auto index = (llvm::Constant*)LLVMC_Initializer_Codegen(lc, nodes, node.aep.index_node_idx);
			return llvm::ConstantExpr::getGetElementPtr(to_llvm(lc, type), ptr, index);
		}
		break;
		case Il_Struct_Initializer:
		{
			auto llvm_Ty = to_llvm(lc, type);

			auto members_value_nodes_ptr = node.si.members_value_nodes;

			if (node.si.member_count > SI_SMALL_COUNT) {
				members_value_nodes_ptr = node.si.members_value_nodes_ptr;
			}

			Array<llvm::Constant*> struct_body;

			for (size_t i = 0; i < node.si.member_count; i++)
			{
				Array_Add(struct_body, (llvm::Constant*)LLVMC_Initializer_Codegen(lc, nodes, members_value_nodes_ptr[i]));
			}

			return llvm::ConstantStruct::get((llvm::StructType*)llvm_Ty, llvm::ArrayRef{ struct_body.data,struct_body.count });
		}
		break;
		case Il_Array_Initializer:
		{
			Array<llvm::Constant*> array_body;

			Il_Node_Array_Init ai = node.ai;

			for (u64 i = 0; i < ai.element_count; i++)
			{
				Array_Add(array_body, (llvm::Constant*)LLVMC_Initializer_Codegen(lc, nodes, ai.element_values[i]));
			}

			auto llvm_array_type = to_llvm(lc, type);

			return llvm::ConstantArray::get((llvm::ArrayType*)llvm_array_type, llvm::ArrayRef{ array_body.data,array_body.count });
		}
		break;
		case Il_String:
		{
			ASSERT(node.string.str.data[node.string.str.count] == 0);
			return lc.llvm_builder->CreateGlobalStringPtr(llvm::StringRef(node.string.str.data, node.string.str.count + 1), "", 0, lc.llvm_module);
		}
		break;
		case Il_Cast:
		{
			auto llvm_type = to_llvm(lc, type);

			switch (node.cast.cast_type)
			{
			case Il_Cast_Ptr:
			{
				return (llvm::Constant*)LLVMC_Initializer_Codegen(lc, nodes, node.cast.castee_node_idx);
			}
			break;
			case Il_Cast_Ptr2Int:
			{
				return llvm::ConstantExpr::getPtrToInt((llvm::Constant*)LLVMC_Initializer_Codegen(lc, nodes, node.cast.castee_node_idx), llvm_type);
			}
			break;
			default:
				GS_ASSERT_UNIMPL();
				break;
			}
		}
		break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return nullptr;
	}

	void LLVMC_Codegen(LLVM_Converter& lc) {

		for (size_t i = 0; i < lc.prog->procedures.count; i++)
		{
			Il_Proc& proc = lc.prog->procedures[i];

			llvm::Function* function = llvm::Function::Create(LLVMC_Get_Func_Type(lc, proc.signature, proc.variadic), llvm::Function::ExternalLinkage, proc.proc_name.data, lc.llvm_module);

			function->setCallingConv(llvm::CallingConv::C);

			Array_Add(lc.proc_to_llvm, function);
		}

		for (size_t i = 0; i < lc.prog->globals.count; i++)
		{
			Il_Global& global = lc.prog->globals[i];

			llvm::GlobalVariable* global_variable = nullptr;

			auto type_size = get_type_size(global.type);
			auto type_flags = get_type_flags(global.type);

			llvm::Type* llvm_type = to_llvm(lc, global.type);

			if (global.initializer == -1) {

				llvm::Value* initializer = nullptr;

				if (type_flags & TN_Struct_Type) {
					initializer = llvm::ConstantAggregateZero::get(llvm_type);
				}
				else if (type_flags & TN_Pointer_Type || global.type->kind == Type_Proc) {
					initializer = llvm::ConstantPointerNull::get((llvm::PointerType*)llvm_type);
				}
				else if (type_flags & TN_Float_Type) {
					initializer = llvm::ConstantFP::get(llvm_type, 0);
				}
				else {
					initializer = llvm::ConstantInt::get(llvm_type, 0);
				}

				global_variable = new llvm::GlobalVariable(*lc.llvm_module, llvm_type, false, llvm::GlobalVariable::ExternalLinkage, (llvm::Constant*)initializer, global.name.data);
			}
			else {

				global_variable = new llvm::GlobalVariable(*lc.llvm_module, llvm_type, false, llvm::GlobalVariable::ExternalLinkage, (llvm::Constant*)LLVMC_Initializer_Codegen(lc, global.initializer_storage, global.initializer), global.name.data);
			}

			lc.global_to_llvm[i] = global_variable;
		}

		for (size_t i = 0; i < lc.prog->procedures.count; i++)
		{
			Il_Proc& proc = lc.prog->procedures[i];
			if (!proc.external)
				LLVMC_Proc_Codegen(lc, proc, (Il_IDX)i);
		}
	}

	void LLVMC_Generate_Output(LLVM_Converter& lc) {

		llvm::InitializeAllTargetInfos();
		llvm::InitializeAllTargets();
		llvm::InitializeAllTargetMCs();
		llvm::InitializeAllAsmParsers();
		llvm::InitializeAllAsmPrinters();

		auto target_triple = llvm::sys::getDefaultTargetTriple();

		std::string error;
		auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

		if (!target) {
			llvm::errs() << error;
			return;
		}

		auto CPU = "generic";
		auto features = "";

		llvm::TargetOptions opt;
		opt.EnableFastISel = true;
		auto target_machine = target->createTargetMachine(
			target_triple, CPU, features, opt, llvm::Reloc::PIC_, {}, llvm::CodeGenOpt::None);

		auto file_name = lc.spec.output_path.data;
		std::error_code EC;
		llvm::raw_fd_ostream dest(file_name, EC, llvm::sys::fs::OF_None);

		if (EC) {
			llvm::errs() << "Could not open file: " << EC.message();
			return;
		}

		llvm::legacy::PassManager pass;
		auto file_type = llvm::CodeGenFileType::CGFT_ObjectFile;

		if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
			llvm::errs() << "TheTargetMachine can't emit a file of this type";
			return;
		}

		pass.run(*lc.llvm_module);
		dest.flush();

		//llvm::outs() << "Wrote " << file_name << "\n";
	}

	bool LLVMC_Run(LLVM_Converter& lc)
	{
		lc.llvm_ctx = new llvm::LLVMContext();
		lc.llvm_module = new llvm::Module("Glass", *lc.llvm_ctx);

		lc.global_to_llvm = Array_Reserved<llvm::GlobalVariable*>(lc.prog->globals.count);
		lc.global_to_llvm.count = lc.prog->globals.count;

		const char* data_layout_string = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128";
		llvm::DataLayout data_layout(data_layout_string);

		std::string target_triple = llvm::sys::getDefaultTargetTriple();

		lc.llvm_module->setDataLayout(data_layout);
		lc.llvm_module->setTargetTriple(target_triple);

		lc.llvm_builder = new llvm::IRBuilder<>(*lc.llvm_ctx);

		lc.proc_to_llvm = Array_Reserved<llvm::Function*>(lc.prog->procedures.count);

		lc.llvm_ptr = llvm::PointerType::get(*lc.llvm_ctx, 0);;

		lc.llvm_i8 = llvm::Type::getInt8Ty(*lc.llvm_ctx);
		lc.llvm_i16 = llvm::Type::getInt16Ty(*lc.llvm_ctx);
		lc.llvm_i32 = llvm::Type::getInt32Ty(*lc.llvm_ctx);
		lc.llvm_i64 = llvm::Type::getInt64Ty(*lc.llvm_ctx);
		lc.llvm_float = llvm::Type::getFloatTy(*lc.llvm_ctx);
		lc.llvm_double = llvm::Type::getDoubleTy(*lc.llvm_ctx);
		lc.llvm_void = llvm::Type::getVoidTy(*lc.llvm_ctx);

		lc.type_to_llvm[get_ts().i8_Ty] = lc.llvm_i8;
		lc.type_to_llvm[get_ts().i16_Ty] = lc.llvm_i16;
		lc.type_to_llvm[get_ts().i32_Ty] = lc.llvm_i32;
		lc.type_to_llvm[get_ts().i64_Ty] = lc.llvm_i64;

		lc.type_to_llvm[get_ts().u8_Ty] = lc.llvm_i8;
		lc.type_to_llvm[get_ts().u16_Ty] = lc.llvm_i16;
		lc.type_to_llvm[get_ts().u32_Ty] = lc.llvm_i32;
		lc.type_to_llvm[get_ts().u64_Ty] = lc.llvm_i64;

		lc.type_to_llvm[get_ts().f32_Ty] = lc.llvm_float;
		lc.type_to_llvm[get_ts().f64_Ty] = lc.llvm_double;

		lc.type_to_llvm[get_ts().int_Ty] = lc.llvm_i32;
		lc.type_to_llvm[get_ts().float_Ty] = lc.llvm_float;
		lc.type_to_llvm[get_ts().void_Ty] = lc.llvm_void;

		lc.llvm_Array = to_llvm(lc, get_ts().Array_Ty);

		LLVMC_Codegen(lc);

		if (lc.spec.validate)
			llvm::verifyModule(*lc.llvm_module, &llvm::outs());

		if (lc.spec.validate)
		{
			std::error_code ELC;
			llvm::raw_fd_ostream outputFile(".bin/llvm.ll", ELC, llvm::sys::fs::OF_Text);
			lc.llvm_module->print(outputFile, nullptr, true, true);
		}

		LLVMC_Generate_Output(lc);

		return true;
	}
}
