#pragma once

#include <set>
#include <map>
#include <vector>
#include <filesystem>

#include "workers.h"

struct FileTree {
	struct Entry {
		std::string epath;
		std::filesystem::file_status estat;
		size_t ename = 0;
	};
	workers worker;
	channel<std::vector<Entry>,1> listings;
	std::vector<Entry> listing;
	FileTree();
	void render(const std::set<std::string>& paths, const std::set<std::string>& subset = {});
	virtual void onSelect(const std::string &spath);
	virtual bool isOpen(const std::string &fpath);
	virtual bool isModified(const std::string &fpath);
	virtual bool isActive(const std::string &fpath);
	virtual void annotate(const std::string &fpath);
	void cache(const std::set<std::string>& paths);
};
