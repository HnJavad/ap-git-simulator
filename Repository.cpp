#include "Repository.h"
#include <algorithm>
#include <ctime>
#include <iostream>

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

void Repository::commit(const FileSystem& fs, const std::string& message) {
    if (is_detached)
        throw GitException("fatal: cannot commit in detached HEAD state");

    UserConfig* cfg = UserConfig::get_instance();
    if (!cfg->is_configured())
        throw GitException("fatal: author identity unknown (configure user.name and user.email)");

    if (message.empty())
        throw GitException("fatal: commit message cannot be empty");

    std::sort(staging.begin(), staging.end());
    std::vector<FileEntry> head_snap = get_head_snapshot();
    std::vector<FileEntry> ws_files = fs.get_all_files_under(repo_root);

    if (staging == head_snap) {
        bool has_mod_or_untracked = false;
        for (size_t i = 0; i < staging.size(); ++i) {
            int w_idx = find_entry_index(ws_files, staging[i].get_path());
            if (w_idx == -1 || ws_files[w_idx].get_lines() != staging[i].get_lines()) {
                has_mod_or_untracked = true;
                break;
            }
        }
        if (!has_mod_or_untracked) {
            for (size_t i = 0; i < ws_files.size(); ++i) {
                if (find_entry_index(staging, ws_files[i].get_path()) == -1) {
                    has_mod_or_untracked = true;
                    break;
                }
            }
        }

        if (has_mod_or_untracked)
            std::cout << "no changes added to commit\n";
        else
            std::cout << "nothing to commit, working tree clean\n";
        return;
    }

    int parent_id = get_head_commit_id();
    Commit* new_commit = new Commit(next_commit_id++,
                                    parent_id,
                                    message,
                                    cfg->get_username(),
                                    cfg->get_email(),
                                    staging);
    commits.push_back(new_commit);

    int b_idx = find_branch_index(current_branch);
    branches[b_idx].set_commit_id(new_commit->get_id());

    std::cout << "[" << current_branch << " " << new_commit->get_id()
              << "] " << message << '\n';
}

void Repository::status(const FileSystem& fs) const {
    if (is_detached)
        std::cout << "HEAD detached at " << detached_commit_id << '\n';
    else
        std::cout << "On branch " << current_branch << '\n';

    std::vector<FileEntry> head_snap = get_head_snapshot();
    std::vector<FileEntry> ws_files = fs.get_all_files_under(repo_root);
    std::vector<FileEntry> sorted_staging = staging;
    std::sort(sorted_staging.begin(), sorted_staging.end());

    // 1. Staged changes
    std::vector<std::string> staged_lines;
    for (size_t i = 0; i < sorted_staging.size(); ++i) {
        int h_idx = find_entry_index(head_snap, sorted_staging[i].get_path());
        if (h_idx == -1) {
            staged_lines.push_back("new file: " + sorted_staging[i].get_path());
        } else if (head_snap[h_idx].get_lines() != sorted_staging[i].get_lines()) {
            staged_lines.push_back("modified: " + sorted_staging[i].get_path());
        }
    }
    for (size_t i = 0; i < head_snap.size(); ++i) {
        if (find_entry_index(sorted_staging, head_snap[i].get_path()) == -1) {
            staged_lines.push_back("deleted: " + head_snap[i].get_path());
        }
    }
    for (size_t i = 0; i < staged_lines.size(); ++i) {
        for (size_t j = i + 1; j < staged_lines.size(); ++j) {
            std::string p1 = staged_lines[i].substr(staged_lines[i].find(": ") + 2);
            std::string p2 = staged_lines[j].substr(staged_lines[j].find(": ") + 2);
            if (p2 < p1)
                std::swap(staged_lines[i], staged_lines[j]);
        }
    }

    // 2. Modified (not staged)
    std::vector<std::string> unstaged_lines;
    for (size_t i = 0; i < sorted_staging.size(); ++i) {
        int w_idx = find_entry_index(ws_files, sorted_staging[i].get_path());
        if (w_idx == -1) {
            unstaged_lines.push_back("deleted: " + sorted_staging[i].get_path());
        } else if (ws_files[w_idx].get_lines() != sorted_staging[i].get_lines()) {
            unstaged_lines.push_back("modified: " + sorted_staging[i].get_path());
        }
    }

    // 3. Untracked files
    std::vector<std::string> untracked_lines;
    for (size_t i = 0; i < ws_files.size(); ++i) {
        if (find_entry_index(sorted_staging, ws_files[i].get_path()) == -1) {
            untracked_lines.push_back(ws_files[i].get_path());
        }
    }

    if (staged_lines.empty() && unstaged_lines.empty() && untracked_lines.empty()) {
        std::cout << "nothing to commit, working tree clean\n";
        return;
    }

    if (!staged_lines.empty()) {
        std::cout << "\nStaged changes:\n";
        for (size_t i = 0; i < staged_lines.size(); ++i)
            std::cout << "  " << staged_lines[i] << '\n';
    }
    if (!unstaged_lines.empty()) {
        std::cout << "\nModified (not staged):\n";
        for (size_t i = 0; i < unstaged_lines.size(); ++i)
            std::cout << "  " << unstaged_lines[i] << '\n';
    }
    if (!untracked_lines.empty()) {
        std::cout << "\nUntracked files:\n";
        for (size_t i = 0; i < untracked_lines.size(); ++i)
            std::cout << "  " << untracked_lines[i] << '\n';
    }
}

void Repository::log() const {
    int curr_id = get_head_commit_id();
    if (curr_id == -1)
        throw GitException("fatal: your current branch does not have any commits yet");

    while (curr_id != -1) {
        const Commit* c = find_commit(curr_id);
        if (c == nullptr)
            break;

        std::cout << "commit " << c->get_id() << '\n';
        std::cout << "Author: " << c->get_author_name()
                  << " <" << c->get_author_email() << ">\n";
        std::cout << "Date: " << c->get_timestamp() << "\n\n";
        std::cout << c->get_message() << '\n';

        curr_id = c->get_parent_id();
        if (curr_id != -1)
            std::cout << '\n';
    }
}

// --- Branching & Phase 2 ---

void Repository::branch_list() const {
    std::vector<Branch> sorted_branches = branches;
    std::sort(sorted_branches.begin(), sorted_branches.end());

    for (size_t i = 0; i < sorted_branches.size(); ++i) {
        if (!is_detached && sorted_branches[i].get_name() == current_branch)
            std::cout << "* " << sorted_branches[i].get_name() << '\n';
        else
            std::cout << "  " << sorted_branches[i].get_name() << '\n';
    }
}

void Repository::branch_create(const std::string& branch_name) {
    int hid = get_head_commit_id();
    if (hid == -1)
        throw GitException("fatal: not a valid object name: 'HEAD' (no commits yet)");

    if (!is_valid_identifier(branch_name) || is_all_digits(branch_name))
        throw GitException("fatal: '" + branch_name + "' is not a valid branch name");

    if (find_branch_index(branch_name) != -1)
        throw GitException("fatal: a branch named '" + branch_name + "' already exists");

    branches.push_back(Branch(branch_name, hid));
}

void Repository::branch_delete(const std::string& branch_name) {
    int idx = find_branch_index(branch_name);
    if (idx == -1)
        throw GitException("error: branch '" + branch_name + "' not found");

    if (!is_detached && current_branch == branch_name)
        throw GitException("error: cannot delete branch '" + branch_name + "' checked out at HEAD");

    branches.erase(branches.begin() + idx);
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