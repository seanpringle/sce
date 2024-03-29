#include "common.h"
#include "project.h"
#include "config.h"
#include "catenate.h"
#include "channel.h"
#include "workers.h"
#include <fstream>
#include <filesystem>
#include <string_view>
#include <regex>

using namespace std;

#include "json.hpp"
using json = nlohmann::json;

extern Config config;

Project::Project() {
	groups.resize(1);
}

Project::~Project() {
	sanity();
	while (views.size()) close();
}

void Project::sanity() {
	active = max(0, min((int)views.size()-1, active));

	set<View*> viewset = {views.begin(), views.end()};

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

int Project::find(const string& vpath) {
	auto it = find_if(views.begin(), views.end(), [&](auto view) { return view->path == vpath; });
	return it == views.end() ? -1: it-views.begin();
}

int Project::find(View* view) {
	auto it = std::find(views.begin(), views.end(), view);
	return it == views.end() ? -1: it-views.begin();
}

View* Project::view() {
	return views.size() ? views[active]: nullptr;
}

View* Project::open(const string& path) {
	using namespace filesystem;
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
		factive = views.size()-1;
	}

	active = factive;

	bubble();
	return view();
}

View* Project::fresh() {
	using namespace filesystem;
	int gactive = group(view());
	auto v = new View();

	groups[gactive].push_back(v);
	views.push_back(v);
	v->path = "untitled";
	active = views.size()-1;

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

void Project::searchPathAdd(const string& path) {
	auto tpath = filesystem::path(path);
	auto apath = filesystem::weakly_canonical(tpath);
	searchPaths.insert(apath.string());
}

void Project::searchPathDrop(const string& path) {
	auto tpath = filesystem::path(path);
	auto apath = filesystem::weakly_canonical(tpath);
	searchPaths.erase(apath.string());
}

void Project::ignorePathAdd(const string& path) {
	auto tpath = filesystem::path(path);
	auto apath = filesystem::weakly_canonical(tpath);
	ignorePaths.insert(apath.string());
}

void Project::ignorePathDrop(const string& path) {
	auto tpath = filesystem::path(path);
	auto apath = filesystem::weakly_canonical(tpath);
	ignorePaths.erase(apath.string());
}

void Project::ignorePatternAdd(const string& pattern) {
	ignorePatterns.insert(pattern);
}

void Project::ignorePatternDrop(const string& pattern) {
	ignorePatterns.erase(pattern);
}

bool Project::interpret(const string& cmd) {
	auto prefix = [&](const string& s) {
		return cmd.find(s) == 0;
	};

	if (prefix("search ") && cmd.size() > 7U) {
		auto path = cmd.substr(7); trim(path);
		searchPathAdd(path);
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

	if (cmd == "close all") {
		while (views.size()) close();
		return true;
	}

	return false;
}

bool Project::load(const string path) {
	config.defaults();

	const string lpath = path.empty() ? ppath: path;
	if (lpath.empty()) return false;

	auto apath = filesystem::path(lpath);
	auto rpath = filesystem::relative(apath);
	ppath = rpath.string();

	auto in = ifstream(lpath);
	if (!in) return false;
	json pstate;
	in >> pstate;
	in.close();

	while (views.size()) close();
	searchPaths.clear();
	ignorePaths.clear();
	ignorePatterns.clear();
	groups.clear();
	groups.resize(1);
	layout = 0;

	if (pstate.contains("autosave")) {
		autosave = pstate["autosave"];
	}

	for (auto vstate: pstate["views"]) {
		auto view = open(vstate["path"]);
		if (!view) continue;
		view->selections.clear();
		for (auto vsel: vstate["selections"]) {
			view->selections.push_back({vsel[0], vsel[1]});
		}
		view->sanity();
	}

	for (auto spath: pstate["searchPaths"]) {
		searchPathAdd(spath);
	}

	for (auto spath: pstate["ignorePaths"]) {
		ignorePathAdd(spath);
	}

	if (pstate.contains("ignorePatterns")) {
		for (auto pattern: pstate["ignorePatterns"]) {
			ignorePatternAdd(pattern);
		}
	}

	groups.clear();

	for (auto jstate: pstate["groups"]) {
		groups.resize(groups.size()+1);
		auto& group = groups.back();
		for (auto vpath: jstate["views"]) {
			int id = find(vpath);
			if (id < 0) continue;
			group.push_back(views[id]);
		}
	}

	active = pstate["active"];
	layout = pstate["layout"];

	bubble();

	ppath = path;

	if (pstate.contains("/config/sidebar/width"_json_pointer)) {
		config.sidebar.width = pstate["/config/sidebar/width"_json_pointer];
	}

	if (pstate.contains("/config/layout2/split"_json_pointer)) {
		config.layout2.split = pstate["/config/layout2/split"_json_pointer];
	}

	return true;
}

bool Project::save(const string path) {
	const string spath = path.empty() ? ppath: path;
	if (spath.empty()) return false;

	auto tpath = filesystem::path(spath);
	auto apath = filesystem::weakly_canonical(tpath);
	ppath = apath.string();

	auto out = ofstream(spath);
	if (!out) return false;

	json pstate;
	pstate["autosave"] = autosave;

	int i = 0;
	for (auto view: views) {
		if (view->modified) view->save();
		auto vpath = filesystem::path(view->path);
		auto apath = filesystem::weakly_canonical(vpath);
		json vstate;
		vstate["path"] = apath;
		int j = 0;
		for (auto selection: view->selections) {
			int k = j++;
			vstate["selections"][k][0] = selection.offset;
			vstate["selections"][k][1] = selection.length;
		}
		pstate["views"][i++] = vstate;
	}

	pstate["active"] = active;
	pstate["layout"] = layout;

	i = 0;
	for (auto group: groups) {
		json gstate;
		int j = 0;
		for (auto view: group) {
			auto vpath = filesystem::path(view->path);
			auto apath = filesystem::weakly_canonical(vpath);
			gstate["views"][j++] = apath;
		}
		pstate["groups"][i++] = gstate;
	}

	i = 0;
	for (auto searchPath: searchPaths) {
		auto spath = filesystem::path(searchPath);
		auto apath = filesystem::weakly_canonical(spath);
		pstate["searchPaths"][i++] = apath;
	}

	i = 0;
	for (auto ignorePath: ignorePaths) {
		auto spath = filesystem::path(ignorePath);
		auto apath = filesystem::weakly_canonical(spath);
		pstate["ignorePaths"][i++] = apath;
	}

	i = 0;
	for (auto ignorePattern: ignorePatterns) {
		pstate["ignorePatterns"][i++] = ignorePattern;
	}

	pstate["config"]["sidebar"]["width"] = config.sidebar.width;
	pstate["config"]["layout2"]["split"] = config.layout2.split;

	out << pstate.dump(4);
	out.close();

	return true;
}

void Project::forget(View* view) {
	for (auto& src: groups) {
		src.erase(remove(src.begin(), src.end(), view), src.end());
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

vector<View*> Project::viewsTree() {
	using namespace filesystem;

	vector<View*> tree = views;
	sort(tree.begin(), tree.end(), [&](auto a, auto b) {
		return weakly_canonical(a->path) < weakly_canonical(b->path);
	});

	return tree;
}

void Project::activeUpTree() {
	vector<View*> tree = viewsTree();

	for (int i = 1, l = tree.size(); i < l; i++) {
		if (tree[i] != views[active]) continue;
		for (int j = 0, l = views.size(); j < l; j++) {
			if (tree[i-1] == views[j]) {
				active = j;
				i = l;
				j = l;
			}
		}
	}
	bubble();
}

void Project::activeDownTree() {
	vector<View*> tree = viewsTree();

	for (int i = 0, l = tree.size(); i < l-1; i++) {
		if (tree[i] != views[active]) continue;
		for (int j = 0, l = views.size(); j < l; j++) {
			if (tree[i+1] == views[j]) {
				active = j;
				i = l;
				j = l;
			}
		}
	}
	bubble();
}

void Project::movePrevGroup() {
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

void Project::moveNextGroup() {
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

vector<string> Project::files() {
	using namespace filesystem;

	set<path> seen;
	vector<string> results;
	vector<regex> patterns;

	for (auto& pattern: ignorePatterns) {
		try {
			patterns.push_back(regex(pattern));
		}
		catch (const regex_error& e) {
			notef("%s", e.what());
		}
	}

	for (auto spath: searchPaths) {
		auto searchPath = path(spath);
		if (!exists(searchPath)) continue;

		auto it = recursive_directory_iterator(searchPath,
			directory_options::skip_permission_denied
		);

		for (const directory_entry& entry: it) {
			if (seen.count(weakly_canonical(entry.path()))) continue;
			seen.insert(weakly_canonical(entry.path()));

			if (!is_regular_file(entry)) continue;
			auto entryPath = entry.path();

			bool ignore = false;
			for (auto path: ignorePaths) {
				ignore = ignore || starts_with(entryPath.string(), path);
			}
			for (auto re: patterns) {
				ignore = ignore || regex_search(entryPath.string(), re);
			}
			if (!ignore) {
				results.push_back(canonical(entryPath.string()));
			}
		}
	}

	return results;
}

vector<Project::Match> Project::search(string needle) {
	workers crew;
	crew.start(8);

	channel<Match,-1> matches;

	auto all = files();
	for (int i = 0, l = all.size(); i < l; i++) {
		crew.job([&,v=i]() {
			View view;
			if (!view.open(all[v])) return;
			for (auto& region: view.search(needle)) {
				int offset = region.offset - view.toSol(region.offset);
				int length = view.toEol(offset);
				matches.send({
					.path = view.path,
					.line = view.extract({offset,length}),
					.region = region,
					.lineno = view.text.cursor(offset).line,
				});
			}
		});
	}

	crew.wait();

	return matches.recv_all();
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
		auto path = filesystem::path(view->path);
		auto ext = path.extension().string();
		if (ext.front() == '.') ext = ext.substr(1);
		int g = config.layout2.left.count(ext) ? 0:1;
		groups[g].push_back(view);
	}

	auto current = active;
	relatedRaise();

	active = current;
	bubble();
}

void Project::relatedRaise() {
	auto path = filesystem::path(view()->path);
	auto ext = path.extension().string();

	auto other = [&](auto rep) {
		auto rpath = path.replace_extension(rep).string();
		auto found = find(rpath);
		if (found >= 0) {
			active = found;
			return true;
		}
		return false;
	};

	if (related.count(ext)) for (auto& oext: related[ext]) if (other(oext)) break;

	bubble();
}

void Project::relatedOpen() {
	auto path = filesystem::path(view()->path);
	auto ext = path.extension().string();

	auto other = [&](auto rep) {
		auto rpath = path.replace_extension(rep).string();
		return open(rpath);
	};

	if (related.count(ext)) for (auto& oext: related[ext]) if (other(oext)) break;

	bubble();
}
