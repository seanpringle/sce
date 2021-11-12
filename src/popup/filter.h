#pragma once

struct FilterPopup : Popup {
	std::mutex sync;
	bool ready = false;
	bool loading = false;
	bool focus = false;
	char input[100];
	std::vector<std::string> options;
	std::vector<int> visible;
	int selected = 0;
	virtual void init() = 0;
	virtual void chosen(int option) = 0;
	void filterOptions();
	void selectNavigate();
	void setup();
	void render();
};
