#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"

typedef struct Any
{
	uint64_t Type;
	void* Data;
} Any;

typedef struct Array
{
	uint64_t Count;
	void* Data;
} Array;

inline void Func(Array anies) {
	anies.Data = 0;
}

int main(void) {
	Any data[3] = {{0},{0},{0}};

	Array array;

	array.Count = 1;
	array.Data = data;

	Func(array);
}