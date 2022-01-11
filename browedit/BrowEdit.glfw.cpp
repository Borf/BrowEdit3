#include "BrowEdit.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char* description)
{
    std::cerr<<"Glfw Error" << error << " -> " << description << std::endl;
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
}


bool BrowEdit::glfwBegin()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return false;
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(1920, 1080, "NOT BrowEdit", NULL, NULL);
    
    if (window == nullptr)
        return false;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

   
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

#ifdef _DEBUG
    glDebugMessageCallback(&onDebug, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEBUG_OUTPUT);
#endif

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