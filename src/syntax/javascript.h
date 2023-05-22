#pragma once

#include "../syntax.h"

struct JavaScript : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token first(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);
	bool hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections);

	const std::set<std::string, std::less<>> keywords = {
		"function",
		"new",
		"if",
		"else",
		"switch",
		"case",
		"for",
		"while",
		"break",
		"continue",
		"return",
		"var",
	};

	bool isoperator(int c) override;

	bool word(const Doc& text, int offset, const std::string& name);

	bool keyword(const Doc& text, int offset);
	bool comment(const Doc& text, int offset);
	bool matchFunction(const Doc& text, int cursor);
	bool matchBlockType(const Doc& text, int cursor);
};

