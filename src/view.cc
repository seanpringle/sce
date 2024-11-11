#include "common.h"
#include "view.h"
#include "syntax.h"
#include "syntax/cpp.h"
#include "syntax/ini.h"
#include "syntax/yaml.h"
#include "syntax/xml.h"
#include "syntax/make.h"
#include "syntax/cmake.h"
#include "syntax/openscad.h"
#include "syntax/javascript.h"
#include "syntax/bash.h"
#include "syntax/docker.h"
#include "syntax/forth.h"
#include "syntax/rela.h"
#include "syntax/haxe.h"
#include "syntax/plaintext.h"
#include "theme.h"
#include "config.h"
#include "keys.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <functional>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl2.h"

extern Theme theme;
extern Config config;

#include "flate.cc"

using namespace std::literals::chrono_literals;

View::View() {
	sanity();
	tabs.hard = config.tabs.hard;
	tabs.width = config.tabs.width;
	syntax = new PlainText();
}

View::View(const View& other) : View() {
	operator=(other);
}

View& View::operator=(const View& other) {
	path = other.path;
	text = other.text;
	selections = other.selections;
	modified = other.modified;
	undos.clear();
	redos.clear();
	autosyntax();
	sanity();
	return *this;
}

View::~View() {
	delete syntax;
	syntax = nullptr;
}

void View::sanity() {
	if (!text.size()) text = {'\n'};

	top = std::max(0, std::min(top, (int)text.lines.size()-1));

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

std::string View::extract(ViewRegion region) {
	return text.extract(region.offset, region.length);
}

void View::nav() {
	Change change;
	change.type = Navigation;
	change.selections = selections;
	undos.push_back(change);
	redos.clear();
}

void View::snap() {
	redos.clear();
	Change change;
	change.type = SnapShot;
	change.selections = selections;
	change.text = deflate(text.exportRaw(), defl);
	undos.push_back(change);
}

bool View::insertion() {
	return undos.size() && undos.back().type == SnapShot && undos.back().text.size/sizeof(int) < text.size();
}

bool View::deletion() {
	return undos.size() && undos.back().type == SnapShot && undos.back().text.size/sizeof(int) > text.size();
}

void View::undo() {
	if (!undos.size()) return;
	modified = true;

	auto& undo = undos.back();

	if (undo.type == SnapShot) {
		Change redo;
		redo.type = SnapShot;
		redo.selections = selections;
		redo.text = deflate(text.exportRaw(), defl);
		redos.push_back(redo);
		text.importRaw(inflate(undo.text));
		selections = undo.selections;
	}

	if (undo.type == Navigation) {
		Change redo;
		redo.type = Navigation;
		redo.selections = selections;
		redos.push_back(redo);
		selections = undo.selections;
	}

	undos.pop_back();
	if (!undos.size()) modified = !(inflate(orig) == text.exportRaw());

	sanity();
}

void View::redo() {
	if (!redos.size()) return;
	modified = true;

	auto& redo = redos.back();

	if (redo.type == SnapShot) {
		Change undo;
		undo.type = SnapShot;
		undo.selections = selections;
		undo.text = deflate(text.exportRaw(), defl);
		undos.push_back(undo);
		text.importRaw(inflate(redo.text));
		selections = redo.selections;
	}

	if (redo.type == Navigation) {
		Change undo;
		undo.type = Navigation;
		undo.selections = selections;
		undos.push_back(undo);
		selections = redo.selections;
	}

	redos.pop_back();
	sanity();
}

bool View::erase() {
	snap();
	bool erased = false;

	for (int i = 0; i < (int)selections.size(); i++) {
		auto& selection = selections[i];

		if (selection.offset < (int)text.size()
			&& selection.length > 0
			&& selection.offset+selection.length <= (int)text.size()
		){
			erased = true;
			auto it = text.begin()+selection.offset;

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
	if (!erased) {
		undos.pop_back();
	}
	if (erased) {
		sanity();
		modified = true;
	}
	return erased;
}

void View::insertAt(ViewRegion selection, int c, bool autoindent) {
	auto it = text.begin()+selection.offset;

	int inserted = 1;
	it = text.insert(it, c)+1;

	if (autoindent && c == '\n') {
		int left = toSol(selection.offset);
		int start = selection.offset-left;

		std::vector<uint32_t> indent;
		while (iswspace(text[start]) && text[start] != '\n') {
			indent.push_back(text[start++]);
		}

		text.insert(it, indent.begin(), indent.end());
		inserted += indent.size();
	}

	for (int j = 0; j < (int)selections.size(); j++) {
		auto& jselection = selections[j];
		if (jselection.offset >= selection.offset) {
			jselection.offset += inserted;
		}
	}
}

void View::insert(int c, bool autoindent) {
	if ((!erase() && !insertion()) || c == '\n') snap();
	for (auto& selection: selections) {
		insertAt(selection, c, autoindent);
	}
	modified = true;
	sanity();
}

bool View::whiteLeading(int offset) {
	if (sol(offset)) return false;
	while (!sol(offset))
		if (!iswspace(get(--offset))) return false;
	return true;
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
	if (!tabs.hard && tabs.width > 1
		&& (int)selections.size() == 1
		&& whiteLeading(selections.front().offset)
	){
		int here = selections.front().offset;
		int offset = toSol(here);
		int steps = offset % tabs.width;
		if (!steps && offset)
			steps = std::min(offset,tabs.width);
		for (int i = 0; i < steps; i++) {
			left(); del(' ');
		}
		sanity();
		return;
	}

	if (!erase()) {
		if (!deletion()) snap();
		for (int i = 0; i < (int)selections.size(); i++) {
			auto selection = selections[i];
			if (!selection.offset) continue;
			auto it = text.begin()+selection.offset;
			if (c && c != get(selection.offset-1)) continue;

			text.erase(it-1, it);
			for (int j = 0; j < (int)selections.size(); j++) {
				auto& jselection = selections[j];
				if (jselection.offset >= selection.offset) {
					jselection.offset -= 1;
				}
			}
		}
		modified = true;
		sanity();
	}
}

void View::delAt(ViewRegion selection) {
	auto it = text.begin()+selection.offset;

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
		if (!deletion()) snap();
		for (int i = 0; i < (int)selections.size(); i++) {
			auto& selection = selections[i];
			if (c && c != get(selection.offset)) continue;
			if (selection.offset == text.size()) continue;
			delAt(selection);
		}
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

	std::string cliptext;
	for (auto& selection: selections) {
		auto it = text.begin()+selection.offset;
		Clip clip;
		clip.text = {it, it+selection.length};
		cliptext = text.extract(it, it+selection.length);
		clip.line = lines.front();
		lines.pop_front();
		clips.push_back(clip);
	}

	if (cliptext.size())
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
	if (!erase()) snap();
	int nclips = clips.size();

	UTF8 clipboard(ImGui::GetClipboardText());

	for (int i = selections.size()-1; i >= 0; --i) {
		auto& selection = selections[i];
		// when single clip/selection, clipboard takes precedence
		auto clipText = nclips > 1 && nclips > i ? clips[i].text: clipboard.codes;
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
	shrink();
	for (auto& selection: selections) {
		while (!sol(selection.offset)) selection.offset--;
	}
	sanity();
}

void View::end() {
	shrink();
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

	auto duplicate = [&](int i) {
		for (auto& other: selections)
			if (other.offset == i) return true;
		return false;
	};

	auto cmp = text.subdoc(pattern.offset, pattern.length);

	auto check = [&](int i) {
		auto a = cmp.begin();
		auto b = text.begin()+i;
		bool match = true;
		for (int j = 0; j < pattern.length && match; j++) {
			match = match && (*a == *b);
			++a; ++b;
		}
		if (match) {
			nav();
			if (marker.offset == skip.offset && marker.length == skip.length)
				selections.pop_back();
			selections.push_back({i,pattern.length});
			skip = {-1,-1};
		}
		return match;
	};

	for (int i = marker.offset+1; i < (int)text.size()-pattern.length && !match; i++) {
		if (!duplicate(i)) match = check(i);
	}
	for (int i = 0; i < pattern.offset && i < (int)text.size()-pattern.length && !match; i++) {
		if (!duplicate(i)) match = check(i);
	}
	sanity();
	return true;
}

void View::selectSkip() {
	skip = selections.back();
}

void View::selectAll() {
	single();
	selections.front() = {0,(int)text.size()};
	sanity();
}

void View::intoView(ViewRegion selection) {
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

void View::single(ViewRegion selection) {
	single();
	selections = {selection};
	sanity();
}

void View::shrink() {
	for (auto& selection: selections) {
		selection.length = 0;
	}
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

bool View::interpret(const std::string& cmd) {
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

	if (prefix("path ") && cmd.size() > 5U) {
		auto path = cmd.substr(5); trim(path);
		if (path.find('/') == std::string::npos) {
			path = fmt("./%s", path);
		}
		auto fpath = std::filesystem::weakly_canonical(path);
		this->path = fpath.string();
		return true;
	}

	if (prefix("find ") && cmd.size() > 5U) {
		int from = selections.back().offset;
		auto needle = cmd.substr(5); trim(needle);

		auto match = [&](const std::string& needle, int offset) {
			for (int i = 0; i < (int)needle.size(); i++) {
				if (get(offset+i) != needle[i]) return false;
			}
			return true;
		};

		find(from, needle, match);
		return true;
	}

	if (prefix("ifind ") && cmd.size() > 6U) {
		int from = selections.back().offset;
		auto needle = cmd.substr(6); trim(needle);

		auto match = [&](const std::string& needle, int offset) {
			for (int i = 0; i < (int)needle.size(); i++) {
				if (std::toupper(get(offset+i)) != std::toupper(needle[i])) return false;
			}
			return true;
		};

		find(from, needle, match);
		return true;
	}

	if (prefix("go ")) {
		int lineno = 0;
		if (text.lines.size() && 1 == std::sscanf(cmd.c_str(), "go %d", &lineno)) {
			lineno = std::max(1, std::min((int)text.lines.size(), lineno));
			selections.clear();
			selections.push_back({(int)text.line_offset(lineno-1), 0});
		}
		sanity();
		return true;
	}

	if (prefix("tabs ")) {
		if (cmd.find("hard ") != std::string::npos) convertTabsHard();

		if (cmd.size() > 10) {
			auto size = cmd.substr(10);
			int width = std::atoi(size.c_str());
			if (width >= 2 && width <= 8) tabs.width = width;
		}

		if (cmd.find("soft ") != std::string::npos) convertTabsSoft();
		return true;
	}

	if (cmd == "trim") {
		trimTailingWhite();
		return true;
	}

	if (cmd == "all") {
		selectAll();
		return true;
	}

	if (cmd == "reload") {
		reload();
		return true;
	}

	if (prefix("syntax ")) {
		auto name = cmd.substr(7);

		if (name == "cpp") {
			delete syntax;
			syntax = new CPP();
			return true;
		}

		if (name == "ini") {
			delete syntax;
			syntax = new INI();
			return true;
		}

		if (name == "yaml") {
			delete syntax;
			syntax = new YAML();
			return true;
		}

		if (name == "xml") {
			delete syntax;
			syntax = new XML();
			return true;
		}

		if (name == "make") {
			delete syntax;
			syntax = new Make();
			return true;
		}

		if (name == "cmake") {
			delete syntax;
			syntax = new CMake();
			return true;
		}

		if (name == "openscad") {
			delete syntax;
			syntax = new OpenSCAD();
			return true;
		}

		if (name == "js") {
			delete syntax;
			syntax = new JavaScript();
			return true;
		}

		if (name == "bash") {
			delete syntax;
			syntax = new Bash();
			return true;
		}

		if (name == "docker") {
			delete syntax;
			syntax = new Docker();
			return true;
		}

		if (name == "forth") {
			delete syntax;
			syntax = new Forth();
			return true;
		}

		if (name == "rela") {
			delete syntax;
			syntax = new Rela();
			return true;
		}

		if (name == "haxe") {
			delete syntax;
			syntax = new Haxe();
			return true;
		}

		if (name == "plaintext") {
			delete syntax;
			syntax = new PlainText();
			return true;
		}
	}

	if (cmd == "lower") {
		convertLower();
		return true;
	}

	if (cmd == "upper") {
		convertUpper();
		return true;
	}

	return false;
}

std::vector<std::string> View::autocomplete() {
	std::vector<std::string> matches;
	for (auto& selection: selections) {
		auto batch = syntax->matches(text, selection.offset);
		matches.insert(matches.end(), batch.begin(), batch.end());
	}
	std::sort(matches.begin(), matches.end());
	matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
	return matches;
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

//	if (Ctrl && ImGui::IsKeyReleased(KeyMap[KEY_A])) { selectAll(); return; }
	if (Ctrl && ImGui::IsKeyReleased(KeyMap[KEY_S])) { save(); return; }
//	if (Ctrl && ImGui::IsKeyReleased(KeyMap[KEY_L])) { reload(); return; }

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
		int steps = (ms < config.mouse.wheelSpeedStep ? 5: 1) * std::abs(io.MouseWheel);
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

void View::autosyntax() {
	delete syntax;
	syntax = nullptr;

	auto fpath = std::filesystem::weakly_canonical(path);
	auto ext = fpath.extension().string();
	auto stem = fpath.stem().string();
	auto name = fpath.filename().string();

	std::string line = text.extract(0, toEol(0));

	auto shebang = [&](std::string sub) {
		return line.find(sub) != std::string::npos;
	};

	if ((std::set<std::string>{".cc", ".cpp", ".cxx", ".c", ".h", ".hpp", ".fs", ".vs", ".ct", ".json", ".php", ".glsl"}).count(ext)) {
		syntax = new CPP();
	}
	else
	if ((std::set<std::string>{".scad"}).count(ext)) {
		syntax = new OpenSCAD();
	}
	else
	if ((std::set<std::string>{".js"}).count(ext)) {
		syntax = new JavaScript();
	}
	else
	if ((std::set<std::string>{".sh"}).count(ext)) {
		syntax = new Bash();
	}
	else
	if ((std::set<std::string>{".ini"}).count(ext)) {
		syntax = new INI();
	}
	else
	if ((std::set<std::string>{".yaml",".yml"}).count(ext)) {
		syntax = new YAML();
	}
	else
	if ((std::set<std::string>{".xml", ".html"}).count(ext)) {
		syntax = new XML();
	}
	else
	if ((std::set<std::string>{".f", ".4th"}).count(ext)) {
		syntax = new Forth();
	}
	else
	if ((std::set<std::string>{".rela", ".lua"}).count(ext)) {
		syntax = new Rela();
	}
	else
	if ((std::set<std::string>{".hx"}).count(ext)) {
		syntax = new Haxe();
	}
	else
	if (name == "CMakeLists.txt") {
		syntax = new CMake();
	}
	else
	if (stem == "Makefile") {
		syntax = new Make();
	}
	else
	if (stem.find("Dockerfile") != std::string::npos) {
		syntax = new Docker();
	}
	else {
		syntax = new PlainText();
	}
}

bool View::open(std::string path) {
	notef("open %s", path);
	auto fpath = std::filesystem::weakly_canonical(path);

	// figure out why wifstream doesn't work
	auto in = std::ifstream(fpath.string());
	if (!in) return false;

	this->path = fpath.string();

	text.clear();
	selections.clear();
	modified = false;
	undos.clear();
	redos.clear();

	std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	in.close();

	for (auto& c: content) if (!c) return false;
	text.push_back(content);

	autosyntax();

	auto tabcfg = syntax->tabs(text);
	tabs.hard = tabcfg.first;
	tabs.width = tabcfg.second;

	orig = deflate(text.exportRaw(), defl);

	sanity();
	return true;
}

void View::convertTabsSoft() {
	snap();
	for (int i = 0; i < (int)text.size(); i++) {
		if (get(i) == '\t') {
			ViewRegion region = {i,0};
			delAt(region);
			for (int j = 0; j < tabs.width; j++) {
				insertAt(region, ' ', false);
			}
		}
	}
	tabs.hard = false;
	modified = true;
	sanity();
}

void View::convertTabsHard() {
	auto indent = [&](int i) {
		for (int j = 0; i+j < (int)text.size() && j < tabs.width; j++) {
			if (get(i+j) != ' ') return false;
		}
		return true;
	};

	for (int i = 0; i < (int)text.size(); i++) {
		if (!i || get(i) == '\n') {
			i++;
			while (i < (int)text.size() && indent(i)) {
				ViewRegion region = {i,0};
				for (int j = 0; j < tabs.width; j++) {
					delAt(region);
				}
				insertAt(region, '\t', false);
				i++;
			}
			i--;
		}
	}
	tabs.hard = true;
	modified = true;
	sanity();
}

void View::convertUpper() {
	auto state = selections;
	for (auto& selection: state) {
		for (int i = 0, l = selection.length; i < l; i++) {
			ViewRegion region = {selection.offset+i,0};
			int c = get(region.offset);
			if (isupper(c)) continue;
			delAt(region);
			insertAt(region, upper(c), false);
		}
	}
	selections = state;
	modified = true;
	sanity();
}

void View::convertLower() {
	auto state = selections;
	for (auto& selection: state) {
		for (int i = 0, l = selection.length; i < l; i++) {
			ViewRegion region = {selection.offset+i,0};
			int c = get(region.offset);
			if (islower(c)) continue;
			delAt(region);
			insertAt(region, lower(c), false);
		}
	}
	selections = state;
	modified = true;
	sanity();
}

void View::trimTailingWhite() {
	std::vector<Doc::Cursor> cursors;
	for (auto& selection: selections) {
		cursors.push_back(text.cursor(selection.offset));
	}
	text.cursor(0);
	for (auto& line: text.lines) {
		while (line.size() > 1 && iswspace(line[line.size()-2])) {
			line.erase(line.begin()+line.size()-2);
			text.count--;
		}
	}
	for (uint i = 0; i < selections.size(); i++) {
		auto& cursor = cursors[i];
		auto& selection = selections[i];
		selection.offset = text.line_offset(cursor.line) + std::min(cursor.cell, (uint)text.lines[cursor.line].size()-1);
	}
	modified = true;
	sanity();
}

std::vector<ViewRegion> View::search(const std::string& needle) {
	std::vector<ViewRegion> hits;
	for (int i = 0, l = text.size(), n = needle.size(); i < l; i++) {
		int j = 0;
		bool hit = get(i) == needle[0];
		for (; hit && j < l && j < n; j++) {
			hit = hit && (needle[j] == get(i+j));
		}
		if (hit && j == n) {
			hits.push_back({i,n});
		}
	}
	return hits;
}

std::string View::selected() {
	return extract(selections.front());
}

std::string View::blurb() {
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now-lastGit);

	if (ms > 100ms) {
		lastGit = now;
		auto repo = Repo::open(path);
		blurbGit = repo ? repo->branch(): "";
	}

	return blurbGit.size() ? blurbGit: "(no branch)";
}

void View::save() {
	if (!path.size()) return;
	trimTailingWhite();
	auto out = std::ofstream(path);
	out << std::string(text);
	out.close();
	modified = false;
	orig = deflate(text.exportRaw(), defl);
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
		std::vector<uint32_t> text;
		bool synhint = false;
		bool selhint = false;
	};

	std::vector<Output> out;

	auto token = Syntax::Token::None;
	auto state = Theme::State::Plain;
	bool synhint = false;
	bool selhint = false;

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

			if (out.back().fg != fg || out.back().bg != bg || out.back().synhint != synhint || out.back().selhint != selhint) {
				out.push_back({col,row,fg,bg,{},synhint,selhint});
			}

			out.back().text.push_back(c);
		}
		col++;
		synhint = false;
		selhint = false;
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
	token = syntax->first(text, cursor);

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
			out.push_back({col,row,fg,bg,{line.begin(),line.end()},false});
			col = lineCol+1;
			format(theme.highlight[token][state]);
			out.push_back({col,row,fg,bg,{},false});
			leftRemaining = leftOffset;
		}

		auto newState = state;

		for (auto& selection: selections) {
			if (selection.offset == cursor) {
				++selecting;
				newState = Theme::State::Selected;
			}
			if (selection.offset + std::max(selection.length,1) == cursor) {
				selecting = std::max(0,selecting-1);
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
		synhint = syntax->hint(text, cursor, selections);
		selhint = selecting && selections.size() > 1;
		cursor++;

		if (c == '\n') {
			if (selecting) {
				emit(' ');
			}
			cr();
			continue;
		}

		if (col < w && c == '\t') {
			format(theme.highlight[Syntax::Token::Indent][selecting ? Theme::State::Selected: Theme::State::Plain]);
			int spaces = std::min(w-col, tabs.width);
			for (int i = 0; i < spaces; i++) emit(i ? 0x20: 0xb7);
			format(theme.highlight[token][state]);
			continue;
		}

		emit(c);
	}

	auto max = ImVec2(origin.x+region.x, origin.y+region.y);

	ImGui::GetWindowDrawList()->AddRectFilled(origin, max,
		ImGui::GetColorU32(theme.highlight[Syntax::Token::None][Theme::State::Plain].bg)
	);

	for (auto& chunk: out) {
		if (!chunk.text.size()) continue;

		auto min = (ImVec2){origin.x+chunk.x*cell.x, origin.y+chunk.y*cell.y};
		auto max = ImVec2(min.x+(cell.x*chunk.text.size()), min.y+cell.y);

		max.x = std::min(max.x, origin.x+region.x);
		max.y = std::min(max.y, origin.y+region.y);

		if (chunk.bg) {
			ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::ImColorSRGB(chunk.bg));
		}

		if (chunk.selhint) {
			ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::ImColorSRGB(0xffff00ff));
		}

		if (chunk.synhint) {
			ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::ImColorSRGB(0xffff00ff));
		}

		for (uint i = 0; i < chunk.text.size(); i++) {
			ImWchar c = chunk.text[i];
			ImVec2 pos = (ImVec2){min.x+(i*cell.x),min.y};
			if (pos.x+cell.x > origin.x+region.x) break;
			if (pos.y+cell.y > origin.y+region.y) break;
			if (c < 32) c = 0xfffd;
			ImGui::GetFont()->RenderChar(ImGui::GetWindowDrawList(), -1.0f, pos, ImGui::ImColorSRGB(chunk.fg), c);
		}
	}

	ImGui::SetCursorPos((ImVec2){origin.x+(w*cell.x), origin.y+(h*cell.y)});
}
