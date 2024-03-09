#pragma once

#include "Base/String.h"
#include "Base/Array.h"
#include "Base/Hash.h"

namespace Glass
{
	enum Type_Name_Flag : u64
	{
		TN_Base_Type = BIT(0),
		TN_Numeric_Type = BIT(1),
		TN_Bool_Type = BIT(2),
		TN_Float_Type = BIT(3),
		TN_Unsigned_Type = BIT(4),
		TN_Struct_Type = BIT(5),
		TN_Enum_Type = BIT(6),
		TN_Pointer_Type = BIT(7),
	};

	using Type_Name_ID = u32;
	using Struct_ID = u32;
	using Type_Name_Flags = u64;
	using Type_IDX = u32;

	struct Type_Name
	{
		String name;
		Type_Name_Flags flags = 0;
		u64 size = -1;
		u64 alignment = -1;
		Struct_ID struct_id = -1;
	};

	struct GS_Type;

	struct GS_Basic_Type
	{
		Type_Name_ID type_name_id;
	};

	struct GS_Pointer_Type
	{
		GS_Type* pointee;
		u32 indirection;
	};

	struct GS_Array_Type
	{
		GS_Type* element_type;
		u64 size;
	};

	struct GS_Dyn_Array_Type
	{
		GS_Type* element_type;
	};

	struct GS_Proc_Type
	{
		GS_Type* return_type;
		Array_UI<GS_Type*> params;
	};

	struct GS_Struct_Type
	{
		Array<GS_Type*> members;
	};

	enum Type_Kind
	{
		Type_Basic,
		Type_Pointer,
		Type_Array,
		Type_Dyn_Array,
		Type_Proc,
	};

	struct GS_Type
	{
		GS_Type() = default;

		Type_Kind kind;
		u64 type_hash;

		union
		{
			GS_Basic_Type		 basic;
			GS_Pointer_Type		 pointer;
			GS_Array_Type		 array;
			GS_Dyn_Array_Type	 dyn_array;
			GS_Proc_Type		 proc;
		};

		GS_Type* get_pointer(int indirection = 1);
		GS_Type* get_dynarray();
		GS_Type* get_array(u64 size);
		u64 size();
	};

	struct GS_Struct
	{
		Array<GS_Type*> members;
		Array<u64> offsets;
	};

	struct Type_System
	{
		Array<Type_Name> type_name_storage;
		Array<GS_Struct> struct_storage;
		Array<GS_Type> type_storage;
		std::unordered_map<u64, GS_Type*> type_lookup;

		GS_Type* void_Ty = nullptr;

		GS_Type* i8_Ty = nullptr;
		GS_Type* i16_Ty = nullptr;
		GS_Type* i32_Ty = nullptr;
		GS_Type* i64_Ty = nullptr;

		GS_Type* u8_Ty = nullptr;
		GS_Type* u16_Ty = nullptr;
		GS_Type* u32_Ty = nullptr;
		GS_Type* u64_Ty = nullptr;

		GS_Type* f64_Ty = nullptr;
		GS_Type* f32_Ty = nullptr;

		GS_Type* int_Ty = nullptr;
		GS_Type* float_Ty = nullptr;
		GS_Type* void_ptr_Ty = nullptr;

		GS_Type* Array_Ty = nullptr;
		GS_Type* Type_Ty = nullptr;
		GS_Type* Any_Ty = nullptr;
		GS_Type* string_Ty = nullptr;
	};

	struct GS_Struct_Data_Layout
	{
		u64 size;
		u64 alignment;
		Array<u64> offsets;
	};


	void init_typesystem();
	Type_Name_ID insert_typename(Type_Name type_name);
	Type_Name_ID insert_typename_struct(Type_Name type_name, GS_Struct strct);

	Type_Name_ID insert_struct(String name, Array<GS_Type*> members);
	void insert_struct(int typename_id, Array<GS_Type*> members);

	Type_IDX get_type_index(GS_Type* type);
	u64 get_type_flags(GS_Type* type);
	u64 get_type_flags(int index);
	GS_Type* get_type_at(int index);

	bool is_type_aggr(GS_Type* type);

	GS_Type* get_type(Type_Name_ID type_name_id);
	GS_Type* get_pointer_type(GS_Type* pointee, u32 indirection);
	GS_Type* get_proc_type(GS_Type* return_type, Array<GS_Type*> params);
	GS_Type* get_dynarray_type(GS_Type* element);
	GS_Type* get_array_type(GS_Type* element, u64 size);

	GS_Struct& get_struct(GS_Type* type);

	u64 get_type_size(GS_Type* type);

	Type_System& get_ts();

	String print_type(GS_Type* type);
	String print_type_index(int type_idx);

	GS_Struct_Data_Layout struct_compute_align_size_offsets(Array<GS_Type*> members);

	void TypeSystem_Init(Type_System& ts);

	Type_Name_ID TypeSystem_Insert_TypeName(Type_System& ts, Type_Name type_name);
	Type_Name_ID TypeSystem_Insert_TypeName_Struct(Type_System& ts, Type_Name type_name, GS_Struct strct);

	inline u64 GS_Basic_Type_Hash(Type_Name_ID type_name_id) {
		return Combine2Hashes(std::hash<u32>{}(type_name_id), std::hash<std::string>{}("basic_type"));
	}

	inline u64 GS_Pointer_Type_Hash(u64 pointee_hash, u32 indirection) {
		return Combine2Hashes(pointee_hash, std::hash<u32>{}(indirection));
	}

	inline u64 GS_Dyn_Array_Type_Hash(u64 element_hash) {
		return Combine2Hashes(std::hash<u64>{}(element_hash), std::hash<std::string>{}("dyn_array_type"));
	}

	inline u64 GS_Array_Type_Hash(u64 element_hash, u64 size) {
		return Combine2Hashes(std::hash<u64>{}(size), Combine2Hashes(std::hash<u64>{}(element_hash), std::hash<std::string>{}("array_type")));
	}

	inline u64 GS_Proc_Type_Hash(u64 return_type_hash, Array<u64> param_hashes) {
		u64 hash = std::hash<std::string>{}("proc_type");

		for (u64 i = 0; i < param_hashes.count; i++)
		{
			hash = Combine2Hashes(hash, param_hashes[i]);
		}

		return Combine2Hashes(hash, return_type_hash);
	}

	Type_IDX TypeSystem_Get_Type_Index(Type_System& ts, GS_Type* type);
	GS_Type* TypeSystem_Get_Basic_Type(Type_System& ts, Type_Name_ID type_name_id);
	GS_Type* TypeSystem_Get_Pointer_Type(Type_System& ts, GS_Type* pointee, u32 indirection);
	GS_Type* TypeSystem_Get_Dyn_Array_Type(Type_System& ts, GS_Type* element);
	GS_Type* TypeSystem_Get_Array_Type(Type_System& ts, GS_Type* element, u64 size);
	GS_Type* TypeSystem_Get_Proc_Type(Type_System& ts, GS_Type* return_type, Array<GS_Type*> params);

	GS_Type* TypeSystem_Increase_Ind(Type_System& ts, GS_Type* type, u32 amount = 1);

	Type_Name_Flags TypeSystem_Get_Type_Flags(Type_System& ts, GS_Type* type);
	Type_Name_Flags TypeSystem_Get_Type_Alignment(Type_System& ts, GS_Type* type);
	u64 TypeSystem_Get_Type_Size(Type_System& ts, GS_Type* type);
	GS_Struct_Data_Layout TypeSystem_Struct_Compute_Align_Size_Offsets(Type_System& ts, Array<GS_Type*> members);

	String TypeSystem_Print_Type(Type_System& ts, GS_Type* type);
	String TypeSystem_Print_Type_Index(Type_System& ts, u64 type_idx);

	GS_Type* TypeSystem_Reduce_Indirection(Type_System& ts, GS_Type* type);
}
