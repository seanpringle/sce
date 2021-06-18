#include "common.h"
#include "view.h"
#include "tui.h"
#include "syntax.h"
#include "theme.h"
#include "config.h"
#include <fstream>

extern TUI tui;
extern Syntax syntax;
extern Theme theme;
extern Config config;

View::View() {
	sanity();
	tabs.hard = config.tabs.hard;
	tabs.width = config.tabs.width;
}

void View::move(int xx, int yy, int ww, int hh) {
	x = xx;
	y = yy;
	w = ww;
	h = hh;
}

void View::sanity() {
	if (!text.size()) text = {'\n'};

	index();

	for (auto it = selections.begin(); it != selections.end(); ) {
		auto& selection = *it;
		selection.length = std::max(0, std::min((int)text.size() - selection.offset, selection.length));
		if ((int)selections.size() > 1 && (selection.offset < 0 || selection.offset > (int)text.size())) {
			it = selections.erase(it);
		} else {
			++it;
		}
	}

	for (auto& selection: selections) {
		if (selection.offset < 0 || selection.offset+selection.length > (int)text.size()) {
			selection.offset = std::max(0, std::min((int)text.size(), selection.offset));
			selection.length = 0;
		}
	}

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

char View::get(int offset) {
	return offset >= 0 && (int)text.size() > offset ? text[offset]: 0;
}

bool View::sol(int offset) {
	return offset <= 0 || get(offset-1) == '\n';
}

bool View::eol(int offset) {
	return offset >= (int)text.size() || get(offset) == '\n';
}

void View::index() {
	lines.clear();
	int offset = 0;
	Region line = {0,0};
	for (auto c: text) {
		line.length++;
		if (c == '\n') {
			lines.push_back(line);
			line = {offset+1,0};
		}
		offset++;
	}
	top = std::max(0, std::min(top, (int)lines.size()));
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

		selections = change.selections;

		if (change.type == Insertion) {
			// find the spot and remove the region
			auto it = text.begin()+change.offset;
			std::string s = {it,it+change.length};
			ensure(s == change.text);
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
			std::string s = {it,it+change.length};
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
			for (int j = i+1; j < (int)selections.size(); j++) {
				selections[j].offset -= selection.length;
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

void View::insert(char c, bool autoindent) {
	erase();
	for (int i = 0; i < (int)selections.size(); i++) {
		auto& selection = selections[i];
		auto it = text.begin()+selection.offset;

		if ((int)selections.size() == 1
			&& undos.size() > 0
			&& undos.back().type == Insertion
			&& undos.back().selections.size() == 1
			&& undos.back().offset+undos.back().length == selection.offset
		){
			undos.back().length++;
			undos.back().text += c;
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

			std::string indent;
			while (isspace(text[start]) && text[start] != '\n') {
				indent += text[start++];
			}

			text.insert(it, indent.begin(), indent.end());
			inserted += indent.size();
		}

		for (int j = i; j < (int)selections.size(); j++) {
			auto& selection = selections[j];
			selection.offset += inserted;
		}
	}
	batches++;
	modified = true;
	sanity();
}

void View::insertTab() {
	if (tabs.hard) {
		insert('\t');
		return;
	}
	for (int i = 0; i < tabs.width; i++) {
		insert(' ');
	}
}

void View::back() {
	if (!erase()) {
		for (int i = 0; i < (int)selections.size(); i++) {
			auto& selection = selections[i];
			if (!selection.offset) continue;
			auto it = text.begin()+selection.offset;

			if ((int)selections.size() == 1
				&& undos.size() > 0
				&& undos.back().type == Deletion
				&& undos.back().selections.size() == 1
				&& undos.back().offset == selection.offset
			){
				undos.back().offset--;
				undos.back().length++;
				undos.back().text.insert(0, 1, text[undos.back().offset]);
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
			for (int j = i; j < (int)selections.size(); j++) {
				auto& selection = selections[j];
				selection.offset -= 1;
			}
		}
		batches++;
		modified = true;
		sanity();
	}
}

void View::del() {
	if (!erase()) {
		for (int i = 0; i < (int)selections.size(); i++) {
			auto& selection = selections[i];
			auto it = text.begin()+selection.offset;

			if ((int)selections.size() == 1
				&& undos.size() > 0
				&& undos.back().type == Deletion
				&& undos.back().selections.size() == 1
				&& undos.back().offset == selection.offset
			){
				undos.back().length++;
				undos.back().text += text[undos.back().offset];
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
			for (int j = i+1; j < (int)selections.size(); j++) {
				auto& selection = selections[j];
				selection.offset -= 1;
			}
		}
		batches++;
		modified = true;
		sanity();
	}
}

void View::cut() {
	if (selections.size() > 1) return;
	auto& selection = selections.front();
	if (selection.length > 0) {
		auto it = text.begin()+selection.offset;
		tui.clip.text = {it, it+selection.length};
		tui.clip.line = false;
		del();
	} else {
		int left = toSol(selection.offset);
		int right = toEol(selection.offset);
		auto it = text.begin()+selection.offset-left;
		tui.clip.text = {it, it+left+right};
		tui.clip.text += '\n';
		tui.clip.line = true;
		for (int i = 0; i < left; i++) back();
		for (int i = 0; i < right+1; i++) del();
	}
	modified = true;
	sanity();
}

void View::copy() {
	if (selections.size() > 1) return;
	auto& selection = selections.front();
	if (selection.length > 0) {
		auto it = text.begin()+selection.offset;
		tui.clip.text = {it, it+selection.length};
		tui.clip.line = false;
	} else {
		int left = toSol(selection.offset);
		int right = toEol(selection.offset);
		auto it = text.begin()+selection.offset-left;
		tui.clip.text = {it, it+left+right};
		tui.clip.text += '\n';
		tui.clip.line = true;
	}
}

void View::paste() {
	if (selections.size() > 1) return;
	auto& selection = selections.front();
	if (selection.length > 0) del();
	if (tui.clip.line) {
		int left = toSol(selection.offset);
		selection.offset -= left;
	}
	for (auto c: tui.clip.text) insert(c, false);
}

void View::findTags() {
	tagRegions = syntax.tags(text);
	tagStrings.clear();
	for (auto& region: tagRegions) {
		auto it = text.begin()+region.offset;
		tagStrings.push_back({it, it+region.length});
	}
	findTag.start(&tagStrings);
}

char View::upper(char c) {
	return toupper(c);
}

char View::lower(char c) {
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

void View::down() {
	for (auto& selection: selections) {
		int left = toSol(selection.offset);
		selection.offset += toEol(selection.offset);
		selection.offset++;
		int right = toEol(selection.offset);
		selection.offset += std::min(left, right);
		selection.length = 0;
	}
	sanity();
}

void View::right() {
	for (auto& selection: selections) {
		selection.offset++;
		selection.length = 0;
	}
	sanity();
}

void View::left() {
	for (auto& selection: selections) {
		selection.offset--;
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
	for (int i = 0; i < h; i++) up();
	sanity();
}

void View::pgdown() {
	single();
	for (int i = 0; i < h; i++) down();
	sanity();
}

void View::selectRight() {
	for (auto& selection: selections) {
		selection.length++;
		if (selection.length == 1) selection.length = 2;
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

void View::selectNext() {
	nav();
	bool match = false;
	auto& selection = selections.back();
	selection.length = std::max(selection.length, 1);
	for (int i = selection.offset+1; i < (int)text.size()-selection.length && !match; i++) {
		auto a = text.begin()+selection.offset;
		auto b = text.begin()+i;
		match = true;
		for (int j = 0; j < selection.length && match; j++) {
			match = match && (*a == *b);
			++a; ++b;
		}
		if (match) {
			if (selection.offset == skip.offset && selection.length == skip.length)
				selections.pop_back();
			selections.push_back({i,selection.length});
			skip = {-1,-1};
		}
	}
	if (!match) {
		for (int i = 0; i < (int)text.size()-selection.length && !match; i++) {
			if (i >= selection.offset) break;
			bool duplicate = false;
			for (auto& other: selections) {
				duplicate = duplicate || other.offset == i;
			}
			if (duplicate) continue;
			auto a = text.begin()+selection.offset;
			auto b = text.begin()+i;
			match = true;
			for (int j = 0; j < selection.length && match; j++) {
				match = match && (*a == *b);
				++a; ++b;
			}
			if (match) {
				if (selection.offset == skip.offset && selection.length == skip.length)
					selections.pop_back();
				selections.push_back({i,selection.length});
				skip = {-1,-1};
			}
		}
	}
	sanity();
}

void View::selectSkip() {
	skip = selections.back();
}

void View::intoView(Region& selection) {
	int lineno = 0;
	for (auto& line: lines) {
		if (line.offset <= selection.offset) lineno++;
	}

	while (lineno+10 > top+h) {
		top++;
	}

	while (lineno-10 <= top) {
		top--;
	}

	top = std::max(0, std::min((int)lines.size()-1, top));
}

void View::intoViewTop(Region& selection) {
	int lineno = 0;
	for (auto& line: lines) {
		if (line.offset <= selection.offset) lineno++;
	}
	top = lineno-10;
	top = std::max(0, std::min((int)lines.size()-1, top));
}

void View::boundaryRight() {
	auto isboundary = [&](int c) {
		return c != '_' && (iscntrl(c) || ispunct(c) || isspace(c));
	};

	for (auto& selection: selections) {
		while (selection.offset < (int)text.size() && !eol(selection.offset) && isboundary(get(selection.offset))) selection.offset++;
		while (selection.offset < (int)text.size() && !eol(selection.offset) && !isboundary(get(selection.offset))) selection.offset++;
		selection.length = 0;
	}
	sanity();
}

void View::boundaryLeft() {
	auto isboundary = [&](int c) {
		return c != '_' && (iscntrl(c) || ispunct(c) || isspace(c));
	};

	for (auto& selection: selections) {
		selection.length = 0;
		while (selection.offset > 0 && !sol(selection.offset) && isboundary(get(selection.offset))) selection.offset--;
		while (selection.offset > 0 && !sol(selection.offset) && !isboundary(get(selection.offset))) selection.offset--;
	}
	sanity();
}

void View::single() {
	skip = {-1,-1};
	selections = {selections.back()};
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

void View::input() {
	if (findTag.active) {
		findTag.input();
		if (!findTag.active && findTag.chosen >= 0) {
			selections = {tagRegions[findTag.filtered[findTag.chosen]]};
			int lineno = 0;
			auto& selection = selections.back();
			for (auto& line: lines) {
				if (line.offset <= selection.offset) lineno++;
			}
			top = std::max(0, lineno-10);
			sanity();
		}
		return;
	}

	if (tui.keys.esc && selections.size() > 1) { single(); sanity(); return; }

	if (tui.keys.alt && tui.keys.shift && tui.keysym == "Down") { addCursorDown(); return; }
	if (tui.keys.alt && tui.keys.shift && tui.keysym == "Up") { addCursorUp(); return; }

	if (tui.keys.ctrl && tui.keysym == "Right") { boundaryRight(); return; }
	if (tui.keys.ctrl && tui.keysym == "Left") { boundaryLeft(); return; }

	if (tui.keys.shift && tui.keysym == "Right") { selectRight(); return; }
	if (tui.keys.shift && tui.keysym == "Left") { selectLeft(); return; }

	if (tui.keys.shift && tui.keysym == "Down") { selectDown(); return; }
	if (tui.keys.shift && tui.keysym == "Up") { selectUp(); return; }

	if (tui.keys.ctrl && tui.keysym == "I") { insertTab(); return; }
	if (tui.keys.ctrl && tui.keysym == "M") { insert('\n', true); return; }
	if (tui.keys.ctrl && tui.keysym == "Z") { undo(); return; }
	if (tui.keys.ctrl && tui.keysym == "Y") { redo(); return; }
	if (tui.keys.ctrl && tui.keysym == "X") { cut(); return; }
	if (tui.keys.ctrl && tui.keysym == "C") { copy(); return; }
	if (tui.keys.ctrl && tui.keysym == "V") { paste(); return; }
	if (tui.keys.ctrl && tui.keysym == "D") { selectNext(); return; }
	if (tui.keys.ctrl && tui.keysym == "K") { selectSkip(); return; }
	if (tui.keys.ctrl && tui.keysym == "R") { findTags(); return; }
	if (tui.keys.ctrl && tui.keysym == "S") { save(); return; }

	if (!tui.keys.mods && tui.keysym == "Up") { up(); return; }
	if (!tui.keys.mods && tui.keysym == "Down") { down(); return; }
	if (!tui.keys.mods && tui.keysym == "Right") { right(); return; }
	if (!tui.keys.mods && tui.keysym == "Left") { left(); return; }
	if (!tui.keys.mods && tui.keysym == "Home") { home(); return; }
	if (!tui.keys.mods && tui.keysym == "End") { end(); return; }

	if (!tui.keys.mods && tui.keysym == "PageUp") { pgup(); return; }
	if (!tui.keys.mods && tui.keysym == "PageDown") { pgdown(); return; }

	if (!tui.keys.mods && tui.keys.back) { back(); return; }
	if (!tui.keys.mods && tui.keys.del) { del(); return; }

	if (tui.escseq.size()) return;

	if (!tui.keys.ctrl && tui.keysym.size() == 1) { insert(tui.keysym.front()); return; }
}

void View::open(std::string path) {
	this->path = path;

	text.clear();
	lines.clear();
	selections.clear();
	batches = 0;
	undos.clear();
	redos.clear();

	auto in = std::ifstream(path);

	for (std::string line; std::getline(in, line);) {
		for (auto c: line) text.push_back(c);
		text.push_back('\n');
	}

	in.close();

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
}

void View::save() {
	if (!path.size()) return;
	auto out = std::ofstream(path);
	for (int i = 0; i < (int)text.size(); ) {
		std::string line;
		while (!eol(i)) line.push_back(text[i++]);
		while (isspace(line.back())) line = line.substr(0, line.size()-1);
		out << line << "\n";
		i++;
	}
	out.close();
	modified = false;
}

void View::draw() {
	std::string blank;
	blank.resize(w, ' ');

	int row = 0;
	int col = 0;
	int cursor = 0;
	int selecting = 0;

	auto token = Syntax::Token::None;
	auto state = Theme::State::Plain;

	int lineCol = std::ceil(std::log10((int)lines.size()+1));
	std::string lineFmt = fmt("%s\e[38;5;238m%%0%dd ", theme.highlight[token][state].bg, lineCol);
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

	tui.to(x,y);
	tui.print("\e[38;5;255m");
	tui.print("\e[48;5;22m");

	auto left = fmt(" %s %s", path, tui.escseq);
	auto right = fmt("%dkB %s ", text.size()/1024U, modified ? "modified": "saved");

	tui.print(left);
	if ((int)(left.size()+right.size()) < w) {
		tui.print(blank.substr(left.size()+right.size()));
	}
	tui.print(right);

	tui.format(theme.highlight[token][state]);

	if (findTag.active) {
		findTag.draw(x, y+1, w, h-1);
		return;
	}

	row++;
	col = 0;

	tui.to(x+col,y+row);
	cursor = lines[top].offset;

	auto emit = [&](int c) {
		if (leftRemaining > 0) {
			leftRemaining--;
			return;
		}
		if (col < w) {
			tui.emit(c);
		}
		col++;
	};

	auto eraseEol = [&]() {
		int spaces = std::max(0, w-col);
		tui.print(blank.substr(w-spaces));
		col += spaces;
	};

	while (cursor <= (int)text.size() && row < h) {

		if (col == 0) {
			tui.to(x+col,y+row);
			tui.print(fmt(lineFmt.c_str(), lineNo++));
			col += lineCol+1;
			tui.format(theme.highlight[token][state]);
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

		auto newToken = syntax.next(text, cursor, token);

		if (newToken != token || newState != state) {
			token = newToken;
			state = newState;
			tui.format(theme.highlight[token][state]);
		}

		char c = cursor < (int)text.size() ? text[cursor]: '\n';
		cursor++;

		if (c == '\n') {
			if (selecting) {
				emit(' ');
				if (col < w) {
					tui.format(theme.highlight[Syntax::Token::None][Theme::State::Plain]);
					eraseEol();
					tui.format(theme.highlight[token][state]);
				}
			}
			else {
				eraseEol();
			}
			row++;
			col = 0;
			continue;
		}

		if (col < w && c == '\t') {
			if (selecting) {
				emit(' ');
				tui.format(theme.highlight[Syntax::Token::None][Theme::State::Plain]);
				int spaces = std::min(w-col, tabs.width-1);
				for (int i = 0; i < spaces; i++) emit(' ');
				tui.format(theme.highlight[token][state]);
			}
			else {
				int spaces = std::min(w-col, tabs.width);
				for (int i = 0; i < spaces; i++) emit(' ');
			}
			continue;
		}

		emit(c);
	}

	tui.format(theme.highlight[Syntax::Token::None][Theme::State::Plain]);

	while (row < h) {
		tui.to(x+col,y+row);
		eraseEol();
		row++;
		col = 0;
	}
}

void SelectList::start(std::vector<std::string>* items) {
	active = true;
	search = "";
	selected = 0;
	chosen = -1;
	all = items;
	filter();
}

void SelectList::input() {
	if (tui.keys.esc) {
		active = false;
		return;
	}

	if (!tui.keys.ctrl && tui.keys.back && search.size()) {
		search = search.substr(0, search.size()-1);
		filter();
		return;
	}

	if (!tui.keys.ctrl && tui.keysym.size() == 1) {
		search += tui.keysym;
		filter();
		return;
	}

	if (tui.keys.ctrl && tui.keysym == "M" && filtered.size() > 0) {
		chosen = selected;
		active = false;
		return;
	}

	if (tui.keysym == "Down") {
		selected = std::max(0, std::min(selected+1, ((int)filtered.size())-1));
		return;
	}

	if (tui.keysym == "Up") {
		selected = std::max(0, std::min(selected-1, ((int)filtered.size())-1));
		return;
	}
}

void SelectList::filter() {
	filtered.clear();
	for (int i = 0; i < (int)all->size(); i++) {
		auto& item = (*all)[i];
		if (!search.size() || item.find(search) != std::string::npos) {
			filtered.push_back(i);
		}
	}
	selected = std::max(0, std::min(((int)filtered.size())-1, selected));
}

void SelectList::draw(int x, int y, int w, int h) {
	std::string blank;
	blank.resize(w, ' ');

	int col = 0;
	int row = 0;
	tui.to(x+col, y+row);

	auto format = [&]() {
		tui.print("\e[38;5;255m\e[48;5;235m");
	};

	format();

	tui.emit('>');
	col++;
	int swidth = std::min((int)search.size(), w-2);
	tui.print(search, swidth);
	col += swidth;
	tui.print(blank.substr(col));
	row++;
	col = 0;
	tui.to(x+col,y+row);

	for (int i = 0; i < (int)filtered.size() && row < h; i++) {
		auto& item = (*all)[filtered[i]];

		tui.emit(' ');
		col++;

		if (selected == i) {
			tui.format(theme.highlight[Syntax::Token::None][Theme::State::Selected]);
		}
		else
		if (std::find(item.begin(), item.end(), '*') != item.end()) {
			tui.print("\e[38;5;226m");
		}

		int iwidth = std::min((int)item.size(), w-col-1);
		tui.print(item, iwidth);
		col += iwidth;

		format();

		tui.print(blank.substr(col));
		row++;
		col = 0;
		tui.to(x+col,y+row);
	}

	format();

	while (row < h) {
		tui.print(blank.substr(col));
		row++;
		col = 0;
		tui.to(x+col,y+row);
	}
}
