#pragma once

#include "../syntax.h"

struct Docker : Syntax {
	std::vector<ViewRegion> tags(const Doc& text);
	std::vector<std::string> matches(const Doc& text, int cursor);
	Token first(const Doc& text, int cursor);
	Token next(const Doc& text, int cursor, Token token);
	bool hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections);

	const std::set<std::string, std::less<>> keywords = {
		"RUN",
		"FROM",
		"COPY",
		"CMD",
		"ARG",
		"USER",
		"ENV",
		"ADD",
		"EXPOSE",
		"WORKDIR",
		"ENTRYPOINT",
		"VOLUME",
	};

	bool keyword(const Doc& text, int offset);
	bool comment(const Doc& text, int offset);
};
