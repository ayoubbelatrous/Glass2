#pragma once

#include "BackEnd/Compiler.h"

namespace llvm {
	class Value;
}

namespace Glass {

	class LLVMBackend {
	public:
		LLVMBackend(const Compiler::MetaData* metadata, IRTranslationUnit* program);

		void Compile();

		void EnumsCodegen();
		void StructsCodeGen();
		void ForeignCodeGen();

		llvm::Value* CodeGen(const IRInstruction* instruction);
		llvm::Value* FunctionCodeGen(const IRFunction* func);

		llvm::Value* ReturnCodeGen(const IRReturn* ret);

		llvm::Value* SSACodeGen(const IRSSA* ssa);
		llvm::Value* SSAValueCodeGen(const IRSSAValue* ssa_value);

		llvm::Value* ConstValueCodeGen(const IRCONSTValue* constant);

		llvm::Value* OpCodeGen(const IRBinOp* op);

		llvm::Value* AllocaCodeGen(const IRAlloca* alloca);

		llvm::Value* LoadCodeGen(const IRLoad* load);
		llvm::Value* StoreCodeGen(const IRStore* store);

		llvm::Value* MemberAccessCodeGen(const IRMemberAccess* member_access);
		llvm::Value* ArrayAccessCodeGen(const IRArrayAccess* array_access);

		llvm::Value* CallCodeGen(const IRFunctionCall* call);

		llvm::Value* DataValueCodeGen(const IRDataValue* data_value);
		llvm::Value* DataCodeGen(const IRData* data);

		llvm::Value* SizeOfCodeGen(const IRSizeOF* size_of);

		llvm::Value* PointerCastCodeGen(const IRPointerCast* ptr_cast);

		llvm::Value* IfCodeGen(const IRIf* _if);
		llvm::Value* WhileCodeGen(const IRWhile* _while);

		static llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* TheFunction, llvm::StringRef VarName);

	private:

		static std::unique_ptr<llvm::LLVMContext> m_LLVMContext;
		std::unique_ptr<llvm::IRBuilder<>> m_LLVMBuilder;
		llvm::Module* m_LLVMModule = nullptr;

		const Compiler::MetaData* m_Metadata = nullptr;
		IRTranslationUnit* m_Program = nullptr;

		void SetFunctionID(u64 id)
		{
			m_CurrentFunctionID = id;
		}

		void InsertFunctionArgNames(u64 func_id, const std::vector<llvm::Value*>& argument_names)
		{
			m_FunctionArgNames[func_id] = argument_names;
		}

		llvm::Value* GetFunctionArgumentName(u64 func_id, u64 name_index)
		{
			return m_FunctionArgNames[func_id][name_index];
		}

		void InsertName(u64 id, llvm::Value* name)
		{
			m_Names[m_CurrentFunctionID][id] = name;
		}

		llvm::Value* GetName(u64 id)
		{
			return m_Names.at(m_CurrentFunctionID).at(id);
		}

		void InsertLLVMType(u64 type_id, llvm::Type* llvm_type) {
			m_LLVMTypes[type_id] = llvm_type;
		}

		llvm::Type* GetLLVMType(u64 type_id) {
			return m_LLVMTypes.at(type_id);
		}

		llvm::Type* GetLLVMTypeFull(const Glass::Type& type) {

			llvm::Type* full_type = GetLLVMType(type.ID);

			if (type.Pointer) {
				for (size_t i = 0; i < type.Pointer; i++)
				{
					full_type = llvm::PointerType::get(full_type, (unsigned)0);
				}
			}

			return full_type;
		}

		llvm::Type* GetLLVMTypeFull(u64 type, u64 pointer) {

			llvm::Type* full_type = GetLLVMType(type);
			if (pointer) {
				for (size_t i = 0; i < pointer; i++)
				{
					full_type = llvm::PointerType::get(full_type, (unsigned)0);
				}
			}

			return full_type;
		}

		void InsertLLVMFunction(u64 function_id, llvm::Function* function) {
			m_LLVMFunctions[function_id] = function;
		}

		llvm::Function* GetLLVMFunction(u64 function_id) {
			return m_LLVMFunctions.at(function_id);
		}

		void InsertLLVMData(u64 data_id, llvm::Value* llvm_data) {
			m_LLVMData[data_id] = llvm_data;
		}

		llvm::Value* GetLLVMData(u64 data_id) {
			return m_LLVMData.at(data_id);
		}

		struct LLVMStructType {
			llvm::StructType* LLVMType;
			std::vector<llvm::Type*> LLVMMembers;

			u64 TypeID = 0;
			u64 StructID = 0;
		};

		void InsertLLVMStructType(u64 struct_id, const LLVMStructType& struct_type) {
			m_LLVMStructTypes[struct_id] = struct_type;
			InsertLLVMType(struct_type.TypeID, struct_type.LLVMType);
		}

		LLVMStructType& GetLLVMStructType(u64 struct_id) {
			return m_LLVMStructTypes.at(struct_id);
		}

		LLVMStructType& GetLLVMStructTypeByType(u64 type_id) {
			return m_LLVMStructTypes.at(m_Metadata->GetStructIDFromType(type_id));
		}

		std::unordered_map<u64, llvm::Type*> m_LLVMTypes;
		std::unordered_map<u64, LLVMStructType> m_LLVMStructTypes;

		u64 m_CurrentFunctionID = 0;
		std::unordered_map<u64, std::unordered_map<u64, llvm::Value*>> m_Names;

		std::unordered_map<u64, std::vector<llvm::Value*>> m_FunctionArgNames;
		std::unordered_map<u64, llvm::Function*> m_LLVMFunctions;

		std::unordered_map<u64, llvm::Value*> m_LLVMData;
	};
}