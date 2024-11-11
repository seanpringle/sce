#include "filetree.h"
#include "repo.h"
#include "common.h"
#include "../imgui/imgui.h"

#include <vector>
#include <functional>

using namespace std;
using namespace ImGui;
using namespace std::filesystem;

void FileTree::onSelect(const string& spath) {
}

bool FileTree::isActive(const string& fpath) {
	return false;
}

bool FileTree::isOpen(const string& fpath) {
	return false;
}

bool FileTree::isModified(const string& fpath) {
	return false;
}

void FileTree::annotate(const string& fpath) {
}

namespace {
	bool is_subpath(const std::filesystem::path& path, const std::filesystem::path& base) {
		return path.string().size() > base.string().size() && starts_with(path.string(), base.string());
	}
}

void FileTree::cache(const set<string>& searchPaths) {
	listing.clear();

	auto known = [&](const path& path) {
		auto it = std::lower_bound(listing.begin(), listing.end(), path);
		return it != listing.end() && *it == path;
	};

	auto record = [&](const path& path) {
		auto it = std::lower_bound(listing.begin(), listing.end(), path);
		if (it == listing.end() || *it != path) listing.insert(it, path);
	};

	function<void(const path&)> walk;

	walk = [&](const path& wpath) {
		if (!exists(wpath)) return;
		if (!is_directory(wpath)) return;

		if (known(wpath)) return;
		record(wpath);

		auto it = directory_iterator(wpath,
			directory_options::skip_permission_denied
		);

		for (const directory_entry& entry: it) {
			if (is_regular_file(entry)) {
				record(entry.path());
				continue;
			}
			if (is_directory(entry)) {
				walk(entry.path());
				continue;
			}
		}
	};

	for (auto& dpath: searchPaths) {
		walk(weakly_canonical(dpath));
	}

	sort(listing.begin(), listing.end());
}

void FileTree::render(const set<string>& searchPaths, const set<string>& openPaths) {
	if (!listing.size()) cache(searchPaths);

	vector<path> spaths, opaths;

	for (auto& spath: searchPaths) spaths.push_back(weakly_canonical(spath));
	for (auto& opath: openPaths) opaths.push_back(weakly_canonical(opath));

	sort(spaths.begin(), spaths.end());
	sort(opaths.begin(), opaths.end());

	auto ingroup = [&](auto& kpath, auto& group) {
		auto it = std::lower_bound(group.begin(), group.end(), kpath);
		return it != group.end() && *it == kpath;
	};

	struct Parent {
		bool open = false;
		path ppath;
	};

	vector<Parent> parent;

	auto istoplevel = [&](auto& epath) {
		return ingroup(epath, spaths);
	};

	auto hasopenchild = [&](auto& epath) {
		auto it = std::lower_bound(opaths.begin(), opaths.end(), epath);
		return it != opaths.end() && is_subpath(*it, epath);
	};

	auto display = [&](auto& epath) {
		if (parent.size() && !parent.back().open)
			return false;
		if (!openPaths.size() && parent.size() && parent.back().open)
			return true;
		if (ingroup(epath, opaths))
			return true;
		if (hasopenchild(epath))
			return true;
		return istoplevel(epath);
	};

	for (auto& epath: listing) {
		while (parent.size() && !is_subpath(epath, parent.back().ppath)) {
			if (parent.back().open) TreePop();
			parent.pop_back();
		}

		if (!display(epath)) continue;

		auto estat = status(epath);

		if (estat.type() == file_type::directory) {
			string label = fmt("%s##%s", epath.filename().string(), epath.string());
			auto& nest = parent.emplace_back();
			uint32_t flags = istoplevel(epath) || hasopenchild(epath) ? ImGuiTreeNodeFlags_DefaultOpen: 0u;
			nest.open = TreeNodeEx(label.c_str(), flags | ImGuiTreeNodeFlags_SpanAvailWidth);
			nest.ppath = epath;
			continue;
		}

		if (estat.type() == file_type::regular) {
			if (isModified(epath))
				PushStyleColor(ImGuiCol_Text, ImColorSRGB(0xffff00ff));
			else if (isOpen(epath))
				PushStyleColor(ImGuiCol_Text, ImColorSRGB(0x99ff99ff));
			else
				PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_Text));

			if (isActive(epath)) {
				GetWindowDrawList()->ChannelsSplit(2);
				GetWindowDrawList()->ChannelsSetCurrent(1);
			}

			if (Selectable(fmtc("%s##%s", epath.filename().string(), epath.string()))) {
				onSelect(epath);
			}

			if (isActive(epath)) {
				if (!IsItemHovered()) {
					GetWindowDrawList()->ChannelsSetCurrent(0);
					GetWindowDrawList()->AddRectFilled(GetItemRectMin(), GetItemRectMax(), GetColorU32(ImGuiCol_Header));
				}
				GetWindowDrawList()->ChannelsMerge();
			}

			PopStyleColor();

			annotate(epath);
			continue;
		}
	}

	while (parent.size()) {
		if (parent.back().open) TreePop();
		parent.pop_back();
	}
}
