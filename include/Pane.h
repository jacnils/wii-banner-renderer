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

#ifndef WII_BNR_PANE_H_
#define WII_BNR_PANE_H_

#include <vector>

#include "Animator.h"
#include "Funcs.h"
#include "WrapGx.h"

namespace WiiBanner
{

struct Resources;

class Pane;
typedef std::vector<Pane*> PaneList;

class Pane : public Animator
{
public:
	typedef Animator Base;

	static const u32 BINARY_MAGIC = MAKE_FOURCC('p', 'a', 'n', '1');

	enum
	{
		NAME_LENGTH = 0x10,
		USER_DATA_LENGTH = 0x8
	};

	void Load(std::istream& file);
	virtual ~Pane();

	void Render(const Resources& resources, u8 parent_alpha, Vec2f adjust) const;
	void SetFrame(FrameNumber frame, u8 key_set);

	bool GetVisible() const { return GetBit(flags, FLAG_VISIBLE); }
	void SetVisible(bool visible) { SetBit(flags, FLAG_VISIBLE, visible); }

	// TODO: these ! are required?
	// TODO: better name?
	bool GetInfluencedAlpha() const { return !GetBit(flags, FLAG_INFLUENCED_ALPHA); }
	void SetInfluencedAlpha(bool influenced) { SetBit(flags, FLAG_INFLUENCED_ALPHA, !influenced); }

	// TODO: better name?
	bool GetPositionAdjust() const { return !GetBit(flags, FLAG_POSITION_ADJUST); }
	void SetPositionAdjust(bool adjust) { SetBit(flags, FLAG_POSITION_ADJUST, !adjust); }

	bool GetHide() const { return hide; }
	void SetHide(bool _hide) { hide = _hide; }

	u8 GetOriginX() const { return origin % 3; }
	u8 GetOriginY() const { return 2 - origin / 3; }

	float GetWidth() const { return width; }
	float GetHeight() const { return height; }

	u8 GetAlpha() const { return alpha; }
	void SetAlpha(u8 _alpha) { alpha = _alpha; }

	const Vec3f& GetTranslate() const { return translate; }
	void SetTranslate(const Vec3f& _translate) { translate = _translate; }

	const Vec3f& GetRotate() const { return rotate; }
	void SetRotate(const Vec3f& _rotate) { rotate = _rotate; }

	const Vec2f& GetScale() const { return scale; }
	void SetScale(const Vec2f& _scale) { scale = _scale; }

	Pane* FindPane(const std::string& name);	// recursive

	PaneList panes;

protected:
	void ProcessHermiteKey(const KeyType& type, float value);
	void ProcessStepKey(const KeyType& type, StepKeyHandler::KeyData data);

private:
	virtual void Draw(const Resources&, u8, Vec2f) const {};

	enum : u8
	{
		FLAG_VISIBLE,
		FLAG_INFLUENCED_ALPHA,
		FLAG_POSITION_ADJUST
	};

	u8 origin;
	u8 flags;
	u8 alpha;
	bool hide;	// used by the groups

	Vec3f translate, rotate;
	Vec2f scale;
	float width, height;
};

// apparently Bounding is just a regular pane
class Bounding : public Pane
{
public:
	static const u32 BINARY_MAGIC = MAKE_FOURCC('b', 'n', 'd', '1');
};

// used by Picture and Window
// TODO: put elsewhere I think
class Quad : public Pane
{
public:
	typedef Pane Base;

	void Load(std::istream& file);

protected:
	Quad() {}

	void ProcessHermiteKey(const KeyType& type, float value);

	void Draw(const Resources& resources, u8 render_alpha, Vec2f adjust) const;

private:
	struct TexCoords
	{
		struct TexCoord
		{
			float s, t;

		} coords[4];
	};
	std::vector<TexCoords> tex_coords;

	u16 material_index;

	GXColor vertex_colors[4];
};

}

#endif
