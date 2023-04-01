#pragma once

#include <git2.h>
#include <filesystem>
#include <deque>
#include <map>

struct Repo {
	git_repository *repo = nullptr;
	std::filesystem::path path;

	Repo(const std::filesystem::path& rpath);
	~Repo();
	bool ok() const;
	std::string branch();

	struct Status {
		int err = -1;
		unsigned int flags = 0;
		std::filesystem::path path;
		std::chrono::time_point<std::chrono::system_clock> stamp;
		Status() = default;
		Status(Repo& repo, const std::filesystem::path& fpath);
		bool ok() const;
		bool is(unsigned int mask) const;
		bool created() const;
		bool modified() const;
	};

	Status status(const std::filesystem::path& fpath);

	struct Diff {
		int err = -1;
		std::filesystem::path path;
		std::chrono::time_point<std::chrono::system_clock> stamp;
		std::string patch;
		Diff() = default;
		Diff(Repo& repo, const std::filesystem::path& fpath);
		bool ok() const;
	};

	Diff diff(const std::filesystem::path& fpath);

	static inline std::deque<Repo> repos;
	static Repo* open(const std::filesystem::path& path);

	static inline std::map<std::filesystem::path,Status> cacheStatus;
	static inline std::map<std::filesystem::path,Diff> cacheDiff;
};

