#include "common.h"

struct Config {
	struct {
		bool hard = true;
		int width = 4;
	} tabs;

	struct {
		bool enabled = true;
	} mouse;

	std::vector<std::string> paths;

	void args(int argc, const char** argv);
};
