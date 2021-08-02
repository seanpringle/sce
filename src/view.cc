#include "common.h"
#include "view.h"
#include "syntax.h"
#include "theme.h"
#include "config.h"
#include "keys.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl.h"
#include "../imgui/imgui_impl_opengl3.h"

extern Theme theme;
extern Config config;

View::View() {
	sanity();
	tabs.hard = config.tabs.hard;
	tabs.width = config.tabs.width;
}

View::~View() {
	delete syntax;
}

void View::sanity() {
	if (!text.size()) text = {'\n'};

	index();

	// bounds
	for (auto& selection: selections) {
		selection.offset = std::max(0, std::min((int)text.size(), selection.offset));
		selection.length = std::max(0, std::min((int)text.size() - selection.offset, selection.length));
	}

	// duplicates, overlaps
	std::vector<ViewRegion> keep;

	auto within = [&](int offset, int length, int p) {
		return p >= offset && p < offset+length;
	};

	for (auto candidate: selections) {
		bool clash = false;
		for (auto& selection: keep) {
			bool same = candidate.offset == selection.offset && candidate.length == selection.length;
			bool overlapA = within(selection.offset, selection.length, candidate.offset);
			bool overlapB = within(selection.offset, selection.length, candidate.offset+std::max(0,candidate.length-1));
			bool overlapC = within(candidate.offset, candidate.length, selection.offset);
			bool overlapD = within(candidate.offset, candidate.length, selection.offset+std::max(0,selection.length-1));
			clash = (same || overlapA || overlapB || overlapC || overlapD);
			if (clash) break;
		}
		if (!clash) keep.push_back(candidate);
	}

	selections = keep;

	if (!selections.size()) {
		selections.push_back({0,0});
	}

	intoView(selections.back());
}

int View::toSol(int offset) {
	int start = offset;
	while (start > 0 && !sol(start)) start--;
	return offset-start;
}

int View::toEol(int offset) {
	int end = offset;
	while (end < (int)text.size() && !eol(end)) end++;
	return end-offset;
}

int View::get(int offset) {
	return offset >= 0 && (int)text.size() > offset ? text[offset]: 0;
}

bool View::sol(int offset) {
	return offset <= 0 || get(offset-1) == '\n';
}

bool View::eol(int offset) {
	return offset >= (int)text.size() || get(offset) == '\n';
}

void View::index() {
	top = std::max(0, std::min(top, (int)text.lines.size()-1));
}

void View::nav() {
	Change change;
	change.batch = batches++;
	change.type = Navigation;
	change.selections = selections;
	undos.push_back(change);
}

void View::undo() {
	if (!undos.size()) return;
	modified = true;

	uint cbatch = undos.back().batch;

	while (undos.size() && undos.back().batch == cbatch) {

		Change change = undos.back();
		undos.pop_back();

		ensuref(change.length == (int)change.text.size(), "[%s] != %d", change.text, change.length);
		selections = change.selections;

		if (change.type == Insertion) {
			// find the spot and remove the region
			auto it = text.begin()+change.offset;
			std::vector<int> s = {it,it+change.length};
			ensuref(s == change.text, "[%s] != [%s]", s, change.text);
			text.erase(it, it+change.length);
		}

		if (change.type == Deletion) {
			// find the spot and reinsert the region
			auto it = text.begin()+change.offset;
			for (auto c: change.text) {
				it = text.insert(it, c);
				++it;
			}
		}

		redos.push_back(change);
	}

	while (undos.size() > 100) {
		undos.erase(undos.begin());
	}

	sanity();
}

void View::redo() {
	if (!redos.size()) return;
	modified = true;

	uint cbatch = redos.back().batch;

	while (redos.size() && redos.back().batch == cbatch) {

		Change change = redos.back();
		redos.pop_back();

		ensure(change.length == (int)change.text.size());
		selections = change.selections;

		if (change.type == Insertion) {
			// find the spot and reinsert the region
			auto it = text.begin()+change.offset;
			for (auto c: change.text) {
				it = text.insert(it, c);
				++it;
			}
		}

		if (change.type == Deletion) {
			// find the spot and remove the region
			auto it = text.begin()+change.offset;
			std::vector<int> s = {it,it+change.length};
			ensure(s == change.text);
			text.erase(it, it+change.length);
		}

		undos.push_back(change);
	}

	while (redos.size() > 100) {
		redos.erase(redos.begin());
	}

	sanity();
}

bool View::erase() {
	bool erased = false;
	for (int i = 0; i < (int)selections.size(); i++) {
		auto& selection = selections[i];

		if (selection.offset < (int)text.size()
			&& selection.length > 0
			&& selection.offset+selection.length <= (int)text.size()
		){
			erased = true;
			auto it = text.begin()+selection.offset;

			Change change;
			change.batch = batches;
			change.type = Deletion;
			change.offset = selection.offset;
			change.length = selection.length;
			change.text = {it, it+selection.length};
			change.selections = selections;
			undos.push_back(change);
			redos.clear();

			text.erase(it, it+selection.length);
			for (int j = 0; j < (int)selections.size(); j++) {
				auto& jselection = selections[j];
				if (jselection.offset > selection.offset) {
					jselection.offset -= selection.length;
				}
			}
			selection.length = 0;
		}
	}
	if (erased) {
		batches++;
		sanity();
		modified = true;
	}
	return erased;
}

void View::insertAt(ViewRegion& selection, int c, bool autoindent) {
	auto it = text.begin()+selection.offset;

	if (selections.size() == 1U
		&& undos.size() > 0
		&& undos.back().type == Insertion
		&& undos.back().selections.size() == 1
		&& undos.back().offset+undos.back().length == selection.offset
	){
		undos.back().length++;
		undos.back().text.push_back(c);
	}
	else {
		Change change;
		change.batch = batches;
		change.type = Insertion;
		change.offset = selection.offset;
		change.length = 1;
		change.text = {c};
		change.selections = selections;
		undos.push_back(change);
		redos.clear();
	}

	int inserted = 1;
	it = text.insert(it, c)+1;

	if (autoindent && c == '\n') {
		int left = toSol(selection.offset);
		int start = selection.offset-left;

		std::vector<int> indent;
		while (iswspace(text[start]) && text[start] != '\n') {
			indent.push_back(text[start++]);
		}

		text.insert(it, indent.begin(), indent.end());
		inserted += indent.size();
		for (int c: indent) undos.back().text.push_back(c);
		undos.back().length += (int)indent.size();
	}

	for (int j = 0; j < (int)selections.size(); j++) {
		auto& jselection = selections[j];
		if (jselection.offset >= selection.offset) {
			jselection.offset += inserted;
		}
	}
}

void View::insert(int c, bool autoindent) {
	erase();
	for (auto& selection: selections) {
		insertAt(selection, c, autoindent);
	}
	batches++;
	modified = true;
	sanity();
}

bool View::indent() {
	if (tabs.hard) {
		insert('\t');
	}
	for (int i = 0; !tabs.hard && i < tabs.width; i++) {
		insert(' ');
	}
	return true;
}

bool View::outdent() {
	if (tabs.hard) {
		del('\t');
	}
	for (int i = 0; !tabs.hard && i < tabs.width; i++) {
		del(' ');
	}
	return true;
}

void View::back(int c) {
	if (!erase()) {
		for (int i = 0; i < (int)selections.size(); i++) {
			auto& selection = selections[i];
			if (!selection.offset) continue;
			auto it = text.begin()+selection.offset;
			if (c && c != get(selection.offset-1)) continue;

			if ((int)selections.size() == 1
				&& undos.size() > 0
				&& undos.back().type == Deletion
				&& undos.back().selections.size() == 1
				&& undos.back().offset == selection.offset
			){
				undos.back().offset--;
				undos.back().length++;
				undos.back().text.insert(undos.back().text.begin(), text[undos.back().offset]);
			}
			else {
				Change change;
				change.batch = batches;
				change.type = Deletion;
				change.offset = selection.offset-1;
				change.length = 1;
				change.text = {it-1, it};
				change.selections = selections;
				undos.push_back(change);
				redos.clear();
			}

			text.erase(it-1, it);
			for (int j = 0; j < (int)selections.size(); j++) {
				auto& jselection = selections[j];
				if (jselection.offset >= selection.offset) {
					jselection.offset -= 1;
				}
			}
		}
		batches++;
		modified = true;
		sanity();
	}
}

void View::delAt(ViewRegion& selection) {
	auto it = text.begin()+selection.offset;

	if ((int)selections.size() == 1
		&& undos.size() > 0
		&& undos.back().type == Deletion
		&& undos.back().selections.size() == 1
		&& undos.back().offset == selection.offset
	){
		undos.back().length++;
		undos.back().text.push_back(text[undos.back().offset]);
	}
	else {
		Change change;
		change.batch = batches;
		change.type = Deletion;
		change.offset = selection.offset;
		change.length = 1;
		change.text = {it, it+1};
		change.selections = selections;
		undos.push_back(change);
		redos.clear();
	}

	text.erase(it, it+1);
	for (int j = 0; j < (int)selections.size(); j++) {
		auto& jselection = selections[j];
		if (jselection.offset > selection.offset) {
			jselection.offset -= 1;
		}
	}
}

void View::del(int c) {
	if (!erase()) {
		for (int i = 0; i < (int)selections.size(); i++) {
			auto& selection = selections[i];
			if (c && c != get(selection.offset)) continue;
			if (selection.offset == text.size()) continue;
			delAt(selection);
		}
		batches++;
		modified = true;
		sanity();
	}
}

void View::clip() {
	clips.clear();
	std::list<bool> lines;
	for (auto& selection: selections) {
		lines.push_back(!selection.length);
		if (!selection.length) {
			int left = toSol(selection.offset);
			int right = toEol(selection.offset)+1;
			selection.offset -= left;
			selection.length = right+left;
		}
	}
	sanity();
	for (auto& selection: selections) {
		auto it = text.begin()+selection.offset;
		Clip clip;
		clip.text = {it, it+selection.length};
		clip.line = lines.front();
		lines.pop_front();
		clips.push_back(clip);
	}
	auto cliptext = std::string({clips.front().text.begin(), clips.front().text.end()});
	ImGui::SetClipboardText(cliptext.c_str());
}

void View::cut() {
	clip();
	erase();
}

void View::copy() {
	clip();
	sanity();
}

void View::paste() {
	erase();
	int nclips = clips.size();

	std::vector<int> clipstring;
	const char* cliptext = ImGui::GetClipboardText();

	while (cliptext && *cliptext) {
		unsigned char c = *cliptext++;
		if (c == 0xc2) {
			unsigned char d = *cliptext++;
			clipstring.push_back(((unsigned int)c << 8)|(unsigned int)d);
			continue;
		}
		clipstring.push_back(c);
	}

	for (int i = selections.size()-1; i >= 0; --i) {
		auto& selection = selections[i];
		// when single clip/selection, clipboard takes precedence
		auto clipText = nclips > 1 && nclips > i ? clips[i].text: clipstring;
		auto clipLine = nclips > 1 && nclips > i ? clips[i].line: false;
		int offset = selection.offset;

		if (clipLine) {
			int left = toSol(selection.offset);
			selection.offset -= left;
		}

		for (auto c: clipText) {
			insertAt(selection, c, false);
		}

		if (clipLine) {
			selection.offset = offset;
			downAt(selection);
		}
	}
	modified = true;
	sanity();
}

bool View::dup() {
	copy();
	for (auto& selection: selections)
		selection.length = 0;
	paste();
	return true;
}

int View::upper(int c) {
	return toupper(c);
}

int View::lower(int c) {
	return tolower(c);
}

void View::up() {
	for (auto& selection: selections) {
		int left = toSol(selection.offset);
		selection.offset -= left;
		selection.offset--;
		selection.offset -= toSol(selection.offset);
		int right = toEol(selection.offset);
		selection.offset += std::min(left, right);
		selection.length = 0;
	}
	sanity();
}

void View::downAt(ViewRegion& selection) {
	int left = toSol(selection.offset);
	selection.offset += toEol(selection.offset);
	selection.offset++;
	int right = toEol(selection.offset);
	selection.offset += std::min(left, right);
	selection.length = 0;
}

void View::down() {
	for (auto& selection: selections) {
		downAt(selection);
	}
	sanity();
}

void View::right() {
	for (auto& selection: selections) {
		selection.offset += selection.length;
		if (!selection.length) selection.offset++;
		selection.length = 0;
	}
	sanity();
}

void View::left() {
	for (auto& selection: selections) {
		if (!selection.length) selection.offset--;
		selection.length = 0;
	}
	sanity();
}

void View::home() {
	for (auto& selection: selections) {
		while (!sol(selection.offset)) selection.offset--;
	}
	sanity();
}

void View::end() {
	for (auto& selection: selections) {
		while (!eol(selection.offset)) selection.offset++;
	}
	sanity();
}

void View::pgup() {
	single();
	for (int i = 0; h > 0 && i < h; i++) up();
	sanity();
}

void View::pgdown() {
	single();
	for (int i = 0; h > 0 && i < h; i++) down();
	sanity();
}

void View::bumpup() {
	top = std::max(0, top-1);
	sanity();
}

void View::bumpdown() {
	top = std::min((int)text.lines.size()-1, top+1);
	sanity();
}

void View::selectRight() {
	for (auto& selection: selections) {
		selection.length++;
		if (selection.length == 1) selection.length = 2;
	}
	sanity();
}

void View::selectRightBoundary() {
	for (auto& selection: selections) {
		int offset = selection.offset+selection.length;
		while (offset < (int)text.size() && !eol(offset) && syntax->isboundary(get(offset))) offset++;
		while (offset < (int)text.size() && !eol(offset) && !syntax->isboundary(get(offset))) offset++;
		selection.length = offset-selection.offset;
	}
	sanity();
}

void View::selectLeft() {
	for (auto& selection: selections) {
		selection.length--;
		if (selection.length == 1) selection.length = 0;
	}
	sanity();
}

void View::selectLeftBoundary() {
	for (auto& selection: selections) {
		int offset = selection.offset+selection.length;
		while (offset > selection.offset && !sol(offset) && syntax->isboundary(get(offset))) offset--;
		while (offset > selection.offset && !sol(offset) && !syntax->isboundary(get(offset))) offset--;
		selection.length = offset-selection.offset;
	}
	sanity();
}

void View::selectDown() {
	for (auto& selection: selections) {
		int left = toSol(selection.offset+selection.length);
		int right = toEol(selection.offset+selection.length);
		selection.length += left+right+1;
	}
	sanity();
}

void View::selectUp() {
	for (auto& selection: selections) {
		int left = toSol(selection.offset+selection.length);
		int right = toEol(selection.offset+selection.length);
		selection.offset -= left+right-1;
		selection.length += left+right-1;
	}
	sanity();
}

bool View::selectNext() {
	bool match = false;
	auto& pattern = selections.front();
	auto& marker = selections.back();
	if (!pattern.length) return false;

	nav();

	for (int i = marker.offset+1; i < (int)text.size()-pattern.length && !match; i++) {
		auto a = text.begin()+pattern.offset;
		auto b = text.begin()+i;
		match = true;
		for (int j = 0; j < pattern.length && match; j++) {
			match = match && (*a == *b);
			++a; ++b;
		}
		if (match) {
			if (marker.offset == skip.offset && marker.length == skip.length)
				selections.pop_back();
			selections.push_back({i,pattern.length});
			skip = {-1,-1};
		}
	}
	if (!match) {
		for (int i = 0; i < pattern.offset && i < (int)text.size()-pattern.length && !match; i++) {
			bool duplicate = false;
			for (auto& other: selections) {
				duplicate = duplicate || other.offset == i;
			}
			if (duplicate) continue;
			auto a = text.begin()+pattern.offset;
			auto b = text.begin()+i;
			match = true;
			for (int j = 0; j < pattern.length && match; j++) {
				match = match && (*a == *b);
				++a; ++b;
			}
			if (match) {
				if (marker.offset == skip.offset && marker.length == skip.length)
					selections.pop_back();
				selections.push_back({i,pattern.length});
				skip = {-1,-1};
			}
		}
	}
	sanity();
	return true;
}

void View::selectSkip() {
	skip = selections.back();
}

void View::intoView(ViewRegion& selection) {
	int lineno = text.cursor(selection.offset+selection.length).line;

	while (lineno+10 > top+h) {
		top++;
	}

	while (lineno-10 <= top) {
		top--;
	}

	top = std::max(0, std::min((int)text.lines.size()-1, top));
}

void View::boundaryRight() {
	for (auto& selection: selections) {
		while (selection.offset < (int)text.size() && !eol(selection.offset) && syntax->isboundary(get(selection.offset))) selection.offset++;
		while (selection.offset < (int)text.size() && !eol(selection.offset) && !syntax->isboundary(get(selection.offset))) selection.offset++;
		selection.length = 0;
	}
	sanity();
}

void View::boundaryLeft() {
	for (auto& selection: selections) {
		selection.length = 0;
		while (selection.offset > 0 && !sol(selection.offset) && syntax->isboundary(get(selection.offset))) selection.offset--;
		while (selection.offset > 0 && !sol(selection.offset) && !syntax->isboundary(get(selection.offset))) selection.offset--;
	}
	sanity();
}

void View::single() {
	skip = {-1,-1};
	if (selections.size() > 1) {
		selections = {selections.back()};
		if (clips.size() > 1) {
			Clip clip;
			for (auto c: clips) {
				clip.text.insert(clip.text.end(), c.text.begin(), c.text.end());
				clip.text.push_back('\n');
			}
			auto cliptext = std::string({clip.text.begin(), clip.text.end()});
			ImGui::SetClipboardText(cliptext.c_str());
			clips.clear();
			clips = {clip};
		}
	}
}

void View::single(ViewRegion& selection) {
	single();
	selections = {selection};
	sanity();
}

void View::addCursorDown() {
	nav();
	selections.push_back(selections.back());

	auto& selection = selections.back();
	int left = toSol(selection.offset);
	selection.offset += toEol(selection.offset);
	selection.offset++;
	int right = toEol(selection.offset);
	selection.offset += std::min(left, right);
	selection.length = 0;

	sanity();
}

void View::addCursorUp() {
	nav();
	selections.insert(selections.begin(), selections.front());

	auto& selection = selections.front();
	int left = toSol(selection.offset);
	selection.offset -= left;
	selection.offset--;
	selection.offset -= toSol(selection.offset);
	int right = toEol(selection.offset);
	selection.offset += std::min(left, right);
	selection.length = 0;

	sanity();
}

void View::unwind() {
	if (selections.size() < 2) return;
	selections.pop_back();
	sanity();
}

void View::interpret(const std::string& cmd) {
	auto prefix = [&](const std::string& s) {
		return cmd.find(s) == 0;
	};

	auto find = [&](int from, const std::string& needle, std::function<bool(const std::string& needle, int offset)> match) {
		selections.clear();

		for (int i = from; !selections.size() && i < (int)text.size(); i++) {
			if (match(needle, i)) selections.push_back({i,(int)needle.size()});
		}

		for (int i = 0; !selections.size() && i < from; i++) {
			if (match(needle, i)) selections.push_back({i,(int)needle.size()});
		}

		sanity();
	};

	if (prefix("find ") && cmd.size() > 5U) {
		int from = selections.back().offset;
		auto needle = cmd.substr(5);

		auto match = [&](const std::string& needle, int offset) {
			for (int i = 0; i < (int)needle.size(); i++) {
				if (get(offset+i) != needle[i]) return false;
			}
			return true;
		};

		find(from, needle, match);
		return;
	}

	if (prefix("ifind ") && cmd.size() > 6U) {
		int from = selections.back().offset;
		auto needle = cmd.substr(6);

		auto match = [&](const std::string& needle, int offset) {
			for (int i = 0; i < (int)needle.size(); i++) {
				if (std::toupper(get(offset+i)) != std::toupper(needle[i])) return false;
			}
			return true;
		};

		find(from, needle, match);
		return;
	}

	if (prefix("go ")) {
		int lineno = 0;
		if (text.lines.size() && 1 == std::sscanf(cmd.c_str(), "go %d", &lineno)) {
			lineno = std::max(1, std::min((int)text.lines.size(), lineno));
			selections.clear();
			selections.push_back({(int)text.line_offset(lineno-1), 0});
		}
		sanity();
		return;
	}
}

std::vector<std::string> View::autocomplete() {
	if (selections.size() > 1U) return {};
	int cursor = selections.back().offset;
	return syntax->matches(text, cursor);
}

void View::input() {
	ImGuiIO& io = ImGui::GetIO();

	//bool Alt = !io.KeyCtrl && io.KeyAlt && !io.KeyShift && !io.KeySuper;
	bool Ctrl = io.KeyCtrl && !io.KeyAlt && !io.KeyShift && !io.KeySuper;
	bool Shift = !io.KeyCtrl && !io.KeyAlt && io.KeyShift && !io.KeySuper;

	bool AltShift = !io.KeyCtrl && io.KeyAlt && io.KeyShift && !io.KeySuper;
	bool CtrlShift = io.KeyCtrl && !io.KeyAlt && io.KeyShift && !io.KeySuper;

	if (AltShift && ImGui::IsKeyPressed(KeyMap[KEY_DOWN])) { addCursorDown(); return; }
	if (AltShift && ImGui::IsKeyPressed(KeyMap[KEY_UP])) { addCursorUp(); return; }

	if (CtrlShift && ImGui::IsKeyPressed(KeyMap[KEY_RIGHT])) { selectRightBoundary(); return; }
	if (CtrlShift && ImGui::IsKeyPressed(KeyMap[KEY_LEFT])) { selectLeftBoundary(); return; }
	if (CtrlShift && ImGui::IsKeyPressed(KeyMap[KEY_D])) { dup(); return; }

	if (Shift && ImGui::IsKeyPressed(KeyMap[KEY_RIGHT])) { selectRight(); return; }
	if (Shift && ImGui::IsKeyPressed(KeyMap[KEY_LEFT])) { selectLeft(); return; }
	if (Shift && ImGui::IsKeyPressed(KeyMap[KEY_DOWN])) { selectDown(); return; }
	if (Shift && ImGui::IsKeyPressed(KeyMap[KEY_UP])) { selectUp(); return; }
	if (Shift && ImGui::IsKeyPressed(KeyMap[KEY_TAB])) { outdent(); return; }

	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_RIGHT])) { boundaryRight(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_LEFT])) { boundaryLeft(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_DOWN])) { bumpdown(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_UP])) { bumpup(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_Z])) { undo(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_Y])) { redo(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_X])) { cut(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_C])) { copy(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_V])) { paste(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_D])) { selectNext(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_K])) { selectSkip(); return; }
	if (Ctrl && ImGui::IsKeyPressed(KeyMap[KEY_B])) { unwind(); return; }

	if (Ctrl && ImGui::IsKeyReleased(KeyMap[KEY_S])) { save(); return; }
	if (Ctrl && ImGui::IsKeyReleased(KeyMap[KEY_L])) { reload(); return; }

	bool mods = io.KeyCtrl || io.KeyShift || io.KeyAlt || io.KeySuper;

	if (ImGui::IsKeyReleased(KeyMap[KEY_ESCAPE])) { single(); sanity(); return; }

	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_TAB])) { indent(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_RETURN])) { insert('\n', true); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_UP])) { up(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_DOWN])) { down(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_RIGHT])) { right(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_LEFT])) { left(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_HOME])) { home(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_END])) { end(); return; }

	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_PAGEUP])) { pgup(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_PAGEDOWN])) { pgdown(); return; }

	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_BACKSPACE])) { back(); return; }
	if (!mods && ImGui::IsKeyPressed(KeyMap[KEY_DELETE])) { del(); return; }

	if (mouseOver && (io.MouseWheel > 0.0f || io.MouseWheel < 0.0f)) {
		auto now = std::chrono::system_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now-lastWheel);
		int steps = ms < config.mouse.wheelSpeedStep ? 5: 1;
		if (io.MouseWheel > 0) { for (int i = 0; i < steps; i++) up(); }
		if (io.MouseWheel < 0) { for (int i = 0; i < steps; i++) down(); }
		lastWheel = now;
		return;
	}

	if (!io.KeyCtrl && !io.KeyAlt && !io.KeySuper) {
		for (auto c: io.InputQueueCharacters) insert(c);
		io.ClearInputCharacters();
	}
}

bool View::open(std::string path) {
	// figure out why wifstream doesn't work
	auto in = std::ifstream(path);
	if (!in) return false;

	this->path = path;

	delete syntax;
	auto fpath = std::filesystem::path(path);

	if ((std::set<std::string>{".cc", ".cpp", ".c", ".h"}).count(fpath.extension().string())) {
		syntax = new CPP();
	}
	else
	if ((std::set<std::string>{".scad"}).count(fpath.extension().string())) {
		syntax = new OpenSCAD();
	}
	else {
		syntax = new PlainText();
	}

	text.clear();
	selections.clear();
	batches = 0;
	modified = false;
	undos.clear();
	redos.clear();

	int c = 0;

	while ((c = in.get()) && c != EOF) {
		if (c == 0xc2) {
			text.push_back((c << 8)|in.get());
			continue;
		}
		text.push_back(c);
	}

	int soft = 0;
	int hard = 0;

	for (int i = 0; i < (int)text.size(); i++) {
		if (sol(i) && get(i) == '\t') {
			hard++;
		}
		if (sol(i) && get(i) == ' ') {
			bool match = true;
			for (int j = 1; match && j < tabs.width; j++) {
				match = match && get(i+j) == ' ';
			}
			if (match) soft++;
		}
	}

	if (soft || hard) {
		tabs.hard = hard > soft;
	}

	sanity();
	undos.clear();
	redos.clear();
	return true;
}

void View::save() {
	if (!path.size()) return;
	auto out = std::ofstream(path);
	for (int i = 0; i < (int)text.size(); ) {
		std::vector<int> line;
		while (!eol(i)) line.push_back(text[i++]);
		while (line.size() && iswspace(line.back())) line.pop_back();
		for (int c: line) {
			if (c&0xff00) {
				out << (unsigned char)((c&0xff00)>>8);
				c = c&0xff;
			}
			out << (unsigned char)c;
		}
		out << '\n';
		i++;
	}
	out.close();
	modified = false;
}

void View::reload() {
	single();
	auto selection = selections.front();
	if (path.size()) open(path);
	selections = {selection};
	sanity();
}

void View::draw() {
	int row = 0;
	int col = 0;
	int cursor = 0;
	int selecting = 0;

	auto origin = ImGui::GetCursorPos();
	origin.x += ImGui::GetWindowPos().x;
	origin.y += ImGui::GetWindowPos().y;

	auto cell = ImGui::CalcTextSize("X");
	cell.y = ImGui::GetTextLineHeightWithSpacing();

	auto region = ImGui::GetContentRegionAvail();

	mouseOver = ImGui::IsMouseHoveringRect(origin,(ImVec2){origin.x+region.x, origin.y+region.y});

	w = std::ceil(region.x/cell.x);
	h = std::ceil(region.y/cell.y);

	struct Output {
		int x = 0;
		int y = 0;
		uint fg = 0xffffffff;
		uint bg = 0x00000000;
		std::vector<int> text;
	};

	std::vector<Output> out;

	auto token = Syntax::Token::None;
	auto state = Theme::State::Plain;

	int lineCol = std::ceil(std::log10((int)text.lines.size()+1));
	std::string lineFmt = fmt("%%0%dd ", lineCol);
	int lineNo = top+1;

	int textCol = 0;

	if (selections.size() == 1) {
		auto& selection = selections.back();
		for (int i = selection.offset-toSol(selection.offset); i < selection.offset; i++) {
			textCol += get(i) == '\t' ? tabs.width: 1;
		}
	}

	int leftOffset = std::max(0, textCol-(w-lineCol-2));
	int leftRemaining = 0;

	unsigned int fg = 0xffffffff;
	unsigned int bg = 0x00000000;

	auto emit = [&](int c) {
		if (leftRemaining > 0) {
			leftRemaining--;
			return;
		}
		if (col < w) {
			if (!out.back().text.size()) {
				out.back().fg = fg;
				out.back().bg = bg;
			}

			if (out.back().fg != fg || out.back().bg != bg) {
				out.push_back({col,row,fg,bg,{}});
			}

			out.back().text.push_back(c);
		}
		col++;
	};

	auto format = [&](auto f) {
		fg = f.fg;
		bg = f.bg;
	};

	auto cr = [&]() {
		col = 0;
		row++;
	};

	format(theme.highlight[token][state]);

	cursor = text.line_offset(top);

	// detect large selection starting off screen
	for (auto& selection: selections) {
		if (selection.offset < cursor && selection.offset+selection.length >= cursor) {
			++selecting;
			state = Theme::State::Selected;
			break;
		}
	}

	while (cursor <= (int)text.size() && row < h) {

		if (col == 0) {
			fg = 0x666666ff;
			bg = 0;
			auto line = fmt(lineFmt.c_str(), lineNo++);
			out.push_back({col,row,fg,bg,{line.begin(),line.end()}});
			col = lineCol+1;
			format(theme.highlight[token][state]);
			out.push_back({col,row,fg,bg,{}});
			leftRemaining = leftOffset;
		}

		auto newState = state;

		for (auto& selection: selections) {
			if (selection.offset == cursor) {
				++selecting;
				newState = Theme::State::Selected;
			}
			if (selection.offset + std::max(selection.length,1) == cursor) {
				--selecting;
				if (!selecting) newState = Theme::State::Plain;
			}
		}

		auto newToken = syntax->next(text, cursor, token);

		if (newToken != token || newState != state) {
			token = newToken;
			state = newState;
			format(theme.highlight[token][state]);
		}

		int c = cursor < (int)text.size() ? text[cursor]: '\n';
		cursor++;

		if (c == '\n') {
			if (selecting) {
				emit(' ');
			}
			cr();
			continue;
		}

		if (col < w && c == '\t') {
			format(theme.highlight[Syntax::Token::Comment][selecting ? Theme::State::Selected: Theme::State::Plain]);
			int spaces = std::min(w-col, tabs.width);
			for (int i = 0; i < spaces; i++) emit(i ? 0x20: 0xc2b7);
			format(theme.highlight[token][state]);
			continue;
		}

		emit(c);
	}

	auto max = ImVec2(origin.x+region.x, origin.y+region.y);
	ImGui::GetWindowDrawList()->AddRectFilled(origin, max, ImGui::GetColorU32(ImGuiCol_FrameBg));

	for (auto& chunk: out) {
		if (!chunk.text.size()) continue;

		auto min = (ImVec2){origin.x+chunk.x*cell.x, origin.y+chunk.y*cell.y};
		auto max = ImVec2(min.x+(cell.x*chunk.text.size()), min.y+cell.y);

		max.x = std::min(max.x, origin.x+region.x);
		max.y = std::min(max.y, origin.y+region.y);

		if (chunk.bg) {
			ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::ImColorSRGB(chunk.bg));
		}

		for (uint i = 0; i < chunk.text.size(); i++) {
			ImWchar c = chunk.text[i];
			ImVec2 pos = (ImVec2){min.x+(i*cell.x),min.y};
			if (pos.x+cell.x > origin.x+region.x) break;
			if (pos.y+cell.y > origin.y+region.y) break;
			ensuref(c >= 32, "bad character %d", c);
			if (c>>8 == 0xc2) c &= 0xff;
			ImGui::GetFont()->RenderChar(ImGui::GetWindowDrawList(), -1.0f, pos, ImGui::ImColorSRGB(chunk.fg), c);
		}
	}

	ImGui::SetCursorPos((ImVec2){origin.x+(w*cell.x), origin.y+(h*cell.y)});
}
