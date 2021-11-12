#include "tags.h"

FilterPopupTags::FilterPopupTags() {
	name = "tags";
}

void FilterPopupTags::init() {
	regions.clear();
	if (project.views.size()) {
		view = project.view();
		View tmp;
		tmp.open(view->path);
		regions = tmp.syntax->tags(tmp.text);
		for (auto region: regions) {
			options.push_back(tmp.extract(region));
		}
	}
}

void FilterPopupTags::chosen(int option) {
	view->single(regions[option]);
}
