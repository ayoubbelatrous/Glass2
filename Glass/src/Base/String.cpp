#include "pch.h"

#include "Base/String.h"

namespace Glass {

	struct LinearAllocator
	{
		LinearAllocator() {
			m_Data = nullptr;
		}

		LinearAllocator(u64 pre_allocate) {
			m_Data = (uint8_t*)malloc(pre_allocate);
			memset(m_Data, 0, m_Capacity);
			m_Capacity = pre_allocate;
		}

		~LinearAllocator() {
			//free(m_Data);
		}

		template<typename T>
		T* Allocate(const T& d) {

			if ((m_Size + sizeof(T)) > m_Capacity) {
				GS_CORE_ASSERT(0, "Out Of Memory");
			}

			void* dst = (void*)(m_Data + m_Size);

			new (dst) T(d);

			T* result = (T*)(m_Data + m_Size);

			m_Size += sizeof(T);

			return result;
		}

		void* Allocate_Bytes(const u64 size) {

			if ((m_Size + 1) > m_Capacity) {
				GS_CORE_ASSERT(0, "Out Of Memory");
			}

			void* data = (void*)(m_Data + m_Size);
			m_Size += size;
			return data;
		}

	private:
		u64 m_Capacity = 0;
		u64 m_Size = 0;
		uint8_t* m_Data = nullptr;
	};

	LinearAllocator string_Allocator = LinearAllocator(1024 * 1024 * 100);

	String String_Make(const std::string& std_str)
	{
		char* bytes = (char*)string_Allocator.Allocate_Bytes(std_str.size() + 1);
		bytes[std_str.size()] = 0;
		memcpy((void*)bytes, (void*)std_str.data(), std_str.size());

		String str;
		str.count = std_str.size();
		str.data = bytes;
		return str;
	}

	String String_Make(char* c_str)
	{
		String str;
		str.count = strlen(c_str);
		str.data = c_str;
		return str;
	}

	String String_Copy(String other)
	{
		char* bytes = (char*)string_Allocator.Allocate_Bytes(other.count + 1);
		bytes[other.count] = 0;
		memcpy((void*)bytes, other.data, other.count);

		String str;
		str.count = other.count;
		str.data = bytes;
		return str;
	}

	bool String_Equal(String a, String b)
	{
		if (a.count != b.count)
			return false;

		if (strncmp(a.data, b.data, a.count) != 0)
			return false;

		return true;
	}
}