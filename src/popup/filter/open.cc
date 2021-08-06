#include "open.h"

FilterPopupOpen::FilterPopupOpen() {
	name = "open";
}

void FilterPopupOpen::init() {
	using namespace std::filesystem;

	for (auto path: project.searchPaths) {
		auto searchPath = weakly_canonical(path);

		auto it = recursive_directory_iterator(searchPath,
			directory_options::skip_permission_denied
		);

		for (const directory_entry& entry: it) {
			if (!is_regular_file(entry)) continue;
			auto entryPath = weakly_canonical(entry.path().string());

			bool ignore = false;
			for (auto path: project.ignorePaths) {
				auto ignorePath = weakly_canonical(path);
				ignore = ignore || starts_with(entryPath.string(), ignorePath.string());
			}
			if (!ignore) {
				options.push_back(std::filesystem::canonical(entryPath.string()));
			}
		}
	}

	std::sort(options.begin(), options.end());
}

void FilterPopupOpen::chosen(int option) {
	project.open(options[option]);
}
