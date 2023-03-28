#pragma once

#include "view.h"
#include "repo.h"
#include <map>

struct Project {
	int active = 0;
	std::vector<View*> views;
	std::string ppath;
	std::set<std::string> searchPaths;
	std::set<std::string> ignorePaths;
	std::set<std::string> ignorePatterns;
	std::vector<std::vector<View*>> groups;
	int layout = 0;
	bool autosave = true;

	Project();
	~Project();
	View* open(const std::string& path);
	int find(const std::string& path);
	int find(View* view);
	void sanity();
	View* view();
	void close();
	View* fresh();

	void searchPathAdd(const std::string& path);
	void searchPathDrop(const std::string& path);
	void ignorePathAdd(const std::string& path);
	void ignorePathDrop(const std::string& path);
	void ignorePatternAdd(const std::string& pattern);
	void ignorePatternDrop(const std::string& pattern);

	bool interpret(const std::string& cmd);
	bool load(const std::string path);
	bool save(const std::string path = "");

	void forget(View* view);
	bool known(View* view);
	int group(View* view);
	void bubble();

	void cycle();
	void activeUpTree();
	void activeDownTree();
	void movePrevGroup();
	void moveNextGroup();

	std::vector<std::string> files();

	struct Match {
		std::string path;
		std::string line;
		ViewRegion region;
		uint lineno;
	};

	std::vector<Match> search(std::string needle);

	std::map<std::string,std::vector<std::string>> related = {
		{".c", {".h"}},
		{".cc", {".h", ".hpp"}},
		{".cpp", {".h", ".hpp"}},
		{".h", {".c", ".cc", ".cpp"}},
		{".hpp", {".cpp", ".cc"}},
		{".fs", {".vs"}},
		{".vs", {".fs"}},
	};

	void layout1();
	void layout2();
	void relatedOpen();
	void relatedRaise();
};
