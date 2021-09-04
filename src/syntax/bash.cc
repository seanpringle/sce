#include "bash.h"

bool Bash::keyword(const Doc& text, int cursor) {
	return wordset(text, cursor, keywords);
}

std::vector<ViewRegion> Bash::tags(const Doc& text) {
	return {};
}

std::vector<std::string> Bash::matches(const Doc& text, int cursor) {
	return {};
}

// is cursor inside a line # comment
bool Bash::comment(const Doc& text, int cursor) {
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};
	while (c()) {
		if (c() == '#') return true;
		if (c(-1) == '\n') return false;
		cursor--;
	}
	return false;
}

Syntax::Token Bash::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token Bash::next(const Doc& text, int cursor, Syntax::Token token) {
	switch (token) {

		case Token::None: {
			if (keyword(text, cursor)) return Token::Keyword;

			if (get(text, cursor) == '#') {
				return Token::Comment;
			}

			if (get(text, cursor) == '"') {
				return Token::StringStart;
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

		case Token::StringStart: {
			if (get(text, cursor) == '"' && (get(text, cursor-1) != '\\' || get(text, cursor-2) == '\\')) return Token::StringFinish;
			break;
		}

		case Token::StringFinish: {
			return next(text, cursor, Token::None);
			break;
		}

		default:
			break;
	}
	return token;
}

bool Bash::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '(', ')')) return true;
		if (hintMatchedPair(text, cursor, selection, '{', '}')) return true;
		if (hintMatchedPair(text, cursor, selection, '[', ']')) return true;
	}
	return false;
}
