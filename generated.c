
#include <stdint.h>
#include <stdio.h>

typedef char i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

const char *__data1 = "Hello";
const char *__data2 = "Hello: %s";
typedef struct Entity
{
	u8 *ID;
} Entity;

i32 main()
{
	const Entity __tmp1;
	const u64 __tmp2 = (u64)(&__tmp1);
	const u64 __tmp3 = (u64)(&((Entity *)__tmp2)->ID);
	const u8 *__tmp4 = (u8 *)__data1;
	*((u64 *)__tmp3) = (u64)__tmp4;
	;
	const u8 *__tmp5 = (u8 *)__data2;
	const u8 *__tmp6 = (void *)__tmp5;
	const u64 __tmp7 = (u64)(&((Entity *)__tmp2)->ID);
	const u8 *__tmp8 = *(u64 *)__tmp7;
	printf(__tmp6, __tmp8);
}
