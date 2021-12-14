#include "yaml.h"

std::vector<ViewRegion> YAML::tags(const Doc& text) {
	return {};
}

std::vector<std::string> YAML::matches(const Doc& text, int cursor) {
	return {};
}

Syntax::Token YAML::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token YAML::next(const Doc& text, int cursor, Syntax::Token token) {

	auto isfield = [&](int c) {
		return isname(c) || c == '-';
	};

	auto isField = [&](auto text, auto cursor) {
		if (!isfield(get(text, cursor))) return false;
		while (isfield(get(text, cursor))) cursor++;
		return get(text, cursor) == ':';
	};

	switch (token) {
		case Token::None: {

			if (isField(text, cursor)) {
				return Token::Namespace;
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

		case Token::Comment: {
			if (get(text, cursor) == '\n')
				return next(text, cursor, Token::None);
			break;
		}
	}
	return token;
}

bool YAML::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	return false;
}

