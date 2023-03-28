#pragma once

#include <string>
#include <vector>

struct UTF8 {
	std::vector<size_t> decodeErrors;
	std::vector<size_t> encodeErrors;
	std::vector<uint32_t> codes;
	std::string text;
	UTF8(const std::string& in);
	UTF8(const std::vector<uint32_t>& in);
	bool ok() const;
};
