#include "syntax.h"

std::vector<View::Region> Syntax::tags(const std::deque<char>& text) {
	std::vector<View::Region> hits;

	int cursor = 0;

	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	// namespace::symbol or symbol
	auto symbolStart = [&]() {
		if (c(-2) == ':' && c(-1) == ':') {
			cursor -= 2;
			while (isname(c(-1))) cursor--;
		}
	};

	auto extract = [&]() {
		symbolStart();
		int offset = cursor;
		while (isname(c()) || c() == ':') cursor++;
		hits.push_back({offset, cursor-offset});
	};

	while (cursor < (int)text.size()) {
		if (matchFunction(text, cursor) || matchBlockType(text, cursor)) extract(); else cursor++;
	}

	return hits;
}

bool Syntax::isname(int c) {
	return isalnum(c) || c == '_';
}

bool Syntax::isnamestart(int c) {
	return isalpha(c) || c == '_';
}

bool Syntax::isboundary(int c) {
	return c != '_' && (iscntrl(c) || ispunct(c) || isspace(c));
}

int Syntax::get(const std::deque<char>& text, int cursor) {
	return cursor >= 0 && cursor < (int)text.size() ? text[cursor]: 0;
}

bool Syntax::word(const std::deque<char>& text, int cursor, const std::string& name) {
	int l = name.size();
	for (int i = 0; i < l; i++) {
		if (get(text, cursor+i) != name[i]) return false;
	}
	return !isname(get(text, cursor+l));
}

bool Syntax::matchBlockType(const std::deque<char>& text, int cursor) {
	auto c = [&]() {
		return get(text, cursor);
	};

	auto white = [&]() {
		while (c() && isspace(c())) cursor++;
	};

	if (!isnamestart(c())) return false;

	cursor--;
	cursor--;

	// namespace::type {
	// keyword type {

	if (!(isalnum(c()) || c() == ':')) return false;
	cursor++;

	if (!(isspace(c()) || c() == ':')) return false;
	cursor++;

	while (c() && isname(c())) cursor++;
	white();
	if (c() != '{') return false;
	return true;
}

bool Syntax::matchFunction(const std::deque<char>& text, int cursor) {
	auto c = [&]() {
		return get(text, cursor);
	};

	auto white = [&]() {
		while (c() && isspace(c())) cursor++;
	};

	if (!isnamestart(c())) return false;

	cursor--;
	cursor--;

	// namespace::function(args) {
	// type function(args) {

	if (!(isalnum(c()) || c() == ':')) return false;
	cursor++;

	if (!(isspace(c()) || c() == ':')) return false;
	cursor++;

	while (c() && isname(c())) cursor++;
	white();
	if (c() != '(') return false;
	cursor++;
	while (c() && c() != ')') cursor++;
	if (c() != ')') return false;
	cursor++;
	white();
	if (c() != '{') return false;
	return true;
}

Syntax::Token Syntax::next(const std::deque<char>& text, int cursor, Syntax::Token token) {

	auto rget = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!rget()) return token;

	switch (token) {
		case Token::None: {
			if (rget() == '/' && rget(1) == '/') {
				return Token::Comment;
			}

			if (isdigit(rget()) && isboundary(rget(-1))) {
				int len = 0;
				while (isdigit(rget(len))) cursor++;
				if (rget(len) != '.') return Token::Integer;
			}

			if (rget() == '"') {
				return Token::StringStart;
			}

			if (rget() == '\'') {
				return Token::CharStringStart;
			}

			if (matchFunction(text, cursor) || matchBlockType(text, cursor)) {
				return Token::Function;
			}

			if (isnamestart(rget())) {
				int len = 0;
				while (rget(len) && isname(rget(len))) len++;
				if (rget(len) == '(') return Token::Call;
				if (rget(len) == ':') return Token::Namespace;
				if (rget(len) == '<') return Token::Call;
			}

			if (isboundary(rget(-1))) {
				for (auto& name: types) {
					if (word(text, cursor, name)) return Token::Type;
				}

				for (auto& name: keywords) {
					if (word(text, cursor, name)) return Token::Keyword;
				}

				for (auto& name: directives) {
					if (word(text, cursor, name)) return Token::Directive;
				}

				for (auto& name: constants) {
					if (word(text, cursor, name)) return Token::Constant;
				}
			}
			break;
		}

		case Token::Comment: {
			if (rget() == '\n') return next(text, cursor, Token::None);
			break;
		}

		case Token::Integer: {
			if (!isdigit(rget())) return next(text, cursor, Token::None);
			break;
		}

		case Token::StringStart: {
			if (rget() == '"' && (rget(-1) != '\\' || rget(-2) == '\\')) return Token::StringFinish;
			break;
		}

		case Token::CharStringStart: {
			if (rget() == '\'' && (rget(-1) != '\\' || rget(-2) == '\\')) return Token::CharStringFinish;
			break;
		}

		case Token::Directive: {
			if (!isalpha(get(text, cursor))) {
				return next(text, cursor, Token::None);
			}
			break;
		}

		case Token::StringFinish: {
			return next(text, cursor, Token::None);
			break;
		}

		case Token::CharStringFinish: {
			return next(text, cursor, Token::None);
			break;
		}

		case Token::Type: {
			if (isboundary(rget())) return next(text, cursor, Token::None);
			break;
		}

		case Token::Keyword: {
			if (!isalpha(rget())) return next(text, cursor, Token::None);
			break;
		}

		case Token::Constant: {
			if (isboundary(rget())) return next(text, cursor, Token::None);
			break;
		}

		case Token::Namespace: {
			if (isboundary(rget())) return next(text, cursor, Token::None);
			break;
		}

		case Token::Call: {
			if (isboundary(rget())) return next(text, cursor, Token::None);
			break;
		}

		case Token::Function: {
			if (!isname(rget())) return next(text, cursor, Token::None);
			break;
		}
	}

	return token;
}
