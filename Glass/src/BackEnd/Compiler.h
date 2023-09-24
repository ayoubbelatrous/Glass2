#pragma once

#include "BackEnd/TypeSystem.h"

namespace Glass
{
	struct TypeSizeDependency {
		u64 TypeID = 0;
		std::vector<TypeSizeDependency*> dependsOn;
		std::vector<TypeSizeDependency*> dependedBy;
	};

	static void ResolveDependencies() {

	}

	class Compiler
	{
	public:

		Compiler(std::vector<CompilerFile*> files);

		void InitTypeSystem();

		IRTranslationUnit* CodeGen();

		void LoadLoop();

		void LoadFileLoads(u64 file_id);

		void LibraryPass();
		std::vector<IRInstruction*> FirstPass();

		void SizingLoop();

		void HandleTopLevelFunction(FunctionNode* fnNode);
		void HandleTopLevelStruct(StructNode* strct);
		void HandleTopLevelEnum(EnumNode* enmNode);

		void LoadCodeGen(LoadNode* loadNode);

		IRInstruction* StatementCodeGen(const Statement* statement);

		IRInstruction* ForeignCodeGen(const ForeignNode* expression);

		IRInstruction* OperatorCodeGen(const OperatorNode* op_node);

		IRInstruction* FunctionCodeGen(FunctionNode* functionNode);

		IRInstruction* VariableCodeGen(const VariableNode* variableNode);
		IRInstruction* ConstantCodeGen(const VariableNode* variableNode);

		IRInstruction* ConstantValueCodeGen(const ConstantDecl* constant);

		IRInstruction* GlobalVariableCodeGen(const VariableNode* variableNode, bool foreign = false);

		IRInstruction* ReturnCodeGen(const ReturnNode* returnNode);
		IRInstruction* StructCodeGen(const StructNode* structNode);

		IRInstruction* EnumCodeGen(const EnumNode* enumNode, u64 type_id = (u64)-1);

		IRInstruction* IfCodeGen(const IfNode* ifNode);
		IRInstruction* WhileCodeGen(const WhileNode* ifNode);

		IRInstruction* ForCodeGen(const ForNode* forNode);

		IRInstruction* ExpressionCodeGen(const Expression* expression);

		IRInstruction* IdentifierCodeGen(const Identifier* identifier);
		IRInstruction* NumericLiteralCodeGen(const NumericLiteral* numericLiteral);
		IRInstruction* StringLiteralCodeGen(const StringLiteral* stringLiteral);
		IRInstruction* BinaryExpressionCodeGen(const BinaryExpression* binaryExpr);
		IRInstruction* AssignmentCodeGen(const BinaryExpression* binaryExpr);
		IRInstruction* FunctionCallCodeGen(const FunctionCall* call);

		IRInstruction* CallPolyMorphicFunction(const FunctionCall* call);

		IRInstruction* MemberAccessCodeGen(const MemberAccess* memberAccess);
		IRInstruction* EnumMemberAccessCodeGen(const MemberAccess* memberAccess);
		IRInstruction* SizeOfCodeGen(const SizeOfNode* size_of);

		IRInstruction* NegateCodeGen(const NegateExpr* negateNode);

		IRInstruction* RangeCodeGen(const RangeNode* rangeNode);

		IRIterator* DynArrayIteratorCodeGen(const Expression* expression, IRRegisterValue* generated);
		IRIterator* IteratorCodeGen(const Expression* expr);

		IRInstruction* FunctionRefCodegen(const Identifier* func);
		IRInstruction* FuncRefCallCodeGen(const FunctionCall* call);

		std::vector<IRInstruction*> ScopeCodeGen(const ScopeNode* scope);

		IRInstruction* ArrayAccessCodeGen(const ArrayAccess* arrayAccess);
		IRInstruction* TypeofCodeGen(const TypeOfNode* typeof);

		IRInstruction* AutoCastCodeGen(const AutoCastNode* autoCastNode);
		IRInstruction* CastNodeCodeGen(const CastNode* castNode);
		IRInstruction* CastCodeGen(TypeStorage* cast_type, IRRegisterValue* code, const Expression* ast_node);

		IRInstruction* NullCodeGen();

		IRInstruction* TypeInfoCodeGen(const FunctionCall* type_info_call);

		IRInstruction* RefCodeGen(const RefNode* refNode);
		IRInstruction* DeRefCodeGen(const DeRefNode* deRefNode);

		IRRegisterValue* GetExpressionByValue(const Expression* expr, IRRegisterValue* generated_code = nullptr);
		IRRegisterValue* PassAsAny(const Expression* expr, IRRegisterValue* pre_generated = nullptr);
		IRRegisterValue* PassAsVariadicArray(const std::vector<Expression*>& arguments, const std::vector<IRRegisterValue*>& pre_generated_arguments, const ArgumentMetadata* decl_arg);
		IRRegisterValue* TypeExpressionCodeGen(TypeExpression* type_expr);
		IRRegisterValue* TypeValueCodeGen(TypeStorage* type);

		IRRegisterValue* CreateLoad(TypeStorage* type, u64 address);
		IRRegisterValue* CreateStore(TypeStorage* type, u64 address, IRInstruction* data);

		IRRegisterValue* CreateConstantInteger(u64 integer_base_type, i64 value);
		IRRegisterValue* CreateConstant(u64 base_type, i64 value_integer, double value_float);

		IRRegisterValue* CreateCopy(TypeStorage* type, IRRegisterValue* loaded_value);

		IRRegisterValue* CreateMemberAccess(const std::string& strct, const std::string& member, u64 address);

		IRRegisterValue* CreatePointerCast(TypeStorage* to_type, u64 address);

		IRFunction* CreateIRFunction(const FunctionNode* functionNode);
		IRRegister* CreateIRRegister();
		IRRegisterValue* CreateIRRegister(IRInstruction* value);
		IRRegisterValue* CreateIRRegister(IRInstruction* value, TypeStorage* semantic_type);
		IRData* CreateIRData();

		TypeStorage* TypeExpressionGetType(TypeExpression* type_expr);

		TypeExpression* TypeGetTypeExpression(TypeStorage* type);

		Glass::Type TSToLegacy(TypeStorage* type);
		TypeStorage* LegacyToTS(const Glass::Type& type);

		void BinaryDispatch(const Expression* left, const Expression* right, TypeStorage** left_type, TypeStorage** right_type, IRRegisterValue** A, IRRegisterValue** B, std::vector<IRRegister*>* a_code = nullptr, std::vector<IRRegister*>* b_code = nullptr);

		bool CheckTypeConversion(u64 a, u64 b)
		{
			const static std::unordered_map<u64, u64> numericTypes =
			{
				{IR_int,0},
				{IR_float,0},

				{IR_i8,0},
				{IR_i16,0},
				{IR_i32,0},
				{IR_i64,0},

				{IR_u8,0},
				{IR_u16,0},
				{IR_u32,0},
				{IR_u64,0},

				{IR_f32,0},
				{IR_f64,0},
			};

			bool both_numeric = false;

			bool a_numeric = numericTypes.find(a) != numericTypes.end();
			bool b_numeric = numericTypes.find(b) != numericTypes.end();

			both_numeric = a_numeric && b_numeric;

			if (both_numeric) {
				return true;
			}

			return a == b;
		}

		const MetaData& GetMetadata() {
			return m_Metadata;
		}

		MetaData& GetMetadataNonConst() {
			return m_Metadata;
		}

		void PushIRData(IRData* data) {
			m_DataStack.push_back(data);
		}

		std::vector<IRData*> PoPIRData() {
			auto cpy = m_DataStack;
			m_DataStack.clear();
			return cpy;
		}

		void PushIRRegister(IRRegister* ir_register) {
			if (m_Scope == 0) {
				m_RegisterStack.push_back(ir_register);
				m_Metadata.RegisterIRRegister(ir_register);
			}
			else {
				m_RegisterStacks[m_Scope].push_back(ir_register);
				m_Metadata.RegisterIRRegister(ir_register);
			}
		}

		void ResetRegisterIDCounter() {
			m_RegisterIDCounter = 1;
		}

		std::vector<IRRegister*> PoPIRRegisters() {
			if (m_Scope == 0) {
				auto cpy = m_RegisterStack;
				m_RegisterStack.clear();
				return cpy;
			}
			else {
				auto cpy = m_RegisterStacks[m_Scope];
				m_RegisterStacks[m_Scope].clear();
				return cpy;
			}
		}

		void PushScope()
		{
			m_Scope++;
		}

		void PopScope()
		{
			m_Scope--;
		}

		void PushIRInstruction(IRInstruction* ir_instruction) {
			m_InstructionStack.push_back(ir_instruction);
		}

		IRRegister* GetRegister(u64 ID) {
			return m_Metadata.GetRegister(ID);
		}

		u64 GetVariableRegister(const std::string& name) {
			return m_Metadata.GetVariableRegister(name);
		}

		u64 RegisterVariable(IRRegister* ir_register, const std::string& name) {
			return m_Metadata.RegisterVariable(ir_register, name);
		}

		void PushMessage(CompilerMessage msg) {
			m_Messages.push_back(msg);
		}

		const std::vector<CompilerMessage>& GetMessages() const {
			return m_Messages;
		}

		std::string PrintTokenLocation(const Token& token)
		{
			return fmt::format("{}:{}:{}", m_Files[m_CurrentFile]->GetPath().string(), token.Line + 1, token.Begin);
		}

		std::string PrintType(const Type& type)
		{
			std::string arr_ptr;

			if (type.Array) {
				arr_ptr += "[]";
			}

			if (type.Pointer) {
				arr_ptr += "*";
			}

			return fmt::format("{}{}", m_Metadata.GetType(type.ID), arr_ptr);
		}

		std::string PrintType(TypeStorage* type)
		{
			if (type->Kind == TypeStorageKind::Pointer) {
				auto as_pointer = (TSPtr*)type;
				std::string stars;

				for (size_t i = 0; i < as_pointer->Indirection; i++) {
					stars.push_back('*');
				}

				return fmt::format("{}{}", PrintType(as_pointer->Pointee), stars);
			}

			if (type->Kind == TypeStorageKind::DynArray) {
				auto as_array = (TSDynArray*)type;
				return fmt::format("{}[..]", PrintType(as_array->ElementType));
			}

			if (type->Kind == TypeStorageKind::Base) {
				return m_Metadata.GetType(type->BaseID);
			}

			if (type->Kind == TypeStorageKind::Function) {
				auto as_func = (TSFunc*)type;

				std::string arguments;
				std::string return_type;

				for (size_t i = 0; i < as_func->Arguments.size(); i++) {
					if (i != 0) {
						arguments.append(",");
					}
					arguments.append(PrintType(as_func->Arguments[i]));
				}

				if (as_func->ReturnType) {
					return_type = ": " + PrintType(as_func->ReturnType);
				}

				return fmt::format("({}){}", arguments, return_type);
			}
		}

		u64 GetTypeID() {
			m_TypeIDCounter++;
			return m_TypeIDCounter;
		}

		u64 GetStructID() {
			m_StructIDCounter++;
			return m_StructIDCounter;
		}

		u64 GetEnumID() {
			m_EnumIDCounter++;
			return m_EnumIDCounter;
		}

		u64 GetFunctionID() {
			m_FunctionIDCounter++;
			return m_FunctionIDCounter;
		}

		bool ContextGlobal() {
			return m_Metadata.CurrentContextID() == 1;
		}

		u64 GetGlobalID() {
			m_GlobalCounter++;
			return m_GlobalCounter;
		}

		void SetLikelyConstantIntegerType(u64 integer_type) {
			m_ConstantIntegerLikelyType = integer_type;
		}

		void ResetLikelyConstantIntegerType() {
			m_ConstantIntegerLikelyType = IR_int;
		}

		u64 GetLikelyConstantIntegerType() {
			return m_ConstantIntegerLikelyType;
		}

		void SetLikelyConstantFloatType(u64 float_type) {
			m_ConstantFloatLikelyType = float_type;
		}

		void ResetLikelyConstantFloatType() {
			m_ConstantFloatLikelyType = IR_float;
		}

		u64 GetLikelyConstantFloatType() {
			return m_ConstantFloatLikelyType;
		}

		void SetLikelyConstantType(u64 type_id) {
			u64 assignment_type_id = type_id;
			u64 assignment_type_flags = m_Metadata.GetTypeFlags(assignment_type_id);

			if (assignment_type_flags & FLAG_NUMERIC_TYPE)
			{
				if (!(assignment_type_flags & FLAG_FLOATING_TYPE)) {
					SetLikelyConstantIntegerType(assignment_type_id);
				}
				else if (assignment_type_flags & FLAG_FLOATING_TYPE) {
					SetLikelyConstantFloatType(assignment_type_id);
				}
			}
		}

		void ResetLikelyConstantType() {
			ResetLikelyConstantIntegerType();
			ResetLikelyConstantFloatType();
		}

		void RegisterDBGLoc(const Statement* stmt) {
			const Token& token = stmt->GetLocation();
			m_CurrentDBGLoc.Line = (u32)token.Line + 1;
			m_CurrentDBGLoc.Col = (u32)token.Begin + 1;
		}

	private:

		void SetExpectedReturnType(TypeStorage* return_type) {
			m_ExpectedReturnType = return_type;
		}

		TypeStorage* GetExpectedReturnType() {
			return m_ExpectedReturnType;
		}

		TypeStorage* m_AutoCastTargetType = nullptr;

		TypeStorage* m_ExpectedReturnType = nullptr;

		DBGSourceLoc m_CurrentDBGLoc;

		u64 m_ConstantIntegerLikelyType = IR_int;
		u64 m_ConstantFloatLikelyType = IR_float;

		bool UseArrayAccessInstruction = true;

		MetaData m_Metadata;

		u64 m_FileIDCounter = 0;
		u64 m_CurrentFile = 0;
		std::map<u64, CompilerFile*> m_Files;
		std::set<std::string> m_LoadedFilesAbsolutePaths;

		u64 InsertFile(const std::string& absolute_path, CompilerFile* file) {

			GS_CORE_ASSERT(m_LoadedFilesAbsolutePaths.find(absolute_path) == m_LoadedFilesAbsolutePaths.end());
			m_LoadedFilesAbsolutePaths.insert(absolute_path);

			GS_CORE_ASSERT(m_Files.find(m_FileIDCounter) == m_Files.end());
			m_Files[m_FileIDCounter] = file;

			auto this_file_id = m_FileIDCounter;

			m_FileIDCounter++;

			return this_file_id;
		}

		bool FileLoaded(const std::string& absolute_path) {
			return m_LoadedFilesAbsolutePaths.find(absolute_path) != m_LoadedFilesAbsolutePaths.end();
		}

		u64 m_FunctionIDCounter = 1;
		u64 m_RegisterIDCounter = 1;
		u64 m_DATAIDCounter = 1;
		u64 m_TypeIDCounter = 99;
		u64 m_StructIDCounter = 99;
		u64 m_EnumIDCounter = 0;
		u64 m_GlobalCounter = 512000;

		std::vector<CompilerMessage> m_Messages;

		u64 m_Scope = 0;

		std::unordered_map<u64, std::vector<IRRegister*> > m_RegisterStacks;

		std::vector<IRInstruction*> m_InstructionStack;
		std::vector<IRRegister*> m_RegisterStack;
		std::vector<IRData*> m_DataStack;
	};
}