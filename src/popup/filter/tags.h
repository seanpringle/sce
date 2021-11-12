#pragma once

struct FilterPopupTags : FilterPopup {
	View* view;
	std::vector<ViewRegion> regions;
	FilterPopupTags();
	void init();
	void chosen(int option);
};
