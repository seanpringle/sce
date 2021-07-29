#include "common.h"
#include "config.h"

using namespace std::chrono_literals;

void Config::args(int argc, const char** argv) {
	mouse.wheelSpeedStep = 30ms;

	font.view.face = "font/LiberationMono-Regular.ttf";
	font.view.size = 18.0f;

	font.ui.face = "font/LiberationSans-Regular.ttf";
	font.ui.size = 20.0f;

	for (int i = 1; i < argc; i++) {
		bool next = i < argc-1;
		auto arg = std::string(argv[i]);

		char pad[100];
		ensure(std::strlen(argv[i]) < 100);
		if (next) ensure(std::strlen(argv[i+1]) < 100);

		if (arg == "-tabs" && next) {
			i++;
			ensuref(2 == std::sscanf(argv[i], "%4s,%d", pad, &tabs.width),
				"what? %s", argv[i]
			);
			tabs.hard = std::string(pad) != "soft";
			note() << fmt("tabs { hard = %s, width = %d }", (tabs.hard ? "hard": "soft"), tabs.width);
			continue;
		}

		paths.push_back(argv[i]);
	}

	auto term = std::string(std::getenv("TERM"));
	if (term.find("screen") != std::string::npos) {
		note() << "GNU Screen detected, disabling mouse";
		mouse.enabled = false;
	}
}
