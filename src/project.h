#pragma once

#include "view.h"

struct Project {
	int active = 0;
	std::vector<View*> views;

	Project() = default;
	~Project();
	View* open(const std::string& path);
	int find(const std::string& path);
	void sanity();
	View* view();
	void close();
};
