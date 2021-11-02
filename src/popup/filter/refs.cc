#include "refs.h"

FilterPopupRefs::FilterPopupRefs() {
	name = "refs";
}

void FilterPopupRefs::init() {
	results = project.search(needle);

	std::sort(results.begin(), results.end(), [](auto a, auto b) {
		return a.path < b.path || (a.path == b.path && a.region.offset < b.region.offset);
	});

	for (auto& result: results) {
		auto line = result.line; ltrim(line);
		options.push_back(fmt("%s:%d\n   %s", result.path, result.lineno, line));
	}
}

void FilterPopupRefs::chosen(int option) {
	auto view = project.open(results[option].path);
	view->single(results[option].region);
}
