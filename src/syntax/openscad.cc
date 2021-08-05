#include "openscad.h"

bool OpenSCAD::keyword(const Doc& text, int cursor) {
	return wordset(text, cursor, keywords);
}

std::vector<ViewRegion> OpenSCAD::tags(const Doc& text) {
	return {};
}

std::vector<std::string> OpenSCAD::matches(const Doc& text, int cursor) {
	return {};
}

// is cursor inside a line // comment
bool OpenSCAD::comment(const Doc& text, int cursor) {
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};
	while (c()) {
		if (c() == '/') return true;
		if (c(-1) == '\n') return false;
		cursor--;
	}
	return false;
}

// is cursor on a module name
bool OpenSCAD::matchModule(const Doc& text, int cursor) {
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isname(c())) return false;

	int start = cursor;
	while (isname(c())) cursor++;
	int length = cursor-start;

	if (keyword(text, start)) return false;

	cursor = start;
	while (c(-1) && iswspace(c(-1))) --cursor;
	while (c(-1) && isname(c(-1))) --cursor;
	if (!wordset(text, cursor, {"module"})) return false;

	cursor = start+length;
	while (c() && iswspace(c())) cursor++;

	// arg list
	if (c() != '(') return false;
	cursor++;
	int levels = 1;
	while (c()) {
		if (c() == '(') levels++;
		if (c() == ')') levels--;
		if (!levels) break;
		cursor++;
	}
	if (c() != ')') return false;
	cursor++;

	while (c() && iswspace(c())) cursor++;
	return c() == '{';
}

Syntax::Token OpenSCAD::next(const Doc& text, int cursor, Syntax::Token token) {
	switch (token) {

		case Token::None: {
			if (keyword(text, cursor)) return Token::Keyword;

			if (get(text, cursor) == '/' && get(text, cursor+1) == '/') {
				return Token::Comment;
			}

			if (isdigit(get(text, cursor)) && isboundary(get(text, cursor-1))) {
				return Token::Integer;
			}

			if (matchModule(text, cursor)) {
				return Token::Function;
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

		case Token::Function: {
			if (!isname(get(text, cursor))) return next(text, cursor, Token::None);
			break;
		}

		default:
			break;
	}
	return token;
}

bool OpenSCAD::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '(', ')')) return true;
		if (hintMatchedPair(text, cursor, selection, '{', '}')) return true;
		if (hintMatchedPair(text, cursor, selection, '[', ']')) return true;
	}
	return false;
}
