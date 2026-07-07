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
	GLEW::GLEW
	OpenGL::GL
	#libEGL
	opengl32
	gdi32
)
