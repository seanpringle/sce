#pragma once

struct View;
struct ViewRegion;

#include <deque>
#include <vector>
#include <list>
#include <string>
#include "syntax.h"

struct SelectList {
	bool active = false;
	std::string search;
	std::vector<std::string>* all;
	std::vector<int> filtered;
	int selected = 0;
	int chosen = -1;
	void start(std::vector<std::string>* items);
	void input();
	void draw(int xx, int yy, int ww, int hh);
	void filter();
};

struct InputBox {
	bool active = false;
	std::string content;
	void start(std::string prefix);
	void input();
	void draw(int xx, int yy, int ww, int hh);
};

struct ViewRegion {
	int offset;
	int length;
};

struct View {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	int top = 0;
	std::deque<int> text;
	std::string path;
	bool modified = false;
	Syntax* syntax = nullptr;
	std::chrono::time_point<std::chrono::system_clock> lastWheel;

	struct {
		bool hard = true;
		int width = 4;
	} tabs;

	std::vector<ViewRegion> lines;
	std::vector<ViewRegion> selections;

	SelectList findTag;
	std::vector<ViewRegion> tagRegions;
	std::vector<std::string> tagStrings;

	SelectList autoComp;
	std::string autoPrefix;
	std::vector<std::string> autoStrings;

	InputBox prompt;

	ViewRegion skip;

	enum ChangeType {
		Insertion,
		Deletion,
		Navigation,
	};

	uint batches = 0;

	struct Change {
		uint batch = 0;
		ChangeType type = Insertion;
		int offset = 0;
		int length = 0;
		std::vector<int> text;
		std::vector<ViewRegion> selections;
	};

	std::vector<Change> undos;
	std::vector<Change> redos;

	View();
	~View();
	void open(std::string path);
	void save();
	void reload();
	void nav();
	void undo();
	void redo();
	void insert(int c, bool autoindent = false);
	bool erase();
	int upper(int c);
	int lower(int c);
	void draw();
	void up();
	void down();
	void right();
	void left();
	void home();
	void end();
	void pgup();
	void pgdown();
	void back(int c = 0);
	void del(int c = 0);
	void cut();
	void copy();
	void paste();
	bool dup();
	void findTags();
	void filterTags();
	void drawTags();
	void selectRight();
	void selectLeft();
	bool selectNext();
	void selectDown();
	void selectUp();
	void selectSkip();
	void intoView(ViewRegion& region);
	void boundaryRight();
	void boundaryLeft();
	void addCursorDown();
	void addCursorUp();
	void unwind();
	void sanity();
	int toSol(int offset);
	int toEol(int offset);
	int get(int offset);
	bool sol(int offset);
	bool eol(int offset);
	void index();
	void move(int xx, int yy, int ww, int hh);
	void input();
	void single();
	bool indent();
	bool outdent();
	bool autocomplete();
	void interpret();
};
