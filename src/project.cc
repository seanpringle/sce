#include "common.h"
#include "project.h"
#include <fstream>

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

int Project::find(View* view) {
	sanity();
	auto it = std::find(views.begin(), views.end(), view);
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

		if (!v->open(path)) {
			delete v;
			return nullptr;
		}

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

bool Project::interpret(const std::string& cmd) {
	auto prefix = [&](const std::string& s) {
		return cmd.find(s) == 0;
	};

	if (prefix("path ") && cmd.size() > 5U) {
		auto path = cmd.substr(5); trim(path);
		paths.push_back(path);
		return true;
	}

	if (prefix("save ") && cmd.size() > 5U) {
		auto path = cmd.substr(5); trim(path);
		save(path);
		return true;
	}

	if (prefix("load ") && cmd.size() > 5U) {
		auto path = cmd.substr(5); trim(path);
		load(path);
		return true;
	}

	return false;
}

bool Project::load(const std::string& path) {
	auto in = std::ifstream(path);
	if (!in) return false;
	for (std::string line; std::getline(in, line); ) {
		notef("%s", line);
		open(line);
	}
	in.close();
	return true;
}

bool Project::save(const std::string& path) {
	auto out = std::ofstream(path);
	if (!out) return false;
	for (auto view: views) {
		notef("%s", view->path);
		out << view->path << '\n';
	}
	out.close();
	return true;
}
