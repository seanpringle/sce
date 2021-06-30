#include "common.h"
#include <chrono>

struct Config {
	struct {
		bool hard = true;
		int width = 4;
	} tabs;

	struct {
		bool enabled = true;
		std::chrono::milliseconds wheelSpeedStep;
	} mouse;

	std::vector<std::string> paths;

	void args(int argc, const char** argv);
};
