#include "forth.h"

bool Forth::isname(int c) {
	return !std::isspace(c);
}

bool Forth::isboundary(int c) {
	return std::isspace(c);
}

bool Forth::isoperator(int c) {
	return strchr("+-*%/=<>@!&|^~", c);
}

std::vector<ViewRegion> Forth::tags(const Doc& text) {
	return {};
}

std::vector<std::string> Forth::matches(const Doc& text, int cursor) {
	return {};
}

Syntax::Token Forth::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token Forth::next(const Doc& text, int cursor, Syntax::Token token) {
	switch (token) {
		case Token::None: {
			bool gapBefore = !cursor || isboundary(get(text, cursor-1));

			if (std::isdigit(get(text, cursor)) && gapBefore) {
				return Token::Integer;
			}

			if (get(text, cursor) == '(' && gapBefore) {
				return Token::Comment;
			}

			if (isname(get(text, cursor)) && gapBefore && get(text, cursor-2) == ':') {
				return Token::Function;
			}

			if (get(text, cursor) == '"') {
				return Token::StringStart;
			}

			if (isoperator(get(text, cursor))) {
				return Token::Operator;
			}

			if (wordset(text, cursor, keywords) && gapBefore) {
				return Token::Keyword;
			}

			if (wordset(text, cursor, corewords) && gapBefore) {
				return Token::Call;
			}

			break;
		}

		case Token::Comment: {
			if (get(text, cursor-1) == ')') {
				return next(text, cursor, Token::None);
			}
			break;
		}

		case Token::Integer: {
			if (!std::isdigit(get(text, cursor))) {
				return next(text, cursor, Token::None);
			}
			break;
		}

		case Token::Operator: {
			if (!isoperator(get(text, cursor))) {
				return next(text, cursor, Token::None);
			}
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

		case Token::Function:
		case Token::Call:
		case Token::Keyword: {
			if (isboundary(get(text, cursor))) {
				return next(text, cursor, Token::None);
			}
			break;
		}
	}
	return token;
}

bool Forth::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	return false;
}
