#include "pch.h"

#include "FrontEnd/Type_System.h"
#include "Type_System.h"

namespace Glass
{
	Type_System global_type_system;

	Type_IDX TypeSystem_Get_Type_Index(Type_System& ts, GS_Type* type)
	{
		return (Type_IDX)(((u64)type - (u64)ts.type_storage.data) / sizeof(GS_Type));
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

	Type_Name_ID insert_typename(Type_Name type_name)
	{
		return TypeSystem_Insert_TypeName(global_type_system, type_name);
	}

	Type_Name_ID insert_typename_struct(Type_Name type_name, GS_Struct strct)
	{
		return TypeSystem_Insert_TypeName_Struct(global_type_system, type_name, strct);
	}

	Type_Name_ID insert_struct(String name, Array<GS_Type*> members)
	{
		GS_Struct_Data_Layout layout = struct_compute_align_size_offsets(members);

		GS_Struct gs_struct;
		gs_struct.members = members;
		gs_struct.offsets = layout.offsets;

		Type_Name tn = {};
		tn.name = String_Copy(name);
		tn.flags = TN_Struct_Type;
		tn.size = layout.size;
		tn.alignment = layout.alignment;

		return insert_typename_struct(tn, gs_struct);
	}

	void insert_struct(int typename_id, Array<GS_Type*> members)
	{
		GS_Struct_Data_Layout layout = struct_compute_align_size_offsets(members);

		GS_Struct gs_struct;
		gs_struct.members = members;
		gs_struct.offsets = layout.offsets;

		Type_Name& tn = global_type_system.type_name_storage[typename_id];
		tn.size = layout.size;
		tn.alignment = layout.alignment;
		tn.struct_id = global_type_system.struct_storage.count;

		Array_Add(global_type_system.struct_storage, gs_struct);
	}

	Type_IDX get_type_index(GS_Type* type)
	{
		return TypeSystem_Get_Type_Index(global_type_system, type);
	}

	GS_Type* get_type(Type_Name_ID type_name_id)
	{
		return TypeSystem_Get_Basic_Type(global_type_system, type_name_id);
	}

	GS_Type* get_pointer_type(GS_Type* pointee, u32 indirection)
	{
		return TypeSystem_Get_Pointer_Type(global_type_system, pointee, indirection);
	}

	GS_Type* get_proc_type(GS_Type* return_type, Array<GS_Type*> params)
	{
		return TypeSystem_Get_Proc_Type(global_type_system, return_type, params);
	}

	GS_Type* get_dynarray_type(GS_Type* element)
	{
		return TypeSystem_Get_Dyn_Array_Type(global_type_system, element);
	}

	GS_Type* get_array_type(GS_Type* element, u64 size)
	{
		return TypeSystem_Get_Array_Type(global_type_system, element, size);
	}

	GS_Struct& get_struct(GS_Type* type)
	{
		return global_type_system.struct_storage[global_type_system.type_name_storage[type->basic.type_name_id].struct_id];
	}

	u64 get_type_size(GS_Type* type)
	{
		return TypeSystem_Get_Type_Size(global_type_system, type);
	}

	Type_System& get_ts()
	{
		return global_type_system;
	}

	String print_type(GS_Type* type)
	{
		return TypeSystem_Print_Type(global_type_system, type);
	}

	String print_type_index(int type_idx)
	{
		return print_type(get_type_at(type_idx));
	}

	GS_Struct_Data_Layout struct_compute_align_size_offsets(Array<GS_Type*> members)
	{
		return TypeSystem_Struct_Compute_Align_Size_Offsets(global_type_system, members);
	}

	u64 get_type_flags(GS_Type* type)
	{
		return TypeSystem_Get_Type_Flags(global_type_system, type);
	}

	u64 get_type_flags(int index)
	{
		return get_type_flags(get_type_at(index));
	}

	GS_Type* get_type_at(int index)
	{
		return &global_type_system.type_storage[index];
	}

	u64 get_type_alignment(GS_Type* type)
	{
		return TypeSystem_Get_Type_Alignment(global_type_system, type);
	}

	GS_Type* reduce_indirection(GS_Type* type)
	{
		return TypeSystem_Reduce_Indirection(global_type_system, type);
	}

	bool is_type_aggr(GS_Type* type)
	{
		if (type == get_ts().void_Ty)
			return false;

		if (type->kind == Type_Array || type->kind == Type_Dyn_Array)
		{
			return true;
		}
		else if (type->kind == Type_Basic) {

			auto flags = get_type_flags(type);
			auto size = type->size();

			bool is_power_of_two = (size > 0) && ((size & (size - 1)) == 0);

			if ((size > 8 || (!is_power_of_two && size <= 8)))
			{
				return true;
			}
		}

		return false;
	}

	void init_typesystem()
	{
		TypeSystem_Init(global_type_system);
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

		while (pointee->kind == Type_Pointer)
		{
			indirection += pointee->pointer.indirection;
			pointee = pointee->pointer.pointee;
		}

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

	GS_Type* TypeSystem_Get_Dyn_Array_Type(Type_System& ts, GS_Type* element)
	{
		ASSERT(element);

		u64 type_hash = GS_Dyn_Array_Type_Hash(element->type_hash);

		auto it = ts.type_lookup.find(type_hash);
		if (it != ts.type_lookup.end()) {
			ASSERT(it->second->kind == Type_Dyn_Array);
			return it->second;
		}

		GS_Type new_type;
		new_type.kind = Type_Dyn_Array;
		new_type.type_hash = type_hash;
		new_type.dyn_array.element_type = element;

		GS_Type* inserted_type = Array_Add(ts.type_storage, new_type);
		ts.type_lookup[type_hash] = inserted_type;

		return inserted_type;
	}

	GS_Type* TypeSystem_Get_Array_Type(Type_System& ts, GS_Type* element, u64 size)
	{
		ASSERT(element);
		ASSERT(size);

		u64 type_hash = GS_Array_Type_Hash(element->type_hash, size);

		auto it = ts.type_lookup.find(type_hash);
		if (it != ts.type_lookup.end()) {
			ASSERT(it->second->kind == Type_Array);
			return it->second;
		}

		GS_Type new_type;
		new_type.kind = Type_Array;
		new_type.type_hash = type_hash;
		new_type.array.element_type = element;
		new_type.array.size = size;

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
		new_type.proc.params = *(Array_UI<GS_Type*>*) & params;

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
		else if (type->kind == Type_Pointer) {
			return TN_Pointer_Type;
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
		else if (type->kind == Type_Dyn_Array) {
			return 8;
		}
		else if (type->kind == Type_Array) {
			return TypeSystem_Get_Type_Alignment(ts, type->array.element_type);
		}
		else if (type->kind == Type_Proc) {
			return 8;
		}
		else {
			ASSERT(nullptr);
			return 0;
		}
	}

	u64 TypeSystem_Get_Type_Size(Type_System& ts, GS_Type* type)
	{
		if (type->kind == Type_Basic) {
			return ts.type_name_storage[type->basic.type_name_id].size;
		}
		else if (type->kind == Type_Pointer) {
			return 8;
		}
		else if (type->kind == Type_Proc) {
			return 8;
		}
		else if (type->kind == Type_Dyn_Array) {
			return 24;
		}
		else if (type->kind == Type_Array) {
			return TypeSystem_Get_Type_Size(ts, type->array.element_type) * type->array.size;
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

	String TypeSystem_Print_Type(Type_System& ts, GS_Type* type)
	{
		std::stringstream stream;

		if (!type)
		{
			stream << "<untyped>";
			return String_Make(stream.str());
		}

		switch (type->kind)
		{
		case Type_Basic:
			stream << ts.type_name_storage[type->basic.type_name_id].name.data;
			break;
		case Type_Pointer:
			for (size_t i = 0; i < type->pointer.indirection; i++)
			{
				stream << '*';
			}
			stream << TypeSystem_Print_Type(ts, type->pointer.pointee).data;
			break;
		case Type_Dyn_Array:
			stream << "[..]";
			stream << TypeSystem_Print_Type(ts, type->dyn_array.element_type).data;
			break;
		case Type_Array:
			stream << '[';
			stream << type->array.size;
			stream << ']';
			stream << TypeSystem_Print_Type(ts, type->array.element_type).data;
			break;
		case Type_Proc:

			stream << '(';

			for (size_t i = 0; i < type->proc.params.count; i++)
			{
				stream << TypeSystem_Print_Type(ts, type->proc.params[i]).data;

				if (i != type->proc.params.count - 1)
					stream << ", ";
			}

			stream << ')';

			stream << " -> ";

			stream << TypeSystem_Print_Type(ts, type->proc.return_type).data;

			break;
		default:
			GS_ASSERT_UNIMPL();
			break;
		}

		return String_Make(stream.str());
	}

	String TypeSystem_Print_Type_Index(Type_System& ts, u64 type_idx)
	{
		return TypeSystem_Print_Type(ts, &ts.type_storage[type_idx]);
	}

	GS_Type* TypeSystem_Reduce_Indirection(Type_System& ts, GS_Type* type)
	{
		if (type->kind == Type_Pointer) {
			if (type->pointer.indirection - 1 != 0) {
				return TypeSystem_Get_Pointer_Type(ts, type->pointer.pointee, type->pointer.indirection - 1);
			}
			else {
				return type->pointer.pointee;
			}
		}
		else {
			return type;
		}
	}

	GS_Type* GS_Type::get_pointer(int indirection)
	{
		return get_pointer_type(this, indirection);
	}

	GS_Type* GS_Type::get_dynarray()
	{
		return get_dynarray_type(this);
	}

	GS_Type* GS_Type::get_array(u64 size)
	{
		return get_array_type(this, size);
	}

	u64 GS_Type::size()
	{
		return get_type_size(this);
	}
}