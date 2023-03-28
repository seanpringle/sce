#pragma once

#include <string>
#include <vector>

struct UTF8 {
	std::vector<size_t> decodeErrors;
	std::vector<size_t> encodeErrors;
	std::vector<uint32_t> codes;
	std::string text;
	bool ok() const;

	UTF8(const std::string& in);
	UTF8(const std::vector<uint32_t>& in);

	operator std::string() const {
		return text;
	}
};
