#include "common.h"
#include "tui.h"
#include "config.h"
#include <sstream>

extern Config config;

TUI::TUI() {
}

TUI::~TUI() {
}

void TUI::start() {
	oldErr = std::cerr.rdbuf();
	std::cerr.rdbuf(err.rdbuf());

	tcgetattr(fileno(stdin),&old_tio);
	new_tio = old_tio;
	new_tio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	new_tio.c_oflag &= ~(OPOST);
	new_tio.c_cflag |= (CS8);
	new_tio.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	new_tio.c_cc[VTIME] = 0;
	new_tio.c_cc[VMIN] = 1;
	tcsetattr(fileno(stdin), TCSANOW, &new_tio);

	print(CURSOR_OFF);
	print("\e[?1049h");
	if (config.mouse.enabled)
		print("\e[?1003h\e[?1015h\e[?1006h");
	clear();
}

void TUI::stop() {
	print("\e[?1049l");
	if (config.mouse.enabled)
		print("\e[?1000l");
	print(CURSOR_ON);
	flush();
	tcsetattr(fileno(stdin), TCSANOW, &old_tio);
	std::cerr.rdbuf(oldErr);
	std::cerr << err.str();
}

void TUI::emit(int c) {
	if (c & 0xff00) {
		out.push_back((c&0xff00)>>8);
		c &= 0xff;
	}
	out.push_back(c);
}

int TUI::print(std::string s, int len) {
	if (len < 0) len = s.size();
	len = std::min(len, (int)s.size());
	for (int i = 0; i < len; i++) {
		out.push_back((unsigned char)s[i]);
	}
	return len;
}

void TUI::flush() {
	ensure((int)out.size() == write(fileno(stdout), out.data(), out.size()));
	out.clear();
}

void TUI::to (int x, int y) {
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	x++; y++;

	print(fmt("\e[%d;%dH", y, x));
}

int TUI::cols() {
	struct winsize ws;
	ioctl(fileno(stdin), TIOCGWINSZ, &ws);
	return ws.ws_col;
}

int TUI::rows() {
	struct winsize ws;
	ioctl(fileno(stdin), TIOCGWINSZ, &ws);
	return ws.ws_row;
}

int TUI::key() {
	unsigned char c = 0;
	int bytes = read(fileno(stdin), &c, 1);
	if (bytes < 1) c = 0;
	return c;
}

bool TUI::keyPeek() {
	struct pollfd fds;
	fds.fd = fileno(stdin);
	fds.events = POLLIN;
	fds.revents = 0;
	return (poll(&fds, 1, 0) > 0);
}

void TUI::accept() {
	keys.mods = false;
	keys.esc = false;
	keys.ctrl = false;
	keys.alt = false;
	keys.shift = false;
	keys.home = false;
	keys.end = false;
	keys.del = false;
	keys.ins = false;
	keys.back = false;
	keys.tab = false;
	keys.pgup = false;
	keys.pgdown = false;
	keys.up = false;
	keys.down = false;
	keys.left = false;
	keys.right = false;
	keys.f1 = false;
	keys.f2 = false;
	keys.f3 = false;
	keys.f4 = false;
	keys.f5 = false;
	keys.f6 = false;
	keys.f7 = false;
	keys.f8 = false;
	keys.f9 = false;
	keys.f10 = false;
	keys.f11 = false;
	keys.f12 = false;

	mouse.col = 0;
	mouse.row = 0;
	mouse.wheel = 0;
	mouse.left = false;
	mouse.right = false;
	mouse.is = false;

	keycode = key();
	keysym.clear();
	escseq.clear();

	if (keycode == '\e') {
		[&]() {
			if (!keyPeek()) {
				keys.esc = true;
				return;
			}

			while (keycode == '\e') {
				keycode = key();
			}

			if (isalpha(keycode)) {
				keys.alt = true;
				return;
			}

			if (keycode == '[') {
				while (!isalpha(escseq.back()) && escseq.back() != '~') escseq.push_back(key());

				if (escseq[0] == '<') { // mouse
					int type = 0;
					if (3 == std::sscanf(escseq.c_str(), "<%d;%d;%dm", &type, &mouse.col, &mouse.row)) {
						if (type == 0) mouse.left = true;
						if (type == 2) mouse.right = true;
						if (type == 65) mouse.wheel = 1;
						if (type == 64) mouse.wheel = -1;
					}
					mouse.is = true;
					return;
				}

				// xterm
				if (escseq == "A") { keys.up = true; return; }
				if (escseq == "B") { keys.down = true; return; }
				if (escseq == "C") { keys.right = true; return; }
				if (escseq == "D") { keys.left = true; return; }
				if (escseq == "F") { keys.end = true; return; }
				if (escseq == "H") { keys.home = true; return; }
				if (escseq == "Z") { keys.shift = true; keys.tab = true; return; }
				if (escseq == "1P") { keys.f1 = true; return; }
				if (escseq == "1Q") { keys.f2 = true; return; }
				if (escseq == "1R") { keys.f3 = true; return; }
				if (escseq == "1S") { keys.f4 = true; return; }
				// vt
				if (escseq == "1~") { keys.home = true; return; }
				if (escseq == "2~") { keys.ins = true; return; }
				if (escseq == "3~") { keys.del = true; return; }
				if (escseq == "4~") { keys.end = true; return; }
				if (escseq == "5~") { keys.pgup = true; return; }
				if (escseq == "6~") { keys.pgdown = true; return; }
				if (escseq == "7~") { keys.home = true; return; }
				if (escseq == "8~") { keys.end = true; return; }
				if (escseq == "11~") { keys.f1 = true; return; }
				if (escseq == "12~") { keys.f2 = true; return; }
				if (escseq == "13~") { keys.f3 = true; return; }
				if (escseq == "14~") { keys.f4 = true; return; }
				if (escseq == "15~") { keys.f5 = true; return; }
				if (escseq == "17~") { keys.f6 = true; return; }
				if (escseq == "18~") { keys.f7 = true; return; }
				if (escseq == "19~") { keys.f8 = true; return; }
				if (escseq == "20~") { keys.f9 = true; return; }
				if (escseq == "21~") { keys.f10 = true; return; }
				if (escseq == "23~") { keys.f11 = true; return; }
				if (escseq == "24~") { keys.f12 = true; return; }

				if (escseq == "1;2A") { keys.shift = true; keys.up = true; return; }
				if (escseq == "1;2B") { keys.shift = true; keys.down = true; return; }
				if (escseq == "1;2C") { keys.shift = true; keys.right = true; return; }
				if (escseq == "1;2D") { keys.shift = true; keys.left = true; return; }
				if (escseq == "1;2F") { keys.shift = true; keys.end = true; return; }
				if (escseq == "1;2H") { keys.shift = true; keys.home = true; return; }
				if (escseq == "3;2~") { keys.alt = true; keys.del = true; return; }
				if (escseq == "2;2~") { keys.alt = true; keys.ins = true; return; }
				if (escseq == "5;2~") { keys.alt = true; keys.pgup = true; return; }
				if (escseq == "6;2~") { keys.alt = true; keys.pgdown = true; return; }

				if (escseq == "1;3A") { keys.alt = true; keys.up = true; return; }
				if (escseq == "1;3B") { keys.alt = true; keys.down = true; return; }
				if (escseq == "1;3C") { keys.alt = true; keys.right = true; return; }
				if (escseq == "1;3D") { keys.alt = true; keys.left = true; return; }
				if (escseq == "1;3F") { keys.alt = true; keys.end = true; return; }
				if (escseq == "1;3H") { keys.alt = true; keys.home = true; return; }
				if (escseq == "3;3~") { keys.alt = true; keys.del = true; return; }
				if (escseq == "2;3~") { keys.alt = true; keys.ins = true; return; }
				if (escseq == "5;3~") { keys.alt = true; keys.pgup = true; return; }
				if (escseq == "6;3~") { keys.alt = true; keys.pgdown = true; return; }

				if (escseq == "1;4A") { keys.alt = true; keys.shift = true; keys.up = true; return; }
				if (escseq == "1;4B") { keys.alt = true; keys.shift = true; keys.down = true; return; }
				if (escseq == "1;4C") { keys.alt = true; keys.shift = true; keys.right = true; return; }
				if (escseq == "1;4D") { keys.alt = true; keys.shift = true; keys.left = true; return; }
				if (escseq == "1;4F") { keys.alt = true; keys.shift = true; keys.end = true; return; }
				if (escseq == "1;4H") { keys.alt = true; keys.shift = true; keys.home = true; return; }
				if (escseq == "3;4~") { keys.alt = true; keys.shift = true; keys.del = true; return; }
				if (escseq == "2;4~") { keys.alt = true; keys.shift = true; keys.ins = true; return; }
				if (escseq == "5;4~") { keys.alt = true; keys.shift = true; keys.pgup = true; return; }
				if (escseq == "6;4~") { keys.alt = true; keys.shift = true; keys.pgdown = true; return; }

				if (escseq == "1;5A") { keys.ctrl = true; keys.up = true; return; }
				if (escseq == "1;5B") { keys.ctrl = true; keys.down = true; return; }
				if (escseq == "1;5C") { keys.ctrl = true; keys.right = true; return; }
				if (escseq == "1;5D") { keys.ctrl = true; keys.left = true; return; }
				if (escseq == "1;5F") { keys.ctrl = true; keys.end = true; return; }
				if (escseq == "1;5H") { keys.ctrl = true; keys.home = true; return; }
				if (escseq == "2;5~") { keys.ctrl = true; keys.ins = true; return; }
				if (escseq == "3;5~") { keys.ctrl = true; keys.del = true; return; }
				if (escseq == "5;5~") { keys.ctrl = true; keys.pgup = true; return; }
				if (escseq == "6;5~") { keys.ctrl = true; keys.pgdown = true; return; }

				if (escseq == "1;6A") { keys.ctrl = true; keys.shift = true; keys.up = true; return; }
				if (escseq == "1;6B") { keys.ctrl = true; keys.shift = true; keys.down = true; return; }
				if (escseq == "1;6C") { keys.ctrl = true; keys.shift = true; keys.right = true; return; }
				if (escseq == "1;6D") { keys.ctrl = true; keys.shift = true; keys.left = true; return; }
				if (escseq == "1;6F") { keys.ctrl = true; keys.shift = true; keys.end = true; return; }
				if (escseq == "1;6H") { keys.ctrl = true; keys.shift = true; keys.home = true; return; }
				if (escseq == "2;6~") { keys.ctrl = true; keys.shift = true; keys.ins = true; return; }
				if (escseq == "3;6~") { keys.ctrl = true; keys.shift = true; keys.del = true; return; }
				if (escseq == "5;6~") { keys.ctrl = true; keys.shift = true; keys.pgup = true; return; }
				if (escseq == "6;6~") { keys.ctrl = true; keys.shift = true; keys.pgdown = true; return; }

				// unrecognised sequence
				keys.esc = true;
				keycode = '\e';
				return;
			}
		}();
	}
	else
	if (keycode < 27) {
		keys.ctrl = true;
		keycode += 64;
	}
	else
	if (keycode == 0xC2) {
		keycode = (keycode << 8) | key();
	}

	keys.mods = keys.mods || keys.ctrl || keys.alt || keys.shift;

	if (mouse.is) {
		return;
	}

	if (keycode == 0x7f) {
		keys.back = true;
	}

	if (keys.ctrl && keycode == 'I') {
		keys.ctrl = false;
		keys.tab = true;
	}

	if (keys.esc) keysym = "Escape";
	else if (keys.home) keysym = "Home";
	else if (keys.end) keysym = "End";
	else if (keys.del) keysym = "Delete";
	else if (keys.ins) keysym = "Insert";
	else if (keys.back) keysym = "Back";
	else if (keys.pgup) keysym = "PageUp";
	else if (keys.pgdown) keysym = "PageDown";
	else if (keys.up) keysym = "Up";
	else if (keys.down) keysym = "Down";
	else if (keys.left) keysym = "Left";
	else if (keys.right) keysym = "Right";
	else if (keys.f1) keysym = "F1";
	else if (keys.f2) keysym = "F2";
	else if (keys.f3) keysym = "F3";
	else if (keys.f4) keysym = "F4";
	else if (keys.f5) keysym = "F5";
	else if (keys.f6) keysym = "F6";
	else if (keys.f7) keysym = "F7";
	else if (keys.f8) keysym = "F8";
	else if (keys.f9) keysym = "F9";
	else if (keys.f10) keysym = "F10";
	else if (keys.f11) keysym = "F11";
	else if (keys.f12) keysym = "F12";
	else keysym = fmt("%c", keycode);
}

void TUI::format(Format f) {
	print(f.fg);
	print(f.bg);
}

void TUI::clear() {
	// gnu screen kills this ?
	//print(CLEAR);
	to(0,0);
	for (int i = 0, l = rows(); i < l; i++) {
		to(0,i);
		print(ERASE);
	}
	to(0,0);
	flush();
}
