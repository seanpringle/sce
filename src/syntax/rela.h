#pragma once

#include "../syntax.h"

struct Rela : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token first(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);
	bool hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections);

	const std::set<std::string, std::less<>> keywords = {
		"function",
		"for",
		"if",
		"else",
		"end",
		"while",
		"return",
		"in",
		"do",
		"then",
		"local",
		"pairs",
		"ipairs",
	};

	bool isoperator(int c) override;

	bool keyword(const Doc& text, int offset);
	bool comment(const Doc& text, int offset);
	bool matchFunction(const Doc& text, int cursor);
};
