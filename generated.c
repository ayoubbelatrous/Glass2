
#include "Examples/Game/glInit.h"


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
			

typedef struct Array
{	
	u64 count;
	u64 data;
} Array;
			
static const type_info __type_info_table[20] = {
{.name="void",.id=0},{.name="float",.id=2},{.name="int",.id=1},{.name="i8",.id=3},{.name="i16",.id=4},{.name="i32",.id=5},{.name="i64",.id=6},{.name="u8",.id=7},{.name="u16",.id=8},{.name="u32",.id=9},{.name="u64",.id=10},{.name="f32",.id=11},{.name="f64",.id=12},{.name="bool",.id=13},{.name="Any",.id=14},{.name="Array",.id=15},{.name="type_info",.id=16},{.name="GLFWwindow",.id=17},{.name="vec3",.id=18},{.name="mat4",.id=19},};

typedef struct GLFWwindow GLFWwindow;i32 printf (u8* format,...);
i32 sprintf (u8* format, u8* buffer,...);
u64* fopen (u8* path, u8* mode);
void fclose (u64* file);
u64* malloc (u64 size);
void free (u64 ptr);
u64 strlen (u8* ptr);
u64 memcpy (u8* dst, u8* src, int count);
void putchar (u8 c);
i32 printf (u8* format,...);
GLFWwindow* glfwCreateWindow (i32 width, i32 height, u8* title, i32* monitor, i32* display);
i32 glfwInit ();
void glfwMakeContextCurrent (GLFWwindow* window);
void glfwSwapBuffers (GLFWwindow* window);
void glfwPollEvents ();
i32 glfwWindowShouldClose (GLFWwindow* window);
i32 glfwGetKey (GLFWwindow* window, i32 keyCode);
void gladInit ();
void glClearColor (f32 r, f32 g, f32 b, f32 a);
void glClear (i32 mask);
void glBegin (i32 mode);
void glEnd ();
void glColor3f (f32 r, f32 g, f32 b);
void glVertex2f (f32 x, f32 y);
typedef struct vec3 vec3;typedef struct mat4 mat4;vec3 add_vec3 (vec3 __arg1, vec3 __arg2);
vec3 sub_vec3 (vec3 __arg1, vec3 __arg2);
vec3 mul_vec3 (vec3 __arg1, vec3 __arg2);
vec3 div_vec3 (vec3 __arg1, vec3 __arg2);
vec3 transform_vec3_mat4 (vec3 __arg1, mat4 __arg2);
vec3 add_vec3_f32 (vec3 __arg1, float __arg2);
vec3 add_vec3_float (vec3 __arg1, float __arg2);
void print_vec3 (u8* __arg1, vec3* __arg2);
vec3 _vec3 (f32 __arg1, f32 __arg2, f32 __arg3);
void print_any (Any __arg1);
void print (u8* __arg1, Array __arg2);
void main ();
const char* __data1 = "%s = {x: %f, y: %f, z: %f}\n";
const char* __data2 = "%s = {x: %f, y: %f, z: %f}\n";
const char* __data3 = "%i";
const char* __data4 = "%i";
const char* __data5 = "%i";
const char* __data6 = "%i";
const char* __data7 = "%f";
const char* __data8 = "%f";
const char* __data9 = "%f";
const char* __data10 = "%f";
const char* __data11 = "%s";
const char* __data12 = "%s";
const char* __data13 = "vec3::{%f, %f, %f}";
const char* __data14 = "vec3::{%f, %f, %f}";
const char* __data15 = "%";
const char* __data16 = "%";
const char* __data17 = "A Window";
const char* __data18 = "A Window";
const char* __data19 = "%";
const char* __data20 = "%";
const char* __data21 = "Closing";
const char* __data22 = "Closing";
const char* __data23 = "%";
const char* __data24 = "%";
const char* __data25 = "Closing";
const char* __data26 = "Closing";
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
*((f32*)__tmp5) = __tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 + __tmp15;;
*((f32*)__tmp11) = __tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 + __tmp21;;
*((f32*)__tmp17) = __tmp22;;__tmp23 = *(vec3 *)__tmp4;
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
*((f32*)__tmp5) = __tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 - __tmp15;;
*((f32*)__tmp11) = __tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 - __tmp21;;
*((f32*)__tmp17) = __tmp22;;__tmp23 = *(vec3 *)__tmp4;
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
*((f32*)__tmp5) = __tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 * __tmp15;;
*((f32*)__tmp11) = __tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 * __tmp21;;
*((f32*)__tmp17) = __tmp22;;__tmp23 = *(vec3 *)__tmp4;
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
*((f32*)__tmp5) = __tmp10;;__tmp11 = ((u64)&((vec3 *)__tmp4)->y);
__tmp12 = ((u64)&((vec3 *)__tmp1)->y);
__tmp13 = *(f32 *)__tmp12;
__tmp14 = ((u64)&((vec3 *)__tmp2)->y);
__tmp15 = *(f32 *)__tmp14;
__tmp16 = __tmp13 / __tmp15;;
*((f32*)__tmp11) = __tmp16;;__tmp17 = ((u64)&((vec3 *)__tmp4)->z);
__tmp18 = ((u64)&((vec3 *)__tmp1)->z);
__tmp19 = *(f32 *)__tmp18;
__tmp20 = ((u64)&((vec3 *)__tmp2)->z);
__tmp21 = *(f32 *)__tmp20;
__tmp22 = __tmp19 / __tmp21;;
*((f32*)__tmp17) = __tmp22;;__tmp23 = *(vec3 *)__tmp4;
return __tmp23;
}

vec3 transform_vec3_mat4 (vec3 __arg1, mat4 __arg2){
 u64 __tmp1;u64 __tmp2;vec3 __tmp3;vec3 __tmp4;u64 __tmp5;vec3 __tmp6; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);
__tmp3 = *(vec3 *)__tmp1;

__tmp5 = (u64)(&__tmp4);
*((vec3*)__tmp5) = __tmp3;;__tmp6 = *(vec3 *)__tmp5;
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
*((f32*)__tmp5) = __tmp9;;__tmp10 = ((u64)&((vec3 *)__tmp4)->y);
__tmp11 = ((u64)&((vec3 *)__tmp1)->y);
__tmp12 = *(f32 *)__tmp11;
__tmp13 = *(float *)__tmp2;
__tmp14 = __tmp12 + __tmp13;;
*((f32*)__tmp10) = __tmp14;;__tmp15 = ((u64)&((vec3 *)__tmp4)->z);
__tmp16 = ((u64)&((vec3 *)__tmp1)->z);
__tmp17 = *(f32 *)__tmp16;
__tmp18 = *(float *)__tmp2;
__tmp19 = __tmp17 + __tmp18;;
*((f32*)__tmp15) = __tmp19;;__tmp20 = *(vec3 *)__tmp4;
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
*((f32*)__tmp5) = __tmp9;;__tmp10 = ((u64)&((vec3 *)__tmp4)->y);
__tmp11 = ((u64)&((vec3 *)__tmp1)->y);
__tmp12 = *(f32 *)__tmp11;
__tmp13 = *(float *)__tmp2;
__tmp14 = __tmp12 + __tmp13;;
*((f32*)__tmp10) = __tmp14;;__tmp15 = ((u64)&((vec3 *)__tmp4)->z);
__tmp16 = ((u64)&((vec3 *)__tmp1)->z);
__tmp17 = *(f32 *)__tmp16;
__tmp18 = *(float *)__tmp2;
__tmp19 = __tmp17 + __tmp18;;
*((f32*)__tmp15) = __tmp19;;__tmp20 = *(vec3 *)__tmp4;
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
*((f32*)__tmp6) = __tmp7;;__tmp8 = ((u64)&((vec3 *)__tmp5)->y);
__tmp9 = *(f32 *)__tmp2;
*((f32*)__tmp8) = __tmp9;;__tmp10 = ((u64)&((vec3 *)__tmp5)->z);
__tmp11 = *(f32 *)__tmp3;
*((f32*)__tmp10) = __tmp11;;__tmp12 = *(vec3 *)__tmp5;
return __tmp12;
}

void print_any (Any __arg1){
 u64 __tmp1;u64 __tmp2;u64 __tmp3;int __tmp4;u64 __tmp5;u8* __tmp6;u8* __tmp7;u64 __tmp8;u64* __tmp9;u64 __tmp10;i32 __tmp11;u64 __tmp12;u64 __tmp13;int __tmp14;u64 __tmp15;u8* __tmp16;u8* __tmp17;u64 __tmp18;u64* __tmp19;u64 __tmp20;i32 __tmp21;u64 __tmp22;u64 __tmp23;int __tmp24;u64 __tmp25;u64 __tmp26;u64 __tmp27;float* __tmp28;u64 __tmp29;u8* __tmp30;u8* __tmp31;float* __tmp32;float __tmp33;i32 __tmp34;u64 __tmp35;u64 __tmp36;int __tmp37;u64 __tmp38;u64 __tmp39;u64 __tmp40;float* __tmp41;u64 __tmp42;u8* __tmp43;u8* __tmp44;float* __tmp45;float __tmp46;i32 __tmp47;u64 __tmp48;u64 __tmp49;int __tmp50;u64 __tmp51;u8* __tmp52;u8* __tmp53;u64 __tmp54;u64 __tmp55;i32 __tmp56;u64 __tmp57;u64 __tmp58;int __tmp59;u64 __tmp60;u64 __tmp61;u64 __tmp62;vec3* __tmp63;u64 __tmp64;u8* __tmp65;u8* __tmp66;u64 __tmp67;f32 __tmp68;u64 __tmp69;f32 __tmp70;u64 __tmp71;f32 __tmp72;i32 __tmp73; __tmp1 = (u64)(&__arg1);
__tmp2 = ((u64)&((Any *)__tmp1)->type);
__tmp3 = *(u64 *)__tmp2;
__tmp4 = 1;
__tmp5 = __tmp3 == __tmp4;;
if (__tmp5) {
__tmp6 = (u8*)__data3;
__tmp7 = (u8*)__data4;
__tmp8 = ((u64)&((Any *)__tmp1)->data);
__tmp9 = *(u64 **)__tmp8;
__tmp10 = *__tmp9;
__tmp11 = printf(__tmp7, __tmp10);
__tmp11;}
;__tmp12 = ((u64)&((Any *)__tmp1)->type);
__tmp13 = *(u64 *)__tmp12;
__tmp14 = 5;
__tmp15 = __tmp13 == __tmp14;;
if (__tmp15) {
__tmp16 = (u8*)__data5;
__tmp17 = (u8*)__data6;
__tmp18 = ((u64)&((Any *)__tmp1)->data);
__tmp19 = *(u64 **)__tmp18;
__tmp20 = *__tmp19;
__tmp21 = printf(__tmp17, __tmp20);
__tmp21;}
;__tmp22 = ((u64)&((Any *)__tmp1)->type);
__tmp23 = *(u64 *)__tmp22;
__tmp24 = 2;
__tmp25 = __tmp23 == __tmp24;;
if (__tmp25) {
__tmp26 = ((u64)&((Any *)__tmp1)->data);
__tmp27 = *(u64 *)__tmp26;

__tmp29 = (u64)(&__tmp28);
*((float**)__tmp29) = __tmp27;;__tmp30 = (u8*)__data7;
__tmp31 = (u8*)__data8;
__tmp32 = *(float **)__tmp29;
__tmp33 = *__tmp32;
__tmp34 = printf(__tmp31, __tmp33);
__tmp34;}
;__tmp35 = ((u64)&((Any *)__tmp1)->type);
__tmp36 = *(u64 *)__tmp35;
__tmp37 = 11;
__tmp38 = __tmp36 == __tmp37;;
if (__tmp38) {
__tmp39 = ((u64)&((Any *)__tmp1)->data);
__tmp40 = *(u64 *)__tmp39;

__tmp42 = (u64)(&__tmp41);
*((float**)__tmp42) = __tmp40;;__tmp43 = (u8*)__data9;
__tmp44 = (u8*)__data10;
__tmp45 = *(float **)__tmp42;
__tmp46 = *__tmp45;
__tmp47 = printf(__tmp44, __tmp46);
__tmp47;}
;__tmp48 = ((u64)&((Any *)__tmp1)->type);
__tmp49 = *(u64 *)__tmp48;
__tmp50 = 7;
__tmp51 = __tmp49 == __tmp50;;
if (__tmp51) {
__tmp52 = (u8*)__data11;
__tmp53 = (u8*)__data12;
__tmp54 = ((u64)&((Any *)__tmp1)->data);
__tmp55 = *(u64 *)__tmp54;
__tmp56 = printf(__tmp53, __tmp55);
__tmp56;}
;__tmp57 = ((u64)&((Any *)__tmp1)->type);
__tmp58 = *(u64 *)__tmp57;
__tmp59 = 18;
__tmp60 = __tmp58 == __tmp59;;
if (__tmp60) {
__tmp61 = ((u64)&((Any *)__tmp1)->data);
__tmp62 = *(u64 *)__tmp61;

__tmp64 = (u64)(&__tmp63);
*((vec3**)__tmp64) = __tmp62;;__tmp65 = (u8*)__data13;
__tmp66 = (u8*)__data14;
__tmp67 = ((u64)&(*(vec3 **)__tmp64)->x);
__tmp68 = *(f32 *)__tmp67;
__tmp69 = ((u64)&(*(vec3 **)__tmp64)->y);
__tmp70 = *(f32 *)__tmp69;
__tmp71 = ((u64)&(*(vec3 **)__tmp64)->z);
__tmp72 = *(f32 *)__tmp71;
__tmp73 = printf(__tmp66, __tmp68, __tmp70, __tmp72);
__tmp73;}
;}

void print (u8* __arg1, Array __arg2){
 u64 __tmp1;u64 __tmp2;int __tmp3;i32 __tmp4;u64 __tmp5;int __tmp6;i32 __tmp7;u64 __tmp8;u8* __tmp9;u8* __tmp10;u64 __tmp11;i32 __tmp12;u64 __tmp13;i32 __tmp14;i32 __tmp15;i32 __tmp16;u8* __tmp17;i32 __tmp18;u64 __tmp19;u64 __tmp20;u64 __tmp21;u64 __tmp22;u64 __tmp23;u8 __tmp24;u8 __tmp25;u64 __tmp26;u8 __tmp27;u8* __tmp28;int __tmp29;u64 __tmp30;u64 __tmp31;u64 __tmp32;u64 __tmp33;u64 __tmp34;u8 __tmp35;u8 __tmp36;i32 __tmp37;u64 __tmp38;u64 __tmp39;u64 __tmp40;u64 __tmp41;u64 __tmp42;u64 __tmp43;Any __tmp44;i32 __tmp45;u64 __tmp46;u64 __tmp47;u64 __tmp48;u64 __tmp49;u64 __tmp50;u64 __tmp51;Any __tmp52;i32 __tmp53;int __tmp54;i32 __tmp55;u8 __tmp56;u8* __tmp57;int __tmp58;u64 __tmp59;u64 __tmp60;u64 __tmp61;u64 __tmp62;u64 __tmp63;u8 __tmp64;u8 __tmp65;u8 __tmp66;i32 __tmp67;int __tmp68;i32 __tmp69; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);
__tmp3 = 0;

__tmp5 = (u64)(&__tmp4);
*((i32*)__tmp5) = __tmp3;;__tmp6 = 0;

__tmp8 = (u64)(&__tmp7);
*((i32*)__tmp8) = __tmp6;;__tmp9 = *(u8 **)__tmp1;
__tmp10 = *(u8 **)__tmp1;
__tmp11 = strlen(__tmp10);

__tmp13 = (u64)(&__tmp12);
*((i32*)__tmp13) = __tmp11;;_l1:
__tmp14 = *(i32 *)__tmp8;;;__tmp15 = *(i32 *)__tmp13;;;__tmp16 = __tmp14 != __tmp15;;;;if(__tmp16) {
__tmp17 = *(u8 **)__tmp1;
__tmp18 = *(i32 *)__tmp8;
__tmp19 = sizeof(u8);
__tmp20 = __tmp18 * __tmp19;;
__tmp21 = (u64)__tmp17;
__tmp22 = __tmp20 + __tmp21;;
__tmp23 = __tmp22;
__tmp24 = *(u8 *)__tmp23;

__tmp26 = (u64)(&__tmp25);
*((u8*)__tmp26) = __tmp24;;__tmp27 = *(u8 *)__tmp26;
__tmp28 = (u8*)__data15;
__tmp29 = 0;
__tmp30 = sizeof(u8);
__tmp31 = __tmp29 * __tmp30;;
__tmp32 = (u64)__tmp28;
__tmp33 = __tmp31 + __tmp32;;
__tmp34 = __tmp33;
__tmp35 = *(u8 *)__tmp34;
__tmp36 = __tmp27 == __tmp35;;
if (__tmp36) {
__tmp37 = *(i32 *)__tmp5;
__tmp38 = sizeof(Any);
__tmp39 = __tmp37 * __tmp38;;
__tmp40 = ((u64)&((Array *)__tmp2)->data);
__tmp41 = *(u64 *)__tmp40;
__tmp42 = __tmp39 + __tmp41;;
__tmp43 = __tmp42;
__tmp44 = *(Any *)__tmp43;
__tmp45 = *(i32 *)__tmp5;
__tmp46 = sizeof(Any);
__tmp47 = __tmp45 * __tmp46;;
__tmp48 = ((u64)&((Array *)__tmp2)->data);
__tmp49 = *(u64 *)__tmp48;
__tmp50 = __tmp47 + __tmp49;;
__tmp51 = __tmp50;
__tmp52 = *(Any *)__tmp51;
print_any(__tmp52);__tmp53 = *(i32 *)__tmp5;
__tmp54 = 1;
__tmp55 = __tmp53 + __tmp54;;
*((i32*)__tmp5) = __tmp55;;}
;__tmp56 = *(u8 *)__tmp26;
__tmp57 = (u8*)__data16;
__tmp58 = 0;
__tmp59 = sizeof(u8);
__tmp60 = __tmp58 * __tmp59;;
__tmp61 = (u64)__tmp57;
__tmp62 = __tmp60 + __tmp61;;
__tmp63 = __tmp62;
__tmp64 = *(u8 *)__tmp63;
__tmp65 = __tmp56 != __tmp64;;
if (__tmp65) {
__tmp66 = *(u8 *)__tmp26;
putchar(__tmp66);}
;__tmp67 = *(i32 *)__tmp8;
__tmp68 = 1;
__tmp69 = __tmp67 + __tmp68;;
*((i32*)__tmp8) = __tmp69;;goto _l1;}
;}

void main (){
 int __tmp1;i32 __tmp2;u64 __tmp3;int __tmp4;i32 __tmp5;u64 __tmp6;int __tmp7;i32 __tmp8;u64 __tmp9;i32 __tmp10;int __tmp11;i32* __tmp12;u64 __tmp13;GLFWwindow* __tmp14;u64 __tmp15;int __tmp16;int __tmp17;int __tmp18;int __tmp19;int __tmp20;int __tmp21;u8* __tmp22;u8* __tmp23;i32* __tmp24;i32* __tmp25;i32* __tmp26;i32* __tmp27;GLFWwindow* __tmp28;GLFWwindow* __tmp29;GLFWwindow* __tmp30;int __tmp31;i32 __tmp32;u64 __tmp33;i32 __tmp34;GLFWwindow* __tmp35;GLFWwindow* __tmp36;float __tmp37;int __tmp38;int __tmp39;int __tmp40;i32 __tmp41;i32 __tmp42;float __tmp43;float __tmp44;float __tmp45;float __tmp46;float __tmp47;float __tmp48;float __tmp49;float __tmp50;float __tmp51;int __tmp52;float __tmp53;int __tmp54;float __tmp55;float __tmp56;float __tmp57;int __tmp58;float __tmp59;int __tmp60;int __tmp61;float __tmp62;int __tmp63;i32 __tmp64;float __tmp65;float __tmp66;float __tmp67;float __tmp68;float __tmp69;float __tmp70;float __tmp71;float __tmp72;float __tmp73;float __tmp74;float __tmp75;int __tmp76;float __tmp77;int __tmp78;float __tmp79;float __tmp80;float __tmp81;int __tmp82;float __tmp83;int __tmp84;int __tmp85;float __tmp86;int __tmp87;GLFWwindow* __tmp88;GLFWwindow* __tmp89;i32 __tmp90;u8* __tmp91;u8* __tmp92;u8* __tmp93;Array __tmp94;u64 __tmp95;u64 __tmp96;u8* __tmp97;Any __tmp98;u64 __tmp99;u64 __tmp100;int __tmp101;u64 __tmp102;u64 __tmp103;u64 __tmp104;u64 __tmp105;u64 __tmp107;u64 __tmp108;u64 __tmp109;int __tmp110;u64 __tmp111;int __tmp112;GLFWwindow* __tmp113;GLFWwindow* __tmp114;i32 __tmp115;i32 __tmp116;u8* __tmp117;u8* __tmp118;u8* __tmp119;Array __tmp120;u64 __tmp121;u64 __tmp122;u8* __tmp123;Any __tmp124;u64 __tmp125;u64 __tmp126;int __tmp127;u64 __tmp128;u64 __tmp129;u64 __tmp130;u64 __tmp131;u64 __tmp133;u64 __tmp134;u64 __tmp135;int __tmp136;u64 __tmp137;int __tmp138; __tmp1 = 256;

__tmp3 = (u64)(&__tmp2);
*((i32*)__tmp3) = __tmp1;;__tmp4 = 16384;

__tmp6 = (u64)(&__tmp5);
*((i32*)__tmp6) = __tmp4;;__tmp7 = 4;

__tmp9 = (u64)(&__tmp8);
*((i32*)__tmp9) = __tmp7;;__tmp10 = glfwInit();
__tmp11 = 0;

__tmp13 = (u64)(&__tmp12);
*((i32**)__tmp13) = __tmp11;;
__tmp15 = (u64)(&__tmp14);
__tmp16 = 1280;
__tmp17 = 2;
__tmp18 = __tmp16 / __tmp17;;
__tmp19 = 720;
__tmp20 = 2;
__tmp21 = __tmp19 / __tmp20;;
__tmp22 = (u8*)__data17;
__tmp23 = (u8*)__data18;
__tmp24 = *(i32 **)__tmp13;
__tmp25 = *(i32 **)__tmp13;
__tmp26 = *(i32 **)__tmp13;
__tmp27 = *(i32 **)__tmp13;
__tmp28 = glfwCreateWindow(__tmp18, __tmp21, __tmp23, __tmp25, __tmp27);
*((GLFWwindow**)__tmp15) = __tmp28;;__tmp29 = *(GLFWwindow **)__tmp15;
__tmp30 = *(GLFWwindow **)__tmp15;
glfwMakeContextCurrent(__tmp30);gladInit();__tmp31 = 1;

__tmp33 = (u64)(&__tmp32);
*((i32*)__tmp33) = __tmp31;;_l1:
__tmp34 = *(i32 *)__tmp33;;;if(__tmp34) {
__tmp35 = *(GLFWwindow **)__tmp15;
__tmp36 = *(GLFWwindow **)__tmp15;
glfwSwapBuffers(__tmp36);glfwPollEvents();__tmp37 = 0.800000;
__tmp38 = 1;
__tmp39 = 1;
__tmp40 = 1;
glClearColor(__tmp37, __tmp38, __tmp39, __tmp40);__tmp41 = *(i32 *)__tmp6;
glClear(__tmp41);__tmp42 = *(i32 *)__tmp9;
glBegin(__tmp42);__tmp43 = 1.000000;
__tmp44 = 0.000000;
__tmp45 = 0.000000;
glColor3f(__tmp43, __tmp44, __tmp45);__tmp46 = 0.000000;
__tmp47 = 1.000000;
glVertex2f(__tmp46, __tmp47);__tmp48 = 0.000000;
__tmp49 = 1.000000;
__tmp50 = 0.000000;
glColor3f(__tmp48, __tmp49, __tmp50);__tmp51 = 0.870000;
__tmp52 = 0;
__tmp53 = 0.500000;
__tmp54 = __tmp52 - __tmp53;;
glVertex2f(__tmp51, __tmp54);__tmp55 = 0.000000;
__tmp56 = 0.000000;
__tmp57 = 1.000000;
glColor3f(__tmp55, __tmp56, __tmp57);__tmp58 = 0;
__tmp59 = 0.870000;
__tmp60 = __tmp58 - __tmp59;;
__tmp61 = 0;
__tmp62 = 0.500000;
__tmp63 = __tmp61 - __tmp62;;
glVertex2f(__tmp60, __tmp63);glEnd();__tmp64 = *(i32 *)__tmp9;
glBegin(__tmp64);__tmp65 = 1.000000;
__tmp66 = 0.000000;
__tmp67 = 0.000000;
glColor3f(__tmp65, __tmp66, __tmp67);__tmp68 = 1.000000;
__tmp69 = 0.000000;
__tmp70 = __tmp68 - __tmp69;;
__tmp71 = 1.000000;
glVertex2f(__tmp70, __tmp71);__tmp72 = 0.000000;
__tmp73 = 1.000000;
__tmp74 = 0.000000;
glColor3f(__tmp72, __tmp73, __tmp74);__tmp75 = 0.870000;
__tmp76 = 0;
__tmp77 = 0.500000;
__tmp78 = __tmp76 - __tmp77;;
glVertex2f(__tmp75, __tmp78);__tmp79 = 0.000000;
__tmp80 = 0.000000;
__tmp81 = 1.000000;
glColor3f(__tmp79, __tmp80, __tmp81);__tmp82 = 0;
__tmp83 = 0.870000;
__tmp84 = __tmp82 - __tmp83;;
__tmp85 = 0;
__tmp86 = 0.500000;
__tmp87 = __tmp85 - __tmp86;;
glVertex2f(__tmp84, __tmp87);glEnd();__tmp88 = *(GLFWwindow **)__tmp15;
__tmp89 = *(GLFWwindow **)__tmp15;
__tmp90 = glfwWindowShouldClose(__tmp89);
if (__tmp90) {
__tmp91 = (u8*)__data19;
__tmp92 = (u8*)__data20;
__tmp93 = (u8*)__data21;

__tmp95 = (u64)(&__tmp94);
__tmp96 = ((u64)&((Array *)__tmp95)->data);
__tmp97 = (u8*)__data22;

__tmp99 = (u64)(&__tmp98);
__tmp100 = ((u64)&((Any *)__tmp99)->type);
__tmp101 = 7;
__tmp102 = *((u64*)__tmp100) = __tmp101;;
__tmp103 = (u64)(&__tmp98);
__tmp104 = ((u64)&((Any *)__tmp103)->data);
__tmp105 = *((u64*)__tmp104) = __tmp97;;
Any __tmp106[1] = {__tmp98};;
__tmp107 = (u64)(&__tmp106);
__tmp108 = *((u64*)__tmp96) = __tmp107;;
__tmp109 = ((u64)&((Array *)__tmp95)->count);
__tmp110 = 1;
__tmp111 = *((u64*)__tmp109) = __tmp110;;
print(__tmp92, __tmp94);__tmp112 = 0;
*((i32*)__tmp33) = __tmp112;;}
;__tmp113 = *(GLFWwindow **)__tmp15;
__tmp114 = *(GLFWwindow **)__tmp15;
__tmp115 = *(i32 *)__tmp3;
__tmp116 = glfwGetKey(__tmp114, __tmp115);
if (__tmp116) {
__tmp117 = (u8*)__data23;
__tmp118 = (u8*)__data24;
__tmp119 = (u8*)__data25;

__tmp121 = (u64)(&__tmp120);
__tmp122 = ((u64)&((Array *)__tmp121)->data);
__tmp123 = (u8*)__data26;

__tmp125 = (u64)(&__tmp124);
__tmp126 = ((u64)&((Any *)__tmp125)->type);
__tmp127 = 7;
__tmp128 = *((u64*)__tmp126) = __tmp127;;
__tmp129 = (u64)(&__tmp124);
__tmp130 = ((u64)&((Any *)__tmp129)->data);
__tmp131 = *((u64*)__tmp130) = __tmp123;;
Any __tmp132[1] = {__tmp124};;
__tmp133 = (u64)(&__tmp132);
__tmp134 = *((u64*)__tmp122) = __tmp133;;
__tmp135 = ((u64)&((Array *)__tmp121)->count);
__tmp136 = 1;
__tmp137 = *((u64*)__tmp135) = __tmp136;;
print(__tmp118, __tmp120);__tmp138 = 0;
*((i32*)__tmp33) = __tmp138;;}
;goto _l1;}
;}

