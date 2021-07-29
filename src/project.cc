#include "common.h"
#include "project.h"

Project::~Project() {
	sanity();
	while (views.size()) close();
}

void Project::sanity() {
	active = std::max(0, std::min((int)views.size()-1, active));
}

int Project::find(const std::string& path) {
	sanity();
	auto it = std::find_if(views.begin(), views.end(), [&](auto view) { return view->path == path; });
	return it == views.end() ? -1: it-views.begin();
}

View* Project::view() {
	sanity();
	return views.size() ? views[active]: nullptr;
}

void Project::open(const std::string& path) {
	active = find(path);

	if (active < 0) {
		auto view = new View();
		view->open(path);
		views.push_back(view);

		std::sort(views.begin(), views.end(), [](auto a, auto b) { return a->path < b->path; });

		active = find(path);
		ensure(active >= 0);
	}
}

void Project::close() {
	auto v = view();
	if (v) {
		delete v;
		views.erase(std::find(views.begin(), views.end(), v));
		sanity();
	}
}
