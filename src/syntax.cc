#include "syntax.h"

Syntax::Syntax() {
	reFunction = std::regex("^([a-zA-Z]+[a-zA-Z0-9_:]*)\\s*\\([^\\)]*?\\)\\s*[{:]");
	reInclude = std::regex("^\\s+<.*?>");
	reBlockType = std::regex("^([a-zA-Z]+[a-zA-Z0-9_:]*)\\s*[{:]");
}

bool Syntax::isname(int c) {
	return isnamestart(c);
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

std::string Syntax::getline(const std::deque<char>& text, int cursor) {
	std::string line;
	for (int len = 0;
		get(text, cursor+len) && get(text, cursor+len) != '\n';
		line.push_back(get(text, cursor+len)), len++
	);
	return line;
}

bool Syntax::word(const std::deque<char>& text, int cursor, const std::string& name) {
	int l = name.size();
	for (int i = 0; i < l; i++) {
		if (get(text, cursor+i) != name[i]) return false;
	}
	return !isname(get(text, cursor+l));
}

Syntax::Token Syntax::next(const std::deque<char>& text, int cursor, Syntax::Token token) {

	auto isboundary = [&](int c) {
		return c != '_' && (iscntrl(c) || ispunct(c) || isspace(c));
	};

	auto rget = [&](int offset = 0) {
		return get(text, cursor+offset);
	};

	auto rgetline = [&](int offset = 0) {
		return getline(text, cursor+offset);
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

			if (isalpha(rget())) {
				std::string line = rgetline();

				{
					std::smatch m;
					bool matchedA = std::regex_match(line, m, reFunction);
					bool type = matchedA && types.count(m[1].str());
					bool constant = matchedA && constants.count(m[1].str());
					bool keyword = matchedA && keywords.count(m[1].str());
					if (matchedA && !type && !keyword && !constant) {
						return Token::Function;
					}
				}

				{
					std::smatch m;
					bool matchedA = std::regex_match(line, m, reBlockType);
					bool type = matchedA && types.count(m[1].str());
					bool constant = matchedA && constants.count(m[1].str());
					bool keyword = matchedA && keywords.count(m[1].str());
					if (matchedA && !type && !keyword && !constant) {
						return Token::Function;
					}
				}
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
				//if (std::regex_match(getline(), reInclude)) return Token::StringStart;
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

std::vector<View::Region> Syntax::tags(const std::deque<char>& text) {
	std::vector<View::Region> hits;

	for (int i = 0; i < (int)text.size(); i++) {
		auto line = getline(text, i);
		{
			std::smatch m;
			bool matched = std::regex_match(line, m, reFunction);
			if (matched && std::find(keywords.begin(), keywords.end(), m[1].str()) == keywords.end()) {
				hits.push_back({i, (int)m[1].str().size()});
			}
			if (matched) {
				i += (int)m[0].str().size()-1;
				continue;
			}
		}
		{
			std::smatch m;
			bool matched = std::regex_match(line, m, reBlockType);
			if (matched && std::find(keywords.begin(), keywords.end(), m[1].str()) == keywords.end()) {
				hits.push_back({i, (int)m[1].str().size()});
			}
			if (matched) {
				i += (int)m[0].str().size()-1;
				continue;
			}
		}
	}
	return hits;
}
