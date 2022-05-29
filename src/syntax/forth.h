#pragma once

#include "../syntax.h"

struct Forth : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token first(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);
	bool hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections);

	bool isname(int c) override;
	bool isboundary(int c) override;
	bool isoperator(int c) override;

	const std::set<std::string, std::less<>> keywords = {
		"if",
		"else",
		"then",
		"begin",
		"while",
		"until",
		"again",
		"for",
		"next",
		"loop",
		":",
		";",
		"as",
	};

	const std::set<std::string, std::less<>> corewords = {
		"push",
		"pop",
		"dup",
		"drop",
		"over",
		"swap",
		"min",
		"max",
	};
};

