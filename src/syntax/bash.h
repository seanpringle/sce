#pragma once

#include "../syntax.h"

struct Bash : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token first(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);
	bool hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections);

	const std::set<std::string, std::less<>> keywords = {
		"for",
		"do",
		"done",
		"if",
		"fi",
		"then",
		"else",
		"while",
		"case",
		"esac",
	};

	bool keyword(const Doc& text, int offset);
	bool comment(const Doc& text, int offset);
};
