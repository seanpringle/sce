#include "make.h"

bool Make::isTarget(const Doc& text, int cursor) {

	auto at = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (at(-1) == '\n') {
		while (at() && at() != '\n' && at() != ':') cursor++;
		if (at() == ':') return true;
	}

	return false;
}

std::vector<ViewRegion> Make::tags(const Doc& text) {
	std::vector<ViewRegion> tags;
	for (uint i = 0; i < text.size(); i++) {
		if (isTarget(text, i)) {
			int a = i;
			while (i < text.size() && get(text, i) != ':') i++;
			tags.push_back({(int)a, (int)(i-a)});
		}
	}
	return tags;
}

std::vector<std::string> Make::matches(const Doc& text, int cursor) {
	return {};
}

Syntax::Token Make::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token Make::next(const Doc& text, int cursor, Syntax::Token token) {

	auto isVariable = [&](auto& text, auto cursor) {

		auto at = [&](int offset = 0) {
			return get(text, cursor+offset);
		};

		if (at(-1) && !isspace(at(-1))) return false;
		if (!isname(at())) return false;

		while (isname(at())) cursor++;
		return at() == '=';
	};

	switch (token) {
		case Token::None: {

			if (isTarget(text, cursor)) {
				return Token::Namespace;
			}

			if (isVariable(text, cursor)) {
				return Token::Variable;
			}

			if (get(text, cursor) == '#') {
				return Token::Comment;
			}

			break;
		}

		case Token::Namespace: {
			if (get(text, cursor) == ':')
				return next(text, cursor, Token::None);
			break;
		}

		case Token::Variable: {
			if (get(text, cursor) == '=')
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

bool Make::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '[', ']')) return true;
	}
	return false;
}
