#include "FileSystem.h"
#include <iostream>

// File methods

File::File(const std::string& n) : name(n) {
    if (!is_valid_identifier(n))
        throw GitException("Invalid file name: " + n);
}

void File::write_overwrite(const std::string& text) {
    lines.clear();
    lines.push_back(text);
}

void File::write_append(const std::string& text) {
    lines.push_back(text);
}

void File::set_lines(const std::vector<std::string>& new_lines) {
    lines = new_lines;
}

void File::print_content() const {
    for (size_t i = 0; i < lines.size(); ++i) {
        std::cout << lines[i] << '\n';
    }
}