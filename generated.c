
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
			

typedef struct Any
{	
	u64 type;
	u64* data;
} Any;
			
static const type_info __type_info_table[18] = {
{.name="void",.id=0},{.name="float",.id=2},{.name="int",.id=1},{.name="i8",.id=3},{.name="i16",.id=4},{.name="i32",.id=5},{.name="i64",.id=6},{.name="u8",.id=7},{.name="u16",.id=8},{.name="u32",.id=9},{.name="u64",.id=10},{.name="f32",.id=11},{.name="f64",.id=12},{.name="bool",.id=13},{.name="Any",.id=14},{.name="type_info",.id=15},{.name="vec3",.id=16},{.name="mat4",.id=17},};


static const type_info __var_type_info_table[0] = {
};

typedef struct vec3 vec3;typedef struct mat4 mat4;vec3 add_vec3 (vec3 __arg1, vec3 __arg2);
vec3 sub_vec3 (vec3 __arg1, vec3 __arg2);
vec3 mul_vec3 (vec3 __arg1, vec3 __arg2);
vec3 div_vec3 (vec3 __arg1, vec3 __arg2);
vec3 transform_vec3_mat4 (vec3 __arg1, mat4 __arg2);
vec3 add_vec3_f32 (vec3 __arg1, float __arg2);
vec3 add_vec3_float (vec3 __arg1, float __arg2);
void print_vec3 (u8* __arg1, vec3* __arg2);
vec3 _vec3 (f32 __arg1, f32 __arg2, f32 __arg3);
i32 main ();
const char* __data1 = "%s = {x: %f, y: %f, z: %f}\n";
const char* __data2 = "%s = {x: %f, y: %f, z: %f}\n";
const char* __data3 = "x";
const char* __data4 = "x";
typedef struct vec3{
	f32 x;
	f32 y;
	f32 z;
}vec3;

typedef struct mat4{
	f32 x;
	f32 y;
	f32 z;
	f32 w;
	f32 x1;
	f32 y1;
	f32 z1;
	f32 w1;
	f32 x2;
	f32 y2;
	f32 z2;
	f32 w2;
	f32 x3;
	f32 y3;
	f32 z3;
	f32 w3;
}mat4;

vec3 add_vec3 (vec3 __arg1, vec3 __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;u64 __tmp4;u64 __tmp5;u64 __tmp6;f32 __tmp7;u64 __tmp8;f32 __tmp9;f32 __tmp10;u64 __tmp11;u64 __tmp12;f32 __tmp13;u64 __tmp14;f32 __tmp15;f32 __tmp16;u64 __tmp17;u64 __tmp18;f32 __tmp19;u64 __tmp20;f32 __tmp21;f32 __tmp22;vec3 __tmp23; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);

__tmp4 = (u64)(&__tmp3);
__tmp5 = ((u64)&((vec3 *)__tmp4)->x);
__tmp6 = ((u64)&((vec3 *)__tmp1)->x);
__tmp7 = *(f32 *)__tmp6;
__tmp8 = ((u64)&((vec3 *)__tmp2)->x);
__tmp9 = *(f32 *)__tmp8;
__tmp10 = __tmp7 + __tmp9;;
*((f32*)__tmp5) = (f32)__tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 + __tmp15;;
*((f32*)__tmp11) = (f32)__tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 + __tmp21;;
*((f32*)__tmp17) = (f32)__tmp22;;__tmp23 = *(vec3 *)__tmp4;
return __tmp23;
}

vec3 sub_vec3 (vec3 __arg1, vec3 __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;u64 __tmp4;u64 __tmp5;u64 __tmp6;f32 __tmp7;u64 __tmp8;f32 __tmp9;f32 __tmp10;u64 __tmp11;u64 __tmp12;f32 __tmp13;u64 __tmp14;f32 __tmp15;f32 __tmp16;u64 __tmp17;u64 __tmp18;f32 __tmp19;u64 __tmp20;f32 __tmp21;f32 __tmp22;vec3 __tmp23; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);

__tmp4 = (u64)(&__tmp3);
__tmp5 = ((u64)&((vec3 *)__tmp4)->x);
__tmp6 = ((u64)&((vec3 *)__tmp1)->x);
__tmp7 = *(f32 *)__tmp6;
__tmp8 = ((u64)&((vec3 *)__tmp2)->x);
__tmp9 = *(f32 *)__tmp8;
__tmp10 = __tmp7 - __tmp9;;
*((f32*)__tmp5) = (f32)__tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 - __tmp15;;
*((f32*)__tmp11) = (f32)__tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 - __tmp21;;
*((f32*)__tmp17) = (f32)__tmp22;;__tmp23 = *(vec3 *)__tmp4;
return __tmp23;
}

vec3 mul_vec3 (vec3 __arg1, vec3 __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;u64 __tmp4;u64 __tmp5;u64 __tmp6;f32 __tmp7;u64 __tmp8;f32 __tmp9;f32 __tmp10;u64 __tmp11;u64 __tmp12;f32 __tmp13;u64 __tmp14;f32 __tmp15;f32 __tmp16;u64 __tmp17;u64 __tmp18;f32 __tmp19;u64 __tmp20;f32 __tmp21;f32 __tmp22;vec3 __tmp23; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);

__tmp4 = (u64)(&__tmp3);
__tmp5 = ((u64)&((vec3 *)__tmp4)->x);
__tmp6 = ((u64)&((vec3 *)__tmp1)->x);
__tmp7 = *(f32 *)__tmp6;
__tmp8 = ((u64)&((vec3 *)__tmp2)->x);
__tmp9 = *(f32 *)__tmp8;
__tmp10 = __tmp7 * __tmp9;;
*((f32*)__tmp5) = (f32)__tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 * __tmp15;;
*((f32*)__tmp11) = (f32)__tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 * __tmp21;;
*((f32*)__tmp17) = (f32)__tmp22;;__tmp23 = *(vec3 *)__tmp4;
return __tmp23;
}

vec3 div_vec3 (vec3 __arg1, vec3 __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;u64 __tmp4;u64 __tmp5;u64 __tmp6;f32 __tmp7;u64 __tmp8;f32 __tmp9;f32 __tmp10;u64 __tmp11;u64 __tmp12;f32 __tmp13;u64 __tmp14;f32 __tmp15;f32 __tmp16;u64 __tmp17;u64 __tmp18;f32 __tmp19;u64 __tmp20;f32 __tmp21;f32 __tmp22;vec3 __tmp23; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);

__tmp4 = (u64)(&__tmp3);
__tmp5 = ((u64)&((vec3 *)__tmp4)->x);
__tmp6 = ((u64)&((vec3 *)__tmp1)->x);
__tmp7 = *(f32 *)__tmp6;
__tmp8 = ((u64)&((vec3 *)__tmp2)->x);
__tmp9 = *(f32 *)__tmp8;
__tmp10 = __tmp7 / __tmp9;;
*((f32*)__tmp5) = (f32)__tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 / __tmp15;;
*((f32*)__tmp11) = (f32)__tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 / __tmp21;;
*((f32*)__tmp17) = (f32)__tmp22;;__tmp23 = *(vec3 *)__tmp4;
return __tmp23;
}

vec3 transform_vec3_mat4 (vec3 __arg1, mat4 __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;vec3 __tmp4;u64 __tmp5;vec3 __tmp6; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);
__tmp3 = *(vec3 *)__tmp1;

__tmp5 = (u64)(&__tmp4);
*((vec3*)__tmp5) = (vec3)__tmp3;;__tmp6 = *(vec3 *)__tmp5;
return __tmp6;
}

vec3 add_vec3_f32 (vec3 __arg1, float __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;u64 __tmp4;u64 __tmp5;u64 __tmp6;f32 __tmp7;float __tmp8;f32 __tmp9;u64 __tmp10;u64 __tmp11;f32 __tmp12;float __tmp13;f32 __tmp14;u64 __tmp15;u64 __tmp16;f32 __tmp17;float __tmp18;f32 __tmp19;vec3 __tmp20; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);

__tmp4 = (u64)(&__tmp3);
__tmp5 = ((u64)&((vec3 *)__tmp4)->x);
__tmp6 = ((u64)&((vec3 *)__tmp1)->x);
__tmp7 = *(f32 *)__tmp6;
__tmp8 = *(float *)__tmp2;
__tmp9 = __tmp7 + __tmp8;;
*((f32*)__tmp5) = (f32)__tmp9;;__tmp10 = ((u64)&((vec3 *)__tmp4)->y);
__tmp11 = ((u64)&((vec3 *)__tmp1)->y);
__tmp12 = *(f32 *)__tmp11;
__tmp13 = *(float *)__tmp2;
__tmp14 = __tmp12 + __tmp13;;
*((f32*)__tmp10) = (f32)__tmp14;;__tmp15 = ((u64)&((vec3 *)__tmp4)->z);
__tmp16 = ((u64)&((vec3 *)__tmp1)->z);
__tmp17 = *(f32 *)__tmp16;
__tmp18 = *(float *)__tmp2;
__tmp19 = __tmp17 + __tmp18;;
*((f32*)__tmp15) = (f32)__tmp19;;__tmp20 = *(vec3 *)__tmp4;
return __tmp20;
}

vec3 add_vec3_float (vec3 __arg1, float __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;u64 __tmp4;u64 __tmp5;u64 __tmp6;f32 __tmp7;float __tmp8;f32 __tmp9;u64 __tmp10;u64 __tmp11;f32 __tmp12;float __tmp13;f32 __tmp14;u64 __tmp15;u64 __tmp16;f32 __tmp17;float __tmp18;f32 __tmp19;vec3 __tmp20; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);

__tmp4 = (u64)(&__tmp3);
__tmp5 = ((u64)&((vec3 *)__tmp4)->x);
__tmp6 = ((u64)&((vec3 *)__tmp1)->x);
__tmp7 = *(f32 *)__tmp6;
__tmp8 = *(float *)__tmp2;
__tmp9 = __tmp7 + __tmp8;;
*((f32*)__tmp5) = (f32)__tmp9;;__tmp10 = ((u64)&((vec3 *)__tmp4)->y);
__tmp11 = ((u64)&((vec3 *)__tmp1)->y);
__tmp12 = *(f32 *)__tmp11;
__tmp13 = *(float *)__tmp2;
__tmp14 = __tmp12 + __tmp13;;
*((f32*)__tmp10) = (f32)__tmp14;;__tmp15 = ((u64)&((vec3 *)__tmp4)->z);
__tmp16 = ((u64)&((vec3 *)__tmp1)->z);
__tmp17 = *(f32 *)__tmp16;
__tmp18 = *(float *)__tmp2;
__tmp19 = __tmp17 + __tmp18;;
*((f32*)__tmp15) = (f32)__tmp19;;__tmp20 = *(vec3 *)__tmp4;
return __tmp20;
}

void print_vec3 (u8* __arg1, vec3* __arg2){
 u64 __tmp1;u64 __tmp2;u8* __tmp3;u8* __tmp4;u8* __tmp5;u64 __tmp6;f32 __tmp7;u64 __tmp8;f32 __tmp9;u64 __tmp10;f32 __tmp11;i32 __tmp12; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);
__tmp3 = (u8*)__data1;
__tmp4 = (u8*)__data2;
__tmp5 = *(u8 **)__tmp1;
__tmp6 = ((u64)&(*(vec3 **)__tmp2)->x);
__tmp7 = *(f32 *)__tmp6;
__tmp8 = ((u64)&(*(vec3 **)__tmp2)->y);
__tmp9 = *(f32 *)__tmp8;
__tmp10 = ((u64)&(*(vec3 **)__tmp2)->z);
__tmp11 = *(f32 *)__tmp10;
__tmp12 = printf(__tmp4, __tmp5, __tmp7, __tmp9, __tmp11);
}

vec3 _vec3 (f32 __arg1, f32 __arg2, f32 __arg3){
 u64 __tmp1;u64 __tmp2;u64 __tmp3;vec3 __tmp4;u64 __tmp5;u64 __tmp6;f32 __tmp7;u64 __tmp8;f32 __tmp9;u64 __tmp10;f32 __tmp11;vec3 __tmp12; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);
__tmp3 = (u64)(&__arg3);

__tmp5 = (u64)(&__tmp4);
__tmp6 = ((u64)&((vec3 *)__tmp5)->x);
__tmp7 = *(f32 *)__tmp1;
*((f32*)__tmp6) = (f32)__tmp7;;__tmp8 = ((u64)&((vec3 *)__tmp5)->y);
__tmp9 = *(f32 *)__tmp2;
*((f32*)__tmp8) = (f32)__tmp9;;__tmp10 = ((u64)&((vec3 *)__tmp5)->z);
__tmp11 = *(f32 *)__tmp3;
*((f32*)__tmp10) = (f32)__tmp11;;__tmp12 = *(vec3 *)__tmp5;
return __tmp12;
}

i32 main (){
 int __tmp1;int __tmp2;int __tmp3;vec3 __tmp4;vec3 __tmp5;u64 __tmp6;int __tmp7;int __tmp8;int __tmp9;vec3 __tmp10;vec3 __tmp11;u64 __tmp12;vec3 __tmp13;float __tmp14;vec3 __tmp15;vec3 __tmp16;float __tmp17;vec3 __tmp18;vec3 __tmp19;u64 __tmp20;u8* __tmp21;u8* __tmp22;vec3* __tmp23;vec3* __tmp24; __tmp1 = 1;
__tmp2 = 1;
__tmp3 = 1;
__tmp4 = _vec3(__tmp1, __tmp2, __tmp3);

__tmp6 = (u64)(&__tmp5);
*((vec3*)__tmp6) = (vec3)__tmp4;;__tmp7 = 1;
__tmp8 = 2;
__tmp9 = 1;
__tmp10 = _vec3(__tmp7, __tmp8, __tmp9);

__tmp12 = (u64)(&__tmp11);
*((vec3*)__tmp12) = (vec3)__tmp10;;__tmp13 = *(vec3 *)__tmp6;
__tmp14 = 2.000000;

__tmp16 = *(vec3 *)__tmp6;
__tmp17 = 2.000000;
__tmp18 = add_vec3_f32(__tmp16, __tmp17);

__tmp20 = (u64)(&__tmp19);
*((vec3*)__tmp20) = (vec3)__tmp18;;__tmp21 = (u8*)__data3;
__tmp22 = (u8*)__data4;
__tmp23 = (void *)__tmp20;
__tmp24 = (void *)__tmp20;
print_vec3(__tmp22, __tmp24);}

