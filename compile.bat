clear
Rem clang-format.exe -style="{BasedOnStyle: Microsoft, PointerAlignment: Left, IndentWidth: 4, UseTab: Always}" -i .\Examples\Main.glass
.\Glass\bin\Release\Glass.exe .\Examples\Main.glass -cL Examples/Game/glad/GLAD.lib Examples/Game/glfw/GLFW.lib kernel32.lib shell32.lib opengl32.lib user32.lib gdi32.lib -cI  Examples/Game/glInit.h
generated.exe