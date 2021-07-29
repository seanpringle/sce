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

View* Project::open(const std::string& path) {
	active = find(path);

	if (active < 0) {
		auto v = new View();
		v->open(path);
		views.push_back(v);

		std::sort(views.begin(), views.end(), [](auto a, auto b) { return a->path < b->path; });

		active = find(path);
		ensure(active >= 0);
	}

	return view();
}

void Project::close() {
	auto v = view();
	if (v) {
		delete v;
		views.erase(std::find(views.begin(), views.end(), v));
		sanity();
	}
}
