#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include "GitException.h"
#include "FileSystem.h"

std::string trim_spaces(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;
    return s.substr(start, end - start);
}

void process_command(const std::string& line, FileSystem& fs, bool& running) {
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

    if (cmd == "pwd") {
        std::string extra;
        if (ss >> extra)
            throw GitException("pwd: too many arguments");
        std::cout << fs.pwd() << '\n';
        return;
    }

    if (cmd == "ls") {
        std::string extra;
        if (ss >> extra)
            throw GitException("ls: too many arguments");
        fs.ls();
        return;
    }

    if (cmd == "mkdir") {
        std::string arg, extra;
        if (!(ss >> arg) || (ss >> extra))
            throw GitException("mkdir: invalid arguments");
        fs.mkdir(arg);
        return;
    }

        if (cmd == "touch") {
        std::string arg, extra;
        if (!(ss >> arg) || (ss >> extra))
            throw GitException("touch: invalid arguments");
        fs.touch(arg);
        return;
    }

    if (cmd == "cat") {
        std::string arg, extra;
        if (!(ss >> arg) || (ss >> extra))
            throw GitException("cat: invalid arguments");
        fs.cat(arg);
        return;
    }

    if (cmd == "rm") {
        std::string arg, extra;
        if (!(ss >> arg) || (ss >> extra))
            throw GitException("rm: invalid arguments");
        fs.rm(arg);
        return;
    }

    throw GitException("Unknown command: " + cmd);
}

int main() {
    FileSystem fs;
    bool running = true;
    std::string line;

    while (running) {
        std::cout << "$ ";
        if (!std::getline(std::cin, line))
            break;

        try {
            process_command(line, fs, running);
        } catch (const GitException& e) {
            std::cout << e.what() << '\n';
        }
    }

    return 0;
}