#include "BrowEdit.h"

#include <iostream>
#include <GLFW/glfw3.h>
#include <Version.h>
#include <browedit/Image.h>
#include <browedit/Map.h>

extern BrowEdit* browEdit; //meh

static void glfw_error_callback(int error, const char* description)
{
    std::cerr<<"Glfw Error" << error << " -> " << description << std::endl;
}

static void glfw_close_callback(GLFWwindow* window)
{
    for (auto m : browEdit->maps)
        browEdit->windowData.showQuitConfirm |= m->changed;

    if (browEdit->windowData.showQuitConfirm)
    {
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        browEdit->windowData.showQuitConfirm = true;
    }
}

#ifdef _WIN32
void APIENTRY onDebug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
#else
void onDebug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
#endif
{
    if (id == 131185 || // VIDEO memory
        id == 131218 || // recompiled based on state
        false)
        return;
    std::cerr<< "OpenGL Debug\t"<<id<<"\t"<<message << std::endl;
    std::cerr << util::callstack() << std::endl;
}

#define Q(x) #x
#define QUOTE(x) Q(x)

bool BrowEdit::glfwBegin()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return false;
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(1920, 1080, "BrowEdit V3." QUOTE(VERSION), NULL, NULL);

    if (window == nullptr)
        return false;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    std::cout << "Using renderer " << glGetString(GL_RENDERER) << std::endl;

    //#ifdef _DEBUG
    if (glDebugMessageCallback)
    {
        glDebugMessageCallback(&onDebug, NULL);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glEnable(GL_DEBUG_OUTPUT);
    }
    //#endif
#ifdef _DEBUG
    glfwSwapInterval(0); //disable swap
#endif
    {
        GLFWimage image;

        std::istream* is = util::FileIO::open("data\\dropper.png");
        is->seekg(0, std::ios_base::end);
        std::size_t len = is->tellg();
        char* fileData = new char[len];
        is->seekg(0, std::ios_base::beg);
        is->read(fileData, len);
        delete is;
        int depth;
        image.pixels = stbi_load_from_memory((stbi_uc*)fileData, (int)len, &image.width, &image.height, &depth, 4);
        dropperCursor = glfwCreateCursor(&image, 1, 29);
    }

    glfwSetWindowCloseCallback(window, glfw_close_callback);

    return true;
}

void BrowEdit::glfwEnd()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool BrowEdit::glfwLoopBegin()
{
    if (glfwWindowShouldClose(window))
        return false;
    glfwPollEvents();
    return true;
}

void BrowEdit::glfwLoopEnd()
{
    scrollDelta = 0;
#ifdef _DEBUG
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
#endif

    glfwSwapBuffers(window);
}