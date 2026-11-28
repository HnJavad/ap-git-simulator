#ifndef GIT_EXCEPTION_H
#define GIT_EXCEPTION_H

#include <string>
#include <vector>
#include <cctype>

class GitException {
public:
    GitException(const std::string& msg) : message(msg) {}
    std::string what() const { return message; }

private:
    std::string message;
};

class FileEntry {
public:
    FileEntry() {}
    FileEntry(const std::string& p, const std::vector<std::string>& l)
        : path(p), lines(l) {}

    const std::string& get_path() const { return path; }
    const std::vector<std::string>& get_lines() const { return lines; }
    void set_lines(const std::vector<std::string>& new_lines) { lines = new_lines; }

    bool operator==(const FileEntry& other) const {
        return path == other.path && lines == other.lines;
    }

    bool operator<(const FileEntry& other) const {
        return path < other.path;
    }

private:
    std::string path;
    std::vector<std::string> lines;
};

inline bool is_valid_identifier(const std::string& name) {
    if (name.empty() || name == "." || name == "..")
        return false;
    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];
        bool ok = std::isalnum(static_cast<unsigned char>(c)) ||
                  c == '.' || c == '-' || c == '_';
        if (!ok)
            return false;
    }
    return true;
}

inline bool is_all_digits(const std::string& s) {
    if (s.empty())
        return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    }
    return true;
}

#endif