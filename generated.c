
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

typedef char i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

const char *__data1 = "i[10]: %zu";
typedef struct Position
{
	i32 x;
	i32 y;
} Position;

typedef struct Player
{
	Position *pos;
} Player;

i32 main()
{
	const i32 __tmp1 = 4;
	const i32 __tmp2 = 68;
	const i32 __tmp3 = __tmp1 * __tmp2;
	;
	const i32 *__tmp4;
	const u64 __tmp5 = (u64)(&__tmp4);
	*((i32 *)__tmp5) = (i32)malloc(__tmp3);
	;
	const i32 __tmp6 = *(i32 *)__tmp5;
	const i32 __tmp7 = 9;
	const u64 __tmp8 = sizeof(i32);
	const u64 __tmp9 = __tmp7 + __tmp8;
	;
	const u64 __tmp10 = __tmp9 + __tmp5;
	;
	const u64 __tmp11 = __tmp10;
	const i32 __tmp12 = 4;
	*((i32 *)__tmp11) = (i32)__tmp12;
	;
	const u8 *__tmp13 = (u8 *)__data1;
	const u8 *__tmp14 = (void *)__tmp13;
	const i32 __tmp15 = *(i32 *)__tmp5;
	const i32 __tmp16 = printf(__tmp14, __tmp15);
}
