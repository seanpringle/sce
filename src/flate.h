#pragma once

#include "catenate.h"
#include <string>
#include <vector>

struct deflation {
	size_t size = 0;
	std::vector<char> cdata;

	bool operator==(const deflation& other) const {
		return size == other.size && cdata == other.cdata;
	}
};

deflation deflate(const std::vector<char>& data, int quality = -1);
std::vector<char> inflate(const deflation& def);
