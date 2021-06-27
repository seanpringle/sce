#include "syntax.h"
#include <string_view>

std::vector<ViewRegion> PlainText::tags(const std::deque<char>& text) {
	return {};
}

std::vector<std::string> PlainText::matches(const std::deque<char>& text, int cursor) {
	return {};
}

Syntax::Token PlainText::next(const std::deque<char>& text, int cursor, CPP::Token token) {
	return Token::None;
}

std::vector<ViewRegion> CPP::tags(const std::deque<char>& text) {
	std::vector<ViewRegion> hits;

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

std::vector<std::string> CPP::matches(const std::deque<char>& text, int cursor) {
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

bool CPP::isname(int c) {
	return isalnum(c) || c == '_';
}

bool CPP::isnamestart(int c) {
	return isalpha(c) || c == '_';
}

bool CPP::isboundary(int c) {
	return c != '_' && (iscntrl(c) || ispunct(c) || isspace(c));
}

bool CPP::isoperator(int c) {
	return strchr("+-*/%=<>&|^!?:", c);
}

int CPP::get(const std::deque<char>& text, int cursor) {
	return cursor >= 0 && cursor < (int)text.size() ? text[cursor]: 0;
}

bool CPP::word(const std::deque<char>& text, int cursor, const std::string& name) {
	int l = name.size();
	for (int i = 0; i < l; i++) {
		if (get(text, cursor+i) != name[i]) return false;
	}
	return !isname(get(text, cursor+l));
}

bool CPP::keyword(const std::deque<char>& text, int cursor) {
	char pad[32]; int len = 0;
	for (int i = 0; i < 31 && isname(get(text, cursor+i)); i++) {
		pad[len++] = get(text, cursor+i);
	}
	pad[len] = 0;
	return keywords.find(std::string_view(pad)) != keywords.end();
}

// is cursor inside a line // comment
bool CPP::comment(const std::deque<char>& text, int cursor) {
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
bool CPP::typelike(const std::deque<char>& text, int cursor) {
	auto prev = [&]() {
		return get(text, cursor-1);
	};

	auto white = [&]() {
		return isspace(prev());
	};

	auto name = [&]() {
		return prev() == ':' || isname(prev());
	};

	auto refptr = [&]() {
		return prev() == '&' || prev() == '*';
	};

	auto skip = [&](auto fn) {
		while (prev() && fn()) --cursor;
	};

	int start = cursor;

	skip(white);

	if (refptr()) --cursor;
	skip(white);

	// templated type. should probably be recursive
	if (prev() == '>') {
		--cursor;
		while (name() || white() || refptr()) cursor--;
		if (prev() != '<') return false;
		--cursor;
	}

	skip(white);
	skip(name);

	if (keyword(text, cursor)) return false;

	int match = cursor;

	skip(white);

	if (isoperator(prev())) return false;

	return match < start;
}

// is cursor on a struct or class definition name
bool CPP::matchBlockType(const std::deque<char>& text, int cursor) {
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
bool CPP::matchFunction(const std::deque<char>& text, int cursor) {
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
		while (c(-1) && (isname(c(-1)) || c(-1) == ':')) --cursor;
		// type name {
		while (c(-1) && isspace(c(-1))) --cursor;
		function = start != cursor && typelike(text, cursor);
	}

	cursor = start;
	// type name
	if (!function) {
		while (c(-1) && isspace(c(-1))) --cursor;
		function = start != cursor && typelike(text, cursor) && !comment(text, cursor-1);
	}

	if (!constructor && !function) return false;

	cursor = start+length;
	while (c() && isspace(c())) cursor++;

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
	while (c() && isspace(c())) cursor++;

	if (word(text, cursor, "const")) {
		cursor += 5;
		while (c() && isspace(c())) cursor++;
	}

	if (!(c() == '{' || c() == ':' || c() == ';')) return false;
	return true;
}

Syntax::Token CPP::next(const std::deque<char>& text, int cursor, CPP::Token token) {

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
