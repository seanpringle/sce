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

void FileTree::render(const set<string>& paths, const set<string>& subset) {
	function<void(const path&, ImGuiTreeNodeFlags)> walk;

	set<path> seen;

	walk = [&](const path& walkPath, ImGuiTreeNodeFlags flags) {
		if (!exists(walkPath)) return;

		bool show = subset.empty();
		for (auto& spath: subset) {
			show = show || starts_with(spath, walkPath.string());
		}
		if (!show) return;

		string label = fmt("%s##%s", walkPath.filename().string(), walkPath.string());
		if (!subset.empty()) flags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (!TreeNodeEx(label.c_str(), flags | ImGuiTreeNodeFlags_SpanAvailWidth)) return;

		auto it = directory_iterator(walkPath,
			directory_options::skip_permission_denied
		);

		vector<directory_entry> entries;

		for (const directory_entry& entry: it) {
			auto wpath = weakly_canonical(entry.path());
			if (is_regular_file(entry) && !subset.empty() && !subset.count(wpath.string())) continue;
			if (seen.count(wpath)) continue;
			seen.insert(wpath);
			entries.push_back(entry);
		}

		sort(entries.begin(), entries.end());

		for (const auto& entry: entries) {

			if (is_directory(entry)) {
				walk(entry.path(), ImGuiTreeNodeFlags_None);
				continue;
			}

			if (is_regular_file(entry)) {
				if (isModified(entry.path()))
					PushStyleColor(ImGuiCol_Text, ImColorSRGB(0xffff00ff));
				else if (isOpen(entry.path()))
					PushStyleColor(ImGuiCol_Text, ImColorSRGB(0x99ff99ff));
				else
					PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_Text));

				if (isActive(entry.path())) {
					GetWindowDrawList()->ChannelsSplit(2);
					GetWindowDrawList()->ChannelsSetCurrent(1);
				}

				if (Selectable(fmtc("%s##%s", entry.path().filename().string(), entry.path().string()))) {
					onSelect(entry.path());
				}

				if (isActive(entry.path())) {
					if (!IsItemHovered()) {
						GetWindowDrawList()->ChannelsSetCurrent(0);
						GetWindowDrawList()->AddRectFilled(GetItemRectMin(), GetItemRectMax(), GetColorU32(ImGuiCol_Header));
					}
					GetWindowDrawList()->ChannelsMerge();
				}

				PopStyleColor();

				annotate(entry.path());
				continue;
			}
		}

		TreePop();
	};

	for (auto& dpath: paths) {
		walk(dpath, ImGuiTreeNodeFlags_DefaultOpen);
	}
}
