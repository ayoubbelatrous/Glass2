#include "pch.h"

#include "Base/Array.h"

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
			free(m_Data);
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

	LinearAllocator array_Allocator = LinearAllocator(1024 * 1024 * 100);

	void* Array_Allocator(u64 size)
	{
		return array_Allocator.Allocate_Bytes(size);
	}

}