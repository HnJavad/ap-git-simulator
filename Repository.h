#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>
#include "GitException.h"
#include "FileSystem.h"

class UserConfig {
public:
    static UserConfig* get_instance();

    void set_username(const std::string& name);
    void set_email(const std::string& mail);

    std::string get_username() const { return username; }
    std::string get_email() const { return email; }
    bool is_configured() const { return !username.empty() && !email.empty(); }

private:
    UserConfig() {}
    std::string username;
    std::string email;
    static UserConfig* instance;
};

class Commit {
public:
    Commit(int cid,
           int pid,
           const std::string& msg,
           const std::string& a_name,
           const std::string& a_email,
           const std::vector<FileEntry>& snap);

    int get_id() const { return id; }
    int get_parent_id() const { return parent_id; }
    std::string get_message() const { return message; }
    std::string get_author_name() const { return author_name; }
    std::string get_author_email() const { return author_email; }
    std::string get_timestamp() const { return timestamp; }
    const std::vector<FileEntry>& get_snapshot() const { return snapshot; }

private:
    int id;
    int parent_id;
    std::string message;
    std::string author_name;
    std::string author_email;
    std::string timestamp;
    std::vector<FileEntry> snapshot;
};

class Branch {
public:
    Branch(const std::string& n, int cid) : name(n), commit_id(cid) {}

    const std::string& get_name() const { return name; }
    int get_commit_id() const { return commit_id; }
    void set_commit_id(int cid) { commit_id = cid; }

    bool operator<(const Branch& other) const { return name < other.name; }

private:
    std::string name;
    int commit_id;
};

class Repository {
public:
    Repository(Folder* root_dir);
    ~Repository();

    Folder* get_repo_root() const { return repo_root; }
    void add(FileSystem& fs, const std::string& target);
    void reset(FileSystem& fs, const std::string& file_name);
    void commit(const FileSystem& fs, const std::string& message);
    void status(const FileSystem& fs) const;
    void log() const;

private:
    Folder* repo_root;
    int next_commit_id;
    std::vector<Commit*> commits;
    std::vector<Branch> branches;
    std::string current_branch;
    bool is_detached;
    int detached_commit_id;
    std::vector<FileEntry> staging;
    
    int get_head_commit_id() const;
    const Commit* find_commit(int id) const;
    std::vector<FileEntry> get_head_snapshot() const;
    int find_branch_index(const std::string& name) const;
    int find_entry_index(const std::vector<FileEntry>& list, const std::string& path) const;
    bool is_path_under_prefix(const std::string& path, const std::string& prefix) const;
};

class RepoManager {
public:
    RepoManager();
    ~RepoManager();

    void init(FileSystem& fs);
    Repository* get_active_repo(const FileSystem& fs);
    const Repository* get_active_repo(const FileSystem& fs) const;

private:
    std::vector<Repository*> repos;
};

#endif