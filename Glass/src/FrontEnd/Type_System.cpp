#include "pch.h"

#include "FrontEnd/Type_System.h"

namespace Glass
{
	u64 TypeSystem_Get_Type_Index(Type_System& ts, GS_Type* type)
	{
		return ((u64)type - (u64)ts.type_storage.data) / sizeof(GS_Type);
	}

	void TypeSystem_Init(Type_System& ts)
	{
		ts.type_name_storage = Array_Reserved<Type_Name>(1024 * 10);
		ts.type_storage = Array_Reserved<GS_Type>(1024 * 100);
	}

	Type_Name_ID TypeSystem_Insert_TypeName(Type_System& ts, Type_Name type_name)
	{
		Array_Add(ts.type_name_storage, type_name);
		return (Type_Name_ID)(ts.type_name_storage.count - 1);
	}

	Type_Name_ID TypeSystem_Insert_TypeName_Struct(Type_System& ts, Type_Name type_name, GS_Struct strct)
	{
		type_name.struct_id = (Type_Name_ID)ts.struct_storage.count;
		Array_Add(ts.type_name_storage, type_name);
		Array_Add(ts.struct_storage, strct);
		return (Type_Name_ID)(ts.type_name_storage.count - 1);
	}

	GS_Type* TypeSystem_Get_Basic_Type(Type_System& ts, Type_Name_ID type_name_id)
	{
		Type_Name tn = ts.type_name_storage[type_name_id]; // check it exists

		u64 type_hash = GS_Basic_Type_Hash(type_name_id);

		auto it = ts.type_lookup.find(type_hash);
		if (it != ts.type_lookup.end()) {
			ASSERT(it->second->kind == Type_Basic);
			return it->second;
		}

		GS_Type new_type;
		new_type.kind = Type_Basic;
		new_type.type_hash = type_hash;
		new_type.basic.type_name_id = type_name_id;

		GS_Type* inserted_type = Array_Add(ts.type_storage, new_type);
		ts.type_lookup[type_hash] = inserted_type;

		return inserted_type;
	}

	GS_Type* TypeSystem_Get_Pointer_Type(Type_System& ts, GS_Type* pointee, u32 indirection)
	{
		ASSERT(pointee);
		ASSERT(indirection);
		ASSERT(pointee->kind != Type_Pointer);

		u64 type_hash = GS_Pointer_Type_Hash(pointee->type_hash, indirection);

		auto it = ts.type_lookup.find(type_hash);
		if (it != ts.type_lookup.end()) {
			ASSERT(it->second->kind == Type_Pointer);
			return it->second;
		}

		GS_Type new_type;
		new_type.kind = Type_Pointer;
		new_type.type_hash = type_hash;
		new_type.pointer.pointee = pointee;
		new_type.pointer.indirection = indirection;

		GS_Type* inserted_type = Array_Add(ts.type_storage, new_type);
		ts.type_lookup[type_hash] = inserted_type;

		return inserted_type;
	}

	GS_Type* TypeSystem_Get_Proc_Type(Type_System& ts, GS_Type* return_type, Array<GS_Type*> params)
	{
		ASSERT(return_type);

		Array<u64> param_hashes;

		for (u64 i = 0; i < params.count; i++)
		{
			ASSERT(params[i]);
			Array_Add(param_hashes, params[i]->type_hash);
		}

		u64 type_hash = GS_Proc_Type_Hash(return_type->type_hash, param_hashes);

		auto it = ts.type_lookup.find(type_hash);
		if (it != ts.type_lookup.end()) {
			ASSERT(it->second->kind == Type_Proc);
			return it->second;
		}

		params = Array_Copy(params);

		GS_Type new_type;
		new_type.kind = Type_Proc;
		new_type.type_hash = type_hash;
		new_type.proc.return_type = return_type;
		new_type.proc.params = params.data;
		new_type.proc.params_count = params.count;

		GS_Type* inserted_type = Array_Add(ts.type_storage, new_type);
		ts.type_lookup[type_hash] = inserted_type;

		return inserted_type;
	}

	GS_Type* TypeSystem_Increase_Ind(Type_System& ts, GS_Type* type, u32 amount /*= 1*/)
	{
		ASSERT(type);

		if (type->kind == Type_Pointer) {
			return TypeSystem_Get_Pointer_Type(ts, type->pointer.pointee, type->pointer.indirection + amount);
		}
		else {
			return TypeSystem_Get_Pointer_Type(ts, type, amount);
		}
	}

	Type_Name_Flags TypeSystem_Get_Type_Flags(Type_System& ts, GS_Type* type)
	{
		if (type->kind == Type_Basic) {
			return ts.type_name_storage[type->basic.type_name_id].flags;
		}
		else {
			return 0;
		}
	}

	Type_Name_Flags TypeSystem_Get_Type_Alignment(Type_System& ts, GS_Type* type)
	{
		if (type->kind == Type_Basic) {
			return ts.type_name_storage[type->basic.type_name_id].alignment;
		}
		else if (type->kind == Type_Pointer) {
			return 8;
		}
		else {
			ASSERT(nullptr);
			return 0;
		}
	}

	Type_Name_Flags TypeSystem_Get_Type_Size(Type_System& ts, GS_Type* type)
	{
		if (type->kind == Type_Basic) {
			return ts.type_name_storage[type->basic.type_name_id].size;
		}
		else if (type->kind == Type_Pointer) {
			return 8;
		}
		else {
			ASSERT(nullptr);
			return 0;
		}
	}

	GS_Struct_Data_Layout TypeSystem_Struct_Compute_Align_Size_Offsets(Type_System& ts, Array<GS_Type*> members)
	{
		GS_Struct_Data_Layout dl;

		dl.alignment = 1;
		dl.offsets = Array_Reserved<u64>(members.count);
		dl.offsets.count = members.count;

		for (size_t i = 0; i < members.count; i++)
		{
			GS_Type* member = members[i];
			u64 member_size = TypeSystem_Get_Type_Size(ts, member);
			u64 member_alignment = TypeSystem_Get_Type_Alignment(ts, member);

			if (member_alignment > dl.alignment) {
				dl.alignment = member_alignment;
			}
		}

		u64 offset = 0;

		for (size_t i = 0; i < members.count; i++)
		{
			GS_Type* member = members[i];
			u64 member_size = TypeSystem_Get_Type_Size(ts, member);
			u64 member_alignment = TypeSystem_Get_Type_Alignment(ts, member);

			dl.offsets[i] = offset;

			if ((offset + member_size) % member_alignment != 0) {
				i64 padding = (member_alignment - ((offset + member_size) % member_alignment)) % member_alignment;
				offset += padding;
				dl.offsets[i] = offset;
				offset += member_size;
			}
			else {
				offset += member_size;
			}
		}

		dl.size = offset;

		i64 finalPadding = (dl.alignment - ((dl.size) % dl.alignment)) % dl.alignment;
		dl.size += finalPadding;

		return dl;
	}

}