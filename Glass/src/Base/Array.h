#pragma once

#include "stdlib.h"
#include <cstring>

namespace Glass
{

	void* Array_Allocator(u64 size);

	template<typename E>
	struct Array;

	template<typename E>
	struct Array_UI;

	template<typename E>
	E* Array_Add(Array<E>& arr, const E& element);

	template<typename E>
	E* Array_Add(Array_UI<E>& arr, const E& element);

	template<typename E>
	struct Array_UI
	{
		u64 count;
		u64 capacity;
		E* data;

		E& operator[](std::size_t index) {
			if (index >= count) {
				ASSERT("Array Out of bounds access");
				return data[count - 1];
			}
			return data[index];
		}

		const E& operator[](std::size_t index) const {
			if (index >= count) {
				ASSERT("Array Out of bounds access");
				return data[count - 1];
			}
			return data[index];
		}
	};

	template<typename E>
	struct Array
	{
		Array() = default;

		Array(Array_UI<E> other) {
			count = other.count;
			capacity = other.capacity;
			data = other.data;
		}

		u64 count = 0;
		u64 capacity = 0;
		E* data = nullptr;

		E& operator[](std::size_t index) {
			if (index >= count) {
				ASSERT(nullptr, "Array Out of bounds access");
				return data[count - 1];
			}
			return data[index];
		}

		const E& operator[](std::size_t index) const {
			if (index >= count) {
				ASSERT(nullptr, "Array Out of bounds access");
				return data[count - 1];
			}
			return data[index];
		}

		void Add(const E& element) {
			Array_Add(*this, element);
		}
	};

	template<typename E>
	Array<E> Array_Reserved(std::size_t capacity) {

		Array<E> arr = {};
		arr.capacity = capacity;
		arr.data = (E*)Array_Allocator(sizeof(E) * capacity);
		return arr;
	}

	template<typename E>
	Array<E> Array_View(E* data, std::size_t count) {

		Array<E> arr = {};
		arr.count = count;
		arr.capacity = count;
		arr.data = data;
		return arr;
	}

	template<typename E>
	Array<E> Array_From_Vec(const std::vector<E>& vec) {

		Array<E> arr = {};
		arr.count = vec.size();
		arr.capacity = vec.capacity();
		arr.data = (E*)Array_Allocator(sizeof(E) * arr.capacity);

		memcpy((void*)arr.data, (void*)vec.data(), arr.count * sizeof(E));

		return arr;
	}

	template<typename E>
	Array<E> Array_Copy(Array<E> other) {

		Array<E> arr = {};
		arr.count = other.count;
		arr.capacity = other.capacity;
		arr.data = (E*)Array_Allocator(sizeof(E) * arr.capacity);

		memcpy((void*)arr.data, (void*)other.data, arr.count * sizeof(E));

		return arr;
	}

	template<typename E>
	E* Array_Add(Array_UI<E>& arr, const E& element) {
		return Array_Add(*(Array<E>*) & arr, element);
	}

	template<typename E>
	E* Array_Add(Array<E>& arr, const E& element) {

		if (arr.capacity == 0) {
			arr.capacity++;
			arr.data = (E*)Array_Allocator(arr.capacity * sizeof(E));
		}

		if (arr.count == arr.capacity) {
			arr.capacity *= 2;

			E* old_data = arr.data;
			arr.data = (E*)Array_Allocator(arr.capacity * sizeof(E));
			memcpy(arr.data, old_data, arr.count * sizeof(E));
		}

		void* dst = (void*)&arr.data[arr.count];

		new (dst) E(element);

		arr.count++;

		return (E*)dst;
	}

	template<typename E>
	E* Array_Insert_After(Array<E>& arr, size_t index, const E& element) {

		if (index < 0 || index > arr.count) {
			ASSERT(nullptr, "Array Out of bounds insert");
			return nullptr;
		}

		if (arr.count == arr.capacity) {
			arr.capacity *= 2;

			E* old_data = arr.data;
			arr.data = (E*)Array_Allocator(arr.capacity * sizeof(E));
			memcpy(arr.data, old_data, arr.count * sizeof(E));
		}

		memcpy(arr.data + index + 1, arr.data + index, (arr.count - index) * sizeof(E));

		arr.count++;

		void* dst = (void*)&arr.data[index];
		new (dst) E(element);

		return (E*)dst;
	}

#define For(arr, expr) for (u64 it_index = 0; it_index < arr.count; it_index++) {auto& it = arr[it_index];\
expr}
}