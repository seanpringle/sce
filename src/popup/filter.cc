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

void FilterPopup::setup() {
	using namespace ImGui;
	SetKeyboardFocusHere();
	options.clear();
	visible.clear();
	selected = 0;
	input[0] = 0;
	init();
}

void FilterPopup::render() {
	using namespace ImGui;
	InputText(fmtc("%s#%s-input", name, name), input, sizeof(input));

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
