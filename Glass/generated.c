
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
			

typedef struct Array
{	
	u64 count;
	u64 data;
} Array;
			
static const type_info __type_info_table[17] = {
{.name="void",.id=0},{.name="float",.id=2},{.name="int",.id=1},{.name="i8",.id=3},{.name="i16",.id=4},{.name="i32",.id=5},{.name="i64",.id=6},{.name="u8",.id=7},{.name="u16",.id=8},{.name="u32",.id=9},{.name="u64",.id=10},{.name="f32",.id=11},{.name="f64",.id=12},{.name="bool",.id=13},{.name="Any",.id=14},{.name="Array",.id=15},{.name="type_info",.id=16},};

void print (u8* __arg1, Array __arg2);
void main ();
const char* __data1 = "%i type: %zu\n";
const char* __data2 = "%i type: %zu\n";
const char* __data3 = "hello\n";
const char* __data4 = "hello\n";
const char* __data5 = "Format";
const char* __data6 = "Format";
const char* __data7 = "world\n";
const char* __data8 = "world\n";
void print (u8* __arg1, Array __arg2){
 u64 __tmp1;u64 __tmp2;int __tmp3;i32 __tmp4;u64 __tmp5;int __tmp6;i32 __tmp7;u64 __tmp8;i32 __tmp9;i32 __tmp10;u64 __tmp11;u64 __tmp12;u64 __tmp13;u64 __tmp14;u64 __tmp15;u64 __tmp16;Any __tmp17;Any __tmp18;u64 __tmp19;u8* __tmp20;u8* __tmp21;i32 __tmp22;u64 __tmp23;u64 __tmp24;i32 __tmp25;i32 __tmp26;int __tmp27;i32 __tmp28;i32 __tmp29;u64 __tmp30;u64 __tmp31;int __tmp32;u64 __tmp33;i32 __tmp34;int __tmp35; __tmp1 = (u64)(&__arg1);
__tmp2 = (u64)(&__arg2);
__tmp3 = 1;

__tmp5 = (u64)(&__tmp4);
*((i32*)__tmp5) = __tmp3;;__tmp6 = 0;

__tmp8 = (u64)(&__tmp7);
*((i32*)__tmp8) = __tmp6;;goto _l1;_l1:
__tmp9 = *(i32 *)__tmp5;;;if(__tmp9) {
__tmp10 = *(i32 *)__tmp8;
__tmp11 = sizeof(Any);
__tmp12 = __tmp10 * __tmp11;;
__tmp13 = ((u64)&((Array *)__tmp2)->data);
__tmp14 = *(u64 *)__tmp13;
__tmp15 = __tmp12 + __tmp14;;
__tmp16 = __tmp15;
__tmp17 = *(Any *)__tmp16;

__tmp19 = (u64)(&__tmp18);
__tmp20 = (u8*)__data1;
__tmp21 = (u8*)__data2;
__tmp22 = *(i32 *)__tmp8;
__tmp23 = ((u64)&((Any *)__tmp19)->type);
__tmp24 = *(u64 *)__tmp23;
__tmp25 = printf(__tmp21, __tmp22, __tmp24);
__tmp26 = *(i32 *)__tmp8;
__tmp27 = 1;
__tmp28 = __tmp26 + __tmp27;;
__tmp29 = *(i32 *)__tmp8;
__tmp30 = ((u64)&((Array *)__tmp2)->count);
__tmp31 = *(u64 *)__tmp30;
__tmp32 = 1;
__tmp33 = __tmp31 - __tmp32;;
__tmp34 = __tmp29 == __tmp33;;
*((Any*)__tmp19) = __tmp17;;__tmp25;*((i32*)__tmp8) = __tmp28;;if (__tmp34) {
__tmp35 = 0;
*((i32*)__tmp5) = __tmp35;;}
;goto _l1;}
}

void main (){
 u8* __tmp1;u8* __tmp2;i32 __tmp3;u8* __tmp4;u8* __tmp5;int __tmp6;Array __tmp7;u64 __tmp8;u64 __tmp9;int __tmp10;Any __tmp11;u64 __tmp12;u64 __tmp13;int __tmp14;u64 __tmp15;u64 __tmp16;u64 __tmp17;u64 __tmp18;u64 __tmp19;int __tmp20;Any __tmp21;u64 __tmp22;u64 __tmp23;int __tmp24;u64 __tmp25;u64 __tmp26;u64 __tmp27;u64 __tmp28;u64 __tmp29;int __tmp30;Any __tmp31;u64 __tmp32;u64 __tmp33;int __tmp34;u64 __tmp35;u64 __tmp36;u64 __tmp37;u64 __tmp38;u64 __tmp39;float __tmp40;Any __tmp41;u64 __tmp42;u64 __tmp43;int __tmp44;u64 __tmp45;u64 __tmp46;u64 __tmp47;u64 __tmp48;u64 __tmp49;float __tmp50;Any __tmp51;u64 __tmp52;u64 __tmp53;int __tmp54;u64 __tmp55;u64 __tmp56;u64 __tmp57;u64 __tmp58;u64 __tmp59;int __tmp60;Any __tmp61;u64 __tmp62;u64 __tmp63;int __tmp64;u64 __tmp65;u64 __tmp66;u64 __tmp67;u64 __tmp68;u64 __tmp69;u64 __tmp71;u64 __tmp72;u64 __tmp73;int __tmp74;u64 __tmp75;u8* __tmp76;u8* __tmp77;i32 __tmp78; __tmp1 = (u8*)__data3;
__tmp2 = (u8*)__data4;
__tmp3 = printf(__tmp2);
__tmp4 = (u8*)__data5;
__tmp5 = (u8*)__data6;
__tmp6 = 10;

__tmp8 = (u64)(&__tmp7);
__tmp9 = ((u64)&((Array *)__tmp8)->data);
__tmp10 = 10;

__tmp12 = (u64)(&__tmp11);
__tmp13 = ((u64)&((Any *)__tmp12)->type);
__tmp14 = 1;
__tmp15 = *((u64*)__tmp13) = __tmp14;;
__tmp16 = (u64)(&__tmp11);
__tmp17 = ((u64)&((Any *)__tmp16)->data);
__tmp18 = (u64)(&__tmp10);
__tmp19 = *((u64*)__tmp17) = __tmp18;;
__tmp20 = 10;

__tmp22 = (u64)(&__tmp21);
__tmp23 = ((u64)&((Any *)__tmp22)->type);
__tmp24 = 1;
__tmp25 = *((u64*)__tmp23) = __tmp24;;
__tmp26 = (u64)(&__tmp21);
__tmp27 = ((u64)&((Any *)__tmp26)->data);
__tmp28 = (u64)(&__tmp20);
__tmp29 = *((u64*)__tmp27) = __tmp28;;
__tmp30 = 20;

__tmp32 = (u64)(&__tmp31);
__tmp33 = ((u64)&((Any *)__tmp32)->type);
__tmp34 = 1;
__tmp35 = *((u64*)__tmp33) = __tmp34;;
__tmp36 = (u64)(&__tmp31);
__tmp37 = ((u64)&((Any *)__tmp36)->data);
__tmp38 = (u64)(&__tmp30);
__tmp39 = *((u64*)__tmp37) = __tmp38;;
__tmp40 = 20.000000;

__tmp42 = (u64)(&__tmp41);
__tmp43 = ((u64)&((Any *)__tmp42)->type);
__tmp44 = 2;
__tmp45 = *((u64*)__tmp43) = __tmp44;;
__tmp46 = (u64)(&__tmp41);
__tmp47 = ((u64)&((Any *)__tmp46)->data);
__tmp48 = (u64)(&__tmp40);
__tmp49 = *((u64*)__tmp47) = __tmp48;;
__tmp50 = 50.000000;

__tmp52 = (u64)(&__tmp51);
__tmp53 = ((u64)&((Any *)__tmp52)->type);
__tmp54 = 2;
__tmp55 = *((u64*)__tmp53) = __tmp54;;
__tmp56 = (u64)(&__tmp51);
__tmp57 = ((u64)&((Any *)__tmp56)->data);
__tmp58 = (u64)(&__tmp50);
__tmp59 = *((u64*)__tmp57) = __tmp58;;
__tmp60 = 60;

__tmp62 = (u64)(&__tmp61);
__tmp63 = ((u64)&((Any *)__tmp62)->type);
__tmp64 = 1;
__tmp65 = *((u64*)__tmp63) = __tmp64;;
__tmp66 = (u64)(&__tmp61);
__tmp67 = ((u64)&((Any *)__tmp66)->data);
__tmp68 = (u64)(&__tmp60);
__tmp69 = *((u64*)__tmp67) = __tmp68;;
Any __tmp70[6] = {__tmp11,__tmp21,__tmp31,__tmp41,__tmp51,__tmp61};;
__tmp71 = (u64)(&__tmp70);
__tmp72 = *((u64*)__tmp9) = __tmp71;;
__tmp73 = ((u64)&((Array *)__tmp8)->count);
__tmp74 = 6;
__tmp75 = *((u64*)__tmp73) = __tmp74;;
print(__tmp5, __tmp7);__tmp76 = (u8*)__data7;
__tmp77 = (u8*)__data8;
__tmp78 = printf(__tmp77);
}

