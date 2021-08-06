#pragma once

struct Popup {
	bool activate = false;
	std::string name;
	virtual void setup() = 0;
	virtual void render() = 0;
	void run();
};
