#pragma once

#include <deque>
#include <vector>
#include <list>
#include <string>

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

struct View {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	int top = 0;
	std::deque<char> text;
	std::string path;
	bool modified = false;

	struct {
		bool hard = true;
		int width = 4;
	} tabs;

	struct Region {
		int offset = 0;
		int length = 0;
	};

	std::vector<Region> lines;
	std::vector<Region> selections;

	SelectList findTag;
	std::vector<View::Region> tagRegions;
	std::vector<std::string> tagStrings;

	Region skip;

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
		std::string text;
		std::vector<Region> selections;
	};

	std::vector<Change> undos;
	std::vector<Change> redos;

	View();
	void open(std::string path);
	void save();
	void nav();
	void undo();
	void redo();
	void insert(char c, bool autoindent = false);
	void insertTab();
	bool erase();
	char upper(char c);
	char lower(char c);
	void draw();
	void up();
	void down();
	void right();
	void left();
	void home();
	void end();
	void pgup();
	void pgdown();
	void back();
	void del();
	void cut();
	void copy();
	void paste();
	void findTags();
	void filterTags();
	void drawTags();
	void selectRight();
	void selectLeft();
	void selectNext();
	void selectDown();
	void selectUp();
	void selectSkip();
	void intoView(Region& region);
	void intoViewTop(Region& region);
	void boundaryRight();
	void boundaryLeft();
	void addCursorDown();
	void addCursorUp();
	void sanity();
	int toSol(int offset);
	int toEol(int offset);
	char get(int offset);
	bool sol(int offset);
	bool eol(int offset);
	void index();
	void move(int xx, int yy, int ww, int hh);
	void input();
	void single();
};
