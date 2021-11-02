#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <new>

/* Go-like channel

channel<int,-1> unlimitedChannel;
channel<int,0> synchronousChannel;
channel<int,100> limitedChannel;

// write
someChannel.send(7);
someChannel.send(13);

// read fifo
int n = someChannel.recv();

// read iterator until close()
for (int n: someChannel) {
	...
}

// done writing
someChannel.close()

*/

template <class V, int depth = 0>
class channel {
	static_assert(depth >= -1, "channel size invalid");

private:
	std::mutex mutex;
	std::condition_variable sendCond;
	std::condition_variable recvCond;
	std::deque<V> packets;
	uint receivers = 0;
	bool accepting = true;

public:
	channel<V,depth>() {
	}

	~channel<V,depth>() {
		close();
	}

	// close a channel to senders; receivers will still be able to
	// drain any messages already queued
	void close() {
		std::unique_lock<std::mutex> m(mutex);
		if (accepting) {
			accepting = false;
			sendCond.notify_all();
			recvCond.notify_all();
		}
		m.unlock();
	}

	// open a channel to senders
	void open() {
		std::unique_lock<std::mutex> m(mutex);
		if (!accepting) {
			accepting = true;
			sendCond.notify_all();
			recvCond.notify_all();
		}
		m.unlock();
	}

	bool send(const V& v) {
		std::unique_lock<std::mutex> m(mutex);

		// zero-size channel writes blocks until a receiver is ready
		if (depth == 0) {
			while (accepting && !receivers) {
				sendCond.wait(m);
			}
		}

		// non-zero-size channel blocks when queue is full
		if (depth > 0) {
			while (accepting && packets.size() >= depth) {
				sendCond.wait(m);
			}
		}

		bool sent = false;

		if (accepting) {
			packets.push_back(v);
			recvCond.notify_one();
			sent = true;
		}

		m.unlock();
		return sent;
	}

	template <typename B>
	bool send_batch(const B& batch) {
		if (!batch.size()) {
			return true;
		}

		if (depth >= 0) {
			for (auto& v: batch) {
				if (!send(v)) return false;
			}
			return true;
		}

		std::unique_lock<std::mutex> m(mutex);

		bool sent = false;

		if (accepting) {
			packets.insert(packets.end(), batch.begin(), batch.end());
			recvCond.notify_all();
			sent = true;
		}

		m.unlock();
		return sent;
	}

	V recv() {
		std::unique_lock<std::mutex> m(mutex);

		// only block if channel is open and queue is empty
		while (accepting && !packets.size()) {
			++receivers;
			sendCond.notify_one();
			recvCond.wait(m);
			--receivers;
		}

		if (!accepting && !packets.size()) {
			V v = V();
			m.unlock();
			return v;
		}

		// receivers can still drain a channel after it's closed
		V v = packets.front();
		packets.pop_front();
		sendCond.notify_one();
		if (packets.size()) recvCond.notify_one();
		m.unlock();
		return v;
	}

	std::vector<V> recv_batch(uint limit) {
		std::unique_lock<std::mutex> m(mutex);

		// only block if channel is open and queue is empty
		while (accepting && !packets.size()) {
			++receivers;
			sendCond.notify_one();
			recvCond.wait(m);
			--receivers;
		}

		std::vector<V> batch;

		if (!accepting && !packets.size()) {
			m.unlock();
			return batch;
		}

		// receivers can still drain a channel after it's closed
		uint size = std::min(limit, (uint)packets.size());
		auto it = packets.begin();

		batch = {it, it+size};
		packets.erase(it, it+size);

		sendCond.notify_one();
		if (packets.size()) recvCond.notify_one();
		m.unlock();
		return batch;
	}

	struct pair {
		bool ok;
		V v;
	};

	pair recv_try() {
		std::unique_lock<std::mutex> m(mutex);

		// only block if channel is open and queue is empty
		while (accepting && !packets.size()) {
			++receivers;
			sendCond.notify_one();
			recvCond.wait(m);
			--receivers;
		}

		if (!accepting && !packets.size()) {
			m.unlock();
			return (pair){false,V()};
		}

		// receivers can still drain a channel after it's closed
		V v = packets.front();
		packets.pop_front();
		sendCond.notify_one();
		if (packets.size()) recvCond.notify_one();
		m.unlock();
		return (pair){true,v};
	}

	std::vector<V> recv_all() {
		std::unique_lock<std::mutex> m(mutex);
		std::vector<V> batch = {packets.begin(), packets.end()};
		packets.clear();
		sendCond.notify_all();
		m.unlock();
		return batch;
	}

	// Forward iteration until closed and drained
	// for (auto v: chan) { ... }
	class iterator {
		channel<V,depth> *c;
		bool last;
		V v;

	public:
		typedef V value_type;
		typedef std::ptrdiff_t difference_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::input_iterator_tag iterator_category;

		explicit iterator(channel<V,depth> *cc, bool llast) {
			c = cc;
			last = llast;
			if (!last) read();
		}

		void read() {
			auto p = c->recv_try();
			last = !p.ok;
			v = p.v;
		}

		V& operator*() {
			return v;
		}

		bool operator==(const iterator& other) const {
			return last == other.last;
		}

		bool operator!=(const iterator& other) const {
			return last != other.last;
		}

		iterator& operator++() {
			read();
			return *this;
		}

		iterator operator++(int) {
			iterator tmp(*this);
			++*this;
			return tmp;
		};
	};

	iterator begin() {
		return iterator(this, false);
	}

	iterator end() {
		return iterator(this, true);
	}
};
