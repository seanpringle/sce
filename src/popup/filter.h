#pragma once

struct FilterPopup : Popup {
	std::mutex sync;
	bool ready = false;
	bool loading = false;
	bool focus = false;
	bool scroll = false;
	char input[100];
	std::vector<std::string> options;
	std::vector<int> visible;
	int selected = 0;
	virtual void init() = 0;
	virtual void chosen(int option) = 0;
	virtual void entered();
	virtual bool multiple();
	virtual bool enterable();
	virtual std::string hint();
	void filterOptions();
	void setup();
	void render();
	virtual void renderOptions();
	bool up();
	bool down();
	bool tab();
};
