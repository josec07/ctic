#pragma once

#include <string>
#include <optional>
#include <vector>
#include <chrono>
#include "auth/device_flow.h"

namespace twitch_chat {

// Stored credentials for a Twitch account
struct UserCredentials {
    std::string username;         // Twitch username
    std::string access_token;     // OAuth token
    std::string refresh_token;    // For getting new access token
    std::chrono::system_clock::time_point expires_at;  // When token expires
    std::string scope;            // Permissions (chat:read chat:edit)
};

// Simple JSON-based credential storage
// Stores in ~/.config/twitch-cli/credentials.json (Linux/Mac)
// or %APPDATA%/twitch-cli/credentials.json (Windows)
class CredentialStore {
public:
    CredentialStore();
    
    // Save credentials for a user
    bool save(const UserCredentials& creds);
    
    // Load credentials for a username
    std::optional<UserCredentials> load(const std::string& username);
    
    // Load default/first available credentials
    std::optional<UserCredentials> loadDefault();
    
    // List all saved accounts
    std::vector<std::string> listAccounts();
    
    // Delete credentials for a user
    bool remove(const std::string& username);
    
    // Get storage path (for debugging)
    std::string getStoragePath() const;
    
private:
    std::string config_dir_;
    std::string creds_file_;
    
    std::string getConfigDir();
};

} // namespace twitch_chat
