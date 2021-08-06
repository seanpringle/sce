#pragma once

struct FilterPopupComplete : FilterPopup {
	std::string prefix;
	FilterPopupComplete();
	void init();
	void chosen(int option);
};
