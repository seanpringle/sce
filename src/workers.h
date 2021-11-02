#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include "channel.h"

template <uint POOL>
class workers {
private:
	std::vector<std::thread> threads;
	channel<std::function<void(void)>,POOL> jobs;
	std::mutex mutex;
	bool active = false;
	uint64_t submitted = 0;
	uint64_t completed = 0;
	std::condition_variable waiting;

	void runner() {
		std::unique_lock<std::mutex> m(mutex);
		m.unlock();

		for (auto job: jobs) {
			job();

			m.lock();
			completed++;
			waiting.notify_all();
			m.unlock();
		}
	}

public:
	workers<POOL>() {
	}

	~workers<POOL>() {
		stop();
	}

	void stop() {
		std::unique_lock<std::mutex> m(mutex);
		if (active) {
			active = false;
			m.unlock();
			jobs.close();
			for (uint i = 0; i < POOL; i++) {
				threads[i].join();
			}
			m.lock();
			threads.clear();
			submitted = 0;
			completed = 0;
		}
		m.unlock();
	}

	// submit a job, starting the threads if necessary
	bool job(std::function<void(void)> fn) {
		std::unique_lock<std::mutex> m(mutex);
		if (!active) {
			active = true;
			for (uint i = 0; i < POOL; i++) {
				threads.push_back(std::thread(&workers<POOL>::runner, this));
			}
		}
		submitted++;
		m.unlock();
		return jobs.send(fn);
	}

	// single-sender pattern
	// wait for jobs already submitted to complete
	void wait() {
		std::unique_lock<std::mutex> m(mutex);
		uint64_t count = submitted;
		while (count > completed) waiting.wait(m);
		m.unlock();
	}
};
