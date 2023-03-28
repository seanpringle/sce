#include "filter.h"

void FilterPopup::filterOptions() {
	visible.clear();
	needles.clear();

	for (auto part: discatenate(input," ")) {
		auto needle = std::string(part);
		trim(needle);
		if (!needle.size()) continue;
		needles.push_back(std::move(needle));
	}

	for (int i = 0, l = (int)options.size(); i < l; i++) {
		std::string_view oview(options[i]);
		size_t needle = 0;
		for (size_t cursor = 0; cursor < oview.size(); ) {
			if (needle < needles.size()) {
				std::string_view nview(needles[needle]);
				if (oview[cursor] == nview.front() && oview.substr(cursor,nview.size()) == nview) {
					cursor += nview.size();
					needle++;
					continue;
				}
			}
			cursor++;
		}
		if (needle == needles.size())
			visible.push_back(i);
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

	filterOptions();

	if (BeginListBox(fmtc("##%s-matches", name), ImVec2(-1,-1))) {
		struct Section {
			ImU32 color = 0;
			size_t offset = 0;
			size_t length = 0;
		};

		std::vector<Section> sections;

		for (int i = 0; i < (int)visible.size(); i++) {
			const auto& option = options[visible[i]];

			GetWindowDrawList()->ChannelsSplit(2);
			GetWindowDrawList()->ChannelsSetCurrent(1);

			float width = GetContentRegionAvail().x;

			BeginGroup();
			Indent(GetStyle().ItemSpacing.x/2);

			sections.clear();
			sections.emplace_back();
			sections.back().color = GetColorU32(ImGuiCol_Text);

			std::string_view oview(option);
			size_t nextNeedle = 0;

			for (size_t cursor = 0; cursor < option.size(); ) {
				bool match = false;
				if (nextNeedle < needles.size()) {
					std::string_view nview(needles[nextNeedle]);
					if (oview[cursor] == nview.front() && oview.substr(cursor,nview.size()) == nview) {
						sections.emplace_back();
						sections.back().color = ImColorSRGB(0xffff00ff);
						sections.back().offset = cursor;
						sections.back().length = nview.size();
						cursor += nview.size();
						nextNeedle++;
						match = true;
						sections.emplace_back();
						sections.back().color = sections.front().color;
						sections.back().offset = cursor;
					}
				}
				if (!match) {
					sections.back().length++;
					cursor++;
				}
			}

			for (
				auto it = sections.begin();
				it != sections.end();
				it = it->length > 0 ? ++it: sections.erase(it)
			);

			for (auto& section: sections) {
				PushStyleColor(ImGuiCol_Text, section.color);
				TextUnformatted(option.c_str() + section.offset, option.c_str() + section.offset + section.length);
				SameLine();
				SetCursorPosX(GetCursorPosX()-GetStyle().ItemSpacing.x);
				PopStyleColor();
			}
			NewLine();

			Unindent(GetStyle().ItemSpacing.x/2);
			SetCursorPosX(width);
			EndGroup();

			if (IsItemHovered()) {
				GetWindowDrawList()->ChannelsSetCurrent(0);
				GetWindowDrawList()->AddRectFilled(GetItemRectMin(), GetItemRectMax(), GetColorU32(ImGuiCol_HeaderHovered));
			}
			else
			if (selected == i) {
				GetWindowDrawList()->ChannelsSetCurrent(0);
				GetWindowDrawList()->AddRectFilled(GetItemRectMin(), GetItemRectMax(), GetColorU32(ImGuiCol_Header));
			}

			GetWindowDrawList()->ChannelsMerge();

			if (IsItemClicked()) {
				selected = i;
				chosen(visible[i]);
				CloseCurrentPopup();
			}

			if (selected == i && !IsWindowHovered()) {
				SetScrollHereY();
			}
		}
		EndListBox();
	}
}
