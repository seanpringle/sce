#pragma once

struct Syntax;

#include <set>
#include <regex>
#include <deque>
#include "view.h"
#include "doc.h"

struct Syntax {
	enum class Token {
		None = 0,
		Comment,
		CommentBlock,
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
		Operator,
	};

	virtual ~Syntax() {};
	virtual std::vector<ViewRegion> tags(const Doc& text) = 0;
	virtual std::vector<std::string> matches(const Doc& text, int cursor) = 0;
	virtual Syntax::Token next(const Doc& text, int cursor, Token token) = 0;

	virtual bool isname(int c);
	virtual bool isboundary(int c);
	virtual bool isoperator(int c);

	int get(const Doc& text, int offset);
	bool wordset(const Doc& text, int offset, const std::set<std::string, std::less<>>& names);
};

struct PlainText : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);
};

struct CPP : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);

	const std::set<std::string, std::less<>> types = {
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
		"float",
		"double",
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

	const std::set<std::string, std::less<>> constants = {
		"NULL",
		"nullptr",
		"true",
		"false",
		"EXIT_SUCCESS",
		"EXIT_FAILURE",
		"__VA_ARGS_"
	};

	const std::set<std::string, std::less<>> keywords = {
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
		"operator",
		"override",
		"virtual",
		"mutable",
	};

	const std::set<std::string, std::less<>> blocktypes = {
		"enum",
		"class",
		"struct",
	};

	const std::set<std::string, std::less<>> directives = {
		"#include",
		"#define",
		"#pragma",
		"#if",
		"#ifdef",
		"#ifndef",
		"#else",
		"#endif",
	};

	const std::set<std::string, std::less<>> specifiers = {
		"override",
		"const",
	};

	bool isoperator(int c) override;

	bool word(const Doc& text, int offset, const std::string& name);

	bool keyword(const Doc& text, int offset);
	bool specifier(const Doc& text, int offset);
	bool comment(const Doc& text, int offset);
	bool typelike(const Doc& text, int offset);
	bool matchFunction(const Doc& text, int cursor);
	bool matchBlockType(const Doc& text, int cursor);
};

struct OpenSCAD : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);

	const std::set<std::string, std::less<>> keywords = {
		"module",
		"for",
		"use",
	};

	bool keyword(const Doc& text, int offset);
	bool comment(const Doc& text, int offset);
	bool matchModule(const Doc& text, int cursor);
};
