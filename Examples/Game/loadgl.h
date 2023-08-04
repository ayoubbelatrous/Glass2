#include "glad/glad.h"
#include "glfw/include/glfw3.h"

int gladInit()
{
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}