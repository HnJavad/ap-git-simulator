#include "FileSystem.h"
#include <iostream>
#include <algorithm>
#include <sstream>

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

// Folder methods

Folder::Folder(const std::string& n, Folder* p) : name(n), parent(p) {}

Folder::~Folder() {
    for (size_t i = 0; i < subfolders.size(); ++i) {
        delete subfolders[i];
    }
    subfolders.clear();
}

bool Folder::has_file(const std::string& fname) const {
    return find_file_const(fname) != nullptr;
}

bool Folder::has_subfolder(const std::string& dname) const {
    return find_subfolder_const(dname) != nullptr;
}

File* Folder::find_file(const std::string& fname) {
    for (size_t i = 0; i < files.size(); ++i) {
        if (files[i].get_name() == fname)
            return &files[i];
    }
    return nullptr;
}

const File* Folder::find_file_const(const std::string& fname) const {
    for (size_t i = 0; i < files.size(); ++i) {
        if (files[i].get_name() == fname)
            return &files[i];
    }
    return nullptr;
}

Folder* Folder::find_subfolder(const std::string& dname) {
    for (size_t i = 0; i < subfolders.size(); ++i) {
        if (subfolders[i]->get_name() == dname)
            return subfolders[i];
    }
    return nullptr;
}

const Folder* Folder::find_subfolder_const(const std::string& dname) const {
    for (size_t i = 0; i < subfolders.size(); ++i) {
        if (subfolders[i]->get_name() == dname)
            return subfolders[i];
    }
    return nullptr;
}

void Folder::add_file(const File& f) {
    files.push_back(f);
}

void Folder::remove_file(const std::string& fname) {
    for (size_t i = 0; i < files.size(); ++i) {
        if (files[i].get_name() == fname) {
            files.erase(files.begin() + i);
            return;
        }
    }
    throw GitException("rm: cannot remove '" + fname + "': No such file");
}

Folder* Folder::add_subfolder(const std::string& dname) {
    if (!is_valid_identifier(dname))
        throw GitException("Invalid directory name: " + dname);
    if (has_subfolder(dname) || has_file(dname))
        throw GitException("mkdir: cannot create directory '" + dname + "': File or directory exists");
    Folder* child = new Folder(dname, this);
    subfolders.push_back(child);
    return child;
}

void Folder::collect_sorted_names(std::vector<std::string>& out_names) const {
    out_names.clear();
    for (size_t i = 0; i < files.size(); ++i)
        out_names.push_back(files[i].get_name());
    for (size_t i = 0; i < subfolders.size(); ++i)
        out_names.push_back(subfolders[i]->get_name());
    std::sort(out_names.begin(), out_names.end());
}

// FileSystem methods

FileSystem::FileSystem() {
    root = new Folder("", nullptr);
    current_dir = root;
}

FileSystem::~FileSystem() {
    delete root;
}

std::string FileSystem::pwd() const {
    if (current_dir == root)
        return "/";
    std::vector<std::string> parts;
    const Folder* curr = current_dir;
    while (curr != nullptr && curr != root) {
        parts.push_back(curr->get_name());
        curr = curr->get_parent();
    }
    std::string full_path = "";
    for (int i = static_cast<int>(parts.size()) - 1; i >= 0; --i) {
        full_path += "/" + parts[i];
    }
    return full_path;
}

void FileSystem::mkdir(const std::string& dir_name) {
    current_dir->add_subfolder(dir_name);
}

void FileSystem::cd(const std::string& address) {
    if (address.empty())
        throw GitException("cd: invalid path");

    Folder* target = (address[0] == '/') ? root : current_dir;
    std::stringstream ss(address);
    std::string token;

    while (std::getline(ss, token, '/')) {
        if (token.empty() || token == ".") {
            continue;
        } else if (token == "..") {
            if (target->get_parent() != nullptr)
                target = target->get_parent();
        } else {
            Folder* next = target->find_subfolder(token);
            if (next == nullptr)
                throw GitException("cd: " + address + ": No such directory");
            target = next;
        }
    }
    current_dir = target;
}

void FileSystem::ls() const {
    std::vector<std::string> names;
    current_dir->collect_sorted_names(names);
    for (size_t i = 0; i < names.size(); ++i) {
        std::cout << names[i] << '\n';
    }
}

void FileSystem::touch(const std::string& file_name) {
    if (!is_valid_identifier(file_name))
        throw GitException("Invalid file name: " + file_name);
    if (!current_dir->has_file(file_name)) {
        current_dir->add_file(File(file_name));
    }
}

void FileSystem::echo_overwrite(const std::string& text, const std::string& file_name) {
    if (!is_valid_identifier(file_name))
        throw GitException("Invalid file name: " + file_name);

    File* f = current_dir->find_file(file_name);
    if (f == nullptr) {
        File new_file(file_name);
        new_file.write_overwrite(text);
        current_dir->add_file(new_file);
    } else {
        f->write_overwrite(text);
    }
}

void FileSystem::echo_append(const std::string& text, const std::string& file_name) {
    if (!is_valid_identifier(file_name))
        throw GitException("Invalid file name: " + file_name);

    File* f = current_dir->find_file(file_name);
    if (f == nullptr) {
        File new_file(file_name);
        new_file.write_append(text);
        current_dir->add_file(new_file);
    } else {
        f->write_append(text);
    }
}

void FileSystem::cat(const std::string& file_name) const {
    const File* f = current_dir->find_file_const(file_name);
    if (f == nullptr)
        throw GitException("cat: " + file_name + ": No such file");
    f->print_content();
}

void FileSystem::rm(const std::string& file_name) {
    current_dir->remove_file(file_name);
}