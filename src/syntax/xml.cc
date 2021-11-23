#include "xml.h"

std::vector<ViewRegion> XML::tags(const Doc& text) {
	return {};
}

std::vector<std::string> XML::matches(const Doc& text, int cursor) {
	return {};
}

Syntax::Token XML::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token XML::next(const Doc& text, int cursor, Syntax::Token token) {
	switch (token) {
		case Token::None: {

			if (get(text, cursor) == '<' && get(text, cursor+1) == '!') {
				return Token::Namespace;
			}

			if (get(text, cursor) == '<') {
				return Token::Type;
			}

			break;
		}

		case Token::Namespace:
		case Token::Type: {
			if (get(text, cursor-1) == '>')
				return next(text, cursor, Token::None);
			break;
		}
	}
	return token;
}

bool XML::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '<', '>')) return true;
	}
	return false;
}
