clear
pushd %~dp0
CALL ..\..\Glass\bin\Debug\Glass.exe ./Game.glass -cI glfw/include/glfw3.h -cL glfw/GLFW.lib
CALL MSBuild.exe .\GameSln.sln
popd
Game.exe