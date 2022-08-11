#include "filter.h"

void FilterPopup::filterOptions() {
	visible.clear();
	auto needles = std::string(input);
	for (int i = 0, l = (int)options.size(); i < l; i++) {
		auto& haystack = options[i];
		bool match = true;
		for (auto needle: discatenate(needles," ")) {
			trim(needle);
			if (!needle.size()) continue;
			if (needle.size() > haystack.size()) { match = false; break; }
			if (haystack.find(needle) == std::string::npos) { match = false; break; }
		}
		if (match) visible.push_back(i);
	}
	selected = std::max(0, std::min((int)visible.size()-1, selected));
}

void FilterPopup::selectNavigate() {
	using namespace ImGui;
	if (IsKeyPressed(KeyMap[KEY_DOWN])) selected++;
	if (IsKeyPressed(KeyMap[KEY_UP])) selected--;
	if (IsKeyPressed(KeyMap[KEY_PAGEDOWN])) selected += 10;
	if (IsKeyPressed(KeyMap[KEY_PAGEUP])) selected -= 10;
	selected = std::max(0, std::min((int)visible.size()-1, selected));
}

bool FilterPopup::multiple() {
	return false;
}

void FilterPopup::setup() {
	sync.lock();
	if (!loading) {
		loading = true;
		options.clear();
		visible.clear();
		selected = 0;
		input[0] = 0;
		ready = false;
		focus = false;
		immediate = true;
		crew.job([&]() {
			init();
			sync.lock();
			ready = true;
			loading = false;
			sync.unlock();
		});
	}
	sync.unlock();
}

void FilterPopup::render() {
	using namespace ImGui;

	sync.lock();
	if (!ready) {
		Print("Loading...");
		sync.unlock();
		return;
	}
	sync.unlock();

	immediate = false;

	if (!focus) {
		SetKeyboardFocusHere();
		focus = true;
		immediate = true;
	}

	BeginTable(fmtc("##%s-toolbar", name), multiple() ? 2: 1);
	TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

	if (multiple())
		TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

	TableNextColumn();
	SetNextItemWidth(-1);
	InputTextWithHint(fmtc("##%s-input", name), name.c_str(), input, sizeof(input));

	if (multiple()) {
		TableNextColumn();
		SetNextItemWidth(-1);
		if (Button("select all")) {
			filterOptions();
			for (auto i: visible) chosen(i);
			CloseCurrentPopup();
		}
	}
	EndTable();

	if (BeginListBox(fmtc("#%s-matches", name), ImVec2(-1,-1))) {
		filterOptions();
		selectNavigate();

		if (IsKeyPressed(KeyMap[KEY_RETURN])) {
			if (visible.size()) chosen(visible[selected]);
			CloseCurrentPopup();
		}

		for (int i = 0; i < (int)visible.size(); i++) {
			auto& option = options[visible[i]];

			if (Selectable(option.c_str(), i == selected)) {
				chosen(visible[i]);
				CloseCurrentPopup();
			}

			if (selected == i) {
				SetScrollHereY();
			}
		}
		EndListBox();
	}
}
