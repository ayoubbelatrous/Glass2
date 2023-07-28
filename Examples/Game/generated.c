
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "glfw/include/glfw3.h"


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

const char* __data1 = "Hello";
const char* __data2 = "Hello";
typedef struct GLFWwindow{
	i32 null0;
}GLFWwindow;

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
const int __tmp12 = 400;
const int __tmp13 = 400;
const u8* __tmp14 = (u8*)__data1;
const u8* __tmp15 = (u8*)__data2;
const i32* __tmp16 = *(i32 **)__tmp9;
const i32* __tmp17 = *(i32 **)__tmp9;
const i32* __tmp18 = *(i32 **)__tmp9;
const i32* __tmp19 = *(i32 **)__tmp9;
const GLFWwindow* __tmp20 = glfwCreateWindow(__tmp12, __tmp13, __tmp15, __tmp17, __tmp19);
*((GLFWwindow**)__tmp11) = (GLFWwindow*)__tmp20;;const GLFWwindow* __tmp21 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp22 = *(GLFWwindow **)__tmp11;
glfwMakeContextCurrent(__tmp22);const int __tmp23 = 1;
while (__tmp23) {
const float __tmp24 = 0.500000;
const float __tmp25 = 0.500000;
const float __tmp26 = 0.500000;
const float __tmp27 = 0.500000;
const i32 __tmp28 = *(i32 *)__tmp6;
const GLFWwindow* __tmp29 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp30 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp31 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp32 = *(GLFWwindow **)__tmp11;
const i32 __tmp33 = glfwWindowShouldClose(__tmp32);
const GLFWwindow* __tmp34 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp35 = *(GLFWwindow **)__tmp11;
const i32 __tmp36 = *(i32 *)__tmp3;
const i32 __tmp37 = glfwGetKey(__tmp35, __tmp36);
glClearColor(__tmp24, __tmp25, __tmp26, __tmp27);glClear(__tmp28);glfwSwapBuffers(__tmp30);glfwPollEvents();if (__tmp33) {
break;}
;if (__tmp37) {
break;}
;}
;while (__tmp23) {
const float __tmp24 = 0.500000;
const float __tmp25 = 0.500000;
const float __tmp26 = 0.500000;
const float __tmp27 = 0.500000;
const i32 __tmp28 = *(i32 *)__tmp6;
const GLFWwindow* __tmp29 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp30 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp31 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp32 = *(GLFWwindow **)__tmp11;
const i32 __tmp33 = glfwWindowShouldClose(__tmp32);
const GLFWwindow* __tmp34 = *(GLFWwindow **)__tmp11;
const GLFWwindow* __tmp35 = *(GLFWwindow **)__tmp11;
const i32 __tmp36 = *(i32 *)__tmp3;
const i32 __tmp37 = glfwGetKey(__tmp35, __tmp36);
glClearColor(__tmp24, __tmp25, __tmp26, __tmp27);glClear(__tmp28);glfwSwapBuffers(__tmp30);glfwPollEvents();if (__tmp33) {
break;}
;if (__tmp37) {
break;}
;}
;}

