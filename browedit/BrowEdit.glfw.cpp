#include "BrowEdit.h"

#include <iostream>
#include <glad/gl.h>
#ifdef _WIN32
    #ifdef _WIN32
    #include <Windows.h>
    #endif
    #include <psapi.h>

    #define GLFW_EXPOSE_NATIVE_WIN32
    #define GLFW_EXPOSE_NATIVE_WGL
    #define GLFW_NATIVE_INCLUDE_NONE
    #include <glad/wgl.h>
#else
    #include <sys/resource.h>
    #include <sys/types.h>
#ifdef __APPLE__
    #include <sys/sysctl.h>
#else
    #include <sys/sysinfo.h>

    // ToDo: Wayland?
    #define GLFW_EXPOSE_NATIVE_X11
    #include <glad/glx.h>
#endif
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
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
void onDebug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
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
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    //#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    //#endif
    if (config.startMaximizedWindow)
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    window = glfwCreateWindow(1920, 1080, "BrowEdit V3." QUOTE(VERSION), NULL, NULL);

    if (window == nullptr)
    {
        std::cerr << "Failed to initialize GLFW window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    #ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(window);
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101)); 
        if (hIcon)
        {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
    #endif

    if (
        !gladLoadGL((GLADloadfunc)glfwGetProcAddress)
        #ifdef _WIN32
            || !gladLoadWGL(GetDC(glfwGetWin32Window(window)), (GLADloadfunc)glfwGetProcAddress)
        #elif !defined(__APPLE__)
            || !gladLoadGLX(glfwGetX11Display(), XDefaultScreen(glfwGetX11Display()), (GLADloadfunc)glfwGetProcAddress)
        #endif
    )
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    std::cout << "Using renderer " << glGetString(GL_RENDERER) << std::endl;

    memset(&memoryLimits, 0, sizeof(memoryLimits));
    calcMemoryLimits();

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

        std::istream* is = util::FileIO::open("data/dropper.png");
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
    calcMemoryLimits();
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

void BrowEdit::calcMemoryLimits()
{
    #ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);

        PROCESS_MEMORY_COUNTERS_EX pmc;
	    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));

        //memoryLimits.systemSize = memInfo.ullAvailPageFile;
        memoryLimits.systemSize = pmc.PagefileUsage; // Note: this basically is the "ensured" value, which changes much less often than the global value, which could be distracting...
        memoryLimits.systemUsed = pmc.WorkingSetSize;

        // ToDo: get shared gpu memory size
        memoryLimits.gpuSharedSize = memInfo.ullTotalPhys / 2;
    #elif defined(__APPLE__)
        int mib[2];
        int64_t physical_memory;
        size_t length;
        mib[0] = CTL_HW;
        mib[1] = HW_MEMSIZE;
        length = sizeof(int64_t);
        sysctl(mib, 2, &physical_memory, &length, NULL, 0);
        memoryLimits.systemSize = physical_memory;
        
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        memoryLimits.systemUsed = usage.ru_maxrss; // bytes on macOS
        memoryLimits.gpuSharedSize = memoryLimits.systemSize / 2;
    #else
        struct sysinfo memInfo;
        sysinfo(&memInfo);

        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);

        memoryLimits.systemSize = (memInfo.freeram + memInfo.freeswap) * memInfo.mem_unit;
        memoryLimits.systemUsed = usage.ru_maxrss * 1024; // kilobytes on Linux

        // ToDo: get shared gpu memory size
        memoryLimits.gpuSharedSize = memoryLimits.systemSize / 2;
    #endif
    memoryLimits.systemFree = memoryLimits.systemSize - memoryLimits.systemUsed;

    GLint64 kb[4];
    memset(&kb, GL_INVALID_VALUE, sizeof(kb));

    if (GLAD_GL_NVX_gpu_memory_info)
    {
        glGetInteger64v(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &kb[0]);
        glGetInteger64v(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &kb[1]);
        glGetInteger64v(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &kb[2]);

        if (kb[0] != GL_INVALID_VALUE) memoryLimits.gpuSize = kb[0] * 1024;
        if (kb[1] != GL_INVALID_VALUE) memoryLimits.gpuFree = kb[1] * 1024;

        if (kb[2] != GL_INVALID_VALUE) memoryLimits.gpuSharedFree = kb[2] * 1024;
    }
    else if (GLAD_GL_ATI_meminfo)
    {
        memoryLimits.gpuSize = 0;

        #ifdef _WIN32
            if (GLAD_WGL_AMD_gpu_association)
            {
                std::vector<UINT> gpus(wglGetGPUIDsAMD(0, 0));
                wglGetGPUIDsAMD(static_cast<UINT>(gpus.size()), gpus.data());
                wglGetGPUInfoAMD(gpus[0], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(memoryLimits.gpuSize), &memoryLimits.gpuSize);
            }
        #elif !defined(__APPLE__)
            if (GLAD_GLX_AMD_gpu_association)
            {
                std::vector<unsigned int> gpus(glxGetGPUIDsAMD(0, 0));
                glxGetGPUIDsAMD(static_cast<UINT>(gpus.size()), gpus.data());
                glxGetGPUInfoAMD(gpus[0], GLX_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(memoryLimits.gpuSize), &memoryLimits.gpuSize);
            }
        #endif
        memoryLimits.gpuSize *= 1024 * 1024;

        glGetInteger64v(GL_TEXTURE_FREE_MEMORY_ATI, kb);

        if (kb[0] != GL_INVALID_VALUE) memoryLimits.gpuFree = kb[0] * 1024;
        if (kb[2] != GL_INVALID_VALUE) memoryLimits.gpuSharedFree = kb[2] * 1024;

        // kb[1] = largest available free block in the pool
        // kb[3] = largest auxiliary free block

        //glGetInteger64v(GL_VBO_FREE_MEMORY_ATI, &kb);
        //glGetInteger64v(GL_RENDERBUFFER_FREE_MEMORY_ATI, &kb);
    }
/*
    else if (glExts.count("GL_INTEL_memory?"))
    {
    }
*/

    if(memoryLimits.gpuSize) memoryLimits.gpuUsed = memoryLimits.gpuSize - memoryLimits.gpuFree;
    if(memoryLimits.gpuSharedSize)
    {
        if(memoryLimits.gpuSharedFree == memoryLimits.gpuSharedSize) memoryLimits.gpuSharedFree = memoryLimits.gpuSize - memoryLimits.gpuUsed;
        memoryLimits.gpuSharedUsed = memoryLimits.gpuSharedSize - memoryLimits.gpuSharedFree;
    }
}
