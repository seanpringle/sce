#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <string>
#include <memory>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <filesystem>

#if defined(_WIN32)
typedef uint32_t uint;
#endif

#define ZERO(s) memset(&s, 0, sizeof(s))

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

class Logger {
    bool first = true;
    bool last = true;
public:
    Logger() = default;
    Logger(const char* prefix) {
        first = false;
        std::cerr << prefix;
    }
    Logger(const char* prefix, const char* file, int line, const char* func) : Logger(prefix) {
        std::cerr << ' ' << std::filesystem::path(file).filename().string() << ':' << line << ' ' << func << "()";
        first = false;
    }
    Logger(Logger && dc) noexcept : first{false} {
        dc.last = false;
    }
    ~Logger() {
        if (last) std::cerr << '\n';
    }
    template <typename T>
    friend Logger operator<<(Logger db, const T& x) {
        if (db.first) db.first = false; else std::cerr << ' ';
        std::cerr << x;
        return db;
    }
};

class NullLogger {
public:
    NullLogger() = default;
    template <typename T>
    friend NullLogger operator<<(NullLogger db, const T& x) {
        return db;
    }
};

#ifndef NDEBUG
#define note() Logger("NOTE", __FILE__, __LINE__, __func__)
#define debug() Logger("DEBUG", __FILE__, __LINE__, __func__)
#else
#define note() Logger()
#define debug() NullLogger()
#endif

