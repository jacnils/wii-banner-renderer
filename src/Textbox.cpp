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

#include <GL/glew.h>
#include <cstring>

#include "Textbox.h"
#include "Layout.h"
#include "Endian.h"

namespace WiiBanner
{

void Textbox::Load(std::istream& file)
{
	Pane::Load(file);

	u16 text_buf_bytes, text_str_bytes;

	file >> BE >> text_buf_bytes >> text_str_bytes
		>> material_index >> font_index >> text_position >> text_alignment;

	file.seekg(2, std::ios::cur);

	u32 text_str_offset;

	file >> BE >> text_str_offset;

	ReadBEArray(file, &colors->r, sizeof(colors));

	file >> BE >> width >> height >> space_char >> space_line;

	// read utf-16 string
	while (true)
	{
		wchar_t c = 0;
		file >> BE >> c;

		if (c)
			text += c;
		else
			break;
	}

	std::wcout << L"Text: " << text << L'\n';
}

void Textbox::Draw(const Resources& resources, u8 render_alpha, Vec2f adjust) const
{
	if (material_index < resources.materials.size())
		resources.materials[material_index]->Apply(resources.textures);

	if (font_index < resources.fonts.size())
		resources.fonts[font_index]->Apply();

	glPushMatrix();

	// origin
	glTranslatef(-GetWidth() / 2 * GetOriginX(), -GetHeight() / 2 * GetOriginY(), 0.f);

	glColor4ub(colors[0].r, colors[0].g, colors[0].b, MultiplyColors(colors[0].a, render_alpha));

	for (wchar_t c : text)
	{
		glBegin(GL_QUADS);
		glTexCoord2f(0.f, 0.f);
		glVertex2f(0.f, 0.f);

		glTexCoord2f(1.f / 0x1b, 0.f);
		glVertex2f(16.f, 0.f);

		glTexCoord2f(1.f / 0x1b, 1.f / 0x80);
		glVertex2f(16.f, height);

		glTexCoord2f(0.f, 1.f / 0x80);
		glVertex2f(0.f, height);
		glEnd();

		glTranslatef(16.f + space_char, 0.f, 0.f);
	}

	glPopMatrix();
}

}
