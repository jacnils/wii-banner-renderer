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

#include "Font.h"
#include "Endian.h"

namespace WiiBanner
{

struct CharWidths
{
	s8 left;                    // left space width of character
	u8 glyph_width;              // glyph width of character
	s8 char_width;               // character width  = left space width  + glyph width  + right space width
};

enum BinaryMagic : u32
{
	BINARY_MAGIC_FONT = MAKE_FOURCC('R', 'F', 'N', 'A'),

	BINARY_MAGIC_GLYPH_GROUP = MAKE_FOURCC('G', 'L', 'G', 'R'),
	BINARY_MAGIC_FONT_INFORMATION = MAKE_FOURCC('F', 'I', 'N', 'F'),
	BINARY_MAGIC_TEXTURE_GLYPH = MAKE_FOURCC('T', 'G', 'L', 'P'),
	BINARY_MAGIC_CHARACTER_CODE_MAP = MAKE_FOURCC('C', 'M', 'A', 'P'),
	BINARY_MAGIC_CHARACTER_WIDTH = MAKE_FOURCC('C', 'W', 'D', 'H')
};

void Font::Load(std::istream& file)
{
	const std::streamoff file_start = file.tellg();

	// read header
	FourCC header_magic;
	u16 endian;
	u16 version;
	u32 filesize;
	u16 offset; // offset to first section
	u16 section_count;

	file >> header_magic >> BE >> endian >> version
		>> filesize >> offset >> section_count;

	if (header_magic != BINARY_MAGIC_FONT
		|| endian != 0xFEFF
		|| version != 0x0104
		)
		return;	// bad header

	// seek to the first section
	file.seekg(file_start + offset, std::ios::beg);

	ReadSections(file, section_count, [&](FourCC magic, std::streamoff section_start)
	{
		if (magic == BINARY_MAGIC_GLYPH_GROUP)	// glyph group
		{
			u32 sheet_size;
			u16 glyphs_per_sheet;

			u16 set_count, sheet_count, cwdh_count, cmap_count;

			file >> BE >> sheet_size >> glyphs_per_sheet
				>> set_count >> sheet_count >> cwdh_count >> cmap_count;

			std::vector<std::string> sets;

			ReadOffsetList<u16>(file, set_count, file_start, [&]
			{
				sets.push_back(ReadNullTerminatedString(file));
			});

			std::vector<u32> sheet_sizes;
			sheet_sizes.resize(sheet_count);
			if (sheet_count)
				ReadBEArray(file, &sheet_sizes[0], sheet_count);

			std::vector<u32> cwdh_sizes;
			cwdh_sizes.resize(cwdh_count);
			if (cwdh_count)
				ReadBEArray(file, &cwdh_sizes[0], cwdh_count);

			std::vector<u32> cmap_sizes;
			cmap_sizes.resize(cmap_count);
			if (cmap_count)
				ReadBEArray(file, &cmap_sizes[0], cmap_count);

			// temporary
			// TODO: read bunch of bitsets
			file.seekg((sheet_count + cwdh_count + cmap_count) / 32, std::ios::cur);
		}
		else if (magic == BINARY_MAGIC_FONT_INFORMATION)	// font information
		{
			u8 font_type;
			s8 linefeed;
			u16 alter_char_index;
			CharWidths default_width;
			u8 encoding;
			u32 pGlyph, pWidth, pMap;
			u8 height, width, ascent;

			file >> BE >> font_type >> linefeed >> alter_char_index
				>> default_width.left >> default_width.glyph_width >> default_width.char_width
				>> encoding >> pGlyph >> pWidth >> pMap >> height >> width >> ascent;

			//std::cout << "finf\n";
		}
		else if (magic == BINARY_MAGIC_TEXTURE_GLYPH)	// texture glyph
		{
			u8 cell_width, cell_height;
			s8 baseline_pos;
			u8 max_char_width;
			u32 sheet_size;
			u16 sheet_count, sheet_format, sheet_row, sheet_line, sheet_width, sheet_height;
			u32 sheet_image;

			file >> BE >> cell_width >> cell_height >> baseline_pos >> max_char_width
				>> sheet_size >> sheet_count >> sheet_format >> sheet_row
				>> sheet_row >> sheet_line >> sheet_width >> sheet_height;

			file >> LE >> sheet_image;

			//std::cout << "tglp\n";

			//std::cout << file.tellg() << '\n';

			// testing
			//sheet_width = sheet_row * cell_width;
			sheet_height = sheet_line * cell_height;

			auto const tex_size = GX_GetTexBufferSize(sheet_width, sheet_height, sheet_format & 0x7fff, 0, 0);

			// TODO: is it ok to assume texture data starts right here?
			file.seekg(sheet_image, std::ios::beg);

			img_ptr = new char[tex_size];
			file.read(img_ptr, tex_size);

			GX_InitTexObj(&texobj, img_ptr, sheet_width, sheet_height, sheet_format & 0x7fff, 0, 0, 0);
		}
		else if (magic == BINARY_MAGIC_CHARACTER_CODE_MAP)	// char code map
		{
			CodeMap cmap;
			file >> BE >> cmap.ccode_begin >> cmap.ccode_end >> cmap.mapping_method;
			file.seekg(2, std::ios::cur);
			file >> BE >> cmap.pNext;

			code_maps.push_back(std::move(cmap));
		}
		else if (magic == BINARY_MAGIC_CHARACTER_WIDTH)	// character width
		{
			u16 index_begin;
			u16 index_end;
			u32 pNext;
			//CharWidths widthTable[];

			file >> BE >> index_begin >> index_end >> pNext;
		}
		else
		{
			std::cout << "UNKNOWN SECTION: ";
			std::cout << magic << '\n';
		}
	});
}

void Font::Apply() const
{
	// TODO:

	GX_LoadTexObj(const_cast<GXTexObj*>(&texobj), 0); // ugly
}

}
