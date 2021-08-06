#include "complete.h"

FilterPopupComplete::FilterPopupComplete() {
	name = "complete";
}

void FilterPopupComplete::init() {
	prefix.clear();
	if (project.views.size()) {
		auto view = project.view();
		options = view->autocomplete();
		if (options.size()) {
			prefix = options.front();
			options.erase(options.begin());
			std::snprintf(input, sizeof(input), "%s", prefix.c_str());
		}
	}
}

void FilterPopupComplete::chosen(int option) {
	auto insert = options[option];
	for (int i = 0; i < (int)insert.size(); i++) {
		if (i < (int)prefix.size()) continue;
		project.view()->insert(insert[i]);
	}
}
