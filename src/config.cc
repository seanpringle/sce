#include "common.h"
#include "config.h"
#include "ini.h"
#include <cstdlib>

using namespace std::chrono_literals;
IniReader ini;

std::vector<std::string> Config::args(int argc, const char** argv) {
	auto HOME = std::getenv("HOME");
	ini = IniReader(fmtc("%s/.sce.ini", HOME));

	auto tilde = [&](auto haystack) {
		auto pos = haystack.find("~");
		while (pos != std::string::npos) {
			haystack = haystack.replace(pos, 1, HOME);
			pos = haystack.find("~");
		}
		return haystack;
	};

	mouse.wheelSpeedStep = 30ms;

	font.prop.face = tilde(ini.getString("window", "font.prop"));
	font.prop.size = ini.getDouble("window", "font.px", 18.0);

	font.mono.face = tilde(ini.getString("window", "font.mono"));
	font.mono.size = font.prop.size;

	view.font = ini.getDouble("edit", "font", 1.0);
	sidebar.font = ini.getDouble("sidebar", "font", 1.0);
	popup.font = ini.getDouble("popup", "font", 1.0);

	tabs.hard = ini.getString("edit", "tabs", "hard") == "hard";
	tabs.width = ini.getInteger("edit", "tabs.width", 4);

	sidebar.split = ini.getDouble("sidebar", "width", 0.1);
	popup.width = ini.getDouble("popup", "width", 0.25);

	layout2.left = ini.getWords("layout2", "left");
	layout2.split = ini.getDouble("layout2", "width", 0.0);

	std::vector<std::string> paths;

	for (int i = 1; i < argc; i++) {
		paths.push_back(argv[i]);
	}

	return paths;
}
