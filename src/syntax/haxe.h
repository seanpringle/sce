#pragma once

#include "../syntax.h"

struct Haxe : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token first(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);
	bool hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections);

//	const std::set<std::string, std::less<>> types = {
//		"void",
//	};

	const std::set<std::string, std::less<>> constants = {
		"true",
		"false",
		"null",
		"this",
	};

	const std::set<std::string, std::less<>> keywords = {
		"abstract",
		"break",
		"case",
		"cast",
		"catch",
		"class",
		"continue",
		"default",
		"do",
		"dynamic",
		"else",
		"extends",
		"extern",
		"final",
		"for",
		"function",
		"if",
		"implements",
		"import",
		"in",
		"inline",
		"interface",
		"macro",
		"new",
		"operator",
		"overload",
		"override",
		"package",
		"private",
		"public",
		"return",
		"static",
		"switch",
		"throw",
		"try",
		"typedef",
		"untyped",
		"using",
		"var",
		"while",
	};

	const std::set<std::string, std::less<>> blocktypes = {
		"class",
		"interface",
		"enum",
	};

	bool isoperator(int c) override;

	bool word(const Doc& text, int offset, const std::string& name);

	bool keyword(const Doc& text, int offset);
	bool specifier(const Doc& text, int offset);
	bool comment(const Doc& text, int offset);
	bool type(const Doc& text, int offset);
	bool matchType(const Doc& text, int cursor);
	bool matchAssign(const Doc& text, int cursor);
	bool matchFunction(const Doc& text, int cursor);
	bool matchBlockType(const Doc& text, int cursor);
};
