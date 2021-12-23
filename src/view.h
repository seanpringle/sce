#pragma once

struct View;
struct ViewRegion;

#include <deque>
#include <vector>
#include <list>
#include <string>
#include <cmath>
#include <chrono>
#include "doc.h"
#include "syntax.h"

struct ViewRegion {
	int offset;
	int length;
};

struct View {
	int w = 0;
	int h = 0;
	int top = 0;
	Doc text;
	std::string path;
	bool modified = false;
	bool mouseOver = false;
	Syntax* syntax = nullptr;
	std::chrono::time_point<std::chrono::system_clock> lastWheel;

	struct {
		bool hard = true;
		int width = 4;
	} tabs;

	std::vector<ViewRegion> selections;

	struct Clip {
		std::vector<int> text;
		bool line = false;
	};

	std::vector<Clip> clips;

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
	View(const View& other);
	View& operator=(const View& other);
	bool open(std::string path);
	bool open(View* other);
	void autosyntax();
	void save();
	void reload();
	void nav();
	void undo();
	void redo();
	void insertAt(ViewRegion& selection, int c, bool autoindent);
	void insert(int c, bool autoindent = false);
	bool erase();
	int upper(int c);
	int lower(int c);
	void draw();
	void up();
	void downAt(ViewRegion& selection);
	void down();
	void right();
	void left();
	void home();
	void end();
	void pgup();
	void pgdown();
	void bumpup();
	void bumpdown();
	void back(int c = 0);
	void delAt(ViewRegion& selection);
	void del(int c = 0);
	void clip();
	void cut();
	void copy();
	void paste();
	bool dup();
	void selectRight();
	void selectRightBoundary();
	void selectLeft();
	void selectLeftBoundary();
	bool selectNext();
	void selectDown();
	void selectUp();
	void selectSkip();
	void selectAll();
	void intoView(const ViewRegion& region);
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
	std::string extract(const ViewRegion& region);
	void move(int xx, int yy, int ww, int hh);
	void input();
	void single();
	void single(ViewRegion& selection);
	void shrink();
	bool indent();
	bool outdent();
	std::vector<std::string> autocomplete();
	bool interpret(const std::string& cmd);
	void convertTabsSoft();
	void convertTabsHard();
	void convertLower();
	void convertUpper();
	void trimTailingWhite();
	std::vector<ViewRegion> search(const std::string& needle);
	std::string selected();
};
