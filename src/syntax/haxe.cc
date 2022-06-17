#include "haxe.h"

std::vector<ViewRegion> Haxe::tags(const Doc& text) {
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

std::vector<std::string> Haxe::matches(const Doc& text, int cursor) {
	std::set<std::string> hits;
	std::vector<std::string> results;

	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	int home = cursor;

	while (isname(c(-1))) --cursor;

	int pstart = cursor;
	int plength = home-pstart;

	if (pstart < home) {
		cursor = 0;

		std::vector<int> prefix;
		for (int i = pstart; i < home; i++) {
			prefix.push_back(get(text, i));
		}

		while (cursor < (int)text.size()) {
			int mstart = cursor;
			for (int i = 0; i < plength; i++, cursor++) {
				if (c() != prefix[i]) break;
			}
			int mlength = cursor-mstart;
			if (mlength == plength) {
				while (isname(c())) cursor++;
				std::string match;
				for (int i = mstart; i < cursor; i++) {
					match += get(text, i);
				}
				if ((int)match.size() > mlength) {
					hits.insert(match);
				}
				continue;
			}
			cursor++;
		}

		results.push_back({prefix.begin(), prefix.end()});

		for (auto& hit: hits) {
			results.push_back(hit);
		}
	}
	return results;
}

bool Haxe::isoperator(int c) {
	return strchr("+-*%/=<>&|^!?:", c);
}

bool Haxe::word(const Doc& text, int cursor, const std::string& name) {
	int l = name.size();
	for (int i = 0; i < l; i++) {
		if (get(text, cursor+i) != name[i]) return false;
	}
	return !isname(get(text, cursor+l));
}

bool Haxe::keyword(const Doc& text, int cursor) {
	return wordset(text, cursor, keywords);
}

// is cursor inside a line // comment
bool Haxe::comment(const Doc& text, int cursor) {
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

bool Haxe::type(const Doc& text, int cursor) {
	// [_A-Z]+
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	bool ucase = c() >= 'A' && c() <= 'Z';
	bool uscore = c() == '_';

	int start = cursor;

	while (isname(c())) cursor++;
	int length = cursor-start;

	return (ucase || uscore) && length > 0;
}

// is cursor on a struct or class definition name
bool Haxe::matchBlockType(const Doc& text, int cursor) {
	// blocktype name {

	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isname(c())) return false;
	int start = cursor;

	while (c(-1) && iswspace(c(-1))) --cursor;
	while (c(-1) && iswalnum(c(-1))) --cursor;

	if (!iswalpha(c())) return false;
	if (comment(text, cursor)) return false;
	if (!wordset(text, cursor, blocktypes)) return false;

	cursor = start;

	while (c() && isname(c())) cursor++;
	while (c() && iswspace(c())) cursor++;
	if (!(c() == '{')) return false;

	return !keyword(text, start);
}

// is cursor on a function or method name
bool Haxe::matchFunction(const Doc& text, int cursor) {
	// function name(

	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isname(c())) return false;
	int start = cursor;

	while (isname(c())) cursor++;
	int length = cursor-start;

	while (iswspace(c())) cursor++;
	bool paren = c() == '(';

	cursor = start;

	while (iswspace(c(-1))) cursor--;
	while (iswalpha(c(-1))) cursor--;

	bool function = word(text, cursor, "function");

	return function && paren && length > 0 && !keyword(text, start);
}

bool Haxe::matchType(const Doc& text, int cursor) {
	// : [_A-Z]+
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	int start = cursor;
	while (iswspace(c(-1))) cursor--;

	return type(text, start) && c(-1) == ':';
}

bool Haxe::matchAssign(const Doc& text, int cursor) {
	// name.field =
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isname(c())) return false;
	int start = cursor;

	while (isname(c())) cursor++;
	int length = cursor-start;

	while (iswspace(c())) cursor++;

	return length > 0 && c() == '=' && c(1) != '=';
}

Syntax::Token Haxe::first(const Doc& text, int cursor) {

	for (int i = cursor; i > 0 && i > cursor-1000; --i) {
		auto a = get(text, i-1);
		auto b = get(text, i);
		if (a == '*' && b == '/') break;
		if (a == '/' && b == '*') return Token::CommentBlock;
		if (a == '*' && b == '*' && get(text, i-2) == '\n') return Token::CommentBlock;
	}

	for (int i = cursor; i < ((int)text.size())-1 && i < cursor+1000; i++) {
		auto a = get(text, i);
		auto b = get(text, i+1);
		if (a == '/' && b == '*') break;
		if (a == '*' && b == '/') return Token::CommentBlock;
		if (a == '*' && b == '*' && get(text, i-1) == '\n') return Token::CommentBlock;
	}

	return Token::None;
}

Syntax::Token Haxe::next(const Doc& text, int cursor, Syntax::Token token) {

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

			if (matchType(text, cursor)) {
				return Token::Type;
			}

			if (matchAssign(text, cursor)) {
				return Token::Variable;
			}

			if (isboundary(rget(-1))) {
				if (keyword(text, cursor)) return Token::Keyword;

				if (get(text, cursor) == '@') return Token::Directive;

				for (auto& name: constants) {
					if (word(text, cursor, name)) return Token::Constant;
				}
			}

			if (isname(rget())) {
				int len = 0;
				while (rget(len) && isname(rget(len))) len++;
				if (len > 0) {
					if (rget(len) == '(') return Token::Call;
					if (rget(len) == '.') return Token::Namespace;
					while (rget(len) && isspace(rget(len))) len++;
					if (rget(len) == ':') return Token::Variable;
				}
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
			if (!(iswalnum(rget()) || rget() == '.')) return next(text, cursor, Token::None);
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
			if (!iswalpha(get(text, cursor))) {
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
			if (!iswalpha(rget())) return next(text, cursor, Token::None);
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

		case Token::Variable: {
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

bool Haxe::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '(', ')')) return true;
		if (hintMatchedPair(text, cursor, selection, '{', '}')) return true;
		if (hintMatchedPair(text, cursor, selection, '[', ']')) return true;
	}
	return false;
}
