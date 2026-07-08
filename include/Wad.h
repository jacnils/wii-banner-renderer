#pragma once

#include <fstream>
#include <string>

void extract_wad(std::ifstream& in, const std::string& out_dir = "");