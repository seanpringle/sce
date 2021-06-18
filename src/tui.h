#pragma once

#include <cstdio>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <poll.h>
#include <string>
#include <sstream>

struct TUI {
	struct termios old_tio, new_tio;
	std::streambuf* oldErr = nullptr;
	std::stringstream err;
	std::string out;

	struct {
		std::string text;
		bool line;
	} clip;

	inline static const char* ERASE = "\e[K";
	inline static const char* CURSOR_ON = "\e[?25h";
	inline static const char* CURSOR_OFF = "\e[?25l";
	inline static const char* CURSOR_SAVE = "\e7";
	inline static const char* CURSOR_BACK = "\e8";
	inline static const char* WRAP_ON = "\e[?7h";
	inline static const char* WRAP_OFF = "\e[?7l";
	inline static const char* CLEAR = "\e[;H";

	struct {
		bool mods = false;
		bool esc = false;
		bool ctrl = false;
		bool alt = false;
		bool shift = false;
		bool home = false;
		bool end = false;
		bool del = false;
		bool ins = false;
		bool back = false;
		bool tab = false;
		bool pgup = false;
		bool pgdown = false;
		bool up = false;
		bool down = false;
		bool left = false;
		bool right = false;
		bool f1 = false;
		bool f2 = false;
		bool f3 = false;
		bool f4 = false;
		bool f5 = false;
		bool f6 = false;
		bool f7 = false;
		bool f8 = false;
		bool f9 = false;
		bool f10 = false;
		bool f11 = false;
		bool f12 = false;
	} keys;

	struct {
		bool is = false;
		bool enabled = false;
		int col = 0;
		int row = 0;
		int wheel = 0;
		bool left = false;
		bool right = false;
	} mouse;

	int keycode = 0;
	std::string keysym;
	std::string escseq;

	struct Format {
		std::string fg;
		std::string bg;
		void emit();
	};

	void emit(char c);

	int print(std::string s, int len = -1);

	void flush();

	void to (int x, int y);

	int cols();

	int rows();

	int key();

	bool keyPeek();

	void accept();

	void format(Format f);

	void clear();

	TUI();
	~TUI();
	void start();
	void stop();
};
