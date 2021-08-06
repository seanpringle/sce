#pragma once

struct CommandPopup : Popup {
	char input[100];
	std::string prefix;
	CommandPopup();
	void setup();
	void render();
};
