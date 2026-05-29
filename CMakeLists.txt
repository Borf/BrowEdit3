cmake_minimum_required(VERSION 3.15)
project(BrowEdit3 VERSION 3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Definitions
add_compile_definitions(GRF_STATIC NOMINMAX)

if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN VC_EXTRALEAN _CONSOLE)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(_DEBUG)
    else()
        add_compile_definitions(NDEBUG)
    endif()
endif()

file(GLOB_RECURSE BROWEDIT_SOURCES "browedit/*.cpp" "browedit/*.h" "browedit/*.c")

set(LIB_SOURCES
    "lib/glad/src/gl.c"
    "lib/grflib/grf.c"
    "lib/grflib/grfcrypt.c"
    "lib/grflib/grfsupport.c"
    "lib/grflib/rgz.c"
    "lib/imGuIZMO.quat/imGuIZMO.quat/imGuIZMOquat.cpp"
    "lib/imgui/backends/imgui_impl_glfw.cpp"
    "lib/imgui/backends/imgui_impl_opengl3.cpp"
    "lib/imgui/imgui.cpp"
    "lib/imgui/imgui_demo.cpp"
    "lib/imgui/imgui_draw.cpp"
    "lib/imgui/imgui_tables.cpp"
    "lib/imgui/imgui_widgets.cpp"
    "lib/imgui/misc/cpp/imgui_stdlib.cpp"
    "lib/lzma/LzmaDec.c"
    "lib/sfl/stb/stb_image.cpp"
    "lib/sfl/stb/stb_image_write.cpp"
    "lib/tinygltf/tiny_gltf.cc"
    "lib/zlib/adler32.c"
    "lib/zlib/compress.c"
    "lib/zlib/crc32.c"
    "lib/zlib/deflate.c"
    "lib/zlib/gzclose.c"
    "lib/zlib/gzlib.c"
    "lib/zlib/gzread.c"
    "lib/zlib/gzwrite.c"
    "lib/zlib/infback.c"
    "lib/zlib/inffast.c"
    "lib/zlib/inflate.c"
    "lib/zlib/inftrees.c"
    "lib/zlib/trees.c"
    "lib/zlib/uncompr.c"
    "lib/zlib/zutil.c"
)

add_executable(BrowEdit3 ${BROWEDIT_SOURCES} ${LIB_SOURCES})

target_include_directories(BrowEdit3 PRIVATE
    "lib/imgui"
    "lib/grflib"
    "lib/zlib"
    "lib/sfl"
    "lib/glm"
    "lib/glad/include"
    "lib/lzma"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "lib/magic_enum/include"
    "lib/imGuIZMO.quat/imGuIZMO.quat"
)

if(WIN32)
    target_include_directories(BrowEdit3 PRIVATE
        "lib/glfw-3.3.8/include"
        "lib/bugtrap"
    )
    target_link_directories(BrowEdit3 PRIVATE
        "lib/glfw-3.3.8/lib"
        "lib/bugtrap"
    )
    target_link_libraries(BrowEdit3 PRIVATE winmm opengl32 glfw3)
elseif(APPLE)
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_library(COCOA Cocoa)
    find_library(IOKIT IOKit)
    find_library(COREVIDEO CoreVideo)
    
    target_link_libraries(BrowEdit3 PRIVATE
        glfw
        OpenGL::GL
        ${COCOA}
        ${IOKIT}
        ${COREVIDEO}
    )
elseif(UNIX)
    # Generic unix support (Linux)
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(BrowEdit3 PRIVATE glfw OpenGL::GL m)
endif()
