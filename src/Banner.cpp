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

#include <fstream>

#include <GL/glew.h>

// hax
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_
// from dolphin
#include "FileHandlerARC.h"

#include "Banner.h"
#include "LZ77.h"
#include "Sound.h"
#include "Endian.h"
#include "Types.h"

namespace WiiBanner
{

enum BinaryMagic : u32
{
	BINARY_MAGIC_U8_ARCHIVE = MAKE_FOURCC('U', 0xAA, '8', '-'),

	BINARY_MAGIC_ANIMATION = MAKE_FOURCC('R', 'L', 'A', 'N'),
	BINARY_MAGIC_PANE_ANIMATION_INFO = MAKE_FOURCC('p', 'a', 'i', '1')
};

// load keyframes from a brlan file
// TODO: can put this function somewhere better :p
FrameNumber LoadAnimators(std::istream& file, Layout& layout, u8 key_set)
{
	const std::streamoff file_start = file.tellg();

	u16 frame_count;

	// read header
	FourCC header_magic;
	u16 endian; // always 0xFEFF
	u16 version; // always 0x0008

	file >> header_magic >> BE >> endian >> version;

	if (header_magic != BINARY_MAGIC_ANIMATION
		|| endian != 0xFEFF
		|| version != 0x008
		)
		return 0;	// bad header


	u32 file_size;
	u16 offset; // offset to the first section
	u16 section_count;

	file >> BE >> file_size >> offset >> section_count;

	// only a single pa*1 section is currently supported
	//if (section_count > 1)
	//	section_count = 1;

	// seek to header of first section
	file.seekg(file_start + offset, std::ios::beg);

	ReadSections(file, section_count, [&](FourCC magic, std::streamoff section_start)
	{
		if (magic == BINARY_MAGIC_PANE_ANIMATION_INFO)
		{
			u8 loop; // ?
			u8 pad;
			u16 file_count; // ?
			u16 animator_count;
			u32 entry_offset;

			file >> BE >> frame_count >> loop
				>> pad >> file_count >> animator_count;

			file >> BE >> entry_offset;
			file.seekg(section_start + entry_offset);

			// read each animator
			ReadOffsetList<u32>(file, animator_count, section_start, [&]
			{
				const std::streamoff origin = file.tellg();

				const std::string animator_name = ReadFixedLengthString<Animator::NAME_LENGTH>(file);

				u8 tag_count;
				u8 is_material;
				u16 apad;

				file >> BE >> tag_count >> is_material >> apad;

				Animator* const animator = is_material ?
					static_cast<Animator*>(layout.FindMaterial(animator_name)) :
					static_cast<Animator*>(layout.FindPane(animator_name));

				if (animator)
					animator->LoadKeyFrames(file, tag_count, origin, key_set);
			});
		}
		else
		{
			std::cout << "UNKNOWN SECTION: ";
			std::cout << magic << '\n';
		}
	});

	return frame_count;
}

Banner::Banner(const std::string& _filename)
	: layout_banner(nullptr)
	, layout_icon(nullptr)
	//, sound(nullptr)
	, filename(_filename)
{
	std::ifstream bnr_file(filename, std::ios::binary | std::ios::in);

	// opening.bnr  archives have 0x600 byte headers
	// 00000000.app archives have 0x640 byte headers
	header_bytes = 0x600;

	bnr_file.seekg(header_bytes, std::ios::cur);

	// lets see if this is an opening.bnr
	FourCC magic;
	bnr_file >> magic;
	if (magic != BINARY_MAGIC_U8_ARCHIVE)
	{
		// lets see if it's a 00000000.app
		bnr_file.seekg(60, std::ios::cur);
		bnr_file >> magic;

		if (magic != BINARY_MAGIC_U8_ARCHIVE)
			return;	// not a 00000000.app either

		header_bytes = 0x640;
	}

	header_bytes += 32;	// the inner-files have bigger headers

	bnr_file.seekg(-4, std::ios::cur);
	DiscIO::CARCFile opening_arc(bnr_file);

	offset_banner = opening_arc.GetFileOffset("meta/" "banner" ".bin");
	offset_icon = opening_arc.GetFileOffset("meta/" "icon" ".bin");
	offset_sound = opening_arc.GetFileOffset("meta/" "sound" ".bin");
}

void Banner::LoadBanner()
{
	if (offset_banner && !layout_banner)
		layout_banner = LoadLayout("Banner", offset_banner, Vec2f(608.f, 456.f));
}

/* defined in header
void Banner::LoadIcon()
{
	if (offset_icon && !layout_icon)
		layout_icon = LoadLayout("Icon", offset_icon, Vec2f(128.f, 96.f));
}
*/

void Banner::LoadSound()
{
    std::cout << "offset_sound = " << offset_sound << "\n";

    if (offset_sound && !sound)
    {
        std::ifstream bnr_file(filename, std::ios::binary | std::ios::in);

        if (!bnr_file)
        {
            std::cerr << "Failed to open banner file\n";
            return;
        }

        bnr_file.seekg(header_bytes + offset_sound, std::ios::beg);

        std::cout << "Loading sound at offset "
                  << header_bytes + offset_sound << "\n";

        auto* const s = new Sound;

        if (s->Load(bnr_file))
        {
            std::cout << "Sound loaded\n";
            sound = s;
        }
        else
        {
            delete s;
            std::cerr << "s->Load() failed\n";
        }
    }
    else
    {
        std::cout << "No sound offset or already loaded\n";
    }
}

Layout* Banner::LoadLayout(const std::string& lyt_name, std::streamoff offset, Vec2f size)
{
	std::ifstream bnr_file(filename, std::ios::binary | std::ios::in);

	bnr_file.seekg(header_bytes + offset, std::ios::beg);

	// LZ77 decompress .bin file
	LZ77Decompressor decomp(bnr_file);
	std::istream& file = decomp.GetStream();

	DiscIO::CARCFile bin_arc(file);
	const auto brlyt_offset = bin_arc.GetFileOffset("arc/blyt/" + lyt_name + ".brlyt");

	if (0 == brlyt_offset)
		return nullptr;

	file.seekg(brlyt_offset, std::ios::beg);
	auto* const layout = new Layout;
	layout->Load(file);

	// override size
	layout->SetWidth(size.x);
	layout->SetHeight(size.y);

	// load textures
	for (Texture* texture : layout->resources.textures)
	{
		auto const texture_offset = bin_arc.GetFileOffset("arc/timg/" + texture->GetName());
		if (texture_offset)
		{
			file.seekg(texture_offset, std::ios::beg);
			texture->Load(file);
		}
	}

	// load fonts
	{
	// this guy is in "User/Wii/shared1"
	std::ifstream font_file("00000003.app", std::ios::binary | std::ios::in);
	DiscIO::CARCFile font_arc(font_file);

	for (Font* font : layout->resources.fonts)
	{
		auto const font_offset = font_arc.GetFileOffset(font->GetName());

		if (font_offset)
		{
			font_file.seekg(font_offset, std::ios::beg);
			font->Load(font_file);

			//std::cout << "font name: " << font->name << '\n';
		}
	}
	}

	// load animations
	FrameNumber length_start = 0, length_loop = 0;

	auto const brlan_start_offset = bin_arc.GetFileOffset("arc/anim/" + lyt_name + "_Start.brlan");
	size_t brlan_loop_offset = 0;

	//std::cout << lyt_name + "_Start.brlan offset is: " << brlan_start_offset << '\n';
	if (brlan_start_offset)
	{
		file.seekg(brlan_start_offset, std::ios::beg);
		length_start = LoadAnimators(file, *layout, 0);

		// banner uses 2 brlan files, a starting one and a looping one
		brlan_loop_offset = bin_arc.GetFileOffset("arc/anim/" + lyt_name + "_Loop.brlan");
	}
	else
	{
		// banner uses a single repeating brlan
		brlan_loop_offset = bin_arc.GetFileOffset("arc/anim/" + lyt_name + ".brlan");
	}

	//std::cout << lyt_name + "[_Loop].brlan offset is: " << brlan_loop_offset << '\n';
	if (brlan_loop_offset)
	{
		file.seekg(brlan_loop_offset, std::ios::beg);
		length_loop = LoadAnimators(file, *layout, 1);
	}

	layout->SetLoopStart(length_start);
	layout->SetLoopEnd(length_start + length_loop);
	// update everything for frame 0
	layout->SetFrame(0);

	return layout;
}

Banner::~Banner()
{
	UnloadBanner();
	UnloadIcon();
	//UnloadSound();
}

}
