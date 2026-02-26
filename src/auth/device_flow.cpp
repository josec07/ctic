#include "auth/device_flow.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

namespace twitch_chat {

// Helper: Write callback for curl
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

DeviceFlowAuth::DeviceFlowAuth(const std::string& client_id) 
    : client_id_(client_id) {}

// Step 1: Request device code from Twitch
// This is where the magic starts! We ask Twitch for a device code.
// POST https://id.twitch.tv/oauth2/device
// Body: client_id=xxx&scopes=chat:read+chat:edit
std::optional<DeviceAuthorization> DeviceFlowAuth::requestDeviceCode() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        last_error_ = "Failed to initialize curl";
        return std::nullopt;
    }
    
    std::string readBuffer;
    std::string postData = "client_id=" + client_id_ + 
                           "&scopes=chat:read+chat:edit";
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://id.twitch.tv/oauth2/device");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        last_error_ = "HTTP request failed: " + std::string(curl_easy_strerror(res));
        return std::nullopt;
    }
    
    try {
        auto json = nlohmann::json::parse(readBuffer);
        
        // Check for errors
        if (json.contains("error")) {
            last_error_ = json.value("error", "Unknown error") + 
                         ": " + json.value("error_description", "");
            return std::nullopt;
        }
        
        DeviceAuthorization auth;
        auth.device_code = json.value("device_code", "");
        auth.user_code = json.value("user_code", "");
        auth.verification_uri = json.value("verification_uri", "");
        auth.expires_in = json.value("expires_in", 1800);
        auth.interval = json.value("interval", 5);
        
        if (auth.device_code.empty() || auth.user_code.empty()) {
            last_error_ = "Invalid response from Twitch";
            return std::nullopt;
        }
        
        return auth;
    } catch (const std::exception& e) {
        last_error_ = "JSON parse error: " + std::string(e.what());
        return std::nullopt;
    }
}

// Step 3: Poll for access token
// We repeatedly ask Twitch: "Did the user approve yet?"
// POST https://id.twitch.tv/oauth2/token
// Body: client_id=xxx&device_code=xxx&grant_type=urn:ietf:params:oauth:grant-type:device_code
std::optional<AccessToken> DeviceFlowAuth::pollForToken(const std::string& device_code) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        last_error_ = "Failed to initialize curl";
        return std::nullopt;
    }
    
    std::string readBuffer;
    std::string postData = "client_id=" + client_id_ +
                           "&device_code=" + device_code +
                           "&grant_type=urn:ietf:params:oauth:grant-type:device_code";
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://id.twitch.tv/oauth2/token");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        last_error_ = "HTTP request failed: " + std::string(curl_easy_strerror(res));
        return std::nullopt;
    }
    
    try {
        auto json = nlohmann::json::parse(readBuffer);
        
        // Check for "authorization_pending" - this means user hasn't approved yet
        if (json.contains("error")) {
            std::string error = json.value("error", "");
            if (error == "authorization_pending") {
                // Not an error, just not ready yet
                last_error_ = "authorization_pending";
                return std::nullopt;
            } else if (error == "slow_down") {
                last_error_ = "slow_down";
                return std::nullopt;
            } else {
                last_error_ = error + ": " + json.value("error_description", "");
                return std::nullopt;
            }
        }
        
        AccessToken token;
        token.access_token = json.value("access_token", "");
        token.refresh_token = json.value("refresh_token", "");
        token.expires_in = json.value("expires_in", 0);
        token.scope = json.value("scope", "");
        
        if (token.access_token.empty()) {
            last_error_ = "Invalid token response";
            return std::nullopt;
        }
        
        return token;
    } catch (const std::exception& e) {
        last_error_ = "JSON parse error: " + std::string(e.what());
        return std::nullopt;
    }
}

// Full interactive flow
// This orchestrates the entire process:
// 1. Get device code
// 2. Show user instructions
// 3. Poll until success or timeout
std::optional<AccessToken> DeviceFlowAuth::authenticateInteractive() {
    std::cout << "Requesting authorization from Twitch...\n";
    
    auto auth = requestDeviceCode();
    if (!auth) {
        std::cerr << "Failed to get device code: " << last_error_ << "\n";
        return std::nullopt;
    }
    
    // Step 2: Show user what to do
    std::cout << "\n========================================\n";
    std::cout << "To authenticate, please:\n";
    std::cout << "1. Open this URL in your browser:\n";
    std::cout << "   " << auth->verification_uri << "\n";
    std::cout << "2. Enter this code: " << auth->user_code << "\n";
    std::cout << "3. Click "Authorize" on the Twitch website\n";
    std::cout << "========================================\n\n";
    std::cout << "Waiting for you to authenticate...\n";
    
    // Step 3: Poll until user approves or timeout
    auto start = std::chrono::steady_clock::now();
    int interval = auth->interval;
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(interval));
        
        auto token = pollForToken(auth->device_code);
        if (token) {
            std::cout << "✓ Authentication successful!\n";
            return token;
        }
        
        // Check if it's just pending or a real error
        if (last_error_ != "authorization_pending" && last_error_ != "slow_down") {
            std::cerr << "Authentication failed: " << last_error_ << "\n";
            return std::nullopt;
        }
        
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        
        if (elapsed >= auth->expires_in) {
            std::cerr << "Authentication timed out (code expired)\n";
            return std::nullopt;
        }
        
        // Show progress every 30 seconds
        if (elapsed % 30 == 0) {
            int remaining = (auth->expires_in - elapsed) / 60;
            std::cout << "Still waiting... (" << remaining << " minutes remaining)\n";
        }
    }
}

} // namespace twitch_chat
