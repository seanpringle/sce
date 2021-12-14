#include "ini.h"

std::vector<ViewRegion> INI::tags(const Doc& text) {
	std::vector<ViewRegion> tags;
	for (uint i = 0; i < text.size(); i++) {
		auto c = get(text, i);
		if (c == '[' && (!i || get(text, i-1) == '\n')) {
			uint a = ++i;
			while (i < text.size() && get(text, i) != ']') i++;
			tags.push_back({(int)a, (int)(i-a)});
		}
	}
	return tags;
}

std::vector<std::string> INI::matches(const Doc& text, int cursor) {
	return {};
}

Syntax::Token INI::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token INI::next(const Doc& text, int cursor, Syntax::Token token) {
	switch (token) {
		case Token::None: {

			if (cursor > 0 && get(text, cursor-1) == '[' && (cursor == 1 || get(text, cursor-2) == '\n')) {
				return Token::Namespace;
			}

			if (iswalpha(get(text, cursor)) && (!cursor || get(text, cursor-1) == '\n')) {
				return Token::Variable;
			}

			if (get(text, cursor) == ';') {
				return Token::Comment;
			}

			break;
		}

		case Token::Namespace: {
			if (get(text, cursor) == ']')
				return next(text, cursor, Token::None);
			break;
		}

		case Token::Variable: {
			if (isspace(get(text, cursor)))
				return next(text, cursor, Token::None);
			break;
		}

		case Token::Comment: {
			if (get(text, cursor) == '\n')
				return next(text, cursor, Token::None);
			break;
		}
	}
	return token;
}

bool INI::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '[', ']')) return true;
	}
	return false;
}
