#include "cpp.h"

std::vector<ViewRegion> CPP::tags(const Doc& text) {
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

std::vector<std::string> CPP::matches(const Doc& text, int cursor) {
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

bool CPP::isoperator(int c) {
	return strchr("+-*%/=<>&|^!?:", c);
}

bool CPP::word(const Doc& text, int cursor, const std::string& name) {
	int l = name.size();
	for (int i = 0; i < l; i++) {
		if (get(text, cursor+i) != name[i]) return false;
	}
	return !isname(get(text, cursor+l));
}

bool CPP::keyword(const Doc& text, int cursor) {
	return wordset(text, cursor, keywords);
}

bool CPP::specifier(const Doc& text, int cursor) {
	return wordset(text, cursor, specifiers);
}

// is cursor inside a line // comment
bool CPP::comment(const Doc& text, int cursor) {
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
bool CPP::typelike(const Doc& text, int cursor) {
	auto prev = [&]() {
		return get(text, cursor-1);
	};

	auto white = [&]() {
		return iswspace(prev());
	};

	auto name = [&]() {
		return prev() == ':' || isname(prev());
	};

	auto refptr = [&]() {
		return prev() == '&' || prev() == '*';
	};

	auto tpl = [&]() {
		return prev() == '<' || prev() == '>';
	};

	auto skip = [&](auto fn) {
		while (prev() && fn()) --cursor;
	};

	int start = cursor;

	skip(white);

	if (refptr()) --cursor;
	skip(white);

	if (prev() == '>') {
		int depth = 0;
		int length = 0;
		while (length < 150 && (tpl() || name() || white() || refptr())) {
			if (prev() == '>') depth++;
			if (prev() == '<') depth--;
			if (!depth) break;
			cursor--;
			length++;
		}
		if (prev() != '<') return false;
		--cursor;
	}

	skip(white);
	skip(name);

	if (keyword(text, cursor)) return false;

	int match = cursor;

	auto gap = [&]() {
		return prev() != '\n' && iswspace(prev());
	};

	skip(gap);

	auto paren = [&]() {
		return prev() == '(' || prev() == ')';
	};

	auto brace = [&]() {
		return prev() == '{' || prev() == '}';
	};

	if (isoperator(prev()) || paren() || brace()) return false;

	return match < start;
}

// is cursor on a struct or class definition name
bool CPP::matchBlockType(const Doc& text, int cursor) {
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isname(c())) return false;
	int start = cursor;

	// blocktype namespace::...::name {
	if (c(-1) == ':') {
		while (c(-1) && (isname(c(-1)) || c(-1) == ':')) --cursor;
	}

	// blocktype name {
	while (c(-1) && iswspace(c(-1))) --cursor;
	while (c(-1) && iswalnum(c(-1))) --cursor;

	if (!iswalpha(c())) return false;
	if (comment(text, cursor)) return false;
	if (!wordset(text, cursor, blocktypes)) return false;

	cursor = start;

	while (c() && isname(c())) cursor++;
	while (c() && iswspace(c())) cursor++;
	if (!(c() == '{' || c() == ':' || c() == ';')) return false;

	return !keyword(text, start);
}

// is cursor on a function or method name
bool CPP::matchFunction(const Doc& text, int cursor) {
	auto c = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	if (!isname(c())) return false;
	int start = cursor;

	while (isname(c())) cursor++;
	int length = cursor-start;

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
		while (c(-1) && iswspace(c(-1))) --cursor;
		function = start != cursor && typelike(text, cursor);
	}

	cursor = start;
	// type name
	if (!function) {
		while (c(-1) && iswspace(c(-1))) --cursor;
		function = start != cursor && typelike(text, cursor) && !comment(text, cursor-1);
	}

	if (!constructor && !function) return false;

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

	if (specifier(text, cursor)) {
		while (c() && isname(c())) cursor++;
		while (c() && iswspace(c())) cursor++;
	}

	bool isFunction = c() == '{';
	bool isDeclaration = (c() == ';' || c() == '=');
	bool isConstructor = c() == ':';

	return (isFunction || isDeclaration || isConstructor) && !keyword(text, start);
}

Syntax::Token CPP::first(const Doc& text, int cursor) {

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

Syntax::Token CPP::next(const Doc& text, int cursor, Syntax::Token token) {

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

			if (isname(rget())) {
				int len = 0;
				while (rget(len) && isname(rget(len))) len++;
				if (rget(len) == '(') return Token::Call;
				if (rget(len) == '<') return Token::Type;
				if (rget(len) == ':' && rget(len+1) == ':') return Token::Namespace;
				while (rget(len) && isspace(rget(len))) len++;
				if (rget(len) == '=' && rget(len+1) != '=') return Token::Variable;
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

bool CPP::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	for (auto& selection: selections) {
		if (hintMatchedPair(text, cursor, selection, '(', ')')) return true;
		if (hintMatchedPair(text, cursor, selection, '{', '}')) return true;
		if (hintMatchedPair(text, cursor, selection, '[', ']')) return true;
	}
	return false;
}
