#pragma once

struct FilterPopupOpen : FilterPopup {
	FilterPopupOpen();
	void init();
	void chosen(int option);
	bool multiple() override { return true; }
};
