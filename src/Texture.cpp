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

	FourCC magic;
	u32 texture_count;
	u32 header_size;

	file >> magic >> BE >> texture_count >> header_size;

	if (magic != BINARY_MAGIC_TEXTURE)
		return;

	file.seekg(header_size - 0xC, std::ios::cur);

	// only support a single texture
	//if (texture_count > 1) {
	//	texture_count = 1;
	//	std::cout << "texture count > 1\n";
	//}
	std::cout << "Texture count: " << texture_count << std::endl;

	std::streamoff next_offset = file.tellg();

	while (texture_count--)
	{
		file.seekg(next_offset, std::ios::beg);

		u32 texture_offset;
		u32 palette_offset;

		file >> BE >> texture_offset >> palette_offset;

		next_offset = file.tellg();

		// palette
		if (palette_offset)
		{
			file.seekg(file_start + palette_offset, std::ios::beg);

			u16 palette_unused;
			u32 palette_data_offset;

			file >> BE >> tlut_count
				>> palette_unused
				>> tlut_format
				>> palette_data_offset;

			file.seekg(file_start + palette_data_offset, std::ios::beg);

			tlut_ptr = new char[tlut_count * 2];

			file.read(tlut_ptr, tlut_count * 2);
		}

		// texture header
		file.seekg(file_start + texture_offset, std::ios::beg);

		u32 format;
		u32 texture_data_offset;

		u16 height;
		u16 width;

		u32 wrap_s;
		u32 wrap_t;

		u32 min_filter;
		u32 mag_filter;

		float lod_bias;
		u8 edge_lod;
		u8 min_lod;
		u8 max_lod;
		u8 unpacked;

		file >> BE
			>> height
			>> width
			>> format
			>> texture_data_offset
			>> wrap_s
			>> wrap_t
			>> min_filter
			>> mag_filter
			>> lod_bias
			>> edge_lod
			>> min_lod
			>> max_lod
			>> unpacked;

		file.seekg(file_start + texture_data_offset, std::ios::beg);

		const u32 tex_size =
			GX_GetTexBufferSize(width, height, format, true, max_lod);

		img_ptr = new char[tex_size];

		file.read(img_ptr, tex_size);

		if (palette_offset)
		{
			file.seekg(file_start + palette_offset, std::ios::beg);

			u16 palette_unused;
			u32 palette_data_offset;

			file >> BE >> tlut_count
				 >> palette_unused
				 >> tlut_format
				 >> palette_data_offset;

			file.seekg(file_start + palette_data_offset, std::ios::beg);

			tlut_ptr = new char[tlut_count * 2];
			file.read(tlut_ptr, tlut_count * 2);

			GX_InitTexObj(
				&texobj,
				img_ptr,
				width,
				height,
				format,
				wrap_s,
				wrap_t,
				true
			);


			GX_InitTexObjTlut(&texobj, 0);
		}
		else
		{
			GX_InitTexObj(
				&texobj,
				img_ptr,
				width,
				height,
				format,
				wrap_s,
				wrap_t,
				true
			);
		}

		GX_InitTexObjFilterMode(
			&texobj,
			min_filter,
			mag_filter
		);
	}
}

void Texture::Apply(u8& tlutName, u8 map_id, u8 wrap_s, u8 wrap_t) const {
	if (map_id >= 8 || tlutName >= 20)
		return;

	GXTexObj tmpTexObj = texobj;

	printf(
	"Apply tex %s img=%p tlut=%p map=%u\n",
	name.c_str(),
	img_ptr,
	tlut_ptr,
	map_id
	);

	if (tlut_ptr)
	{
		GXTlutObj tlutobj;

		GX_InitTlutObj(
			&tlutobj,
			tlut_ptr,
			tlut_format,
			tlut_count
		);

		GX_LoadTlut(
			&tlutobj,
			tlutName
		);

		GX_InitTexObjTlut(
			&tmpTexObj,
			tlutName
		);

		tlutName++;
	}

	GX_InitTexObjWrapMode(
		&tmpTexObj,
		wrap_s,
		wrap_t
	);

	GX_LoadTexObj(
		&tmpTexObj,
		map_id
	);
}

}
