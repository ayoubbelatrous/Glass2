#include <windows.h>
#include <GL/gl.h>

typedef float f32;
typedef unsigned short u16;
typedef unsigned int u32;

typedef struct vec2
{
    f32 x;
    f32 y;
} vec2;

typedef struct vec3
{
    f32 x;
    f32 y;
    f32 z;
} vec3;

typedef struct vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} vec4;

typedef struct Color
{
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} Color;

enum BufferElementTypeEnum
{
    BYTE,
    INT,
    UINT,
    FLOAT,
    DOUBLE,
    VEC2,
    VEC3,
    VEC4,
};

typedef u16 BufferElementType;

typedef struct VertexBufferLayout
{
    BufferElementType Elements[64];
    u32 ElementCount;
} VertexBufferLayout;

typedef struct VertexBuffer
{
    u32 RendererID;

    u32 Count;

    VertexBufferLayout Layout;
} VertexBuffer;

enum Buffer

    typedef void (*impl_ClearBuffer_t)();
impl_ClearBuffer_t impl_ClearBuffer = 0;

typedef void (*impl_ClearBufferColor_t)(f32 r, f32 g, f32 b, f32 a);
impl_ClearBufferColor_t impl_ClearBufferColor = 0;

typedef void (*impl_Init_t)(void *);
impl_Init_t impl_Init = 0;

void Shatter_ClearBufferColor(Color color)
{
    impl_ClearBufferColor(color.r, color.g, color.b, color.a);
}

void Shatter_Init()
{
    Shatter_GL_Windows_Init();

    impl_Init = &Shatter_GL_Load_Funcs;
    impl_Init(0);
}

typedef void (*impl_GLLoaderFunc_t)(const char *);

impl_GLLoaderFunc_t GL_Loader_Func;

void Shatter_GL_Windows_Init(void *platform_handle)
{
    GL_Loader_Func = &wglGetProcAddress;
}

void Shatter_GL_Load_Funcs()
{
    impl_ClearBufferColor = GL_Loader_Func("glClearColor");
    impl_ClearBuffer = GL_Loader_Func("glClear");
}