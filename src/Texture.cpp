/*
Copyright (c) 2010 - Wii Banner Player Project

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
#include <set>

#include "Texture.h"
#include "Endian.h"

namespace WiiBanner
{

enum BinaryMagic : u32
{
	BINARY_MAGIC_TEXTURE = MAKE_FOURCC(0x00, ' ', 0xAF, 0x30)
};

static std::set<u32> g_occupied_tlut_names;

u32 GetFreeTlutName()
{
	u32 ret = 1;
	while (g_occupied_tlut_names.end() != g_occupied_tlut_names.find(ret))
		++ret;

	return ret;
}

Texture::~Texture()
{
	delete[] img_ptr;
	delete[] tlut_ptr;

	g_occupied_tlut_names.erase(tlut_name);
}

void Texture::Load(std::istream& file)
{
	const std::streamoff file_start = file.tellg();

	// read file header
	FourCC magic;
	u32 texture_count;
	u32 header_size; // always 0xC

	file >> magic >> BE >> texture_count >> header_size;

	if (magic != BINARY_MAGIC_TEXTURE)
		return;	// bad header

	// seek to end of header
	file.seekg(header_size - 0xC, std::ios::cur);

	// only support a single texture
	if (texture_count > 1)
	{
		texture_count = 1;

		std::cout << "texture count > 1\n";
		std::cin.get();
	}

	// read texture offsets
	std::streamoff next_offset = file.tellg();
	while (texture_count--)
	{
		file.seekg(next_offset, std::ios::beg);

		// header offsets
		u32 texture_offset;
		u32 palette_offset; // 0 if no palette

		file >> BE >> texture_offset >> palette_offset;

		next_offset = file.tellg();

		// seek to/read palette header
		if (palette_offset)
		{
			file.seekg(file_start + palette_offset, std::ios::beg);

			u16 palette_count;
			u16 palette_unused; // u8 unpacked, u8 pad8
			u32 palette_format;
			u32 palette_data_offset;

			file >> BE >> palette_count >> palette_unused
				>> palette_format >> palette_data_offset;

			// TODO: check if > sizeof(g_tlut_read_buffer)

			// seek to/read palette data
			file.seekg(file_start + palette_data_offset, std::ios::beg);
			tlut_ptr = new char[palette_count * 2];
			file.read(tlut_ptr, palette_count * 2);

			// load tlut
			GXTlutObj tlutobj;
			GX_InitTlutObj(&tlutobj, tlut_ptr, palette_format, palette_count);

			g_occupied_tlut_names.insert(tlut_name = GetFreeTlutName());

			GX_LoadTlut(&tlutobj, tlut_name);
			GX_InitTexObjTlut(&texobj, tlut_name);
		}

		// seek to texture header
		file.seekg(file_start + texture_offset, std::ios::beg);

		// read texture header
		u32 format;
		u32 texture_data_offset;

		u16 height, width;
		u32 wrap_s, wrap_t;
		u32 min_filter, mag_filter;

		float lod_bias;
		u8 edge_lod, min_lod, max_lod;

		u8 unpacked;

		file >> BE >> height >> width >> format >> texture_data_offset
			>> wrap_s >> wrap_t >> min_filter >> mag_filter
			>> lod_bias >> edge_lod >> min_lod >> max_lod >> unpacked;

		// seek to texture data
		file.seekg(file_start + texture_data_offset, std::ios::beg);

		const u32 tex_size = GX_GetTexBufferSize(width, height, format, true, max_lod);

		//if (tex_size > sizeof(g_texture_read_buffer))
			//std::cout << "texture is too large\n";
		//else
		{
			img_ptr = new char[tex_size];

			file.read(img_ptr, tex_size);

			// load the texture
			GX_InitTexObj(&texobj, img_ptr,	width, height, format, wrap_s, wrap_t, true);

			// filter mode
			GX_InitTexObjFilterMode(&texobj, min_filter, mag_filter);
		}
	}
}

}
