#include "diff.h"

DiffPopup::DiffPopup() {
	name = "diff";
}

void DiffPopup::setup() {
	auto repo = Repo::open(path);
	diff = repo && repo->ok() ? repo->diff(path): Repo::Diff();
}

void DiffPopup::render() {
	using namespace ImGui;
	PushFont(fontView);
	if (diff.ok()) {
		for (auto line: discatenate(diff.patch, "\n")) {
			int pushed = 0;
			if (line.size() && line.front() == '+') {
				PushStyleColor(ImGuiCol_Text, ImColorSRGB(0x009900ff));
				pushed++;
			}
			if (line.size() && line.front() == '-') {
				PushStyleColor(ImGuiCol_Text, ImColorSRGB(0xcc0000ff));
				pushed++;
			}
			TextUnformatted(&line.front(), &line.back()+1);
			PopStyleColor(pushed);
		}
	}
	PopFont();
}
