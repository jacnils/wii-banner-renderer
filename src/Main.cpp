/*
Copyright (c) 2010 - Wii Banner Player Project
Copyright (c) 2026 - Jacob Nilsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#endif

#include <GL/glew.h>
#include <iostream>
#include <filesystem>

#include "Banner.h"
#include "Types.h"
#include "Renderer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: wii-banner-renderer <00000000.app/opening.bnr>\n";
        return EXIT_FAILURE;
    }

    if (!std::filesystem::is_regular_file(argv[1])) {
    	std::cerr << "Input file does not exist or cannot be read." << "\n";
	return EXIT_FAILURE;
    }

    Renderer renderer(1920, 1080);

    WiiBanner::Banner banner(argv[1]);

    banner.LoadBanner();
    banner.LoadSound();

    banner.GetBanner()->SetLanguage("ENG");

    if (!banner.GetSound()) {
    	throw std::runtime_error{"GetSound() failed"};
    }

    banner.GetSound()->WriteWAV("output.wav");

    const int fps = 60;
    auto sfx_length = banner.GetSound()->GetDurationSeconds();
    bool save_frames = false;

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

    FILE* ffmpeg = POPEN(
        "ffmpeg -y "
        "-f rawvideo "
        "-loglevel error "
        "-pixel_format rgba "
        "-video_size 1920x1080 "
        "-framerate 60 "
        "-i - "
        "-i output.wav "
        "-vf \"crop=933:403:970:545,scale=trunc(iw/2)*2:trunc(ih/2)*2\" "
        "-c:v libx264 "
        "-pix_fmt yuv420p "
        "-c:a aac "
        "-shortest "
        "output.mp4",
        "w"
    );

#ifdef _WIN32
    _setmode(_fileno(ffmpeg), _O_BINARY);
#endif

    for (int i = 0; i < fps * sfx_length; i++) {
    	renderer.BeginFrame();
    	
	banner.GetBanner()->Render(
    		16.0f / 9.0f,
    		596.0f / 608.0f
	);

    	renderer.EndFrame();

	if (save_frames) {
    		char filename[64];
		sprintf(filename, "output-%04d.png", i);

		renderer.SavePNG(filename, 1920, 1080);
  	}
	
  	renderer.ReadPixelsTo(ffmpeg);
    	banner.GetBanner()->AdvanceFrame();
    }


    banner.UnloadBanner();

    return 0;
}
