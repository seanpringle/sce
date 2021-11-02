#include "open.h"

FilterPopupOpen::FilterPopupOpen() {
	name = "open";
}

void FilterPopupOpen::init() {
	options = project.files();
	std::sort(options.begin(), options.end());
}

void FilterPopupOpen::chosen(int option) {
	project.open(options[option]);
}
