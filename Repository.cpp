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