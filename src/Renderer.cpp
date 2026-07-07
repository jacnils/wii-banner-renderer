#include "Renderer.h"
#include "WrapGx.h"
#include <GL/glew.h>
#include <GL/gl.h>

#include <algorithm>
#include <stdexcept>
#include <cstring>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Renderer::Renderer(int width, int height)
    :
    m_width(width),
    m_height(height),
    m_pixels(width * height * 4)
{
    this->CreateContext();

    GX_Init(0, 0);
    glViewport(0, 0, width, height);

    glOrtho(0.0, 1.0, 1.0, 0.0, -1000.0, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
}


Renderer::~Renderer()
{
    DestroyContext();
}

void Renderer::CreateContext()
{

WNDCLASS wc = {};
wc.style = CS_OWNDC;
wc.lpfnWndProc = DefWindowProc;
wc.hInstance = GetModuleHandle(nullptr);
wc.lpszClassName = "DummyGLWindow";


RegisterClass(&wc);

// 2. Create a hidden window
HWND hwnd = CreateWindow(
    wc.lpszClassName,
    "Hidden GL Window",
    WS_OVERLAPPEDWINDOW,
    0, 0, m_width, m_height,
    nullptr,
    nullptr,
    wc.hInstance,
    nullptr
);

if (!hwnd)
    throw std::runtime_error("CreateWindow failed");

HDC hdc = GetDC(hwnd);

// 3. Set pixel format
PIXELFORMATDESCRIPTOR pfd = {};
pfd.nSize = sizeof(pfd);
pfd.nVersion = 1;
pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
pfd.iPixelType = PFD_TYPE_RGBA;
pfd.cColorBits = 32;
pfd.cDepthBits = 24;
pfd.cStencilBits = 8;
pfd.iLayerType = PFD_MAIN_PLANE;

int pixelFormat = ChoosePixelFormat(hdc, &pfd);
if (pixelFormat == 0)
    throw std::runtime_error("ChoosePixelFormat failed");

if (!SetPixelFormat(hdc, pixelFormat, &pfd))
    throw std::runtime_error("SetPixelFormat failed");

HGLRC context = wglCreateContext(hdc);
if (!context)
    throw std::runtime_error("wglCreateContext failed");

if (!wglMakeCurrent(hdc, context))
    throw std::runtime_error("wglMakeCurrent failed");

GLenum err = glewInit();
if (err != GLEW_OK)
{
    throw std::runtime_error(
        reinterpret_cast<const char*>(glewGetErrorString(err))
    );
}

printf("Vendor: %s\n", glGetString(GL_VENDOR));
printf("Renderer: %s\n", glGetString(GL_RENDERER));
printf("Version: %s\n", glGetString(GL_VERSION));

m_hwnd = hwnd;
m_hdc = hdc;
m_context = context;

}

void Renderer::DestroyContext()
{
	if (!m_context)
		return;

	wglMakeCurrent(nullptr, nullptr);

	wglDeleteContext(static_cast<HGLRC>(m_context));

	ReleaseDC(static_cast<HWND>(m_hwnd), static_cast<HDC>(m_hdc));
	DestroyWindow(static_cast<HWND>(m_hwnd));

	m_context = nullptr;
	m_hdc = nullptr;
	m_hwnd = nullptr;
}


void Renderer::BeginFrame()
{
    glClearColor(0, 0, 0, 1);

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT |
        GL_STENCIL_BUFFER_BIT
    );
}


void Renderer::EndFrame()
{
    glFinish();

    glReadPixels(
        0,
        0,
        m_width,
        m_height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        m_pixels.data()
    );
}

bool Renderer::SavePNG(const std::string& path, int width, int height)
{
    const int channels = 4;
    const int rowSize = width * channels;

    std::vector<unsigned char> flipped(m_pixels.size());

    for (int y = 0; y < height; y++)
    {
        memcpy(
            flipped.data() + y * rowSize,
            m_pixels.data() + (height - 1 - y) * rowSize,
            rowSize
        );
    }


    stbi_flip_vertically_on_write(true);
    stbi_write_png(
        path.c_str(),
        width,
        height,
        channels,
        flipped.data(), 
        width * channels
    );

    return true;
}

void Renderer::ReadPixelsTo(FILE* output)
{
    const int channels = 4;
    const int rowSize = m_width * channels;

    fwrite(
        m_pixels.data(),
        1,
        m_pixels.size(),
        output
    );
}