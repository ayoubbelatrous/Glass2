
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "glad/glad.h"
#include "glfw/include/glfw3.h"
#include "loadgl.h"


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
			
static const type_info __type_info_table[19] = {
{.name="void",.id=0},{.name="float",.id=2},{.name="int",.id=1},{.name="i8",.id=3},{.name="i16",.id=4},{.name="i32",.id=5},{.name="i64",.id=6},{.name="u8",.id=7},{.name="u16",.id=8},{.name="u32",.id=9},{.name="u64",.id=10},{.name="f32",.id=11},{.name="f64",.id=12},{.name="bool",.id=13},{.name="type_info",.id=14},{.name="GLFWwindow",.id=15},{.name="vec2",.id=16},{.name="vec3",.id=17},{.name="vec4",.id=18},};


static const type_info __var_type_info_table[1] = {
{.id=18,.pointer=0,.name="vec4",},};

const char* __data1 = "Glass Game Example";
const char* __data2 = "Glass Game Example";
const char* __data3 = "%s";
const char* __data4 = "%s";
typedef struct vec2{
	f32 x;
	f32 y;
}vec2;

typedef struct vec3{
	f32 x;
	f32 y;
	f32 z;
}vec3;

typedef struct vec4{
	f32 x;
	f32 y;
	f32 z;
	f32 w;
}vec4;

i32 main (){
const int __tmp1 = 256;
const i32 __tmp2;
const u64 __tmp3 = (u64)(&__tmp2);
*((i32*)__tmp3) = (i32)__tmp1;;const int __tmp4 = 16384;
const i32 __tmp5;
const u64 __tmp6 = (u64)(&__tmp5);
*((i32*)__tmp6) = (i32)__tmp4;;glfwInit();const int __tmp7 = 0;
const i32* __tmp8;
const u64 __tmp9 = (u64)(&__tmp8);
*((i32**)__tmp9) = (i32*)__tmp7;;const GLFWwindow* __tmp10;
const u64 __tmp11 = (u64)(&__tmp10);
const int __tmp12 = 1280;
const int __tmp13 = 720;
const u8* __tmp14 = (u8*)__data1;
const u8* __tmp15 = (u8*)__data2;
const i32* __tmp16 = *(i32 **)__tmp9;
const i32* __tmp17 = *(i32 **)__tmp9;
const i32* __tmp18 = *(i32 **)__tmp9;
const i32* __tmp19 = *(i32 **)__tmp9;
const GLFWwindow* __tmp20 = glfwCreateWindow(__tmp12, __tmp13, __tmp15, __tmp17, __tmp19);
*((GLFWwindow**)__tmp11) = (GLFWwindow*)__tmp20;;const GLFWwindow* __tmp21 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp22 = *(GLFWwindow **)__tmp11;
glfwMakeContextCurrent(__tmp22);const i32 __tmp23 = gladInit();
const int __tmp24 = 1;
while (__tmp24) {
const GLFWwindow* __tmp25 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp26 = *(GLFWwindow **)__tmp11;
const float __tmp27 = 0.800000;
const int __tmp28 = 1;
const int __tmp29 = 1;
const int __tmp30 = 1;
const i32 __tmp31 = *(i32 *)__tmp6;
const GLFWwindow* __tmp32 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp33 = *(GLFWwindow **)__tmp11;
const i32 __tmp34 = glfwWindowShouldClose(__tmp33);
const GLFWwindow* __tmp35 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp36 = *(GLFWwindow **)__tmp11;
const i32 __tmp37 = *(i32 *)__tmp3;
const i32 __tmp38 = glfwGetKey(__tmp36, __tmp37);
glfwSwapBuffers(__tmp26);glfwPollEvents();glClearColor(__tmp27, __tmp28, __tmp29, __tmp30);glClear(__tmp31);if (__tmp34) {
break;}
;if (__tmp38) {
break;}
;}
;const type_info* __tmp39 = (&__var_type_info_table[0]);
const type_info* __tmp40;
const u64 __tmp41 = (u64)(&__tmp40);
*((type_info**)__tmp41) = (type_info*)__tmp39;;const u8* __tmp42 = (u8*)__data3;
const u8* __tmp43 = (u8*)__data4;
const u64 __tmp44 = ((u64)&(*(type_info **)__tmp41)->name);
const u8* __tmp45 = *(u8 **)__tmp44;
const i32 __tmp46 = printf(__tmp43, __tmp45);
}

