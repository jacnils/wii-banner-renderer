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

#ifndef WII_BNR_LZ77_H_
#define WII_BNR_LZ77_H_

#include <sstream>

#include "Endian.h"
#include "Types.h"

enum : u32
{
	BINARY_MAGIC_LZ77 = MAKE_FOURCC('L', 'Z', '7', '7')
};

class LZ77Decompressor
{
public:
	LZ77Decompressor(std::istream& in)
	{
		const u32 TYPE_LZ77 = 1;

		u32 magic;
		in >> BE >> magic;

		if (magic != BINARY_MAGIC_LZ77)
		{
			// LZ77 header not present
			in.seekg(-4, std::ios::cur);
			ret_stream = &in;
			return;
		}

		ret_stream = &data;

		u32 hdr;
		in >> LE >> hdr;

		const u32 uncompressed_length = hdr >> 8;
		const u32 compression_type = hdr >> 4 & 0xf;

		if (TYPE_LZ77 != compression_type)
			return;

		u32 written = 0;
		while (written != uncompressed_length)
		{
			u8 flags = in.get();
			for (int f = 0; f != 8; ++f)
			{
				if (flags & 0x80)
				{
					u16 info;
					in >> BE >> info;

					const u8 num = 3 + (info >> 12);
					const u16 disp = info & 0xFFF;
					u32 ptr = written - disp - 1;

					data.seekg(ptr, std::ios::beg);
					for (u8 p = 0; p != num; ++p)
					{
						char c;
						data.get(c);
						data.put(c);
						++written;

						if (written == uncompressed_length)
							break;
					}
				}
				else
				{
					char c;
					in.get(c);
					data.put(c);
					++written;
				}

				flags <<= 1;

				if (written == uncompressed_length)
					break;
			}
		}

		data.seekg(0, std::ios::beg);
	}

	std::istream& GetStream()
	{
		return *ret_stream;
	}

private:
	std::stringstream data;
	std::istream* ret_stream;
};

#endif
