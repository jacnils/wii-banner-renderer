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

#pragma once

#include "Animator.h"
#include "Texture.h"

namespace WiiBanner
{

class Material : public Animator
{
public:
	typedef Animator Base;

	enum
	{
		NAME_LENGTH = 20
	};

	void Load(std::istream& file);

	void Apply(const TextureList& textures) const;

protected:
	void ProcessHermiteKey(const KeyType& type, float value);
	void ProcessStepKey(const KeyType& type, StepKeyHandler::KeyData data);

private:
	struct TextureMap
	{
		u16 tex_index;
		u8 wrap_s, wrap_t;
	};
	std::vector<TextureMap> texture_maps;

	struct TextureCoordGen
	{
		u8 tgen_type, tgen_src, mtrx_src;
	};
	std::vector<TextureCoordGen> texture_coord_gens;

	struct TextureSrt
	{
		TextureSrt()
		{
			translate.x = translate.y = rotate = 0.f;
			scale.x = scale.y = 1.f;
		}

		Vec2f translate, scale;
		float rotate;
	};
	std::vector<TextureSrt> texture_srts;

	struct
	{
		u8 type, src_factor, dst_factor, logical_op;

	} blend_mode;

	struct
	{
		u8 function, op, ref0, ref1;

	} alpha_compare;

	union
	{
		u8 value;

		struct
		{
			u8 r : 2;
			u8 g : 2;
			u8 b : 2;
			u8 a : 2;
		};

	} tev_swap_table[4];

	union TevStage
	{
		char data[0x10];

		struct
		{
			u8 tex_coord;
			u8 color;

			u16 tex_map : 9;
			u16 ras_sel : 2;
			u16 tex_sel : 2;
			u16 empty1 : 3;

			struct
			{
				u8 a : 4;
				u8 b : 4;

				u8 c : 4;
				u8 d : 4;

				u8 op : 4;
				u8 bias: 2;
				u8 scale : 2;

				u8 clamp : 1;
				u8 reg_id : 2;
				u8 constant_sel : 5;

			} color_in, alpha_in;

			struct
			{
				u8 tex_id : 2;
				u8 empty1 : 6;

				u8 bias : 3;
				u8 mtx : 4;
				u8 empty2 : 1;

				u8 wrap_s : 3;
				u8 wrap_t : 3;
				u8 empty3 : 2;

				u8 format : 2;
				u8 add_prev : 1;
				u8 utc_lod : 1;
				u8 alpha : 2;
				u8 empty4 : 2;

			} ind;
		};
	};
	std::vector<TevStage> tev_stages;

	GXColor color;	// TODO: where is "color" used?

	GXColorS10 color_regs[3];
	GXColor color_constants[4];
};

class MaterialList : public std::vector<Material*>
{
public:
	static const u32 BINARY_MAGIC = MAKE_FOURCC('m', 'a', 't', '1');
};

}
