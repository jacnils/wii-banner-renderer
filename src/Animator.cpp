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

#include <cmath>

#include "Animator.h"
#include "Endian.h"
#include "Funcs.h"

namespace WiiBanner
{

void Animator::LoadKeyFrames(std::istream& file, u8 tag_count, std::streamoff origin, u8 key_set)
{
	ReadOffsetList<u32>(file, tag_count, origin, [&]
	{
		const std::streamoff frame_origin = file.tellg();

		u32 animation_type;
		u8 entry_count;

		file >> BE >> animation_type >> entry_count;
		file.seekg(3, std::ios::cur);	// some padding

		ReadOffsetList<u32>(file, entry_count, frame_origin, [&]
		{
			u8 index;
			u8 target;
			u8 data_type;
			u8 pad;
			u16 key_count;
			u16 pad1;
			u32 offset;	// TODO: handle this

			file >> BE >> index >> target >> data_type >> pad
				>> key_count >> pad1 >> offset;

			const KeyType frame_type(static_cast<AnimationType>(animation_type), index, target);

			switch (data_type)
			{
				// step key frame
			case 0x01:
				keys[key_set].step_keys[frame_type].Load(file, key_count);
				break;

				// hermite key frame
			case 0x02:
				keys[key_set].hermite_keys[frame_type].Load(file, key_count);
				break;

			default:
				std::cout << "UNKNOWN FRAME DATA TYPE!!\n";
				break;
			}
		});
	});
}

void Animator::SetFrame(FrameNumber frame_number, u8 key_set)
{
	for (auto& frame_handler : keys[key_set].hermite_keys)
	{
		const auto& frame_type = frame_handler.first;
		const auto frame_value = frame_handler.second.GetFrame(frame_number);

		ProcessHermiteKey(frame_type, frame_value);
	}

	for (auto& frame_handler : keys[key_set].step_keys)
	{
		const auto& frame_type = frame_handler.first;
		auto const frame_data = frame_handler.second.GetFrame(frame_number);

		ProcessStepKey(frame_type, frame_data);
	}
}

void StepKeyHandler::Load(std::istream& file, u16 count)
{
	while (count--)
	{
		FrameNumber frame;
		file >> BE >> frame;

		auto& data = keys[frame];
		file >> BE >> data.data1 >> data.data2;

		file.seekg(2, std::ios::cur);

		//std::cout << "\t\t\t" "frame: " << frame << ' ' << pair.first << " " << pair.second << '\n';
	}
}

void HermiteKeyHandler::Load(std::istream& file, u16 count)
{
	while (count--)
	{
		std::pair<FrameNumber, KeyData> pair;

		// read the frame number, value and slope
		file >> BE >> pair.first
			>> pair.second.value >> pair.second.slope;

		keys.insert(pair);

		//std::cout << "\t\t\t" "frame: " << frame << ' ' << keys[frame] << '\n';
	}
}

StepKeyHandler::KeyData StepKeyHandler::GetFrame(FrameNumber frame_number) const
{
	// assuming not empty, a safe assumption currently

	// find the current frame, or the one after it
	auto frame_it = keys.lower_bound(frame_number);

	// current frame is higher than any keyframe, use the last keyframe
	if (keys.end() == frame_it)
		--frame_it;

	// if this is after the current frame and not the first keyframe, use the previous one
	if (frame_number < frame_it->first && keys.begin() != frame_it)
		--frame_it;

	return frame_it->second;
}

float HermiteKeyHandler::GetFrame(FrameNumber frame_number) const
{
	// assuming not empty, a safe assumption currently

	// find the current keyframe, or the one after it
	auto next = keys.lower_bound(frame_number);

	// current frame is higher than any keyframe, use the last keyframe
	if (keys.end() == next)
		--next;

	auto prev = next;

	// if this is after the current frame and not the first keyframe, use the previous one
	if (frame_number < prev->first && keys.begin() != prev)
		--prev;

	const float nf = next->first - prev->first;
	if (std::abs(nf) < 0.01)
	{
		// same frame numbers, just return the first's value
		return prev->second.value;
	}
	else
	{
		// different frames, blend them together
		// this is a "Cubic Hermite spline" apparently

		frame_number = Clamp(frame_number, prev->first, next->first);

		const float t = (frame_number - prev->first) / nf;

		// old curve-less code
		//return prev->second.value + (next->second.value - prev->second.value) * t;

		// curvy code from marcan, :p
		return
			prev->second.slope * nf * (t + powf(t, 3) - 2 * powf(t, 2)) +
			next->second.slope * nf * (powf(t, 3) - powf(t, 2)) +
			prev->second.value * (1 + (2 * powf(t, 3) - 3 * powf(t, 2))) +
			next->second.value * (-2 * powf(t, 3) + 3 * powf(t, 2));
	}
}

void Animator::ProcessHermiteKey(const KeyType& type, float value)
{
	std::cout << "unhandled key (" << GetName() << "): type: " << FourCC(type.type)
	<< " index: " << (int)type.index
	<< " target: " << (int)type.target
	<< " value: " << value
	<< '\n';
}

void Animator::ProcessStepKey(const KeyType& type, StepKeyHandler::KeyData data)
{
	std::cout << "unhandled key (" << GetName() << "): type: " << FourCC(type.type)
	<< " index: " << (int)type.index
	<< " target: " << (int)type.target
	<< " data:" << (int)data.data1 << " " << (int)data.data2
	<< '\n';
}

}
