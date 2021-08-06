#pragma once

struct FilterPopupTags : FilterPopup {
	std::vector<ViewRegion> regions;
	FilterPopupTags();
	void init();
	void chosen(int option);
};
