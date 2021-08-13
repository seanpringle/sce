#include "syntax.h"
#include <string_view>

bool Syntax::isname(int c) {
	return iswalnum(c) || c == '_';
}

bool Syntax::isboundary(int c) {
	return c != '_' && (iswcntrl(c) || iswpunct(c) || iswspace(c));
}

bool Syntax::isoperator(int c) {
	return strchr("+-*/%=<>", c);
}

int Syntax::get(const Doc& text, int cursor) {
	return cursor >= 0 && cursor < (int)text.size() ? text[cursor]: 0;
}

bool Syntax::wordset(const Doc& text, int cursor, const std::set<std::string,std::less<>>& names) {
	char pad[32]; int len = 0;
	for (int i = 0; i < 31 && isname(get(text, cursor+i)); i++) {
		pad[len++] = get(text, cursor+i);
	}
	pad[len] = 0;
	return len > 0 && names.count(std::string_view(pad)) > 0;
}

bool Syntax::hintMatchedPair(const Doc& text, int cursor, const ViewRegion& selection, int open, int close) {
	auto isBalancedPair = [&](int start, int finish) {
		int count = 1;
		for (int i = start+1; i <= finish; i++) {
			int c = get(text, i);
			if (c == open) count++;
			if (c == close) count--;
			if (!count && i < finish) return false;
		}
		return count == 0;
	};

	int c = get(text, cursor);

	if (c == open && selection.offset > cursor) {
		int s = get(text, selection.offset);
		if (s == close && isBalancedPair(cursor, selection.offset)) {
			return true;
		}
	}

	if (c == close && selection.offset < cursor) {
		int s = get(text, selection.offset);
		if (s == open && isBalancedPair(selection.offset, cursor)) {
			return true;
		}
	}

	return false;
}

#include "syntax/cpp.cc"
#include "syntax/ini.cc"
#include "syntax/cmake.cc"
#include "syntax/openscad.cc"
#include "syntax/forth.cc"
#include "syntax/plaintext.cc"
