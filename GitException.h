#ifndef GIT_EXCEPTION_H
#define GIT_EXCEPTION_H

#include <string>
#include <cctype>

class GitException {
public:
    GitException(const std::string& msg) : message(msg) {}
    std::string what() const { return message; }

private:
    std::string message;
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

#endif