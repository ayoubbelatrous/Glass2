typedef void *(*GLADloadproc)(const char *name);
int gladLoadGLLoader(GLADloadproc);
typedef void (*GLFWglproc)(void);
GLFWglproc glfwGetProcAddress(const char *procname);

void gladInit()
{
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}
