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
	std::vector<std::string> openPaths;
	std::vector<std::string> openItems;

	std::vector<View*> lefts;
	std::vector<View*> rights;

	auto refreshTabs = [&]() {
	};

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
				refreshTabs();
				return;
			}
		}
		for (auto it = rights.begin(); it != rights.end(); ++it) {
			auto buf = *it;
			if (buf->path == path) {
				rights.erase(it);
				rights.insert(rights.begin(), buf);
				view = 1;
				refreshTabs();
				return;
			}
		}

		if (std::filesystem::path(path).extension() == ".h") {
			lefts.insert(lefts.begin(), new View());
			lefts.front()->open(path);
			refreshTabs();
			view = 0;
			return;
		}
		rights.insert(rights.begin(), new View());
		rights.front()->open(path);
		refreshTabs();
		view = 1;
	};

	for (auto path: config.paths) {
		open(path);
	}

	auto within = [&](View* view, int col, int row) {
		return col >= view->x && col < view->x+view->w && row >= view->y && row < view->y+view->h;
	};

	auto clear = [&]() {
		tui.print("\e[38;5;255m\e[48;5;235m");
		tui.clear();
	};

	auto check = [&]() {
		if (resized) {
			resized = false;
			int side = 0;
			int cols = tui.cols();
			int rows = tui.rows();

			int width = (cols-side-2)/3;

			for (auto view: lefts)
				view->move(1,0,width,rows);

			for (auto view: rights)
				view->move(width+2,0,width*2,rows);

			clear();
			refreshTabs();
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
				if (view == 0 && lefts.size() && !lefts.front()->modified) {
					delete lefts.front();
					lefts.erase(lefts.begin());
				}
				if (view == 1 && rights.size() && !rights.front()->modified) {
					delete rights.front();
					rights.erase(rights.begin());
				}
				resized = true;
				check();
			}

			if (tui.mouse.left && lefts.size() && within(lefts.front(), tui.mouse.col, tui.mouse.row)) {
				view = 0;
			}

			if (tui.mouse.left && rights.size() && within(rights.front(), tui.mouse.col, tui.mouse.row)) {
				view = 1;
			}

			if (tui.keys.ctrl && tui.keysym == "P") {
				openPaths.clear();
				openItems.clear();
				for (const std::experimental::filesystem::directory_entry& entry: std::experimental::filesystem::recursive_directory_iterator(".")) {
					auto path = entry.path().string();
					if (path.size() > 1 && path.substr(0,2) == "./") {
						path = path.substr(2);
					}
					openPaths.push_back(path);
					bool isModified = false;
					for (auto view: lefts) {
						if (view->path == path) isModified = view->modified;
					}
					for (auto view: rights) {
						if (view->path == path) isModified = view->modified;
					}
					openItems.push_back(fmt("%s%s", path, isModified ? "*": ""));
				}
				opener.start(&openItems);
			}

			[&]() {
				if (opener.active) {
					opener.input();
					if (!opener.active && opener.chosen >= 0) {
						open(openPaths[opener.filtered[opener.chosen]]);
					}
					if (!opener.active) {
						resized = true;
						check();
					}
					return;
				}

				if (view == 0 && lefts.size()) {
					lefts.front()->input();
					return;
				}

				if (view == 1 && rights.size()) {
					rights.front()->input();
					return;
				}
			}();

			refreshTabs();
			draw = true;
		}

		if (draw) {
			if (lefts.size()) lefts.front()->draw();
			if (rights.size()) rights.front()->draw();
			if (opener.active) {
				opener.draw(tui.cols()/4,tui.rows()/4,tui.cols()/2,tui.rows()/2);
			}
			tui.flush();
			continue;
		}

		std::this_thread::sleep_for(16ms);
	}

	for (auto view: lefts) delete view;
	for (auto view: rights) delete view;

	tui.stop();
	return 0;
}
