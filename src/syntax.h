#pragma once

#include <set>
#include <regex>
#include <deque>
#include "view.h"

struct Syntax {

	enum class Token {
		None = 0,
		Comment,
		Function,
		Call,
		Integer,
		StringStart,
		StringFinish,
		CharStringStart,
		CharStringFinish,
		Type,
		Keyword,
		Directive,
		Constant,
		Namespace,
	};

	const std::set<std::string> types = {
		"void",
		"auto",
		"bool",
		"unsigned",
		"long",
		"char",
		"uint32_t",
		"uint64_t",
		"uint",
		"int32_t",
		"int64_t",
		"int",
		"size_t",
		"off_t",
		"string",
		"vector",
		"list",
		"deque",
		"map",
		"set",
		"FILE",
	};

	const std::set<std::string> constants = {
		"NULL",
		"nullptr",
		"true",
		"false",
		"EXIT_SUCCESS",
		"EXIT_FAILURE",
		"__VA_ARGS_"
	};

	const std::set<std::string> keywords = {
		"typedef",
		"enum",
		"new",
		"delete",
		"if",
		"else",
		"switch",
		"case",
		"for",
		"while",
		"break",
		"continue",
		"return",
		"const",
		"default",
		"template",
		"typename",
		"class",
		"struct",
		"public",
		"private",
		"protected",
		"dynamic_cast",
		"static_cast",
		"try",
		"catch",
		"throw",
		"extern",
		"using",
		"inline",
		"static",
		"friend",
		"constexpr",
		"namespace",
	};

	const std::set<std::string> directives = {
		"#include",
		"#define",
		"#pragma",
		"#if",
		"#else",
		"#endif",
	};

	std::regex reFunction;
	std::regex reInclude;
	std::regex reBlockType;

	Token next(const std::deque<char>& text, int cursor, Token token);

	bool isname(int c);
	bool isnamestart(int c);
	bool isboundary(int c);
	int get(const std::deque<char>& text, int offset);
	std::string getline(const std::deque<char>& text, int offset);
	bool word(const std::deque<char>& text, int offset, const std::string& name);

	Syntax();
	std::vector<View::Region> tags(const std::deque<char>& text);
};
