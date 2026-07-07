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

#include "Pane.h"
#include "Layout.h"
#include "Endian.h"
#include "Funcs.h"

namespace WiiBanner
{

void Pane::Load(std::istream& file)
{
	hide = false;

	file >> BE >> flags >> origin >> alpha;
	file.seekg(1, std::ios::cur);

	SetName(ReadFixedLengthString<NAME_LENGTH>(file));

	ReadFixedLengthString<USER_DATA_LENGTH>(file);	// user data

	file >> BE
		>> translate.x >> translate.y >> translate.z
		>> rotate.x >> rotate.y >> rotate.z
		>> scale.x >> scale.y
		>> width >> height;
}

Pane::~Pane()
{
	// delete children
	for (Pane* pane : panes)
		delete pane;
}

void Pane::SetFrame(FrameNumber frame, u8 key_set)
{
	// setframe on self
	Animator::SetFrame(frame, key_set);

	// setframe on children
	for (Pane* pane : panes)
		pane->SetFrame(frame, key_set);
}

void Pane::Render(const Resources& resources, u8 parent_alpha, Vec2f adjust) const
{
	if (!GetVisible() || GetHide())
		return;

	// TODO: is this handled correctly?
	const u8 render_alpha = GetInfluencedAlpha() ? MultiplyColors(parent_alpha, GetAlpha()) : GetAlpha();

	glPushMatrix();

	// position
	glTranslatef(GetTranslate().x * adjust.x, GetTranslate().y * adjust.y, GetTranslate().z);

	if (!GetPositionAdjust())
	{
		adjust.x = 1.f;
		adjust.y = 1.f;
	}

	// rotate
	glRotatef(GetRotate().x, 1.f, 0.f, 0.f);
	glRotatef(GetRotate().y, 0.f, 1.f, 0.f);
	glRotatef(GetRotate().z, 0.f, 0.f, 1.f);

	// scale
	glScalef(GetScale().x, GetScale().y, 1.f);

	// render self
	Draw(resources, render_alpha, adjust);

	// render children
	for (Pane* pane : panes)
		pane->Render(resources, render_alpha, adjust);

	glPopMatrix();
}

Pane* Pane::FindPane(const std::string& find_name)
{
	if (find_name == GetName())
		return this;

	Pane* found = nullptr;

	for (Pane* pane : panes)
	{
		found = pane->FindPane(find_name);
		if (found)
			break;
	}

	return found;
}

void Pane::ProcessHermiteKey(const KeyType& type, float value)
{
	if (type.type == ANIMATION_TYPE_VERTEX_COLOR)	// vertex color
	{
		// only alpha is supported for Panes afaict
		if (0x10 == type.target)
		{
			alpha = (u8)value;
			return;
		}
	}
	else if (type.type == ANIMATION_TYPE_PANE)	// pane animation
	{
		if (type.target < 10)
		{
			float* const values[] =
			{
				&translate.x,
				&translate.y,
				&translate.z,

				&rotate.x,
				&rotate.y,
				&rotate.z,

				&scale.x,
				&scale.y,

				&width,
				&height,
			};

			*values[type.target] = value;
			return;
		}
	}

	Base::ProcessHermiteKey(type, value);
}

void Pane::ProcessStepKey(const KeyType& type, StepKeyHandler::KeyData data)
{
	if (type.type == ANIMATION_TYPE_VISIBILITY)	// visibility
	{
		SetVisible(!!data.data2);
		return;
	}

	Base::ProcessStepKey(type, data);
}

void Quad::Load(std::istream& file)
{
	ReadBEArray(file, &vertex_colors->r, sizeof(vertex_colors));

	u8 tex_coord_count;

	file >> BE >> material_index >> tex_coord_count;
	file.seekg(1, std::ios::cur);

	tex_coords.resize(tex_coord_count);
	if (tex_coord_count)
		ReadBEArray(file, &tex_coords[0].coords->s, sizeof(TexCoords) / sizeof(float) * tex_coord_count);
}

void Quad::Draw(const Resources& resources, u8 render_alpha, Vec2f adjust) const
{
	if (material_index < resources.materials.size())
		resources.materials[material_index]->Apply(resources.textures);

	// go lambda
	auto const quad_vertex = [=](unsigned int v, float x, float y)
	{
		// color
		glColor4ub(vertex_colors[v].r, vertex_colors[v].g, vertex_colors[v].b,
			MultiplyColors(vertex_colors[v].a, render_alpha));	// apply alpha

		// tex coords
		if (1 == tex_coords.size())
		{
			auto* const fv = &tex_coords.front().coords[v].s;
			for (GLenum t = GL_TEXTURE0; t != GL_TEXTURE8; ++t)
				glMultiTexCoord2fv(t, fv);
		}
		else
		{
			GLenum target = GL_TEXTURE0;
			for (auto& tc : tex_coords)
				glMultiTexCoord2fv(target++, &tc.coords[v].s);
		}

		// position
		glVertex2f(x, y);
	};

	glPushMatrix();

	// size
	glScalef(GetWidth() * adjust.x, GetHeight() * adjust.y, 1.f);

	// origin
	glTranslatef(-0.5f * GetOriginX(), -0.5f * GetOriginY(), 0.f);

	glBegin(GL_QUADS);
	quad_vertex(2, 0.f, 0.f);
	quad_vertex(3, 1.f, 0.f);
	quad_vertex(1, 1.f, 1.f);
	quad_vertex(0, 0.f, 1.f);
	glEnd();

	glPopMatrix();
}

void Quad::ProcessHermiteKey(const KeyType& type, float value)
{
	if (type.type == ANIMATION_TYPE_VERTEX_COLOR)	// vertex color
	{
		if (type.target < 0x10)
		{
			// vertex colors
			(&vertex_colors->r)[type.target] = (u8)value;
			return;
		}
	}

	Base::ProcessHermiteKey(type, value);
}

}
