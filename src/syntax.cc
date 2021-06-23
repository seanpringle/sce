#include "syntax.h"
#include <string_view>

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

std::vector<std::string> Syntax::matches(const std::deque<char>& text, int cursor) {
	std::set<std::string> hits;
	std::vector<std::string> results;

	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	int home = cursor;

	while (isname(c(-1))) --cursor;

	int pstart = cursor;

	while (isname(c())) ++cursor;

	int plength = cursor-pstart;

	if (pstart < home) {
		cursor = 0;
		while (cursor < (int)text.size()) {
			int mstart = cursor;
			for (int i = 0; i < plength; i++, cursor++) {
				if (c() != get(text, pstart+i)) break;
			}
			int mlength = cursor-mstart;
			if (mlength == plength) {
				while (isname(c())) cursor++;
				std::string match;
				for (int i = mstart; i < cursor; i++) {
					match += get(text, i);
				}
				hits.insert(match);
				continue;
			}
			cursor++;
		}

		std::string prefix;
		for (int i = pstart; i < home; i++) {
			prefix += get(text, i);
		}

		results.push_back(prefix);

		for (auto& hit: hits) {
			results.push_back(hit);
		}
	}
	return results;
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

bool Syntax::isoperator(int c) {
	return strchr("+-*/%=<>&|^!?:", c);
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

bool Syntax::keyword(const std::deque<char>& text, int cursor) {
	char pad[32]; int len = 0;
	for (int i = 0; i < 31 && isname(get(text, cursor+i)); i++) {
		pad[len++] = get(text, cursor+i);
	}
	pad[len] = 0;
	return keywords.find(std::string_view(pad)) != keywords.end();
}

// is cursor inside a line // comment
bool Syntax::comment(const std::deque<char>& text, int cursor) {
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

// is cursor in a [namespace::]type[<namespace::type>[&*]]
bool Syntax::type(const std::deque<char>& text, int cursor) {
	auto prev = [&]() {
		return get(text, cursor-1);
	};

	auto white = [&]() {
		return isspace(prev());
	};

	auto name = [&]() {
		return prev() == ':' || isname(prev());
	};

	auto skip = [&](auto fn) {
		while (prev() && fn()) --cursor;
	};

	skip(white);
	int start = cursor;

	if (prev() == '&' || prev() == '*')
		--cursor;

	if (prev() == '>') {
		--cursor;
		while (prev() != '<' && (name() || white())) cursor--;
		if (prev() != '>') return false;
		--cursor;
	}

	skip(white);
	skip(name);

	if (keyword(text, cursor)) return false;

	return start != cursor && isname(get(text, cursor));
}

// is cursor on a struct or class definition name
bool Syntax::matchBlockType(const std::deque<char>& text, int cursor) {
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isnamestart(c())) return false;
	int start = cursor;

	if (keyword(text, cursor)) return false;

	// blocktype namespace::...::name {
	if (c(-1) == ':') {
		while (c(-1) && (isname(c(-1)) || c(-1) == ':')) --cursor;
	}

	// blocktype name {
	while (c(-1) && isspace(c(-1))) --cursor;
	while (c(-1) && isalnum(c(-1))) --cursor;

	if (!isalpha(c())) return false;
	bool blocktype = false;
	for (auto& name: blocktypes) {
		if (word(text, cursor, name)) {
			blocktype = true;
			break;
		}
	}

	if (comment(text, cursor)) return false;
	if (!blocktype) return false;

	cursor = start;

	while (c() && isname(c())) cursor++;
	while (c() && isspace(c())) cursor++;
	if (!(c() == '{' || c() == ':' || c() == ';')) return false;

	return true;
}

// is cursor on a function or method name
bool Syntax::matchFunction(const std::deque<char>& text, int cursor) {
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isnamestart(c())) return false;
	int start = cursor;
	while (isname(c())) cursor++;
	int length = cursor-start;

	if (keyword(text, start)) return false;

	bool constructor = false;
	bool function = false;

	cursor = start;
	// name::name constructor
	if (c(-1) == ':' && c(-2) == ':') {
		cursor = start-2;
		while (c(-1) && isname(c(-1))) --cursor;
		int cstart = cursor;
		int clength = start-cursor-2;
		constructor = clength == length;
		for (int i = 0; constructor && i < clength; i++) {
			constructor = get(text, cstart+i) == get(text, start+i);
		}
	}

	cursor = start;
	// type namespace::...::name
	if (c(-1) == ':' && c(-2) == ':') {
		cursor = start;
		while (c(-1) && (isname(c(-1)) || c(-1) == ':')) --cursor;
		// type name {
		while (c(-1) && isspace(c(-1))) --cursor;
		if (c(-1) == '&' || c(-1) == '*') --cursor;
		function = start != cursor && type(text, cursor);
	}

	cursor = start;
	// type name
	if (!function) {
		while (c(-1) && isspace(c(-1))) --cursor;
		function = type(text, cursor) && !comment(text, cursor-1);
	}

	if (!constructor && !function) return false;

	cursor = start+length;
	while (c() && isspace(c())) cursor++;

	// arg list
	if (c() != '(') return false;
	cursor++;
	while (c() && c() != ')') cursor++;
	if (c() != ')') return false;
	cursor++;
	while (c() && isspace(c())) cursor++;

	if (!(c() == '{' || c() == ':' || c() == ';')) return false;
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

			if (rget() == '/' && rget(1) == '*') {
				return Token::CommentBlock;
			}

			if (isdigit(rget()) && isboundary(rget(-1))) {
				return Token::Integer;
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

			if (isboundary(rget(-1))) {
				for (auto& name: types) {
					if (word(text, cursor, name)) return Token::Type;
				}

				if (keyword(text, cursor)) return Token::Keyword;

				if (get(text, cursor) == '#') {
					for (auto& name: directives) {
						if (word(text, cursor, name)) return Token::Directive;
					}
				}

				for (auto& name: constants) {
					if (word(text, cursor, name)) return Token::Constant;
				}
			}

			if (isnamestart(rget())) {
				int len = 0;
				while (rget(len) && isname(rget(len))) len++;
				if (rget(len) == '(') return Token::Call;
				if (rget(len) == '<') return Token::Type;
				if (rget(len) == ':' && rget(len+1) == ':') return Token::Namespace;
			}

			if (isoperator(rget())) {
				return Token::Operator;
			}
			break;
		}

		case Token::Comment: {
			if (rget() == '\n') return next(text, cursor, Token::None);
			break;
		}

		case Token::CommentBlock: {
			if (rget(-2) == '*' && rget(-1) == '/') return next(text, cursor, Token::None);
			break;
		}

		case Token::Integer: {
			if (!(isalnum(rget()) || rget() == '.')) return next(text, cursor, Token::None);
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

		case Token::Operator: {
			if (!isoperator(rget())) return next(text, cursor, Token::None);
			break;
		}
	}

	return token;
}
