#include "common.h"
#include "project.h"
#include "config.h"
#include <fstream>
#include <filesystem>

extern Config config;

Project::Project() {
	groups.resize(1);
}

Project::~Project() {
	sanity();
	while (views.size()) close();
}

void Project::sanity() {
	active = std::max(0, std::min((int)views.size()-1, active));

	std::set<View*> viewset = {views.begin(), views.end()};

	for (auto it = groups.begin(); it != groups.end(); ) {
		auto &group = *it;
		if (!group.size() && groups.size() > 1U) {
			it = groups.erase(it);
			continue;
		}
		for (auto view: group) {
			ensure(viewset.count(view));
			viewset.erase(view);
		}
		++it;
	}

	ensure(!viewset.size());
}

int Project::find(const std::string& path) {
	auto vpath = std::filesystem::weakly_canonical(path).string();
	auto it = std::find_if(views.begin(), views.end(), [&](auto view) { return view->path == vpath; });
	return it == views.end() ? -1: it-views.begin();
}

int Project::find(View* view) {
	auto it = std::find(views.begin(), views.end(), view);
	return it == views.end() ? -1: it-views.begin();
}

View* Project::view() {
	return views.size() ? views[active]: nullptr;
}

View* Project::open(const std::string& path) {
	int gactive = group(view());
	int factive = find(path);

	if (factive < 0) {
		auto v = new View();

		if (!v->open(path)) {
			delete v;
			return nullptr;
		}

		groups[gactive].push_back(v);
		views.push_back(v);

		std::sort(views.begin(), views.end(), [](auto a, auto b) { return a->path < b->path; });

		factive = find(path);
		ensure(factive >= 0);
	}

	active = factive;

	bubble();
	return view();
}

void Project::close() {
	auto v = view();
	if (v) {
		forget(v);
		delete v;
		views.erase(std::find(views.begin(), views.end(), v));
		bubble();
	}
}

void Project::pathAdd(const std::string& path) {
	auto tpath = std::filesystem::path(path);
	auto apath = std::filesystem::weakly_canonical(tpath);
	if (std::find(paths.begin(), paths.end(), apath.string()) == paths.end()) {
		paths.push_back(apath.string());
	}
}

void Project::pathDrop(const std::string& path) {
	auto tpath = std::filesystem::path(path);
	auto apath = std::filesystem::weakly_canonical(tpath);
	paths.erase(std::remove(paths.begin(), paths.end(), apath.string()));
}

bool Project::interpret(const std::string& cmd) {
	auto prefix = [&](const std::string& s) {
		return cmd.find(s) == 0;
	};

	if (prefix("path ") && cmd.size() > 5U) {
		auto path = cmd.substr(5); trim(path);
		pathAdd(path);
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

bool Project::load(const std::string path) {
	const std::string lpath = path.empty() ? ppath: path;
	if (lpath.empty()) return false;

	auto apath = std::filesystem::path(lpath);
	auto rpath = std::filesystem::relative(apath);
	ppath = rpath.string();

	auto in = std::ifstream(lpath);
	if (!in) return false;

	std::string line;
	int nviews = 0, ngroups = 0, npaths = 0;

	ensure(std::getline(in, line));
	ensure(1 == std::sscanf(line.c_str(), "%d", &nviews));

	for (int i = 0; i < nviews; i++) {
		ensure(std::getline(in, line));
		notef("%s", line);
		open(line);
	}

	ensure(std::getline(in, line));
	ensure(1 == std::sscanf(line.c_str(), "%d", &active));

	ensure(std::getline(in, line));
	ensure(1 == std::sscanf(line.c_str(), "%d", &layout));

	ensure(std::getline(in, line));
	ensure(1 == std::sscanf(line.c_str(), "%d", &ngroups));

	groups.clear();
	groups.resize(ngroups);

	for (auto& group: groups) {
		int gsize = 0;
		ensure(std::getline(in, line));
		ensure(1 == std::sscanf(line.c_str(), "%d", &gsize));
		for (int i = 0; i < gsize; i++) {
			ensure(std::getline(in, line));
			int id = find(line);
			if (id >= 0) group.push_back(views[id]);
		}
	}

	ensure(std::getline(in, line));
	ensure(1 == std::sscanf(line.c_str(), "%d", &npaths));

	for (int i = 0; i < npaths; i++) {
		ensure(std::getline(in, line)); trim(line);
		pathAdd(line);
	}

	in.close();

	bubble();

	ppath = path;
	return true;
}

bool Project::save(const std::string path) {
	const std::string spath = path.empty() ? ppath: path;
	if (spath.empty()) return false;

	auto tpath = std::filesystem::path(spath);
	auto apath = std::filesystem::weakly_canonical(tpath);
	ppath = apath.string();

	auto out = std::ofstream(spath);
	if (!out) return false;

	out << fmt("%llu views", views.size()) << '\n';
	for (auto view: views) {
		auto vpath = std::filesystem::path(view->path);
		auto apath = std::filesystem::weakly_canonical(vpath);
		out << apath.string() << '\n';
	}
	out << fmt("%d active", active) << '\n';
	out << fmt("%d layout", layout) << '\n';
	out << fmt("%llu groups", groups.size()) << '\n';
	for (auto group: groups) {
		out << fmt("%llu views", group.size()) << '\n';
		for (auto view: group) {
			auto vpath = std::filesystem::path(view->path);
			auto apath = std::filesystem::weakly_canonical(vpath);
			out << apath.string() << '\n';
		}
	}
	out << fmt("%llu paths", paths.size()) << '\n';
	for (auto searchPath: paths) {
		auto spath = std::filesystem::path(searchPath);
		auto apath = std::filesystem::weakly_canonical(spath);
		out << apath.string() << '\n';
	}
	out.close();
	return true;
}

void Project::forget(View* view) {
	for (auto& src: groups) {
		src.erase(std::remove(src.begin(), src.end(), view), src.end());
	}
}

bool Project::known(View* view) {
	for (auto& src: groups) {
		if (std::find(src.begin(), src.end(), view) != src.end()) return true;
	}
	return false;
}

int Project::group(View* view) {
	if (!view) return 0;
	for (int i = 0; i < (int)groups.size(); i++) {
		auto& group = groups[i];
		auto it = std::find(group.begin(), group.end(), view);
		if (it != group.end()) return i;
	}
	throw view;
}

void Project::bubble() {
	sanity();
	auto& grp = groups[group(view())];

	if (view() && grp.front() != view()) {
		forget(view());
		grp.insert(grp.begin(), view());
		sanity();
	}
}

void Project::layout1() {
	groups.clear();
	groups.resize(1);
	layout = 1;
	for (auto view: views) {
		groups[0].push_back(view);
	}
	bubble();
}

void Project::layout2() {
	groups.clear();
	groups.resize(2);
	layout = 2;
	for (auto view: views) {
		auto path = std::filesystem::path(view->path);
		auto ext = path.extension().string();
		if (ext.front() == '.') ext = ext.substr(1);
		int g = config.layout2.left.count(ext) ? 0:1;
		groups[g].push_back(view);
	}
	bubble();
}

void Project::cycle() {
	if (groups.size() < 2U) return;

	bool found = false;
	auto gactive = group(view());

	for (int i = gactive+1; !found && i < (int)groups.size(); i++) {
		if (groups[i].size()) {
			gactive = i;
			found = true;
		}
	}

	for (int i = 0; !found && i < gactive; i++) {
		if (groups[i].size()) {
			gactive = i;
			found = true;
		}
	}

	if (found) {
		active = find(groups[gactive].front());
	}

	bubble();
}

void Project::prev() {
	active--;
	bubble();
}

void Project::next() {
	active++;
	bubble();
}

void Project::movePrev() {
	auto gactive = group(view());
	if (gactive == 0) {
		forget(view());
		groups.insert(groups.begin(), {view()});
	} else {
		auto& dst = groups[gactive-1];
		forget(view());
		dst.insert(dst.begin(), view());
	}
	bubble();
}

void Project::moveNext() {
	auto gactive = group(view());
	if (gactive == (int)groups.size()-1) {
		forget(view());
		groups.push_back({view()});
	} else {
		auto& dst = groups[gactive+1];
		forget(view());
		dst.insert(dst.begin(), view());
	}
	bubble();
}

