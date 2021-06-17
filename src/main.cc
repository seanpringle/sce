#include "common.h"
#include "tui.h"
#include "syntax.h"
#include "theme.h"
#include "view.h"
#include "config.h"

#include <deque>
#include <chrono>
#include <thread>
#include <filesystem>
#include <experimental/filesystem>

using namespace std::chrono_literals;

TUI tui;
Syntax syntax;
Theme theme;
Config config;

bool resized = true;
bool running = true;

void interrupt(int sig) {
	if (sig == SIGWINCH) resized = true;
}

int main(int argc, char const *argv[]) {
	config.args(argc, argv);

	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = interrupt;
	ensure(0 == sigaction(SIGWINCH, &sa, NULL));

	tui.start();

	int view = 0;
	bool draw = false;
	resized = true;

	SelectList opener;
	std::vector<std::string> openItems;

	std::vector<View*> lefts;
	std::vector<View*> rights;

	auto open = [&](std::string path) {
		if (path.size() > 1 && path.substr(0,2) == "./") {
			path = path.substr(2);
		}

		for (auto it = lefts.begin(); it != lefts.end(); ++it) {
			auto buf = *it;
			if (buf->path == path) {
				lefts.erase(it);
				lefts.insert(lefts.begin(), buf);
				view = 0;
				return;
			}
		}
		for (auto it = rights.begin(); it != rights.end(); ++it) {
			auto buf = *it;
			if (buf->path == path) {
				rights.erase(it);
				rights.insert(rights.begin(), buf);
				view = 1;
				return;
			}
		}

		if (std::filesystem::path(path).extension() == ".h") {
			rights.insert(rights.begin(), new View());
			rights.front()->open(path);
			view = 1;
			return;
		}
		lefts.insert(lefts.begin(), new View());
		lefts.front()->open(path);
		view = 0;
	};

	for (auto path: config.paths) {
		open(path);
	}

	auto within = [&](View* view, int col, int row) {
		return col >= view->x && col < view->x+view->w && row >= view->y && row < view->y+view->h;
	};

	auto check = [&]() {
		if (!lefts.size()) {
			lefts.push_back(new View());
			resized = true;
		}

		if (!rights.size()) {
			rights.push_back(new View());
			resized = true;
		}

		if (resized) {
			resized = false;
			int panel = (tui.cols()-2)/2;

			for (auto view: lefts)
				view->move(1,0,panel,tui.rows()+1);

			for (auto view: rights)
				view->move(panel+2,0,panel,tui.rows()+1);

			tui.clear();
			draw = true;
		}
	};

	view = 0;

	while (running) {
		draw = false;

		check();

		if (tui.keyPeek()) {
			tui.accept();

			if (tui.keys.f12) {
				running = false;
				continue;
			}

			if (tui.keys.ctrl && tui.keycode == 64) {
				view = view ? 0: 1;
			}

			if (tui.keys.ctrl && tui.keysym == "W") {
				if (view == 0 && !lefts.front()->modified) {
					delete lefts.front();
					lefts.erase(lefts.begin());
				}
				if (view == 1 && !rights.front()->modified) {
					delete rights.front();
					rights.erase(rights.begin());
				}
				check();
			}

			if (tui.mouse.left && within(lefts.front(), tui.mouse.col, tui.mouse.row)) {
				view = 0;
			}

			if (tui.mouse.left && within(rights.front(), tui.mouse.col, tui.mouse.row)) {
				view = 1;
			}

			if (tui.keys.ctrl && tui.keysym == "P") {
				openItems.clear();
				for (const std::experimental::filesystem::directory_entry& entry: std::experimental::filesystem::recursive_directory_iterator(".")) {
					const auto& path = entry.path();
					openItems.push_back(path.string());
				}
				opener.start(&openItems);
			}

			[&]() {
				if (opener.active) {
					opener.input();
					if (!opener.active && opener.chosen >= 0) {
						open(openItems[opener.filtered[opener.chosen]]);
						resized = true;
						check();
					}
					return;
				}

				if (view == 0) {
					lefts.front()->input();
					return;
				}

				if (view == 1) {
					rights.front()->input();
					return;
				}
			}();

			draw = true;
		}

		if (draw) {
			lefts.front()->draw();
			rights.front()->draw();

			if (opener.active) {
				opener.draw(tui.cols()/4,tui.rows()/4,tui.cols()/2,tui.rows()/2);
			}
			tui.flush();
			continue;
		}

		std::this_thread::sleep_for(16ms);
	}
	tui.stop();
	return 0;
}
