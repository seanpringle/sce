#pragma once

#include "common.h"
#include "utf8.h"
#include <deque>
#include <vector>
#include <cstring>

struct Doc {
	Doc() = default;
	Doc(std::initializer_list<uint32_t> l) {
		for (auto v: l) push_back(v);
	}
	Doc(const Doc& other) {
		operator=(other);
	}

	Doc& operator=(const Doc& other) {
		clear();
		count = other.count;
		lines = other.lines;
		last = other.last;
		return *this;
	}

	bool operator==(const Doc& other) const {
		return count == other.count && lines == other.lines;
	}

	uint count = 0;
	std::vector<std::vector<uint32_t>> lines;

	struct Cursor {
		uint index = 0;
		uint line = 0;
		uint cell = 0;
	};

	bool sanity() {
		uint sum = 0;
		for (uint i = 0; i < lines.size()-1; i++) {
			if (lines[i].back() != '\n') return false;
			sum += lines[i].size();
		}
		if (lines.size()) {
			sum += lines.back().size();
		}
		return sum == count;
	};

	mutable Cursor last;

	void clear() {
		lines.clear();
		count = 0;
		last.index = 0;
		last.line = 0;
		last.cell = 0;
	};

	Cursor cursor(uint index) const {
		ensure(index <= size());

		if (index == 0) {
			last = {0,0,0};
			return last;
		}

		if (index == size()) {
			last = {index,(uint)lines.size()-1,(uint)lines.back().size()};
			return last;
		}

		while (last.index > index) {
			// try to jump straight the beginning of the current line
			if (last.cell > 0 && last.index-last.cell >= index) {
				last.index -= last.cell;
				last.cell = 0;
				continue;
			}
			// walk backward on the current line
			if (last.cell > 0) {
				last.cell--;
				last.index--;
				continue;
			}
			ensure(!last.cell);
			ensure(last.line > 0);
			last.line--;
			last.cell = lines[last.line].size()-1;
			last.index--;
		}

		while (last.index < index) {
			uint len = lines[last.line].size();
			ensure(len > 0);
			uint rem = len-last.cell-1;
			// try to jump straight the end of the current line
			if (last.cell < len-1 && last.index+rem <= index) {
				last.index += rem;
				last.cell = len-1;
				continue;
			}
			// walk forward on the current line
			if (last.cell < len-1) {
				last.cell++;
				last.index++;
				continue;
			}
			ensure(last.cell == len-1);
			ensure(last.line < lines.size()-1);
			last.line++;
			last.cell = 0;
			last.index++;
		}

		ensure(last.index == index);
		return last;
	}

	uint line_offset(uint line) {
		ensure(line < lines.size());

		last.index -= last.cell;
		last.cell = 0;

		while (last.line > line) {
			last.line--;
			last.index -= lines[last.line].size();
		}

		while (last.line < line) {
			last.index += lines[last.line].size();
			last.line++;
		}

		return last.index;
	}

	uint32_t* cell(uint index) const {
		ensure(index < size());
		auto cur = cursor(index);
		return (uint32_t*)&lines[cur.line][cur.cell];
	};

	uint size() const {
		return count;
	};

	void push_back(uint32_t v) {
		insert(end(), v);
	}

	void push_back(std::string s) {
		UTF8 utf8(s);
		UTF8 test(utf8.text);
		if (utf8.codes != test.codes) notef("in codes don't match");
		if (utf8.text != test.text) notef("in texts don't match");
		for (auto c: utf8.codes) push_back(c);
	}

	std::string extract(size_t offset, size_t length) const {
		std::vector<uint32_t> out;
		for (size_t i = offset; i < offset+length && i < size(); i++)
			out.push_back(*cell(i));
		UTF8 utf8(out);
		UTF8 test(utf8.text);
		if (utf8.codes != test.codes) notef("out codes don't match");
		if (utf8.text != test.text) notef("out texts don't match");
		return utf8.text;
	}

	operator std::string() const {
		return extract((size_t)0, size());
	}

	class iterator {
	public:
		uint ii;
		const Doc* doc;

		typedef uint32_t V;
		typedef uint32_t value_type;
		typedef uint difference_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::input_iterator_tag iterator_category;

		explicit iterator(const Doc* ddoc, uint iii) {
			doc = ddoc;
			ii = std::min(doc->size(), iii);
		}

		bool valid() const {
			return ii >= 0 && ii <= doc->size();
		}

		Cursor cursor() {
			ensure(valid());
			return doc->cursor(ii);
		}

		V& operator*() const {
			ensure(valid());
			return *doc->cell(ii);
		}

		V* operator->() const {
			ensure(valid());
			return doc->cell(ii);
		}

		bool operator==(const iterator& other) const {
			ensure(valid() && other.valid());
			return ii == other.ii;
		}

		bool operator!=(const iterator& other) const {
			ensure(valid() && other.valid());
			return ii != other.ii;
		}

		bool operator<(const iterator& other) const {
			ensure(valid() && other.valid());
			return ii < other.ii;
		}

		bool operator>(const iterator& other) const {
			ensure(valid() && other.valid());
			return ii > other.ii;
		}

		iterator& operator+=(difference_type d) {
			ii += d;
			ii = std::min(doc->size(), ii);
			ensure(valid());
			return *this;
		}

		iterator operator+(const iterator& other) const {
			return iterator(doc, ii+other.ii);
		}

		iterator operator+(difference_type d) const {
			return iterator(doc, ii+d);
		}

		iterator& operator-=(difference_type d) {
			ii = d > ii ? 0: ii-d;
			ensure(valid());
			return *this;
		}

		iterator operator-(const iterator& other) const {
			return operator-(other.ii);
		}

		iterator operator-(difference_type d) const {
			return iterator(doc, d > ii ? 0: ii-d);
		}

		difference_type operator-(const iterator& other) {
			return ii - other.ii;
		}

		iterator& operator++() {
			ii = ii < doc->size() ? ii+1: doc->size();
			return *this;
		}

		iterator operator++(int) {
			iterator tmp(*this);
			++*this;
			return tmp;
		};

		iterator& operator--() {
			--ii;
			return *this;
		}

		iterator operator--(int) {
			iterator tmp(*this);
			--*this;
			return tmp;
		};
	};

	iterator begin() const {
		return iterator(this, 0);
	};

	iterator end() const {
		return iterator(this, size());
	};

	const uint32_t& operator[](uint index) const {
		return *cell(index);
	};

	iterator insert(iterator it, uint32_t v) {
		if (!lines.size()) {
			lines.push_back({v});
			count++;
			if (v == '\n') lines.push_back({});
			return begin();
		}
		if (it == end() && v != '\n') {
			lines.back().push_back(v);
			count++;
			return iterator(this, it.ii);
		}
		if (it == end() && v == '\n') {
			lines.back().push_back(v);
			count++;
			lines.push_back({});
			return iterator(this, it.ii);
		}

		auto cur = it.cursor();

		// move the cached cursor out of the way
		if (last.index >= it.ii) {
			cursor(std::max(0, (int)it.ii-1));
		}

		auto& line = lines[cur.line];
		auto cit = line.begin()+cur.cell;
		auto pos = line.insert(cit, v);
		count++;
		if (v == '\n') {
			auto split = pos+1;
			std::vector<uint32_t> eol = {split, line.end()};
			line.erase(split, line.end());
			auto lit = lines.begin()+cur.line;
			lines.insert(lit+1, eol);
		}
		return iterator(this, it.ii);
	};

	template <typename IT>
	iterator insert(iterator it, IT a, IT b) {
		for (auto c = it; a != b; ) {
			insert(c, *a);
			++c;
			++a;
		}
		return iterator(this, it.ii);
	};

	iterator erase(iterator it) {
		if (it == end()) {
			return it;
		}

		auto cur = it.cursor();

		// move the cached cursor out of the way
		if (last.index >= it.ii) {
			cursor(std::max(0, (int)it.ii-1));
		}

		auto& line = lines[cur.line];
		ensure(line.size());
		uint32_t c = line[cur.cell];
		line.erase(line.begin()+cur.cell);
		count--;
		if (c == '\n') {
			ensure(cur.cell == line.size());
			if (cur.line < lines.size()-1) {
				auto& next = lines[cur.line+1];
				line.insert(line.end(), next.begin(), next.end());
				lines.erase(lines.begin()+cur.line+1);
			}
		}
		return iterator(this, it.ii);
	};

	iterator erase(iterator it, uint n) {
		for (uint i = 0; i < n; i++) it = erase(it);
		return it;
	};

	iterator erase(iterator a, iterator b) {
		return erase(a, b-a);
	};

	std::string extract(iterator a, iterator b) {
		return a < b ? extract(a-begin(), b-a): "";
	}

	std::vector<char> exportRaw() {
		std::vector<uint32_t> cells = {begin(), end()};
		size_t bytes = cells.size() * sizeof(uint32_t);
		std::vector<char> raw(bytes, 0);
		std::memmove(raw.data(), cells.data(), bytes);
		return raw;
	}

	void importRaw(const std::vector<char>& raw) {
		clear();
		std::vector<uint32_t> cells(raw.size() / sizeof(int));
		std::memmove(cells.data(), raw.data(), raw.size());
		for (auto c: cells) push_back(c);
	}
};
