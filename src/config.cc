#include "common.h"
#include "config.h"

void Config::args(int argc, const char** argv) {
	for (int i = 1; i < argc; i++) {
		bool next = i < argc-1;
		auto arg = std::string(argv[i]);

		char pad[100];
		ensure(std::strlen(argv[i]) < 100);
		if (next) ensure(std::strlen(argv[i+1]) < 100);

		if (arg == "-tabs" && next) {
			ensuref(2 == std::sscanf(argv[i+1], "%4s,%d", pad, &tabs.width),
				"what? %s", argv[i+1]
			);
			tabs.hard = std::string(pad) != "soft";
			note() << fmt("tabs { hard = %s, width = %d }", (tabs.hard ? "hard": "soft"), tabs.width);
			i++;
			continue;
		}

		paths.push_back(argv[i]);
	}
}
