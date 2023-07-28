
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
typedef float f32;
typedef double f64;

const char *__data1 = "Renderer.ID: %i";
const char *__data2 = "Renderer.ID: %i";
typedef struct GLFWwindow
{
} GLFWwindow;

typedef struct Renderer
{
	i32 ID;
} Renderer;

Renderer *RendererAllocate()
{
	const int __tmp1 = 4;
	const Renderer *__tmp2;
	const u64 __tmp3 = (u64)(&__tmp2);
	*((Renderer **)__tmp3) = (Renderer *)malloc(__tmp1);
	;
	const u64 __tmp4 = ((u64) & (*(Renderer **)__tmp3)->ID);
	const int __tmp5 = 30;
	*((i32 *)__tmp4) = (i32)__tmp5;
	;
	const Renderer *__tmp6 = *(Renderer **)__tmp3;
	return __tmp6;
}

void RendererInit(Renderer *__arg1)
{
	const u64 __tmp1 = (u64)(&__arg1);
	const u8 *__tmp2 = (u8 *)__data1;
	const u8 *__tmp3 = (u8 *)__data2;
	const u64 __tmp4 = ((u64) & (*(Renderer **)__tmp1)->ID);
	const u64 __tmp5 = *(u64 *)__tmp4;
	const i32 __tmp6 = printf(__tmp3, __tmp5);
}

i32 main()
{
	const float __tmp1 = 1.500000;
	const f32 __tmp2;
	const u64 __tmp3 = (u64)(&__tmp2);
	*((f32 *)__tmp3) = (f32)__tmp1;
	;
	const Renderer *__tmp4 = RendererAllocate();
	const Renderer *__tmp5;
	const u64 __tmp6 = (u64)(&__tmp5);
	*((Renderer **)__tmp6) = (Renderer *)__tmp4;
	;
	const Renderer *__tmp7 = *(Renderer **)__tmp6;
	const Renderer *__tmp8 = *(Renderer **)__tmp6;
	RendererInit(__tmp8);
}
