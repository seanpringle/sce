#include "filter.h"

void FilterPopup::filterOptions() {
	visible.clear();
	auto needles = std::string(input);
	for (int i = 0, l = (int)options.size(); i < l; i++) {
		auto& haystack = options[i];
		bool match = true;
		for (auto part: discatenate(needles," ")) {
			auto needle = std::string(part);
			trim(needle);
			if (!needle.size()) continue;
			if (needle.size() > haystack.size()) { match = false; break; }
			if (haystack.find(needle) == std::string::npos) { match = false; break; }
		}
		if (match) visible.push_back(i);
	}
	selected = std::max(0, std::min((int)visible.size()-1, selected));
}

bool FilterPopup::multiple() {
	return false;
}

bool FilterPopup::enterable() {
	return false;
}

void FilterPopup::entered() {
}

std::string FilterPopup::hint() {
	return name;
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
		scroll = false;
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

bool FilterPopup::up() {
	return selected > 0;
}

bool FilterPopup::down() {
	return selected < visible.size();
}

bool FilterPopup::tab() {
	return selected < visible.size();
}

namespace {
	int callback(ImGuiInputTextCallbackData* data) {
		FilterPopup* popup = (FilterPopup*)(data->UserData);

		if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion && data->EventKey == ImGuiKey_Tab && popup->tab()) {
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, popup->options[popup->visible[popup->selected]].c_str());
			data->SelectAll();
		}

		if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory && data->EventKey == ImGuiKey_UpArrow && popup->up()) {
			popup->selected--;
		}

		if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory && data->EventKey == ImGuiKey_DownArrow && popup->down()) {
			popup->selected++;
		}

		return 0;
	}
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

	ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue
		| ImGuiInputTextFlags_CallbackHistory
		| ImGuiInputTextFlags_CallbackCompletion;

	if (InputTextWithHint(fmtc("##%s-input", name), hint().c_str(), input, sizeof(input), flags, callback, this)) {
		if (enterable() && std::string(input).size()) {
			entered();
		}
		else
		if (visible.size() > selected) {
			chosen(visible[selected]);
		}
		CloseCurrentPopup();
	}

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

		for (int i = 0; i < (int)visible.size(); i++) {
			auto& option = options[visible[i]];
			if (Selectable(option.c_str(), i == selected)) {
				selected = i;
				chosen(visible[i]);
				CloseCurrentPopup();
			}
			if (selected == i && scroll) {
				SetScrollHereY();
			}
		}
		EndListBox();
		scroll = false;
	}
}
