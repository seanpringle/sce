#include "common.h"
#include <chrono>
#include <set>
#include <vector>

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
			std::string face;
			float size = 0.0f;
		} prop;
		struct {
			std::string face;
			float size = 0.0f;
		} mono;
	} font;

	struct {
		float font = 1.0f;
	} view;

	struct {
		float font = 1.0f;
		float split = 0.1f;
	} sidebar;

	struct {
		float font = 1.0f;
		float width = 0.25f;
	} popup;

	struct {
		std::set<std::string> left;
		float split = 0.0f;
	} layout2;

	bool newProject = false;

	std::vector<std::string> args(int argc, const char** argv);
};
