#include "repo.h"
#include "common.h"

using namespace std::literals::chrono_literals;

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
		if (branch) result = fmt("%s", branch);
		git_reference_free(head);
	}
	return result;
}

Repo::Status Repo::status(const std::filesystem::path& fpath) {
	auto stamp = std::chrono::system_clock::now();

	if (cache.count(fpath)) {
		auto& status = cache[fpath];
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(stamp-status.stamp);
		if (ms < 100ms) return status;
	}

	auto status = Status(*this, fpath);
	cache[status.path] = status;

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
