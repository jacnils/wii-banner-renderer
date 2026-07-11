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
#include <cstdlib>
#include <io.h>
#include <fcntl.h>
#endif

#include <iostream>
#include <filesystem>
#include <cmath>

#include "Banner.h"
#include "Renderer.h"
#include "Wad.h"

struct Settings {
	int fps = 60; // fps to render at
	int minimum_length = 10; // min seconds
	int maximum_length = -1; // max seconds
	bool save_frames = false; // save frames? (wastes your time, useful for debugging)
	int frames_to_save = -1; // -1 = save all frames iterated through
	bool no_audio = false; // no audio, for debugging
	bool no_crop = false; // no crop, for debugging

	void print_settings() const {
		std::cout << "FPS: " << fps << "\n";
		std::cout << "Minimum length: " << (minimum_length == -1 ? "No limit" : std::to_string(minimum_length)) << " sec\n";
		std::cout << "Maximum length: " << (maximum_length == -1 ? "No limit" : std::to_string(maximum_length)) << " sec\n";
		std::cout << "Save frames: " << save_frames << "\n";
		std::cout << "Frames to save (if enabled): " << (frames_to_save == -1 ? "all" : std::to_string(frames_to_save)) << "\n";
		std::cout << "No audio: " << no_audio << "\n";
		std::cout << "No crop: " << no_crop << "\n";
	}
};

// wrapper for opening processes
struct ProcPtr {
	ProcPtr(const std::string& params, const std::string& mode = "w") {
		ptr =
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
		_popen(params.c_str(), mode.c_str());
#else
			popen(params.c_str(), mode.c_str());
#endif

		if (!ptr) {
			throw std::runtime_error{"failed to popen()"};
		}

		_setmode(_fileno(ptr), _O_BINARY);

	}
	~ProcPtr() {
		if (ptr)
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
			_pclose(ptr);
#else
				pclose(ptr);
#endif
	}

	[[nodiscard]] FILE* get() const {
		if (!ptr) {
			throw std::runtime_error{"uninitialized"};
		}
		return ptr;
	}

private:
	FILE* ptr{nullptr};

};

std::string get_crop(const std::vector<Vec2f>& points) {
	if (points.empty())
		return "";

	float min_x = points[0].x;
	float max_x = points[0].x;
	float min_y = points[0].y;
	float max_y = points[0].y;

	for (const auto& p : points)
	{
		min_x = std::min(min_x, p.x);
		max_x = std::max(max_x, p.x);
		min_y = std::min(min_y, p.y);
		max_y = std::max(max_y, p.y);
	}

	float width  = max_x - min_x;
	float height = max_y - min_y;

	std::ostringstream ss;
	ss << width << ":" << height << ":" << min_x << ":" << min_y;
	return ss.str();
}

int process(const std::string& input_opening, Settings settings = {}) {
	std::string opening = input_opening;

	std::cout << "Processing: " << opening << "\n";

	if (std::filesystem::path(opening).extension() == ".wad") {
		std::ifstream in(opening, std::ios::binary);
		if (!in) {
			std::cerr << "Input file does not exist or cannot be read: " << input_opening << "\n";
			return EXIT_FAILURE;
		}
		extract_wad(in, "tmp");

		for (const auto& entry : std::filesystem::directory_iterator("tmp"))
		{
			if (!entry.is_regular_file())
				continue;

			if (entry.path().extension() != ".app")
				continue;

			const std::string path = entry.path().string();

			if (WiiBanner::Banner::is_valid(path)) {
				opening = path;
				break;
			}
		}

		if (opening.empty())
		{
			std::cerr << "No valid .app file found, probably invalid .wad\n";
			return EXIT_FAILURE;
		}
	}

	std::string base_filename = std::filesystem::path(input_opening).filename().string();
	auto ext = base_filename.find_last_of('.');
	if (ext != std::string::npos) {
		base_filename = base_filename.substr(0, ext);
	}

    Renderer renderer(1920, 1080);

    WiiBanner::Banner banner(opening);

    banner.LoadBanner();

	if (!settings.no_audio)
		banner.LoadSound();

    banner.GetBanner()->SetLanguage("ENG");

	if (!settings.no_audio) {
		if (!banner.GetSound()) {
			throw std::runtime_error{"GetSound() failed"};
		}
	}
	double runtime{};

	if (!settings.no_audio) {
		banner.GetSound()->WriteWAV(base_filename + ".wav");
		runtime = banner.GetSound()->GetDurationSeconds();
	}

	if (settings.maximum_length >= 0) {
		runtime = std::min(runtime, static_cast<double>(settings.maximum_length));
	}
	if (settings.minimum_length >= 0) {
		runtime = std::max(runtime, static_cast<double>(settings.minimum_length));
	}

	if (!settings.no_audio) {
		banner.GetSound()->WriteWAVLooped(
			base_filename + ".wav",
			runtime
		);
	}

	std::string audio_param;
	if (!settings.no_audio) {
		audio_param = "-i " "\"" + base_filename + ".wav" + "\"" " ";
	}

	std::string crop;
	if (!settings.no_crop) {
		crop = "-vf \"crop=827:403:1026:0,scale=trunc(iw/2)*2:trunc(ih/2)*2\" ";
	}

	ProcPtr ffmpeg{
		"ffmpeg -y "
	"-f rawvideo "
	"-loglevel error "
	"-pixel_format rgba "
	"-video_size 1920x1080 "
	"-framerate " + std::to_string(settings.fps) + " "
	"-i - " + audio_param + "-t " + std::to_string(runtime) + " " + crop +
	"-c:v libx264 "
	"-pix_fmt yuv420p "
	"-c:a aac "
	"\"" + base_filename + ".mp4" + "\"", "w"};

    for (int i = 0; i < settings.fps * runtime; i++) {
    	renderer.BeginFrame();
    	banner.GetBanner()->Render(1.0f, 0xff, true);
		renderer.EndFrame();

		if (settings.save_frames && settings.frames_to_save <= i && settings.frames_to_save >= 0) {
			char filename[64];
			sprintf(filename, "output-%04d.png", i);

			renderer.SavePNG(filename, 1920, 1080);
  		}

  		renderer.ReadPixelsTo(ffmpeg.get());
    	banner.GetBanner()->AdvanceFrame();
    }

    banner.UnloadBanner();

	if (std::filesystem::is_directory("tmp")) {
		std::filesystem::remove_all("tmp");
	}

	std::cout << "Processed " << input_opening << "\n";

    return 0;
}

static std::string trim(const std::string& s) {
	const char* whitespace = " \t\n\r\f\v";

	size_t start = s.find_first_not_of(whitespace);
	if (start == std::string::npos)
		return "";

	size_t end = s.find_last_not_of(whitespace);
	return s.substr(start, end - start + 1);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "usage: wii-banner-renderer <00000000.app/opening.bnr/*.wad> -w|-m|-nc|-min int|-max int|-s int\n";
        return EXIT_FAILURE;
    }

	Settings settings{};
	std::vector<std::string> openings;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "-w") {
			std::string opening;
			std::cout << "input file(s) (comma split): " << std::flush;
			std::getline(std::cin, opening);

			std::stringstream ss(opening);
			std::string item;

			while (std::getline(ss, item, ',')) {
				item = trim(item);
				if (!item.empty())
					openings.emplace_back(item);
			}
		}
		if (arg == "-m") {
			// mute
			settings.no_audio = true;
		}
		if (arg == "-nc") {
			// no crop
			settings.no_crop = true;
		}

		if (arg == "-s") {
			settings.save_frames = true;

			if (i + 1 < argc) {
				std::string sec_arg = argv[i + 1];

				try {
					size_t pos = 0;
					int value = std::stoi(sec_arg, &pos);

					if (pos == sec_arg.size()) {
						settings.frames_to_save = value;
						++i;
					}
				} catch (...) {
					continue;
				}
			}
		}

		if (arg == "-min") {
			if (i + 1 < argc) {
				std::string sec_arg = argv[i + 1];

				try {
					size_t pos = 0;
					int value = std::stoi(sec_arg, &pos);

					if (pos == sec_arg.size()) { // ensure entire string is integers
						settings.minimum_length = value;
					} else {
						std::cerr << "invalid integer passed\n";
						return EXIT_FAILURE;
					}
				} catch (std::exception&) {
					std::cerr << "-min flag requires an integer\n";
				}
			} else {
				std::cerr << "-min flag requires an integer\n";
				return EXIT_FAILURE;
			}
		}

		if (arg == "-max") {
			if (i + 1 < argc) {
				std::string sec_arg = argv[i + 1];

				try {
					size_t pos = 0;
					int value = std::stoi(sec_arg, &pos);

					if (pos == sec_arg.size()) { // ensure entire string is integers
						settings.maximum_length = value;
					} else {
						std::cerr << "invalid integer passed\n";
						return EXIT_FAILURE;
					}
				} catch (std::exception&) {
					std::cerr << "-max flag requires an integer\n";
				}
			} else {
				std::cerr << "-max flag requires an integer\n";
				return EXIT_FAILURE;
			}
		}

		if (std::filesystem::is_regular_file(arg)) {
			openings.emplace_back(arg);
		}
	}

	std::cout << "Settings:\n";
	settings.print_settings();

	int ret_val = EXIT_SUCCESS;
	for (const auto& it : openings) {
		auto ret = process(it, settings);

		if (ret != EXIT_SUCCESS) {
			ret_val = EXIT_FAILURE;
		}
	}

	return ret_val;
}