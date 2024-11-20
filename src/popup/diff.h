#pragma once

struct DiffPopup : Popup {
	std::string path;
	Repo::Diff diff;
	DiffPopup();
	void setup();
	void render();
};

