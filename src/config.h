#include "common.h"

struct Config {
	struct {
		bool hard = true;
		int width = 4;
	} tabs;

	std::vector<std::string> paths;

	void args(int argc, const char** argv);
};
