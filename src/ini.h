#pragma once

#include <map>
#include <string>
#include <fstream>
#include <algorithm>
#include "catenate.h"

class IniReader {

    std::map<std::string,std::map<std::string,std::string>> sections;

public:

    IniReader() = default;

    IniReader(const char* path) {
        auto in = std::ifstream(path);
        if (!in.is_open()) return;
        auto content = std::string(
            (std::istreambuf_iterator<char>(in)),
            (std::istreambuf_iterator<char>())
        );
        in.close();
        parse(content);
    }

    void parse(std::string text) {
        std::string section;

        for (auto ln: discatenate(text, "\n")) {
            auto line = std::string(ln);

            trim(line);

            if (starts_with(line, "[")) {
                section.clear();
            }

            if (line.size() > 2 && starts_with(line, "[") && ends_with(line, "]")) {
                section = line.substr(1, line.size()-2);
                continue;
            }

            if (!section.size()) {
                continue;
            }

            if (!line.size()) {
                continue;
            }

            if (starts_with(line, ";") || starts_with(line, "#")) {
                continue;
            }

            auto eq = line.find_first_of("=");

            if (!eq) {
                continue;
            }

            auto left = line.substr(0, eq);
            auto right = line.substr(eq+1);

            trim(left);
            trim(right);

            if (starts_with(right, "\"") && ends_with(right, "\"")) {
                std::string parsed;
                for (size_t i = 1; i < right.size()-1; ) {
                    char c = right[i++];
                    if (c == '\\') {
                        c = right[i++];
                        if (c == 'n') c = '\n';
                        if (c == 't') c = '\t';
                        if (c == 'r') c = '\r';
                        if (c == 'a') c = '\a';
                        if (c == 'b') c = '\b';
                    }
                    parsed += c;
                }
                right = parsed;
            }

            if (left.size() && right.size()) {
                sections[section][left] = right;
            }
        }
    }

    bool has(const std::string& section, const std::string& key) {
        return sections.count(section) && sections[section].count(key);
    }

    const char* getCString(const std::string& section, const std::string& key, const char* def = nullptr) {
        return has(section, key) ? sections[section][key].c_str(): def;
    }

    std::string getString(const std::string& section, const std::string& key, std::string def = "") {
        return has(section, key) ? sections[section][key]: def;
    }

    int64_t getInteger(const std::string& section, const std::string& key, int64_t def = 0) {
        if (has(section, key)) {
            auto& val = sections[section][key];
            int base = starts_with(val, "0x") || ends_with(val, "h") ? 16: 10;
            const char* start = val.c_str();
            char* end = nullptr;
            int64_t ival = strtoll(start, &end, base);
            if ((size_t)(end-start) == val.size()) {
                return ival;
            }
            fprintf(stderr, "IniReader: invalid integer %s\n", val.c_str());
        }
        return def;
    }

    double getDouble(const std::string& section, const std::string& key, double def = 0.0f) {
        if (has(section, key)) {
            auto& val = sections[section][key];
            const char* start = val.c_str();
            char* end = nullptr;
            double dval = strtod(start, &end);
            if ((size_t)(end-start) == val.size()) {
                return dval;
            }
            fprintf(stderr, "IniReader: invalid double %s\n", val.c_str());
        }
        return def;
    }

    std::set<std::string> getWords(const std::string& section, const std::string& key) {
        std::set<std::string> words;
        if (has(section, key)) {
            for (auto wd: discatenate(sections[section][key], " "))
                words.insert(std::string(wd));
        }
        return words;
    }
};
