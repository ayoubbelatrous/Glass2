clear
pushd %~dp0
CALL ..\..\Glass\bin\Debug\Glass.exe ./Game.glass -cI glad/glad.h glfw/include/glfw3.h loadgl.h -cL glfw/GLFW.lib glad/GLAD.lib
CALL MSBuild.exe .\GameSln.sln
popd
Game.exe