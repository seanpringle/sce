#include "plaintext.h"

std::vector<ViewRegion> PlainText::tags(const Doc& text) {
	return {};
}

std::vector<std::string> PlainText::matches(const Doc& text, int cursor) {
	return {};
}

Syntax::Token PlainText::first(const Doc& text, int cursor) {
	return Token::None;
}

Syntax::Token PlainText::next(const Doc& text, int cursor, Syntax::Token token) {
	return Token::None;
}

bool PlainText::hint(const Doc& text, int cursor, const std::vector<ViewRegion>& selections) {
	return false;
}
