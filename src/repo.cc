#include "repo.h"

using namespace std::literals::chrono_literals;

namespace {
	bool starts_with(const std::string& str, const std::string& prefix) {
	    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
	}
}

Repo::Repo(const std::filesystem::path& rpath) {
	if (0 != git_repository_open_ext(&repo, rpath.string().c_str(), 0, nullptr)) return;
	if (ok()) {
		path = std::filesystem::weakly_canonical(git_repository_path(repo));
		path = path.parent_path();
	}
}

Repo::~Repo() {
	if (repo) git_repository_free(repo);
}

bool Repo::ok() const {
	return repo;
}

std::string Repo::branch() {
	std::string result;
	if (!ok()) return result;
	git_reference *head = nullptr;
	git_repository_head(&head, repo);
	if (head) {
		const char *branch = git_reference_shorthand(head);
		if (branch) result = branch;
		git_reference_free(head);
	}
	return result;
}

Repo::Status Repo::status(const std::filesystem::path& fpath) {
	auto stamp = std::chrono::system_clock::now();

	if (cacheStatus.count(fpath)) {
		auto& status = cacheStatus[fpath];
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(stamp-status.stamp);
		if (ms < 100ms) return status;
	}

	auto status = Status(*this, fpath);
	cacheStatus[status.path] = status;

	return status;
}

Repo::Status::Status(Repo& repo, const std::filesystem::path& fpath) {
	stamp = std::chrono::system_clock::now();
	path = fpath;
	if (repo.ok() && std::filesystem::exists(path) && starts_with(fpath, repo.path)) {
		std::string rel = fpath.string().substr(repo.path.string().size()+1);
		err = git_status_file(&flags, repo.repo, rel.c_str());
	}
}

bool Repo::Status::ok() const {
	return err == 0;
}

bool Repo::Status::is(unsigned int mask) const {
	return ok() && (flags & mask) != 0;
}

bool Repo::Status::created() const {
	return is(GIT_STATUS_WT_NEW|GIT_STATUS_INDEX_NEW);
}

bool Repo::Status::modified() const {
	return is(GIT_STATUS_WT_MODIFIED|GIT_STATUS_INDEX_MODIFIED);
}

namespace {
	int line_cb(const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload) {
		std::stringstream& ss(*((std::stringstream*)payload));
		char prefix = ' ';

		if (line->origin == GIT_DIFF_LINE_ADDITION) {
			prefix = '+';
		}

		if (line->origin == GIT_DIFF_LINE_DELETION) {
			prefix = '-';
		}

		ss << prefix;

		std::string_view view(line->content, line->content_len);

		for (int i = 0, l = view.size(); i < l; i++) {
			ss << view[i];
			if (view[i] == '\n' && i+1 < l)
				ss << prefix;
		}

		return 0;
	}
}

Repo::Diff::Diff(Repo& repo, const std::filesystem::path& fpath) {
	path = fpath;

	if (repo.ok() && std::filesystem::exists(path) && starts_with(fpath, repo.path)) {
		std::string rel = fpath.string().substr(repo.path.string().size()+1);
		const char* rstr = rel.c_str();

		git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
		git_diff* diff = nullptr;

		opts.context_lines = 3;
		opts.interhunk_lines = 2;
		opts.flags |= GIT_DIFF_DISABLE_PATHSPEC_MATCH;
		opts.pathspec.strings = (char**)&rstr;
		opts.pathspec.count = 1;

		err = git_diff_index_to_workdir(&diff, repo.repo, nullptr, &opts);

		if (!err) {
			std::stringstream ss;
			git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, line_cb, &ss);
			patch = ss.str();
		}

		if (diff) git_diff_free(diff);
	}
}

bool Repo::Diff::ok() const {
	return err == 0;
}

Repo::Diff Repo::diff(const std::filesystem::path& fpath) {
	auto stamp = std::chrono::system_clock::now();

	if (cacheDiff.count(fpath)) {
		auto& status = cacheDiff[fpath];
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(stamp-status.stamp);
		if (ms < 100ms) return status;
	}

	auto diff = Diff(*this, fpath);
	cacheDiff[diff.path] = diff;

	return diff;
}

Repo* Repo::open(const std::filesystem::path& opath) {
	for (auto& repo: repos) {
		if (opath == repo.path) return &repo;
		if (starts_with(opath, repo.path)) return &repo;
	}
	auto& repo = repos.emplace_back(opath);
	if (repo.ok()) return &repo;
	repos.pop_back();
	return nullptr;
}
