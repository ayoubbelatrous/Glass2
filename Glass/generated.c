
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
			
static const type_info __type_info_table[16] = {
{.name="void",.id=0},{.name="float",.id=2},{.name="int",.id=1},{.name="i8",.id=3},{.name="i16",.id=4},{.name="i32",.id=5},{.name="i64",.id=6},{.name="u8",.id=7},{.name="u16",.id=8},{.name="u32",.id=9},{.name="u64",.id=10},{.name="f32",.id=11},{.name="f64",.id=12},{.name="bool",.id=13},{.name="type_info",.id=14},{.name="Test",.id=15},};


static const type_info __var_type_info_table[0] = {
};

typedef struct Test Test;i32 main ();
typedef struct Test{
	i32 Hello;
}Test;

i32 main (){
 i32 __tmp1;u64 __tmp2;int __tmp3; 
__tmp2 = (u64)(&__tmp1);
__tmp3 = 30;
*((i32*)__tmp2) = (i32)__tmp3;;}

