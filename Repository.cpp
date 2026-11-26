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
int Repository::get_head_commit_id() const {
    if (is_detached)
        return detached_commit_id;
    int idx = find_branch_index(current_branch);
    if (idx == -1)
        return -1;
    return branches[idx].get_commit_id();
}

const Commit* Repository::find_commit(int id) const {
    for (size_t i = 0; i < commits.size(); ++i) {
        if (commits[i]->get_id() == id)
            return commits[i];
    }
    return nullptr;
}

std::vector<FileEntry> Repository::get_head_snapshot() const {
    int hid = get_head_commit_id();
    if (hid == -1)
        return std::vector<FileEntry>();
    const Commit* c = find_commit(hid);
    if (c == nullptr)
        return std::vector<FileEntry>();
    return c->get_snapshot();
}

int Repository::find_branch_index(const std::string& name) const {
    for (size_t i = 0; i < branches.size(); ++i) {
        if (branches[i].get_name() == name)
            return static_cast<int>(i);
    }
    return -1;
}

int Repository::find_entry_index(const std::vector<FileEntry>& list,
                                 const std::string& path) const {
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].get_path() == path)
            return static_cast<int>(i);
    }
    return -1;
}

bool Repository::is_path_under_prefix(const std::string& path, const std::string& prefix) const {
    return prefix.empty() ||
           path == prefix ||
           path.find(prefix + "/") == 0;
}

void Repository::add(FileSystem& fs, const std::string& target) {
    if (target == ".") {
        Folder* curr = fs.get_current_dir();
        std::string dir_prefix = fs.get_relative_path(repo_root, curr);
        std::vector<FileEntry> ws_files = fs.get_all_files_under(repo_root);

        for (size_t i = 0; i < ws_files.size(); ++i) {
            if (!is_path_under_prefix(ws_files[i].get_path(), dir_prefix))
                continue;

            int s_idx = find_entry_index(staging, ws_files[i].get_path());
            if (s_idx == -1) {
                staging.push_back(ws_files[i]);
            } else {
                staging[s_idx].set_lines(ws_files[i].get_lines());
            }
        }

        for (int i = static_cast<int>(staging.size()) - 1; i >= 0; --i) {
            if (!is_path_under_prefix(staging[i].get_path(), dir_prefix))
                continue;

            if (find_entry_index(ws_files, staging[i].get_path()) == -1) {
                staging.erase(staging.begin() + i);
            }
        }

        std::sort(staging.begin(), staging.end());
        return;
    }

    if (!is_valid_identifier(target))
        throw GitException("fatal: pathspec '" + target + "' did not match any files");

    if (fs.get_current_dir()->has_subfolder(target))
        throw GitException("fatal: '" + target + "' is a directory (use 'git add .')");

    std::string rel_path = fs.get_relative_path(repo_root, fs.get_current_dir(), target);
    const File* ws_file = fs.get_file_in_current(target);
    int s_idx = find_entry_index(staging, rel_path);

    if (ws_file != nullptr) {
        if (s_idx == -1) {
            staging.push_back(FileEntry(rel_path, ws_file->get_lines()));
        } else {
            staging[s_idx].set_lines(ws_file->get_lines());
        }
        std::sort(staging.begin(), staging.end());
    } else if (s_idx != -1) {
        staging.erase(staging.begin() + s_idx);
    } else {
        throw GitException("fatal: pathspec '" + target + "' did not match any files");
    }
}

void Repository::reset(FileSystem& fs, const std::string& file_name) {
    if (!is_valid_identifier(file_name))
        throw GitException("fatal: pathspec '" + file_name + "' did not match any files");

    std::string rel_path = fs.get_relative_path(repo_root, fs.get_current_dir(), file_name);
    std::vector<FileEntry> head_snap = get_head_snapshot();

    int c_idx = find_entry_index(head_snap, rel_path);
    int s_idx = find_entry_index(staging, rel_path);
    const File* ws_file = fs.get_file_in_current(file_name);

    if (c_idx == -1 && s_idx == -1 && ws_file == nullptr)
        throw GitException("fatal: pathspec '" + file_name + "' did not match any files");

    if (c_idx != -1) {
        if (s_idx != -1) {
            staging[s_idx].set_lines(head_snap[c_idx].get_lines());
        } else {
            staging.push_back(head_snap[c_idx]);
            std::sort(staging.begin(), staging.end());
        }
    } else {
        if (s_idx != -1) {
            staging.erase(staging.begin() + s_idx);
        }
    }
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
        Folder* existing_root = repos[i]->get_repo_root();
        if (fs.is_ancestor_or_same(existing_root, curr) ||
            fs.is_ancestor_or_same(curr, existing_root)) {
            throw GitException("fatal: cannot initialize nested git repository");
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