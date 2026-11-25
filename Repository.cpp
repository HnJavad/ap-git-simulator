#include "Repository.h"
#include <algorithm>
#include <ctime>

// --- UserConfig ---

UserConfig* UserConfig::instance = nullptr;

UserConfig* UserConfig::get_instance() {
    if (instance == nullptr) {
        instance = new UserConfig();
    }
    return instance;
}

void UserConfig::set_username(const std::string& name) {
    if (name.empty())
        throw GitException("Invalid user.name");
    username = name;
}

void UserConfig::set_email(const std::string& mail) {
    if (mail.empty())
        throw GitException("Invalid user.email");
    email = mail;
}

// --- Commit ---

Commit::Commit(int cid,
               int pid,
               const std::string& msg,
               const std::string& a_name,
               const std::string& a_email,
               const std::vector<FileEntry>& snap)
    : id(cid),
      parent_id(pid),
      message(msg),
      author_name(a_name),
      author_email(a_email),
      snapshot(snap) {
    std::time_t now = std::time(nullptr);
    std::tm* local_tm = std::localtime(&now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", local_tm);
    timestamp = std::string(buf);
    std::sort(snapshot.begin(), snapshot.end());
}

// --- Repository ---

Repository::Repository(Folder* root_dir)
    : repo_root(root_dir),
      next_commit_id(1),
      current_branch("main"),
      is_detached(false),
      detached_commit_id(-1) {
    branches.push_back(Branch("main", -1));
}

Repository::~Repository() {
    for (size_t i = 0; i < commits.size(); ++i) {
        delete commits[i];
    }
    commits.clear();
}

// --- RepoManager ---

RepoManager::RepoManager() {}

RepoManager::~RepoManager() {
    for (size_t i = 0; i < repos.size(); ++i) {
        delete repos[i];
    }
    repos.clear();
}

void RepoManager::init(FileSystem& fs) {
    Folder* curr = fs.get_current_dir();
    for (size_t i = 0; i < repos.size(); ++i) {
        if (repos[i]->get_repo_root() == curr) {
            throw GitException("fatal: repository already initialized");
        }
    }
    repos.push_back(new Repository(curr));
}

Repository* RepoManager::get_active_repo(const FileSystem& fs) {
    Folder* curr = fs.get_current_dir();
    for (size_t i = 0; i < repos.size(); ++i) {
        if (fs.is_ancestor_or_same(repos[i]->get_repo_root(), curr))
            return repos[i];
    }
    throw GitException("fatal: not a git repository");
}

const Repository* RepoManager::get_active_repo(const FileSystem& fs) const {
    Folder* curr = fs.get_current_dir();
    for (size_t i = 0; i < repos.size(); ++i) {
        if (fs.is_ancestor_or_same(repos[i]->get_repo_root(), curr))
            return repos[i];
    }
    throw GitException("fatal: not a git repository");
}