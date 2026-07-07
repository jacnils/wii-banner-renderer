#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <GL/glew.h>
#include <windows.h>

class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer();

    void BeginFrame();
    void EndFrame();
    bool SavePNG(const std::string& path, int width, int height);
    void ReadPixelsTo(FILE* output);

    const uint8_t* Pixels() const
    {
        return m_pixels.data();
    }

    int Width() const
    {
        return m_width;
    }

    int Height() const
    {
        return m_height;
    }

private:
    void CreateContext();
    void DestroyContext();

    int m_width;
    int m_height;

    std::vector<uint8_t> m_pixels;

    void* m_display = nullptr;
    void* m_surface = nullptr;
    void* m_context = nullptr;

    HWND m_hwnd = nullptr;
    HDC m_hdc = nullptr;
    HGLRC m_glrc = nullptr;
};
