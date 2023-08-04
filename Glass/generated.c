
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
{	u64 id;
	bool pointer;
	const char* name;
} type_info;
			
static const type_info __type_info_table[15] = {
{.name="void",.id=0},{.name="float",.id=2},{.name="int",.id=1},{.name="i8",.id=3},{.name="i16",.id=4},{.name="i32",.id=5},{.name="i64",.id=6},{.name="u8",.id=7},{.name="u16",.id=8},{.name="u32",.id=9},{.name="u64",.id=10},{.name="f32",.id=11},{.name="f64",.id=12},{.name="bool",.id=13},{.name="type_info",.id=14},};


static const type_info __var_type_info_table[0] = {
};

i32 main ();
const char* __data1 = "A";
const char* __data2 = "char is: %c";
const char* __data3 = "char is: %c";
i32 main (){
const u8* __tmp1 = (u8*)__data1;
const int __tmp2 = 0;
const u64 __tmp3 = sizeof(u8);
const u64 __tmp4 = __tmp2 * __tmp3;;
const u64 __tmp5 = (u64)__tmp1;
const u64 __tmp6 = __tmp4 + __tmp5;;
const u64 __tmp7 = __tmp6;
const u8 __tmp8;
const u64 __tmp9 = (u64)(&__tmp8);
*((u8*)__tmp9) = (u8)__tmp7;;const u8* __tmp10 = (u8*)__data2;
const u8* __tmp11 = (u8*)__data3;
const u8 __tmp12 = *(u8 *)__tmp9;
const i32 __tmp13 = printf(__tmp11, __tmp12);
}

