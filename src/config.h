#include "common.h"
#include <chrono>

struct Config {
	struct {
		int width = 1920;
		int height = 1080;
		bool vsync = false;
	} window;

	struct {
		bool hard = true;
		int width = 4;
	} tabs;

	struct {
		bool enabled = true;
		std::chrono::milliseconds wheelSpeedStep;
	} mouse;

	struct {
		struct {
			const char* face = nullptr;
			float size = 0.0f;
		} view;
		struct {
			const char* face = nullptr;
			float size = 0.0f;
		} ui;
	} font;

	std::vector<std::string> paths;

	void args(int argc, const char** argv);
};
