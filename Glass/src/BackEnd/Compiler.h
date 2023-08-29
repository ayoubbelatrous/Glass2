#pragma once

#include "BackEnd/TypeSystem.h"

namespace Glass
{
	class Compiler
	{
	public:

		Compiler(std::vector<CompilerFile*> files);

		void InitTypeSystem();

		IRTranslationUnit* CodeGen();

		IRInstruction* StatementCodeGen(const Statement* statement);

		IRInstruction* ForeignCodeGen(const ForeignNode* expression);

		IRInstruction* OperatorCodeGen(const OperatorNode* op_node);

		IRInstruction* FunctionCodeGen(FunctionNode* functionNode);

		IRInstruction* VariableCodeGen(const VariableNode* variableNode);
		IRInstruction* GlobalVariableCodeGen(const VariableNode* variableNode);

		IRInstruction* ReturnCodeGen(const ReturnNode* returnNode);
		IRInstruction* StructCodeGen(const StructNode* structNode);

		IRInstruction* EnumCodeGen(const EnumNode* enumNode, u64 type_id = (u64)-1);

		IRInstruction* IfCodeGen(const IfNode* ifNode);
		IRInstruction* WhileCodeGen(const WhileNode* ifNode);

		IRInstruction* ExpressionCodeGen(const Expression* expression);

		IRInstruction* IdentifierCodeGen(const Identifier* identifier);
		IRInstruction* NumericLiteralCodeGen(const NumericLiteral* numericLiteral);
		IRInstruction* StringLiteralCodeGen(const StringLiteral* stringLiteral);
		IRInstruction* BinaryExpressionCodeGen(const BinaryExpression* binaryExpr);
		IRInstruction* AssignmentCodeGen(const BinaryExpression* binaryExpr);
		IRInstruction* FunctionCallCodeGen(const FunctionCall* call);
		IRInstruction* MemberAccessCodeGen(const MemberAccess* memberAccess);
		IRInstruction* EnumMemberAccessCodeGen(const MemberAccess* memberAccess);
		IRInstruction* SizeOfCodeGen(const SizeOfNode* size_of);

		IRInstruction* FunctionRefCodegen(const Identifier* func);

		std::vector<IRInstruction*> ScopeCodeGen(const ScopeNode* scope);

		IRInstruction* ArrayAccessCodeGen(const ArrayAccess* arrayAccess);
		IRInstruction* TypeofCodeGen(const TypeOfNode* typeof);
		IRInstruction* CastCodeGen(const CastNode* typeof);

		IRInstruction* NullCodeGen();

		IRInstruction* TypeInfoCodeGen(const FunctionCall* type_info_call);

		IRInstruction* RefCodeGen(const RefNode* refNode);
		IRInstruction* DeRefCodeGen(const DeRefNode* deRefNode);

		IRSSAValue* GetExpressionByValue(const Expression* expr);
		IRSSAValue* PassAsAny(const Expression* expr);
		IRSSAValue* PassAsVariadicArray(u64 start, const std::vector<Expression*>& arguments, const ArgumentMetadata* decl_arg);
		IRSSAValue* TypeExpressionCodeGen(TypeExpression* type_expr);
		IRSSAValue* TypeValueCodeGen(TypeStorage* type);

		IRFunction* CreateIRFunction(const FunctionNode* functionNode);
		IRSSA* CreateIRSSA();
		IRData* CreateIRData();

		IRFunction* CreatePolyMorhOverload(u64 ID, const PolyMorphOverloads& overloads);

		TypeStorage* TypeExpressionGetType(TypeExpression* type_expr);

		Glass::Type TSToLegacy(TypeStorage* type);
		TypeStorage* LegacyToTS(const Glass::Type& type);

		const IRFunction* GetPolyMorphOverLoad(u64 ID, const PolyMorphOverloads& overloads)
		{
			auto metadata = m_Metadata.GetFunctionMetadata(ID);
			if (metadata->PolyMorhOverLoads.find(overloads) != metadata->PolyMorhOverLoads.end()) {
				return metadata->PolyMorhOverLoads.at(overloads);
			}
			else {
				IRFunction* ir_func = CreatePolyMorhOverload(ID, overloads);
				if (ir_func == nullptr) {
					return nullptr;
				}
				else {
					metadata->PolyMorhOverLoads.emplace(overloads, ir_func);
				}
			}
			return GetPolyMorphOverLoad(ID, overloads);
		}

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

		void PushIRData(IRData* data) {
			m_DataStack.push_back(data);
		}

		std::vector<IRData*> PoPIRData() {
			auto cpy = m_DataStack;
			m_DataStack.clear();
			return cpy;
		}

		void PushIRSSA(IRSSA* ssa) {
			if (m_Scope == 0) {
				m_SSAStack.push_back(ssa);
				m_Metadata.RegisterSSA(ssa);
			}
			else {
				m_SSAStacks[m_Scope].push_back(ssa);
				m_Metadata.RegisterSSA(ssa);
			}
		}

		void ResetSSAIDCounter() {
			m_SSAIDCounter = 1;
		}

		std::vector<IRSSA*> PoPIRSSA() {
			if (m_Scope == 0) {
				auto cpy = m_SSAStack;
				m_SSAStack.clear();
				return cpy;
			}
			else {
				auto cpy = m_SSAStacks[m_Scope];
				m_SSAStacks[m_Scope].clear();
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

		void PushIRInstruction(IRInstruction* ssa) {
			m_InstructionStack.push_back(ssa);
		}

		IRSSA* GetSSA(u64 ID) {
			return m_Metadata.GetSSA(ID);
		}

		u64 GetVariableSSA(const std::string& name) {
			return m_Metadata.GetVariableSSA(name);
		}

		u64 RegisterVariable(IRSSA* ssa, const std::string& name) {
			return m_Metadata.RegisterVariable(ssa, name);
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

		DBGSourceLoc m_CurrentDBGLoc;

		u64 m_ConstantIntegerLikelyType = IR_int;
		u64 m_ConstantFloatLikelyType = IR_float;

		bool UseArrayAccessInstruction = true;

		MetaData m_Metadata;

		std::vector<CompilerFile*> m_Files;

		u64 m_FunctionIDCounter = 1;
		u64 m_SSAIDCounter = 1;
		u64 m_DATAIDCounter = 1;
		u64 m_TypeIDCounter = 99;
		u64 m_StructIDCounter = 99;
		u64 m_EnumIDCounter = 0;
		u64 m_GlobalCounter = 512000;

		std::vector<CompilerMessage> m_Messages;
		u64 m_CurrentFile = 0;

		u64 m_Scope = 0;

		std::unordered_map<u64, std::vector<IRSSA*> > m_SSAStacks;

		std::vector<IRInstruction*> m_InstructionStack;
		std::vector<IRSSA*> m_SSAStack;
		std::vector<IRData*> m_DataStack;
	};
}