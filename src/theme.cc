#include "theme.h"

Theme::Theme() {

	uint text = 0xd6d6d6ff;
	uint keyword = 0xff6666ff;
	uint quotedstring = 0xffff66ff;
	uint comment = 0x797979ff;
	uint type = 0x66c8ffff;
	uint call = 0x66c8ffff;
	uint func = 0x40cc40ff;
	uint op = 0xffdd88ff;
	uint ns = 0x40c2bbff;
	uint constant = 0xb695c0ff;
	uint number = constant;
	uint directive = keyword;

	highlight[Syntax::Token::None][State::Plain] = { .fg = text, .bg = 0x00000000 };
	highlight[Syntax::Token::None][State::Selected] = { .fg = 0x000000ff, .bg = 0xddddddff };

	highlight[Syntax::Token::Comment][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Comment][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Comment][State::Plain].fg = comment;

	highlight[Syntax::Token::CommentBlock][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::CommentBlock][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::CommentBlock][State::Plain].fg = comment;

	highlight[Syntax::Token::Integer][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Integer][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Integer][State::Plain].fg = number;

	highlight[Syntax::Token::StringStart][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::StringStart][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::StringStart][State::Plain].fg = quotedstring;

	highlight[Syntax::Token::StringFinish][State::Plain] = highlight[Syntax::Token::StringStart][State::Plain];
	highlight[Syntax::Token::StringFinish][State::Selected] = highlight[Syntax::Token::StringStart][State::Selected];

	highlight[Syntax::Token::CharStringStart][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::CharStringStart][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::CharStringStart][State::Plain].fg = quotedstring;

	highlight[Syntax::Token::CharStringFinish][State::Plain] = highlight[Syntax::Token::CharStringStart][State::Plain];
	highlight[Syntax::Token::CharStringFinish][State::Selected] = highlight[Syntax::Token::CharStringStart][State::Selected];

	highlight[Syntax::Token::Type][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Type][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Type][State::Plain].fg = type;

	highlight[Syntax::Token::Keyword][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Keyword][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Keyword][State::Plain].fg = keyword;

	highlight[Syntax::Token::Constant][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Constant][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Constant][State::Plain].fg = constant;

	highlight[Syntax::Token::Namespace][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Namespace][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Namespace][State::Plain].fg = ns;

	highlight[Syntax::Token::Directive][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Directive][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Directive][State::Plain].fg = directive;

	highlight[Syntax::Token::Call][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Call][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Call][State::Plain].fg = call;

	highlight[Syntax::Token::Function][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Function][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Function][State::Plain].fg = func;

	highlight[Syntax::Token::Operator][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Operator][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Operator][State::Plain].fg = op;
}
