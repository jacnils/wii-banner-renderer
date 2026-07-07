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

#ifndef WII_BNR_FONT_H_
#define WII_BNR_FONT_H_

#include <list>
#include <vector>

#include "Pane.h"

namespace WiiBanner
{

class Font : public Named
{
public:
	Font() : img_ptr(nullptr) {}
	~Font() { delete[] img_ptr; }

	void Load(std::istream& file);

	void Apply() const;

private:
	struct CodeMap
	{
		u16 ccode_begin;
		u16 ccode_end;
		u16 mapping_method;

		u32 pNext;
		//u16 map_info[];
	};
	std::list<CodeMap> code_maps;

	GXTexObj texobj;

	char* img_ptr;
};

class FontList : public std::vector<Font*>
{
public:
	static const u32 BINARY_MAGIC = MAKE_FOURCC('f', 'n', 'l', '1');
};

}

#endif
