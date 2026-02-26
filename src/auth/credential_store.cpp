#include "auth/credential_store.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>

namespace twitch_chat {

CredentialStore::CredentialStore() {
    config_dir_ = getConfigDir();
    creds_file_ = config_dir_ + "/credentials.json";
}

std::string CredentialStore::getConfigDir() {
    // Linux/macOS: ~/.config/twitch-chat-cli/
    // Windows: %APPDATA%/twitch-chat-cli/
    
    const char* home = std::getenv("HOME");
    if (!home) {
        home = std::getenv("USERPROFILE");  // Windows
    }
    
    if (!home) {
        return ".";  // Fallback to current directory
    }
    
    std::string config_dir = std::string(home) + "/.config/twitch-chat-cli";
    
    // Create directory if it doesn't exist
    #ifdef _WIN32
        _mkdir(config_dir.c_str());
    #else
        mkdir(config_dir.c_str(), 0700);
    #endif
    
    return config_dir;
}

bool CredentialStore::save(const UserCredentials& creds) {
    try {
        // Load existing credentials
        nlohmann::json j;
        std::ifstream in(creds_file_);
        if (in.good()) {
            in >> j;
        }
        in.close();
        
        // Add/update this user's credentials
        nlohmann::json user_json;
        user_json["username"] = creds.username;
        user_json["access_token"] = creds.access_token;
        user_json["refresh_token"] = creds.refresh_token;
        user_json["expires_at"] = std::chrono::duration_cast<std::chrono::seconds>(
            creds.expires_at.time_since_epoch()).count();
        user_json["scope"] = creds.scope;
        
        j[creds.username] = user_json;
        
        // Save back to file
        std::ofstream out(creds_file_);
        if (!out.good()) {
            return false;
        }
        out << j.dump(2);
        out.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save credentials: " << e.what() << "\n";
        return false;
    }
}

std::optional<UserCredentials> CredentialStore::load(const std::string& username) {
    try {
        std::ifstream in(creds_file_);
        if (!in.good()) {
            return std::nullopt;
        }
        
        nlohmann::json j;
        in >> j;
        in.close();
        
        if (!j.contains(username)) {
            return std::nullopt;
        }
        
        auto user_json = j[username];
        UserCredentials creds;
        creds.username = user_json.value("username", "");
        creds.access_token = user_json.value("access_token", "");
        creds.refresh_token = user_json.value("refresh_token", "");
        creds.scope = user_json.value("scope", "");
        
        auto expires_at_sec = user_json.value("expires_at", 0);
        creds.expires_at = std::chrono::system_clock::time_point(
            std::chrono::seconds(expires_at_sec));
        
        return creds;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load credentials: " << e.what() << "\n";
        return std::nullopt;
    }
}

std::optional<UserCredentials> CredentialStore::loadDefault() {
    try {
        std::ifstream in(creds_file_);
        if (!in.good()) {
            return std::nullopt;
        }
        
        nlohmann::json j;
        in >> j;
        in.close();
        
        // Return first available account
        for (auto& [username, data] : j.items()) {
            return load(username);
        }
        
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

std::vector<std::string> CredentialStore::listAccounts() {
    std::vector<std::string> accounts;
    
    try {
        std::ifstream in(creds_file_);
        if (!in.good()) {
            return accounts;
        }
        
        nlohmann::json j;
        in >> j;
        in.close();
        
        for (auto& [username, data] : j.items()) {
            accounts.push_back(username);
        }
    } catch (...) {
        // Return empty list on error
    }
    
    return accounts;
}

bool CredentialStore::remove(const std::string& username) {
    try {
        std::ifstream in(creds_file_);
        if (!in.good()) {
            return false;
        }
        
        nlohmann::json j;
        in >> j;
        in.close();
        
        if (!j.contains(username)) {
            return false;
        }
        
        j.erase(username);
        
        std::ofstream out(creds_file_);
        if (!out.good()) {
            return false;
        }
        out << j.dump(2);
        out.close();
        
        return true;
    } catch (...) {
        return false;
    }
}

std::string CredentialStore::getStoragePath() const {
    return creds_file_;
}

} // namespace twitch_chat
