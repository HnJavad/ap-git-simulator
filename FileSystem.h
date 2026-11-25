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

class Folder {
public:
    Folder(const std::string& n, Folder* p);
    ~Folder();

    std::string get_name() const { return name; }
    Folder* get_parent() const { return parent; }

    bool has_file(const std::string& fname) const;
    bool has_subfolder(const std::string& dname) const;

    File* find_file(const std::string& fname);
    const File* find_file_const(const std::string& fname) const;
    Folder* find_subfolder(const std::string& dname);
    const Folder* find_subfolder_const(const std::string& dname) const;

    void add_file(const File& f);
    void remove_file(const std::string& fname);
    Folder* add_subfolder(const std::string& dname);

    void collect_sorted_names(std::vector<std::string>& out_names) const;

private:
    std::string name;
    Folder* parent;
    std::vector<File> files;
    std::vector<Folder*> subfolders;
};

class FileSystem {
public:
    FileSystem();
    ~FileSystem();

    Folder* get_root() const { return root; }
    Folder* get_current_dir() const { return current_dir; }

    std::string pwd() const;
    void mkdir(const std::string& dir_name);
    void cd(const std::string& address);
    void ls() const;
    void touch(const std::string& file_name);
    void echo_overwrite(const std::string& text, const std::string& file_name);
    void echo_append(const std::string& text, const std::string& file_name);
    void cat(const std::string& file_name) const;
    void rm(const std::string& file_name);
    bool is_ancestor_or_same(const Folder* anc, const Folder* desc) const;

private:
    Folder* root;
    Folder* current_dir;
};

#endif