.\Glass\bin\Debug\Glass.exe .\Examples\Basic\Main.glass -no-link
fasm .\output.s
clang.exe -g output.obj .\Libraries\msdfatlasgen/MsdfAtlasGen.C.lib .\Libraries\msdfatlasgen/MsdfAtlasGen.C.dll .\ext\glfw3.dll -lgdi32 -luser32 -lkernel32 -lopengl32 hypatia.lib