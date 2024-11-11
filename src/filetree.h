#pragma once

#include <set>
#include <map>
#include <vector>
#include <filesystem>

struct FileTree {
	std::vector<std::filesystem::path> listing;
	FileTree() = default;
	void render(const std::set<std::string>& paths, const std::set<std::string>& subset = {});
	virtual void onSelect(const std::string &spath);
	virtual bool isOpen(const std::string &fpath);
	virtual bool isModified(const std::string &fpath);
	virtual bool isActive(const std::string &fpath);
	virtual void annotate(const std::string &fpath);
	void cache(const std::set<std::string>& paths);
};
