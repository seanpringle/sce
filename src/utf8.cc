#include "utf8.h"
#include "common.h"

#define UTF8_2 0b11000000
#define UTF8_2_MASK 0b11100000
#define UTF8_2_PAYLOAD 0b00011111
#define UTF8_3 0b11100000
#define UTF8_3_MASK 0b11110000
#define UTF8_3_PAYLOAD 0b00001111
#define UTF8_4 0b11110000
#define UTF8_4_MASK 0b11111000
#define UTF8_4_PAYLOAD 0b00000111
#define UTF8_MARKER 0b10000000
#define UTF8_PAYLOAD 0b00111111

bool UTF8::ok() const {
	return encodeErrors.size() == 0 && decodeErrors.size() == 0;
}

UTF8::UTF8(const std::string& in) {
	text = in;
	size_t cursor = 0;

	auto more = [&]() {
		return cursor < in.size();
	};

	auto next = [&]() {
		return (uint32_t)in[cursor++];
	};

	int errors = 0;

	while (more()) {
		auto ch = next();

		if (ch < 0x7F) {
			codes.push_back(ch);
			continue;
		}

		if ((ch & UTF8_4_MASK) == UTF8_4) {
			uint32_t a, b, c, d;
			a = (ch & UTF8_4_PAYLOAD);
			b = (next() & UTF8_PAYLOAD);
			c = (next() & UTF8_PAYLOAD);
			d = (next() & UTF8_PAYLOAD);
			uint32_t code = (a<<18) | (b<<12) | (c<<6) | d;
			codes.push_back(code);
			continue;
		}

		if ((ch & UTF8_3_MASK) == UTF8_3) {
			uint32_t a, b, c;
			a = (ch & UTF8_3_PAYLOAD);
			b = (next() & UTF8_PAYLOAD);
			c = (next() & UTF8_PAYLOAD);
			uint32_t code = (a<<12) | (b<<6) | c;
			codes.push_back(code);
			continue;
		}

		if ((ch & UTF8_2_MASK) == UTF8_2) {
			uint32_t a, b;
			a = (ch & UTF8_2_PAYLOAD);
			b = (next() & UTF8_PAYLOAD);
			uint32_t code = (a<<6) | b;
			codes.push_back(code);
			continue;
		}

		decodeErrors.push_back(cursor-1);
		codes.push_back('?');
	}

	if (decodeErrors.size())
		notef("invalid UTF8 decode");
}

UTF8::UTF8(const std::vector<uint32_t>& in) {
	codes = in;

	for (size_t cursor = 0; cursor < in.size(); cursor++) {
		auto co = in[cursor];

		if (co <= 0x7F) {
			text.push_back((char)co);
			continue;
		}

		if (co <= 0x7FF) {
			uint32_t a, b;
			b = co & UTF8_PAYLOAD;
			a = (co>>6) & UTF8_2_PAYLOAD;
			text.push_back(a | UTF8_2);
			text.push_back(b | UTF8_MARKER);
			continue;
		}

		if (co <= 0xFFFF) {
			uint32_t a, b, c;
			c = co & UTF8_PAYLOAD;
			b = (co>>6) & UTF8_PAYLOAD;
			a = (co>>12) & UTF8_3_PAYLOAD;
			text.push_back(a | UTF8_3);
			text.push_back(b | UTF8_MARKER);
			text.push_back(c | UTF8_MARKER);
			continue;
		}

		if (co <= 0x10FFFF) {
			uint32_t a, b, c, d;
			d = co & UTF8_PAYLOAD;
			c = (co>>6) & UTF8_PAYLOAD;
			b = (co>>12) & UTF8_PAYLOAD;
			a = (co>>18) & UTF8_4_PAYLOAD;
			text.push_back(a | UTF8_4);
			text.push_back(b | UTF8_MARKER);
			text.push_back(c | UTF8_MARKER);
			text.push_back(d | UTF8_MARKER);
			continue;
		}

		encodeErrors.push_back(cursor);
		text.push_back('?');
	}

	if (encodeErrors.size())
		notef("invalid UTF8 encode");
}
