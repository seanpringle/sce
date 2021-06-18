#include "theme.h"

Theme::Theme() {
	highlight[Syntax::Token::None][State::Plain] = { .fg = "\e[38;5;255m", .bg = "\e[48;5;234m" };
	highlight[Syntax::Token::None][State::Selected] = { .fg = "\e[38;5;0m", .bg = "\e[48;5;250m" };

	highlight[Syntax::Token::Comment][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Comment][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Comment][State::Plain].fg = "\e[38;5;240m";

	highlight[Syntax::Token::CommentBlock][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::CommentBlock][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::CommentBlock][State::Plain].fg = "\e[38;5;240m";

	highlight[Syntax::Token::Integer][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Integer][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Integer][State::Plain].fg = "\e[38;5;166m";

	highlight[Syntax::Token::StringStart][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::StringStart][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::StringStart][State::Plain].fg = "\e[38;5;228m";

	highlight[Syntax::Token::StringFinish][State::Plain] = highlight[Syntax::Token::StringStart][State::Plain];
	highlight[Syntax::Token::StringFinish][State::Selected] = highlight[Syntax::Token::StringStart][State::Selected];

	highlight[Syntax::Token::CharStringStart][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::CharStringStart][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::CharStringStart][State::Plain].fg = "\e[38;5;228m";

	highlight[Syntax::Token::CharStringFinish][State::Plain] = highlight[Syntax::Token::CharStringStart][State::Plain];
	highlight[Syntax::Token::CharStringFinish][State::Selected] = highlight[Syntax::Token::CharStringStart][State::Selected];

	highlight[Syntax::Token::Type][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Type][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Type][State::Plain].fg = "\e[38;5;111m";

	highlight[Syntax::Token::Keyword][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Keyword][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Keyword][State::Plain].fg = "\e[38;5;161m";

	highlight[Syntax::Token::Constant][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Constant][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Constant][State::Plain].fg = "\e[38;5;133m";

	highlight[Syntax::Token::Namespace][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Namespace][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Namespace][State::Plain].fg = "\e[38;5;175m";

	highlight[Syntax::Token::Directive][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Directive][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Directive][State::Plain].fg = "\e[38;5;161m";

	highlight[Syntax::Token::Call][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Call][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Call][State::Plain].fg = "\e[38;5;39m";

	highlight[Syntax::Token::Function][State::Plain] = highlight[Syntax::Token::None][State::Plain];
	highlight[Syntax::Token::Function][State::Selected] = highlight[Syntax::Token::None][State::Selected];

	highlight[Syntax::Token::Function][State::Plain].fg = "\e[38;5;112m";
}
