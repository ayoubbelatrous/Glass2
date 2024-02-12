#pragma once

#include "stdlib.h"
#include <cstring>

namespace Glass
{
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
		arr.data = (E*)malloc(sizeof(E) * capacity);
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
		arr.data = (E*)malloc(sizeof(E) * arr.capacity);

		memcpy((void*)arr.data, (void*)vec.data(), arr.count * sizeof(E));

		return arr;
	}

	template<typename E>
	Array<E> Array_Copy(Array<E> other) {

		Array<E> arr = {};
		arr.count = other.count;
		arr.capacity = other.capacity;
		arr.data = (E*)malloc(sizeof(E) * arr.capacity);

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
			arr.data = (E*)malloc(arr.capacity * sizeof(E));
		}

		if (arr.count == arr.capacity) {
			arr.capacity *= 2;

			E* old_data = arr.data;
			arr.data = (E*)malloc(arr.capacity * sizeof(E));
			memcpy(arr.data, old_data, arr.count * sizeof(E));
			free(old_data);
		}

		void* dst = (void*)&arr.data[arr.count];

		new (dst) E(element);

		arr.count++;

		return (E*)dst;
	}
}