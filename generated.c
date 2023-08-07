
#include <stdio.h>
#include <string.h>
#include <malloc.h>


#define true 1;		
#define false 0;		
typedef signed char i8;		
typedef short i16;	
typedef int i32;	
typedef long long i64;	

typedef unsigned char u8;	
typedef unsigned short u16;	
typedef unsigned int u32;	
typedef unsigned long long u64;	
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
const char* __data1 = "Hello World";
const char* __data2 = "Hello World";
const char* __data3 = "Hello World: %i";
const char* __data4 = "Hello World: %i";
i32 main (){
const u8* __tmp1 = (u8*)__data1;
const u8* __tmp2 = (u8*)__data2;
const i32 __tmp3 = printf(__tmp2);
const i32 __tmp4;
const u64 __tmp5 = (u64)(&__tmp4);
const int __tmp6 = 30;
*((i32*)__tmp5) = (i32)__tmp6;;const u8* __tmp7 = (u8*)__data3;
const u8* __tmp8 = (u8*)__data4;
const i32 __tmp9 = *(i32 *)__tmp5;
const i32 __tmp10 = printf(__tmp8, __tmp9);
}

