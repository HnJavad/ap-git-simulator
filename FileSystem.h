#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>
#include "GitException.h"

class File {
public:
    File(const std::string& n);

    std::string get_name() const { return name; }
    const std::vector<std::string>& get_lines() const { return lines; }

    void write_overwrite(const std::string& text);
    void write_append(const std::string& text);
    void set_lines(const std::vector<std::string>& new_lines);
    void print_content() const;

private:
    std::string name;
    std::vector<std::string> lines;
};

#endif