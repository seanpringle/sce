#pragma once

struct Popup {
	static inline workers<1> crew;

	bool activate = false;
	bool immediate = false;
	std::string name;
	virtual void setup() = 0;
	virtual void render() = 0;
	bool run();
};
