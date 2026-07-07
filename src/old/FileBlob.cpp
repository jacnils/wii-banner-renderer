// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#include "Blob.h"
#include "FileBlob.h"
//#include "FileUtil.h"

// moved here from FileUtil
namespace File
{
// Returns the size of filename (64bit)
u64 GetSize(const char *filename)
{
	std::ifstream file(filename);
	file.seekg(0, std::ios::end);
	return file.tellg();
}

}

namespace DiscIO
{

PlainFileReader::PlainFileReader(const char* filename)
{
	file.open(filename, std::ios::in | std::ios::binary);
	size = File::GetSize(filename);
}

PlainFileReader* PlainFileReader::Create(const char* filename)
{
	return new PlainFileReader(filename);
}

PlainFileReader::~PlainFileReader()
{
	file.close();
}

bool PlainFileReader::Read(u64 offset, u64 nbytes, u8* out_ptr)
{
	file.seekg(offset, std::ios::beg);
	auto const pos = file.tellg();

	file.read((char*)out_ptr, nbytes);
	return file.tellg() - pos;
}

}  // namespace
