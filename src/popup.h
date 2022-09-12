#pragma once

struct Popup {
	static inline workers crew;

	bool activate = false;
	bool immediate = false;
	std::string name;
	virtual void setup() = 0;
	virtual void render() = 0;
	bool run();
	Popup() { crew.start(1); }
};
