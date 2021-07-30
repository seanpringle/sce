#include "common.h"
#include "config.h"
#include "ini.h"
#include <cstdlib>

using namespace std::chrono_literals;
IniReader ini;

std::vector<std::string> Config::args(int argc, const char** argv) {
	auto HOME = std::getenv("HOME");
	ini = IniReader(fmtc("%s/.sce.ini", HOME));

	mouse.wheelSpeedStep = 30ms;

	font.view.face = ini.getCString("edit", "font");
	font.view.size = ini.getDouble("edit", "px", 18.0);

	font.ui.face = ini.getCString("ui", "font");
	font.ui.size = ini.getDouble("ui", "px", 18.0);

	tabs.hard = ini.getString("edit", "tabs", "hard") == "hard";
	tabs.width = ini.getInteger("edit", "tabs.width", 4);

	sidebar.width = ini.getInteger("sidebar", "width", 300);

	std::vector<std::string> paths;

	for (int i = 1; i < argc; i++) {
		paths.push_back(argv[i]);
	}

	return paths;
}
