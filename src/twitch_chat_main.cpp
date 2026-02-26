#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include "twitch_irc.h"
#include "auth/device_flow.h"
#include "auth/credential_store.h"

// You'll need to register a Twitch app to get a client ID
// For testing, users can get tokens from twitchtokengenerator.com
const std::string TWITCH_CLIENT_ID = "YOUR_CLIENT_ID_HERE";  // TODO: Replace with real ID

using namespace twitch_chat;

void printUsage(const char* program) {
    std::cout << "Twitch Chat CLI - Interactive chat with authentication\n\n";
    std::cout << "Usage: " << program << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  login              Authenticate with Twitch (device flow)\n";
    std::cout << "  logout [username]  Remove saved credentials\n";
    std::cout << "  list-accounts      Show saved accounts\n";
    std::cout << "  chat --channel <ch> [--anonymous]  Join chat\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program << " login                    # Authenticate\n";
    std::cout << "  " << program << " chat --channel shroud   # Join as authenticated user\n";
    std::cout << "  " << program << " chat --channel shroud --anonymous  # Join anonymously\n";
}

int cmdLogin() {
    std::cout << "Twitch Chat CLI - Login\n";
    std::cout << "=======================\n\n";
    
    DeviceFlowAuth auth(TWITCH_CLIENT_ID);
    auto token = auth.authenticateInteractive();
    
    if (!token) {
        std::cerr << "Authentication failed.\n";
        return 1;
    }
    
    // We need to validate the token to get the username
    // For now, ask the user
    std::cout << "\nEnter your Twitch username: ";
    std::string username;
    std::getline(std::cin, username);
    
    // Save credentials
    UserCredentials creds;
    creds.username = username;
    creds.access_token = token->access_token;
    creds.refresh_token = token->refresh_token;
    creds.expires_at = std::chrono::system_clock::now() + 
                       std::chrono::seconds(token->expires_in);
    creds.scope = token->scope;
    
    CredentialStore store;
    if (store.save(creds)) {
        std::cout << "\n✓ Login successful! Credentials saved.\n";
        std::cout << "Storage location: " << store.getStoragePath() << "\n";
        return 0;
    } else {
        std::cerr << "\n✗ Failed to save credentials.\n";
        return 1;
    }
}

int cmdLogout(const std::string& username) {
    CredentialStore store;
    
    if (username.empty()) {
        // List accounts and ask which to remove
        auto accounts = store.listAccounts();
        if (accounts.empty()) {
            std::cout << "No saved accounts found.\n";
            return 0;
        }
        
        std::cout << "Select account to logout:\n";
        for (size_t i = 0; i < accounts.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << accounts[i] << "\n";
        }
        std::cout << "Enter number: ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore();  // Clear newline
        
        if (choice < 1 || choice > static_cast<int>(accounts.size())) {
            std::cerr << "Invalid selection.\n";
            return 1;
        }
        
        if (store.remove(accounts[choice - 1])) {
            std::cout << "✓ Logged out " << accounts[choice - 1] << "\n";
            return 0;
        }
    } else {
        if (store.remove(username)) {
            std::cout << "✓ Logged out " << username << "\n";
            return 0;
        } else {
            std::cerr << "✗ Account not found: " << username << "\n";
            return 1;
        }
    }
    
    return 0;
}

int cmdListAccounts() {
    CredentialStore store;
    auto accounts = store.listAccounts();
    
    if (accounts.empty()) {
        std::cout << "No saved accounts.\n";
        std::cout << "Run 'twitch_chat login' to authenticate.\n";
        return 0;
    }
    
    std::cout << "Saved accounts:\n";
    for (const auto& account : accounts) {
        auto creds = store.load(account);
        if (creds) {
            auto now = std::chrono::system_clock::now();
            bool expired = now > creds->expires_at;
            std::cout << "  - " << account;
            if (expired) {
                std::cout << " [expired]";
            } else {
                std::cout << " [valid]";
            }
            std::cout << "\n";
        }
    }
    
    return 0;
}

// Rate limiter: 20 messages per 30 seconds
class RateLimiter {
public:
    bool canSend() {
        auto now = std::chrono::steady_clock::now();
        
        // Remove messages older than 30 seconds
        while (!messages_.empty() && 
               now - messages_.front() > std::chrono::seconds(30)) {
            messages_.pop();
        }
        
        // Check if under limit
        if (messages_.size() < 20) {
            messages_.push(now);
            return true;
        }
        
        return false;
    }
    
    int secondsUntilNext() {
        if (messages_.size() < 20) return 0;
        
        auto now = std::chrono::steady_clock::now();
        auto oldest = messages_.front();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - oldest).count();
        
        return std::max(0, 30 - static_cast<int>(elapsed));
    }
    
private:
    std::queue<std::chrono::steady_clock::time_point> messages_;
};

int cmdChat(int argc, char* argv[]) {
    std::string channel;
    bool anonymous = false;
    std::string username;
    
    // Parse args
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--channel" && i + 1 < argc) {
            channel = argv[++i];
        } else if (arg == "--anonymous") {
            anonymous = true;
        } else if (arg == "--username" && i + 1 < argc) {
            username = argv[++i];
        }
    }
    
    if (channel.empty()) {
        std::cerr << "Error: --channel is required\n";
        return 1;
    }
    
    chatclipper::TwitchIRC irc;
    
    // Connect
    std::cout << "Connecting to Twitch IRC...\n";
    if (!irc.connect()) {
        std::cerr << "Connection failed: " << irc.getLastError() << "\n";
        return 1;
    }
    
    // Authenticate
    std::string oauth_token;
    if (!anonymous) {
        // Try to load credentials
        CredentialStore store;
        std::optional<UserCredentials> creds;
        
        if (!username.empty()) {
            creds = store.load(username);
        } else {
            creds = store.loadDefault();
        }
        
        if (!creds) {
            std::cerr << "No credentials found. Run 'twitch_chat login' first, "
                     << "or use --anonymous for read-only mode.\n";
            return 1;
        }
        
        oauth_token = "oauth:" + creds->access_token;
        username = creds->username;
        
        std::cout << "Authenticating as " << username << "...\n";
        if (!irc.authenticate(oauth_token, username)) {
            std::cerr << "Authentication failed: " << irc.getLastError() << "\n";
            std::cerr << "Your token may have expired. Run 'twitch_chat login' again.\n";
            return 1;
        }
    } else {
        std::cout << "Connecting anonymously (read-only)...\n";
        if (!irc.authenticateAnonymous()) {
            std::cerr << "Anonymous auth failed: " << irc.getLastError() << "\n";
            return 1;
        }
    }
    
    // Join channel
    if (!irc.joinChannel(channel)) {
        std::cerr << "Failed to join channel: " << irc.getLastError() << "\n";
        return 1;
    }
    
    std::cout << "\n✓ Connected to #" << channel << "\n";
    std::cout << "Type messages to chat. Press Ctrl+C to exit.\n\n";
    
    // Interactive chat loop
    RateLimiter limiter;
    std::atomic<bool> running{true};
    
    // Message reader thread
    std::thread reader([&]() {
        while (running) {
            auto msg = irc.readMessage(1000);
            if (msg) {
                std::cout << "[" << msg->username << "] " << msg->content << "\n";
            }
        }
    });
    
    // Input loop
    std::string input;
    while (running && std::getline(std::cin, input)) {
        if (input.empty()) continue;
        
        if (!anonymous) {
            if (limiter.canSend()) {
                irc.sendRaw("PRIVMSG #" + channel + " :" + input);
                std::cout << "[You] " << input << "\n";
            } else {
                int wait = limiter.secondsUntilNext();
                std::cout << "[Rate limited: wait " << wait << "s]\n";
            }
        } else {
            std::cout << "[Anonymous mode: cannot send messages]\n";
        }
    }
    
    running = false;
    reader.join();
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string cmd = argv[1];
    
    if (cmd == "login") {
        return cmdLogin();
    } else if (cmd == "logout") {
        std::string username;
        if (argc > 2) username = argv[2];
        return cmdLogout(username);
    } else if (cmd == "list-accounts" || cmd == "list") {
        return cmdListAccounts();
    } else if (cmd == "chat") {
        return cmdChat(argc, argv);
    } else if (cmd == "--help" || cmd == "-h") {
        printUsage(argv[0]);
        return 0;
    } else {
        std::cerr << "Unknown command: " << cmd << "\n\n";
        printUsage(argv[0]);
        return 1;
    }
}
