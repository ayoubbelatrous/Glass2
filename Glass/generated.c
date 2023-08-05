
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
i32 main (){
const int __tmp1 = 0;
const int __tmp2 = 1;
const i32 __tmp3 = ;
if (__tmp3) {
}
;}

