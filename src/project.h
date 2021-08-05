#pragma once

#include "view.h"

struct Project {
	int active = 0;
	std::vector<View*> views;
	std::string ppath;
	std::set<std::string> searchPaths;
	std::set<std::string> ignorePaths;
	std::vector<std::vector<View*>> groups;
	int layout = 0;

	Project();
	~Project();
	View* open(const std::string& path);
	int find(const std::string& path);
	int find(View* view);
	void sanity();
	View* view();
	void close();

	void searchPathAdd(const std::string& path);
	void searchPathDrop(const std::string& path);
	void ignorePathAdd(const std::string& path);
	void ignorePathDrop(const std::string& path);

	bool interpret(const std::string& cmd);
	bool load(const std::string path);
	bool save(const std::string path = "");

	void forget(View* view);
	bool known(View* view);
	int group(View* view);
	void bubble();

	void layout1();
	void layout2();

	void cycle();
	void prev();
	void next();
	void movePrev();
	void moveNext();
};
