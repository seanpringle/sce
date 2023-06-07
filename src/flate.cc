#include "common.h"
#include "flate.h"
#include <vector>
#include <zlib.h>

using namespace std;

deflation deflate(const vector<char>& data, int quality) {
	if (!data.size()) return {};

	uLongf clen = compressBound(data.size());

	vector<char> cdata(clen,0);

	quality = max(-1,min(9,quality));
	ensure(0 == compress2((Bytef*)cdata.data(), &clen, (Bytef*)data.data(), data.size(), quality));

	cdata.resize(clen);
	cdata.shrink_to_fit();

	return {data.size(), move(cdata)};
}

vector<char> inflate(const deflation& def) {
	if (!def.size) return {};

	auto& cdata = def.cdata;

	uLongf dlen = compressBound(cdata.size());

	vector<char> data(dlen,0);

	ensure(0 == uncompress((Bytef*)data.data(), &dlen, (Bytef*)cdata.data(), cdata.size()));

	data.resize(dlen);
	data.shrink_to_fit();

	return data;
}
