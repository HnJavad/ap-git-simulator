#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include "GitException.h"

std::string trim_spaces(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;
    return s.substr(start, end - start);
}

void process_command(const std::string& line, bool& running) {
    std::string trimmed = trim_spaces(line);
    if (trimmed.empty())
        return;

    std::stringstream ss(trimmed);
    std::string cmd;
    ss >> cmd;

    if (cmd == "exit") {
        std::string extra;
        if (ss >> extra)
            throw GitException("exit: too many arguments");
        running = false;
        return;
    }

    throw GitException("Unknown command: " + cmd);
}

int main() {
    bool running = true;
    std::string line;

    while (running) {
        std::cout << "$ ";
        if (!std::getline(std::cin, line))
            break;

        try {
            process_command(line, running);
        } catch (const GitException& e) {
            std::cout << e.what() << '\n';
        }
    }

    return 0;
}