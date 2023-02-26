#include "common.h"
#include "config.h"
#include "ini.h"
#include <cstdlib>

using namespace std::chrono_literals;
IniReader ini;

void Config::args(int argc, const char** argv) {
	this->argc = argc;
	this->argv = argv;
}

void Config::defaults() {
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
	font.prop.size = ini.getDouble("window", "font.pt", 13.0);

	font.mono.face = tilde(ini.getString("window", "font.mono"));
	font.mono.size = font.prop.size;

	view.font = ini.getDouble("edit", "font", 1.0);
	sidebar.font = ini.getDouble("sidebar", "font", 1.0);
	popup.font = ini.getDouble("popup", "font", 1.0);

	tabs.hard = ini.getString("edit", "tabs", "hard") == "hard";
	tabs.width = ini.getInteger("edit", "tabs.width", 4);

	sidebar.width = ini.getDouble("sidebar", "width", 0.1);
	popup.width = ini.getDouble("popup", "width", 0.25);

	layout2.left = ini.getWords("layout2", "left");
	layout2.split = ini.getDouble("layout2", "width", 0.5);
}

bool Config::interpret(const std::string& cmd) {
	auto prefix = [&](const std::string& s) {
		return cmd.find(s) == 0;
	};

	if (prefix("sidebar.width ") && cmd.size() > 14U) {
		auto arg = cmd.substr(14); trim(arg);
		sidebar.width = std::strtod(arg.c_str(), nullptr);
		return true;
	}

	if (prefix("layout2.split ") && cmd.size() > 14U) {
		auto arg = cmd.substr(14); trim(arg);
		layout2.split = std::strtod(arg.c_str(), nullptr);
		return true;
	}

	return false;
}

