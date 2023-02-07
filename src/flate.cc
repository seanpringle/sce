#include "common.h"
#include "flate.h"

#define SDEFL_IMPLEMENTATION
#include "sdefl.h"

#define SINFL_IMPLEMENTATION
#include "sinfl.h"

deflation deflate(const std::vector<char>& data, int quality) {
	if (!data.size()) return {};

	quality = std::max(0, std::min(9, quality));
	std::size_t bounds = sdefl_bound(data.size());

	std::vector<char> cdata;
	cdata.insert(cdata.begin(), bounds, 0);

	struct sdefl sdefl;
	std::memset(&sdefl, 0, sizeof(sdefl));

	std::size_t clen = sdeflate(&sdefl, (unsigned char*)cdata.data(), (unsigned char*)data.data(), data.size(), quality);

	return {data.size(), {cdata.begin(), cdata.begin() + clen}};
}

std::vector<char> inflate(const deflation& def) {
	if (!def.size) return {};

	std::vector<char> data;
	data.insert(data.begin(), def.size, 0);

	std::size_t dlen = sinflate((unsigned char*)data.data(), (unsigned char*)def.cdata.data(), def.cdata.size());
	if (dlen != def.size) throw std::runtime_error("inflation incorrect size");

	return data;
}
