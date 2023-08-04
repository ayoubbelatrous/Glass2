
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define true 1;
#define false 0;
typedef char i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef u8 bool;

typedef struct type_info
{
	u64 id;
	bool pointer;
	const char *name;
} type_info;

static const type_info __type_info_table[16] = {
	{.name = "void", .id = 0},
	{.name = "float", .id = 2},
	{.name = "int", .id = 1},
	{.name = "i8", .id = 3},
	{.name = "i16", .id = 4},
	{.name = "i32", .id = 5},
	{.name = "i64", .id = 6},
	{.name = "u8", .id = 7},
	{.name = "u16", .id = 8},
	{.name = "u32", .id = 9},
	{.name = "u64", .id = 10},
	{.name = "f32", .id = 11},
	{.name = "f64", .id = 12},
	{.name = "bool", .id = 13},
	{.name = "type_info", .id = 14},
	{.name = "string", .id = 15},
};

static const type_info __var_type_info_table[0] = {};

typedef struct string string;
string new_string(u8 *__arg1);
void free_string(string *__arg1);
i32 main();
const char *__data1 = "Hello world";
const char *__data2 = "Hello world";
typedef struct string
{
	u8 *data;
	u64 size;
} string;

string new_string(u8 *__arg1)
{
	const u64 __tmp1 = (u64)(&__arg1);
	const string __tmp2;
	const u64 __tmp3 = (u64)(&__tmp2);
	const u64 __tmp4 = ((u64) & ((string *)__tmp3)->data);
	const u8 *__tmp5 = *(u8 **)__tmp1;
	*((u8 **)__tmp4) = (u8 *)__tmp5;
	;
	const u64 __tmp6 = ((u64) & ((string *)__tmp3)->size);
	const u8 *__tmp7 = *(u8 **)__tmp1;
	const u8 *__tmp8 = *(u8 **)__tmp1;
	const u64 __tmp9 = strlen(__tmp8);
	*((u64 *)__tmp6) = (u64)__tmp9;
	;
	const string __tmp10 = *(string *)__tmp3;
	return __tmp10;
}

void free_string(string *__arg1)
{
	const u64 __tmp1 = (u64)(&__arg1);
	const u64 __tmp2 = ((u64) & (*(string **)__tmp1)->data);
	const u8 *__tmp3 = *(u8 **)__tmp2;
	const u64 *__tmp4;
	const u64 __tmp5 = (u64)(&__tmp4);
	*((u64 **)__tmp5) = (u64 *)__tmp3;
	;
	const u64 *__tmp6 = *(u64 **)__tmp5;
	const u64 *__tmp7 = *(u64 **)__tmp5;
	free(__tmp7);
	const u64 __tmp8 = ((u64) & (*(string **)__tmp1)->data);
	const int __tmp9 = 0;
	*((u8 **)__tmp8) = (u8 *)__tmp9;
	;
	const u64 __tmp10 = ((u64) & (*(string **)__tmp1)->size);
	const int __tmp11 = 0;
	*((u64 *)__tmp10) = (u64)__tmp11;
	;
}

i32 main()
{
	const u8 *__tmp1 = (u8 *)__data1;
	const u8 *__tmp2 = (u8 *)__data2;
	const string __tmp3 = new_string(__tmp2);
	const string __tmp4;
	const u64 __tmp5 = (u64)(&__tmp4);
	*((string *)__tmp5) = (string)__tmp3;
	;
}
