#include "cmake.h"

bool CMake::keyword(const Doc& text, int cursor) {
	return wordset(text, cursor, keywords);
}

std::vector<ViewRegion> CMake::tags(const Doc& text) {
	return {};
}

std::vector<std::string> CMake::matches(const Doc& text, int cursor) {
	return {};
}

Syntax::Token CMake::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token CMake::next(const Doc& text, int cursor, Syntax::Token token) {
	switch (token) {
		case Token::None: {
			if (keyword(text, cursor)) return Token::Keyword;

			if (get(text, cursor) == '#') {
				return Token::Comment;
			}

			if (isdigit(get(text, cursor)) && isboundary(get(text, cursor-1))) {
				return Token::Integer;
			}

			if (isname(get(text, cursor))) {
				int len = 0;
				while (isname(get(text, cursor+len))) len++;
				if (get(text, cursor+len) == '(') return Token::Call;
			}

			break;
		}

		case Token::Keyword: {
			if (!iswalpha(get(text, cursor))) return next(text, cursor, Token::None);
			break;
		}

		case Token::Comment: {
			if (get(text, cursor) == '\n') return next(text, cursor, Token::None);
			break;
		}

		case Token::Integer: {
			if (!(iswalnum(get(text, cursor)) || get(text, cursor) == '.')) return next(text, cursor, Token::None);
			break;
		}

		case Token::Call: {
			if (isboundary(get(text, cursor))) return next(text, cursor, Token::None);
			break;
		}

		default:
			break;
	}
	return token;
}

bool CMake::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '(', ')')) return true;
	}
	return false;
}

