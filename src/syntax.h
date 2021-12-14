#pragma once

struct Syntax;

#include <set>
#include "view.h"
#include "doc.h"
#include <cstring>
#include <cwctype>

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
		Indent,
		Variable,
	};

	virtual ~Syntax() {};

	// Identify document tags/symbols/functions/types
	virtual std::vector<ViewRegion> tags(const Doc& text) = 0;

	// Find autocomplete strings based on cursor position
	virtual std::vector<std::string> matches(const Doc& text, int cursor) = 0;

	// Token for first cursor position dispayed on screen
	virtual Syntax::Token first(const Doc& text, int cursor) = 0;

	// Token for a cursor position, potentially based on last token
	virtual Syntax::Token next(const Doc& text, int cursor, Token token) = 0;

	// A hint should be displayed at cursor position (eg brace/bracket pairs)
	virtual bool hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) = 0;

	virtual bool isname(int c);
	virtual bool isboundary(int c);
	virtual bool isoperator(int c);

	int get(const Doc& text, int offset);
	bool wordset(const Doc& text, int offset, const std::set<std::string, std::less<>>& names);

	// (..(..)..)
	bool hintMatchedPair(const Doc& text, int cursor, const ViewRegion& selection, int open, int close);
};
