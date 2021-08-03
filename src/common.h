#pragma once

#include <string>
#include <memory>
#include <stdexcept>
#include <algorithm>

// Convert all std::strings to const char* using constexpr if (C++17)
template<typename T>
auto fmtConvert(T&& t) {
	if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value) {
		return std::forward<T>(t).c_str();
	}
	else {
		return std::forward<T>(t);
	}
}

// printf like formatting for C++ with std::string
// https://gist.github.com/Zitrax/a2e0040d301bf4b8ef8101c0b1e3f1d5
template<typename ... Args>
std::string fmtInternal(const std::string& format, Args&& ... args)
{
	size_t size = std::snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args) ..., NULL) + 1;
	if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args ..., NULL);
	return std::string(buf.get(), buf.get() + size - 1);
}

template<typename ... Args>
std::string fmt(std::string fmt, Args&& ... args) {
	return fmtInternal(fmt, fmtConvert(std::forward<Args>(args))...);
}

#define notef(...) { fprintf(stderr, "%s", fmt(__VA_ARGS__).c_str()); fputc('\n', stderr); }
#define fatalf(...) { notef(__VA_ARGS__); exit(EXIT_FAILURE); }

#define ensure(cond,...) if (!(cond)) { throw; }
#define ensuref(cond,...) if (!(cond)) { notef(__VA_ARGS__); throw; }

#define fmtc(...) fmt(__VA_ARGS__).c_str()

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch) { return !std::isspace(ch); }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch) { return !std::isspace(ch); }).base(), s.end());
}

static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

static inline bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

static inline bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}
