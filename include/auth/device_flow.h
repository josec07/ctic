#pragma once

#include <string>
#include <optional>
#include <chrono>

namespace twitch_chat {

// Step 1: Device Authorization Response
// When we request a device code, Twitch returns this
struct DeviceAuthorization {
    std::string device_code;      // Internal code for polling (like "abc123xyz")
    std::string user_code;        // Code user types in browser (like "ABCD-EFGH")
    std::string verification_uri; // URL user visits (https://www.twitch.tv/activate)
    int expires_in;               // Seconds until code expires (usually 1800 = 30 min)
    int interval;                 // Seconds between polls (usually 5)
};

// Step 3: Access Token Response
// When polling succeeds, we get this
struct AccessToken {
    std::string access_token;     // The actual OAuth token (oauth:xxxxx)
    std::string refresh_token;    // Token to get new access_token when it expires
    int expires_in;               // Seconds until this token expires
    std::string scope;            // Permissions granted (chat:read chat:edit)
};

// Main Device Flow handler
class DeviceFlowAuth {
public:
    DeviceFlowAuth(const std::string& client_id);
    
    // Step 1: Request device code from Twitch
    // Returns: Device codes + instructions for user, or error
    std::optional<DeviceAuthorization> requestDeviceCode();
    
    // Step 3: Poll for access token
    // Returns: Access token when user approves, nullopt if pending/error
    // Should be called every 'interval' seconds (usually 5s)
    std::optional<AccessToken> pollForToken(const std::string& device_code);
    
    // Full flow: Request code → wait for user → poll → return token
    // This blocks and handles the entire flow with user prompts
    std::optional<AccessToken> authenticateInteractive();
    
    const std::string& getLastError() const { return last_error_; }
    
private:
    std::string client_id_;
    std::string last_error_;
    
    // HTTP helper - we'll implement this
    std::string httpPost(const std::string& url, const std::string& data);
};

} // namespace twitch_chat
