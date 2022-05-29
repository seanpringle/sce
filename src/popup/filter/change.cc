#include "change.h"

FilterPopupChange::FilterPopupChange() {
	name = "change";
}

void FilterPopupChange::init() {
	using namespace std::filesystem;

	auto walk = [&](auto path) {
		auto it = directory_iterator(path,
			directory_options::skip_permission_denied
		);

		for (const directory_entry& entry: it) {
			try {
				if (!is_regular_file(entry)) continue;
			} catch (std::filesystem::filesystem_error e) {
				continue;
			}
			auto ext = entry.path().extension().string();
			auto name = entry.path().filename().string();
			if (ext == ".sce-project" || name == ".sce-project") {
				options.push_back(canonical(entry.path().string()).string());
			}
		}
	};

	auto HOME = std::getenv("HOME");

	walk(weakly_canonical(HOME));
	walk(weakly_canonical(fmt("%s/src", HOME)));

	std::sort(options.begin(), options.end());
}

void FilterPopupChange::chosen(int option) {
	project.save();
	project.load(options[option]);
}
