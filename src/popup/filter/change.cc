#include "change.h"

FilterPopupChange::FilterPopupChange() {
	name = "change";
}

void FilterPopupChange::init() {
	using namespace std::filesystem;

	auto HOME = std::getenv("HOME");
	auto path = weakly_canonical(HOME);

	auto it = recursive_directory_iterator(path,
		directory_options::skip_permission_denied
	);

	for (const directory_entry& entry: it) {
		if (!is_regular_file(entry)) continue;
		auto ext = entry.path().extension().string();
		auto name = entry.path().filename().string();
		if (ext == ".sce-project" || name == ".sce-project") {
			options.push_back(canonical(entry.path().string()).string());
		}
	}

	std::sort(options.begin(), options.end());
}

void FilterPopupChange::chosen(int option) {
	project.save();
	project.load(options[option]);
}
