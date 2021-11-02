#pragma once

struct FilterPopupRefs : FilterPopup {
	std::vector<Project::Match> results;
	std::string needle;
	FilterPopupRefs();
	void init();
	void chosen(int option);
};
