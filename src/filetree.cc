#include "filetree.h"
#include "repo.h"
#include "common.h"
#include "../imgui/imgui.h"

#include <vector>
#include <functional>

using namespace std;
using namespace ImGui;
using namespace std::filesystem;

FileTree::FileTree() {
	worker.start(1);
}

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
	bool is_subpath(const string& path, const string& base) {
		return path.size() > base.size() && starts_with(path, base);
	}
}

void FileTree::cache(const set<string>& searchPaths) {
	worker.job([&,paths=searchPaths]() {
		vector<Entry> listing;

		auto lfind = [&](const string& fpath) {
			return std::lower_bound(listing.begin(), listing.end(), fpath, [](auto& a, auto& b) { return a.epath < b; });
		};

		auto known = [&](const string& kpath) {
			auto it = lfind(kpath);
			return it != listing.end() && it->epath == kpath;
		};

		auto record = [&](const string& rpath) {
			auto it = lfind(rpath);
			if (it == listing.end() || it->epath != rpath) {
				auto fp = weakly_canonical(rpath);
				listing.insert(it, {rpath,status(fp),fp.filename().string().size()});
			}
		};

		function<void(const string&)> walk;

		walk = [&](const string& wpath) {
			if (known(wpath)) return;

			record(wpath);

			auto it = directory_iterator(wpath,
				directory_options::skip_permission_denied
			);

			for (const directory_entry& entry: it) {
				if (entry.is_regular_file()) {
					record(entry.path().string());
					continue;
				}
				if (entry.is_directory()) {
					walk(entry.path().string());
					continue;
				}
			}
		};

		for (auto& dpath: paths) {
			auto spath = weakly_canonical(dpath);
			if (is_directory(spath)) walk(spath.string());
		}

		listings.send(listing);
	});
}

void FileTree::render(const set<string>& searchPaths, const set<string>& openPaths) {
	for (auto batch: listings.recv_all()) {
		listing = batch;
	}

	if (!listing.size() && !worker.pending() && searchPaths.size()) {
		cache(searchPaths);
	}

	vector<string> spaths, opaths;

	for (auto& spath: searchPaths) spaths.push_back(weakly_canonical(spath).string());
	for (auto& opath: openPaths) opaths.push_back(weakly_canonical(opath).string());

	sort(spaths.begin(), spaths.end());
	sort(opaths.begin(), opaths.end());

	auto ingroup = [&](auto& kpath, auto& group) {
		auto it = std::lower_bound(group.begin(), group.end(), kpath);
		return it != group.end() && *it == kpath;
	};

	struct Parent {
		bool open = false;
		string ppath;
	};

	static vector<Parent> parent;
	parent.clear();

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

	stringstream ss;

	for (auto& [epath,estat,ename]: listing) {
		while (parent.size() && !is_subpath(epath, parent.back().ppath)) {
			if (parent.back().open) TreePop();
			parent.pop_back();
		}

		if (!display(epath)) continue;

		ss.str("");
		ss << string_view(epath.begin() + epath.size()-ename, epath.end());
		ss << "##" << epath;

		if (estat.type() == file_type::directory) {
			auto& nest = parent.emplace_back();
			uint32_t flags = istoplevel(epath) || hasopenchild(epath) ? ImGuiTreeNodeFlags_DefaultOpen: 0u;
			nest.open = TreeNodeEx(ss.str().c_str(), flags | ImGuiTreeNodeFlags_SpanAvailWidth);
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

			if (Selectable(ss.str().c_str())) {
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
