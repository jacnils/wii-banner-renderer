cmake_minimum_required(VERSION 3.11)
project(wii-banner-renderer)

set(CMAKE_CXX_FLAGS "-w -std=c++17")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/old)

add_executable(wii-banner-renderer 
	# junk from dolphin emulator, TODO: remove this
	src/old/Blob.cpp
	src/old/CDUtils.cpp
	src/old/FileBlob.cpp
	src/old/FileHandlerARC.cpp
	src/old/TextureDecoder.cpp

	src/Animator.cpp
	src/Banner.cpp
	src/Endian.cpp
	src/Font.cpp
	src/Layout.cpp
	src/Main.cpp
	src/Material.cpp
	src/Pane.cpp
	src/Picture.cpp
	src/Sound.cpp
	src/Textbox.cpp
	src/Texture.cpp
	src/Window.cpp
	src/WrapGx.cpp
	src/Renderer.cpp
)

find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(wii-banner-renderer
    PRIVATE
        GLEW::GLEW
        OpenGL::GL
)

if (WIN32)
    target_link_libraries(wii-banner-renderer PRIVATE opengl32 gdi32)
endif()


if (UNIX)
	find_library(EGL_LIBRARY NAMES EGL)
        find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h)

        if (EGL_LIBRARY AND EGL_INCLUDE_DIR)
            add_library(EGL::EGL UNKNOWN IMPORTED)
            set_target_properties(EGL::EGL PROPERTIES
                IMPORTED_LOCATION ${EGL_LIBRARY}
                INTERFACE_INCLUDE_DIRECTORIES ${EGL_INCLUDE_DIR}
            )
        endif()

	target_link_libraries(wii-banner-renderer PRIVATE EGL::EGL)
endif()
