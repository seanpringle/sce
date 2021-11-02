#pragma once

struct SetupPopup : Popup {
	char setupProjectSavePath[100];
	char setupProjectAddSearchPath[100];
	char setupProjectAddIgnorePath[100];
	char setupProjectAddIgnorePattern[100];
	SetupPopup();
	void setup();
	void render();
};
