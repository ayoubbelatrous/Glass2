#pragma once

#include "BackEnd/Compiler.h"

namespace Glass {

	class LLVMBackend {
	public:
		LLVMBackend(const MetaData* metadata, IRTranslationUnit* program);

		void Compile();

		void EnumsCodegen();
		void StructsCodeGen();
		void ForeignCodeGen();

		void GenerateTypeInfo();

		llvm::Value* CodeGen(const IRInstruction* instruction);
		void FunctionSignatureCodeGen(const IRFunction* func);
		llvm::Value* FunctionCodeGen(const IRFunction* func);

		llvm::Value* ReturnCodeGen(const IRReturn* ret);
		llvm::Value* BreakCodeGen(const IRBreak* brk);

		llvm::Value* RegisterCodeGen(const IRRegister* ir_register);
		llvm::Value* RegisterValueCodeGen(const IRRegisterValue* ir_register_value);

		llvm::Value* GlobalAddrCodeGen(const IRGlobalAddress* global_address);

		llvm::Value* ConstValueCodeGen(const IRCONSTValue* constant);

		llvm::Value* OpCodeGen(const IRBinOp* op);

		llvm::Value* AllocaCodeGen(const IRAlloca* alloca);
		void GlobalVariableCodeGen(const IRGlobalDecl* global_decl);

		llvm::Value* LoadCodeGen(const IRLoad* load);
		llvm::Value* StoreCodeGen(const IRStore* store);

		llvm::Value* MemberAccessCodeGen(const IRMemberAccess* member_access);
		llvm::Value* ArrayAccessCodeGen(const IRArrayAccess* array_access);

		llvm::Value* CallCodeGen(const IRFunctionCall* call);

		llvm::Value* DataValueCodeGen(const IRDataValue* data_value);
		llvm::Value* DataCodeGen(const IRData* data);

		llvm::Value* SizeOfCodeGen(const IRSizeOF* size_of);

		llvm::Value* CastCodeGen(const IRCast* ptr_cast);
		//llvm::Value* PointerCastCodeGen(const IRPointerCast* ptr_cast);
		llvm::Value* NullPtrCodeGen(const IRNullPtr* null_ptr);

		llvm::Value* IfCodeGen(const IRIf* _if);
		llvm::Value* WhileCodeGen(const IRWhile* _while);

		llvm::Value* AnyCodeGen(const IRAny* any);
		llvm::Value* AnyArrayCodeGen(const IRAnyArray* any_array);

		//TypeInfo
		llvm::Value* TypeOfCodeGen(const IRTypeOf* type_of);
		llvm::Value* TypeInfoCodeGen(const IRTypeInfo* type_info);
		llvm::Value* TypeValueCodeGen(const IRTypeValue* type_value);
		/////////////////////////

		llvm::Value* FuncRefCodeGen(const IRFuncRef* func_ref);

		llvm::Value* CallFuncRefCodeGen(const IRCallFuncRef* func_ref);

		llvm::Value* LexicalBlockCodeGen(const IRLexBlock* lexical_block);

		llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Type* type, llvm::Constant* arraySize = nullptr);

		std::string MangleName(const std::string& name, TSFunc* signature);

	private:

		static std::unique_ptr<llvm::LLVMContext> m_LLVMContext;
		std::unique_ptr<llvm::IRBuilder<>> m_LLVMBuilder;
		llvm::Module* m_LLVMModule = nullptr;

		const MetaData* m_Metadata = nullptr;
		IRTranslationUnit* m_Program = nullptr;

		//TypeInfo
		llvm::Value* m_GlobalTypeInfoArray = nullptr;
		llvm::StructType* m_TypeInfoElemTy = nullptr;
		/////////////////////
		bool break_encountered = false;
		bool return_encountered = false;
		std::vector<llvm::BasicBlock*> m_BreakTargets;

		llvm::Value* m_ReturnRegister;
		llvm::BasicBlock* m_ReturnBlock;

		void GenerateObjFile();

		void SetFunctionID(u64 id)
		{
			m_CurrentFunctionID = id;
		}

		u64 GetFunctionHash(const std::string& name, u64 signature) {
			return Combine2Hashes(std::hash<std::string>{}(name), signature);
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

		llvm::Value* InsertGlobalVariable(u64 id, llvm::GlobalVariable* global_variable)
		{
			return m_GlobalVariables[id] = global_variable;
		}

		llvm::Value* GetGlobalVariable(u64 id)
		{
			return m_GlobalVariables.at(id);
		}

		void InsertLLVMType(u64 type_id, llvm::Type* llvm_type) {
			m_LLVMTypes[type_id] = llvm_type;
		}

		llvm::Type* GetLLVMType(u64 type_id) {
			return m_LLVMTypes.at(type_id);
		}

		llvm::Type* GetLLVMTypeFull(const Glass::Type& type) {
			return GetLLVMTypeFull(type.ID, type.Pointer);
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

		llvm::Type* GetLLVMType(TypeStorage* type) {

			llvm::Type* llvm_Type = nullptr;

			if (type->Kind == TypeStorageKind::Pointer) {

				auto type_as_pointer = (TSPtr*)type;

				llvm_Type = GetLLVMType(type_as_pointer->Pointee);

				for (size_t i = 0; i < type_as_pointer->Indirection; i++)
				{
					llvm_Type = llvm::PointerType::get(llvm_Type, (unsigned)0);
				}
				return llvm_Type;
			}

			if (type->Kind == TypeStorageKind::Base) {
				return GetLLVMType(type->BaseID);
			}

			if (type->Kind == TypeStorageKind::Function) {

				auto type_as_func = (TSFunc*)type;

				std::vector<llvm::Type*> llvm_Arguments;
				llvm::Type* llvm_ReturnType = nullptr;

				for (auto type_arg : type_as_func->Arguments) {
					llvm_Arguments.push_back(GetLLVMType(type_arg));
				}

				llvm_ReturnType = GetLLVMType(type_as_func->ReturnType);

				return llvm::FunctionType::get(llvm_ReturnType, llvm_Arguments, false)->getPointerTo();
			}

			if (type->Kind == TypeStorageKind::DynArray) {
				return GetLLVMType(IR_array);
			}

			return nullptr;
		}

		struct LLVMFunction {
			llvm::Function* llvmFunction = nullptr;
			std::map<TSFunc*, llvm::Function*> Overloads;
		};

		void InsertLLVMFunction(u64 function_id, llvm::Function* function) {
			m_LLVMFunctions[function_id].llvmFunction = function;
		}

		void InsertLLVMFunctionOverload(u64 function_id, TSFunc* signature, llvm::Function* function) {
			m_LLVMFunctions[function_id].Overloads[signature] = function;
		}

		LLVMFunction& GetLLVMFunction(u64 function_id) {
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
		std::unordered_map<u64, llvm::GlobalVariable*> m_GlobalVariables;

		std::unordered_map<u64, std::vector<llvm::Value*>> m_FunctionArgNames;
		std::unordered_map<u64, LLVMFunction> m_LLVMFunctions;

		std::unordered_map<u64, llvm::Value*> m_LLVMData;

		llvm::Type* Opaque_Type = nullptr;

		llvm::DIBuilder* m_DBuilder = nullptr;
		llvm::DICompileUnit* m_DCU;

		std::vector<llvm::DIScope*> m_DLexicalBlocks;

		llvm::DIScope* mDContext = nullptr;

		std::unordered_map<u64, llvm::DIType*> m_LLVMDebugTypes;

		void InsertLLVMDebugType(u64 type_id, llvm::DIType* di_type) {
			m_LLVMDebugTypes[type_id] = di_type;
		}

		llvm::DIType* GetLLVMDebugType(const Glass::Type& type) {
			auto it = m_LLVMDebugTypes.find(type.ID);
			if (it != m_LLVMDebugTypes.end()) {

				llvm::DIType* pointer_type = it->second;

				for (size_t i = 0; i < type.Pointer; i++) {
					pointer_type = m_DBuilder->createPointerType(pointer_type, 64);
				}

				return pointer_type;
			}
			return nullptr;
		}

		llvm::DIType* GetLLVMDebugType(TypeStorage* type) {

			if (type->Kind == TypeStorageKind::Function) {
				return GetLLVMDebugType(TypeSystem::GetPtr(TypeSystem::GetBasic(IR_void), 1));
			}

			if (type->Kind == TypeStorageKind::DynArray) {
				return GetLLVMDebugType(TypeSystem::GetBasic(IR_array));
			}

			if (type->Kind == TypeStorageKind::Base) {

				auto it = m_LLVMDebugTypes.find(type->BaseID);

				if (it != m_LLVMDebugTypes.end()) {
					return it->second;
				}
				else {
					return nullptr;
				}
			}

			if (type->Kind == TypeStorageKind::Pointer) {
				auto as_pointer_type = (TSPtr*)type;

				llvm::DIType* debug_type = GetLLVMDebugType(as_pointer_type->Pointee);

				for (u16 i = 0; i < as_pointer_type->Indirection; i++)
				{
					debug_type = m_DBuilder->createPointerType(debug_type, 64);
				}

				return debug_type;
			}

			return nullptr;
		}

		llvm::DISubroutineType* GetFunctionDebugType(TSFunc* signature) {

			std::vector<llvm::Metadata*> dbg_param_types;

			dbg_param_types.push_back(GetLLVMDebugType(signature->ReturnType));

			for (auto argument_type : signature->Arguments) {
				dbg_param_types.push_back(GetLLVMDebugType(argument_type));
			}

			llvm::DISubroutineType* func_dbg_type = m_DBuilder->createSubroutineType(
				m_DBuilder->getOrCreateTypeArray(dbg_param_types));

			return func_dbg_type;
		}

		void SetLLVMFile(const std::string& file_name, const std::string& directory) {
			mDContext = m_DBuilder->createFile(file_name, directory);
		}

		void FunctionDebugInfo(const FunctionMetadata* func_metadata, llvm::Function* llvm_func) {

			auto function_dbg_type = GetFunctionDebugType((TSFunc*)func_metadata->Signature);

			u32 LineNo = (u32)func_metadata->Symbol.Line;
			u32 ScopeLine = (u32)func_metadata->Symbol.Line + 1;

			llvm::DIScope* FContext = mDContext;

			llvm::DISubprogram* SP = m_DBuilder->createFunction(
				FContext,
				func_metadata->Symbol.Symbol,
				MangleName(func_metadata->Symbol.Symbol, (TSFunc*)func_metadata->Signature),
				(llvm::DIFile*)mDContext,
				LineNo,
				function_dbg_type,
				ScopeLine,
				llvm::DINode::DIFlags::FlagPrototyped,
				llvm::DISubprogram::SPFlagDefinition);

			llvm_func->setSubprogram(SP);
			m_DLexicalBlocks.push_back(SP);
		}

		void FinalizeFunctionDebugInfo(llvm::Function* llvm_func) {
			m_DBuilder->finalizeSubprogram(llvm_func->getSubprogram());
		}

		void PopDBGLexicalBlock() {
			m_DLexicalBlocks.pop_back();
		}

		void SetDBGLocation(DBGSourceLoc loc) {

			if (loc.Line == 0)
				return;

			if (loc.Col == 0)
				return;

			llvm::DIScope* Di_Scope = nullptr;

			if (m_DLexicalBlocks.empty()) {
				Di_Scope = m_DCU;
			}
			else {
				Di_Scope = m_DLexicalBlocks.back();
			}

			auto Di_Loc = llvm::DILocation::get(Di_Scope->getContext(), loc.Line, loc.Col, Di_Scope);

			m_LLVMBuilder->SetCurrentDebugLocation(Di_Loc);
		}

		void InitDebug();
		void DumpDebugInfo();
	};
}