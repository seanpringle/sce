#pragma once

#include "syntax.h"
#include <map>

struct Theme {

	enum class State {
		Plain = 0,
		Selected,
	};

	struct Format {
		uint fg = 0xffffffff;
		uint bg = 0x000000ff;
	};

	std::map<Syntax::Token,std::map<State,Format>> highlight;

	Theme();
};
