#pragma once

#include "tui.h"
#include "syntax.h"
#include <map>

struct Theme {

	enum class State {
		Plain = 0,
		Selected,
	};

	std::map<Syntax::Token,std::map<State,TUI::Format>> highlight;

	Theme();
};