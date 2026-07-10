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

#include <stack>

#include <GL/glew.h>

#include "Layout.h"
#include "Picture.h"
#include "Textbox.h"
#include "Window.h"
#include "Endian.h"

namespace WiiBanner
{

enum BinaryMagic : u32
{
	BINARY_MAGIC_LAYOUT = MAKE_FOURCC('R', 'L', 'Y', 'T'),

	BINARY_MAGIC_PANE_PUSH = MAKE_FOURCC('p', 'a', 's', '1'),
	BINARY_MAGIC_PANE_POP = MAKE_FOURCC('p', 'a', 'e', '1'),
	BINARY_MAGIC_GROUP_PUSH = MAKE_FOURCC('g', 'r', 's', '1'),
	BINARY_MAGIC_GROUP_POP = MAKE_FOURCC('g', 'r', 'e', '1')
};

template <typename P>
Pane* LoadNewPane(std::istream& file)
{
	P* const pane = new P;
	pane->Load(file);
	return pane;
}

void Layout::Load(std::istream& file)
{
	const std::streamoff file_start = file.tellg();

	frame_current = frame_loop_start = frame_loop_end = 0.f;
	width = height = 0.f;
	centered = 0;

	// read header
	FourCC header_magic;
	u16 endian;
	u16 version;
	u32 filesize;
	u16 first_section_offset; // offset to first section
	u16 section_count;

	file >> header_magic >> BE >> endian >> version
		>> filesize >> first_section_offset >> section_count;

	if (header_magic != BINARY_MAGIC_LAYOUT
		|| endian != 0xFEFF
		|| version != 0x0008
		)
		return;	// bad header

	// temporary stacks
	Group* last_group = nullptr;
	std::stack<std::map<std::string, Group>*> group_stack;
	group_stack.push(&groups);

	Pane* last_pane = nullptr;
	std::stack<std::vector<Pane*>*> pane_stack;
	pane_stack.push(&panes);

	auto const add_pane = [&](Pane* pane)
	{
		pane_stack.top()->push_back(last_pane = pane);
	};

	// seek to the first section
	file.seekg(file_start + first_section_offset, std::ios::beg);

	ReadSections(file, section_count, [&](FourCC magic, std::streamoff section_start)
	{
		if (magic == Layout::BINARY_MAGIC)
		{
			// read layout
			file >> BE >> centered;
			file.seekg(3, std::ios::cur);
			file >> BE >> width >> height;
		}
		else if (magic == TextureList::BINARY_MAGIC)
		{
			// load texture list
			u16 texture_count;
			u16 offset;

			file >> BE >> texture_count >> offset;

			ReadOffsetList<u32>(file, texture_count, file.tellg(), [&]
			{
				auto* const texture = new Texture;
				texture->SetName(ReadNullTerminatedString(file));

				resources.textures.push_back(texture);

			}, 4);

			std::cout << "Loaded " << resources.textures.size() << " Textures\n";
		}
		else if (magic == FontList::BINARY_MAGIC)
		{
			// load font list
			u16 font_count;
			u16 offset;

			file >> BE >> font_count >> offset;

			ReadOffsetList<u32>(file, font_count, file.tellg(), [&]
			{
				auto* const font = new Font;
				font->SetName(ReadNullTerminatedString(file));

				resources.fonts.push_back(font);

			}, 4);

			std::cout << "Loaded " << resources.fonts.size() << " Fonts\n";
		}
		else if (magic == MaterialList::BINARY_MAGIC)
		{
			// load materials
			u16 material_count;
			u16 offset;

			file >> BE >> material_count >> offset;

			ReadOffsetList<u32>(file, material_count, section_start, [&]
			{
				auto* const mat = new Material;
				mat->Load(file);
				resources.materials.push_back(mat);
			});

			std::cout << "Loaded " << resources.materials.size() << " Materials\n";
		}
		else if (magic == Pane::BINARY_MAGIC)
		{
			add_pane(LoadNewPane<Pane>(file));
		}
		else if (magic == Bounding::BINARY_MAGIC)
		{
			add_pane(LoadNewPane<Bounding>(file));
		}
		else if (magic == Picture::BINARY_MAGIC)
		{
			add_pane(LoadNewPane<Picture>(file));
		}
		else if (magic == Window::BINARY_MAGIC)
		{
			add_pane(LoadNewPane<Window>(file));
		}
		else if (magic == Textbox::BINARY_MAGIC)
		{
			add_pane(LoadNewPane<Textbox>(file));
		}
		else if (magic == BINARY_MAGIC_PANE_PUSH)
		{
			if (last_pane)
				pane_stack.push(&last_pane->panes);
		}
		else if (magic == BINARY_MAGIC_PANE_POP)
		{
			if (pane_stack.size() > 1)
				pane_stack.pop();
		}
		else if (magic == Layout::Group::BINARY_MAGIC)
		{
			Group& group_ref = (*group_stack.top())[ReadFixedLengthString<Layout::Group::NAME_LENGTH>(file)];

			u16 sub_count;
			file >> BE >> sub_count;
			file.seekg(2, std::ios::cur);

			while (sub_count--)
			{
				group_ref.panes.push_back(ReadFixedLengthString<Pane::NAME_LENGTH>(file));
			}

			last_group = &group_ref;
		}
		else if (magic == BINARY_MAGIC_GROUP_PUSH)
		{
			if (last_group)
				group_stack.push(&last_group->groups);
		}
		else if (magic == BINARY_MAGIC_GROUP_POP)
		{
			if (group_stack.size() > 1)
				group_stack.pop();
		}
		else
		{
			std::cout << "UNKNOWN SECTION: ";
			std::cout << magic << '\n';
		}
	});
}

Layout::~Layout()
{
	for (Pane* pane : panes)
		delete pane;

	for (Material* material : resources.materials)
		delete material;

	for (Texture* texture : resources.textures)
		delete texture;

	for (Font* font : resources.fonts)
		delete font;
}

void Layout::Render(float aspect_ratio, float zoom) const
{

	// TODO: make this work good :p
	Vec2f adjust;
	adjust.x = aspect_ratio / 4 * 3;
	adjust.y = 1.f;

	glPushMatrix();

	//glOrtho(0.f, width * adjust.x, 0.f, height * adjust.y, -1000.f, 1000.f);
	// assuming always centered, hope this isn't an issue
	//glTranslatef(width / 2 * adjust.x, height / 2 * adjust.y, 0.f);

	glTranslatef(0.5, 0.5, 0);
	glScalef(zoom / (width * adjust.x), -zoom / (height * adjust.y), 1.f);

	//glScalef(zoom, zoom, 1.f);

	for (Pane* pane : panes) {
		pane->Render(resources, 0xff, adjust);
	}
	glPopMatrix();
}

void Layout::SetFrame(FrameNumber frame_number)
{
	frame_current = frame_number;

	const u8 key_set = (frame_current >= frame_loop_start);
	if (key_set)
		frame_number -= frame_loop_start;

	for (Pane* pane : panes)
		pane->SetFrame(frame_number, key_set);

	for (Material* material : resources.materials)
		material->SetFrame(frame_number, key_set);
}

void Layout::AdvanceFrame()
{
	++frame_current;

	// should i just use == ?
	if (frame_current >= frame_loop_end)
		frame_current = frame_loop_start;

	SetFrame(frame_current);
}

void Layout::SetLanguage(const std::string& language)
{
	// TODO: i'd like an empty language to unhide everything, maybe

	// hide panes of non-matching languages
	for (auto& group : groups["RootGroup"].groups)
	{
		// some hax, there are some odd "Rso0" "Rso1" groups that shouldn't be hidden
		// only the 3 character language groups should be
		if (group.first != language && group.first.length() == 3)
		{
			for (auto& pane : group.second.panes)
			{
				if (Pane* const found = FindPane(pane))
					found->SetHide(true);
			}
		}
	}

	// unhide panes of matching language, some banners list language specific panes in multiple language groups
	for (auto& pane : groups["RootGroup"].groups[language].panes)
	{
		Pane* const found = FindPane(pane);
		if (found)
			found->SetHide(false);
	}
}

Texture* Layout::FindTexture(const std::string& find_name) {
	for(auto & texture : resources.textures) {
		if (find_name == texture->GetName())
			return texture;
	}

	return nullptr;
}

void Layout::AddPalette(const std::string& name, u8 key_set) {
	std::cout << "AddPalette: " << name << "\n";

	resources.palettes[key_set].push_back(name);

	if (FindTexture(name))
	{
		std::cout << "already exists\n";
		return;
	}



	auto* texture = new Texture;
	texture->SetName(name);

	std::cout << "Creating texture "
		  << texture
		  << " name="
		  << texture->GetName()
		  << "\n";

	resources.textures.push_back(texture);

	std::cout << "added texture: " << texture->GetName() << "\n";
}
Pane* Layout::FindPane(const std::string& find_name)
{
	Pane* found = nullptr;

	for (Pane* pane : panes)
	{
		found = pane->FindPane(find_name);
		if (found)
			break;
	}

	return found;
}

Material* Layout::FindMaterial(const std::string& find_name)
{
	for (Material* material : resources.materials)
	{
		if (find_name == material->GetName())
			return material;
	}

	return nullptr;
}

}
