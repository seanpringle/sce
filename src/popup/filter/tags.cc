#include "tags.h"

FilterPopupTags::FilterPopupTags() {
	name = "tags";
}

void FilterPopupTags::init() {
	regions.clear();
	if (project.views.size()) {
		auto view = project.view();
		auto it = view->text.begin();
		regions = view->syntax->tags(view->text);
		for (auto region: regions) {
			auto tag = std::string(it+region.offset, it+region.offset+region.length);
			options.push_back(tag);
		}
	}
}

void FilterPopupTags::chosen(int option) {
	project.view()->single(regions[option]);
}
