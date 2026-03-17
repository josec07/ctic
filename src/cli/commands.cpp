#include "../../include/cli/commands.h"
#include "../../include/core/config.h"
#include "../../include/providers/twitch_url.h"
#include "../../include/providers/twitch_irc.h"
#include "../../include/core/detection.h"
#include "../../include/core/text.h"
#include "../../include/core/monitor.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <csignal>
#include <atomic>
#include <algorithm>

namespace ctic {
namespace cli {

// Forward declaration
void toggle_tier(core::CreatorConfig& creator, const std::string& tier);

static std::atomic<bool> g_running{true};
static core::Monitor* g_monitor = nullptr;

void signal_handler(int) {
    g_running = false;
    std::cout << "\n[SHUTDOWN] Stopping monitor..." << std::endl;
    if (g_monitor) {
        g_monitor->stop();
    }
}

int cmd_configure(const std::string& url_or_channel) {
    core::ConfigManager config_mgr;
    config_mgr.ensure_ctic_dir();
    
    std::string channel = providers::parse_url_or_channel(url_or_channel);
    if (channel.empty()) {
        std::cerr << "Error: Invalid Twitch URL or channel name" << std::endl;
        return 1;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "CTIC - Single Streamer Configuration" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Channel: " << channel << std::endl;
    std::cout << std::endl;
    
    // Test connection
    std::cout << "[TEST] Connecting to chat..." << std::endl;
    
    providers::TwitchIRC irc;
    if (!irc.connect(channel)) {
        std::cerr << "Error: Failed to connect to #" << channel << std::endl;
        return 1;
    }
    
    std::cout << "[TEST] Connected. Sampling chat activity (10s)..." << std::endl;
    
    int msg_count = 0;
    auto start = std::chrono::steady_clock::now();
    
    while (g_running && std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count() < 10) {
        std::string line = irc.read_line();
        if (!line.empty()) {
            std::string username, content;
            if (irc.parse_message(line, username, content)) {
                msg_count++;
                if (msg_count <= 3) {
                    std::cout << "  " << username << ": " << content << std::endl;
                }
            }
        }
    }
    
    std::cout << "[TEST] Sampled " << msg_count << " messages" << std::endl;
    irc.disconnect();
    std::cout << std::endl;
    
    // Load or create configuration
    core::CreatorConfig creator;
    
    if (config_mgr.creator_exists(channel)) {
        std::cout << "Loading existing configuration..." << std::endl;
        creator = config_mgr.load_creator(channel);
    } else {
        std::cout << "Creating new configuration..." << std::endl;
        creator.name = channel;
        creator.channel = channel;
        creator.twitch_url = "https://twitch.tv/" + channel;
        creator.enabled_tiers = {"high", "high-negative", "medium"};
        creator.profile = "balanced";
        creator.detector_config_id = "default";
        creator.created_at = core::format_timestamp(std::chrono::system_clock::now());
    }
    
    // Display current configuration
    std::cout << std::endl;
    std::cout << "Current Detection Profile: " << creator.profile << std::endl;
    std::cout << "Enabled Tiers: ";
    for (const auto& tier : creator.enabled_tiers) {
        std::cout << tier << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    
    // Configuration menu
    std::cout << "Configuration Options:" << std::endl;
    std::cout << "  1. Change detection profile (balanced/conservative/aggressive)" << std::endl;
    std::cout << "  2. Toggle tier: high (clutch moments)" << std::endl;
    std::cout << "  3. Toggle tier: high-negative (fails/criticism)" << std::endl;
    std::cout << "  4. Toggle tier: medium (general hype)" << std::endl;
    std::cout << "  5. Toggle tier: easy (background chat)" << std::endl;
    std::cout << "  6. Save and exit" << std::endl;
    std::cout << "  0. Cancel and exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Enter choice (0-6): ";
    
    int choice;
    std::cin >> choice;
    std::cin.ignore();
    
    switch (choice) {
        case 1: {
            std::cout << "Enter profile (balanced/conservative/aggressive): ";
            std::string profile;
            std::getline(std::cin, profile);
            if (profile == "balanced" || profile == "conservative" || profile == "aggressive") {
                creator.profile = profile;
                std::cout << "Profile updated to: " << profile << std::endl;
            } else {
                std::cout << "Invalid profile. Keeping: " << creator.profile << std::endl;
            }
            break;
        }
        case 2: toggle_tier(creator, "high"); break;
        case 3: toggle_tier(creator, "high-negative"); break;
        case 4: toggle_tier(creator, "medium"); break;
        case 5: toggle_tier(creator, "easy"); break;
        case 6:
            if (config_mgr.save_creator(creator)) {
                std::cout << std::endl;
                std::cout << "Configuration saved!" << std::endl;
                std::cout << "Config file: .ctic/creators/" << channel << ".json" << std::endl;
                std::cout << std::endl;
                std::cout << "To start monitoring, run:" << std::endl;
                std::cout << "  ./ctic monitor " << channel << std::endl;
            } else {
                std::cerr << "Error: Failed to save configuration" << std::endl;
                return 1;
            }
            break;
        case 0:
            std::cout << "Configuration cancelled." << std::endl;
            return 0;
        default:
            std::cout << "Invalid choice. No changes made." << std::endl;
            return 0;
    }
    
    return 0;
}

void toggle_tier(core::CreatorConfig& creator, const std::string& tier) {
    auto it = std::find(creator.enabled_tiers.begin(), creator.enabled_tiers.end(), tier);
    if (it != creator.enabled_tiers.end()) {
        creator.enabled_tiers.erase(it);
        std::cout << "Tier '" << tier << "' disabled" << std::endl;
    } else {
        creator.enabled_tiers.push_back(tier);
        std::cout << "Tier '" << tier << "' enabled" << std::endl;
    }
}

int cmd_monitor(const std::string& channel) {
    core::ConfigManager config_mgr;
    
    std::string target_channel = channel;
    if (target_channel.empty()) {
        // Try to load the last configured channel
        auto creators = config_mgr.list_creators();
        if (creators.empty()) {
            std::cerr << "Error: No channel configured." << std::endl;
            std::cerr << "Run: ./ctic configure <channel>" << std::endl;
            return 1;
        }
        target_channel = creators[0];
        std::cout << "Using configured channel: " << target_channel << std::endl;
    }
    
    if (!config_mgr.creator_exists(target_channel)) {
        std::cerr << "Error: Channel '" << target_channel << "' not configured." << std::endl;
        std::cerr << "Run: ./ctic configure " << target_channel << std::endl;
        return 1;
    }
    
    auto creator_config = config_mgr.load_creator(target_channel);
    
    std::cout << "========================================" << std::endl;
    std::cout << "CTIC - Clip Detection Engine" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Channel: " << target_channel << std::endl;
    std::cout << "Profile: " << creator_config.profile << std::endl;
    std::cout << "Tiers: ";
    for (const auto& tier : creator_config.enabled_tiers) {
        std::cout << tier << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "Output: .ctic/outputs/" << target_channel << "/" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop monitoring" << std::endl;
    std::cout << std::endl;
    
    // Setup signal handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Create and run monitor
    g_monitor = new core::Monitor(target_channel, config_mgr);
    
    if (!g_monitor->start()) {
        std::cerr << "Error: Failed to start monitor" << std::endl;
        delete g_monitor;
        g_monitor = nullptr;
        return 1;
    }
    
    // Main loop
    auto last_status = std::chrono::steady_clock::now();
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Print status every 30 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_status).count() >= 30) {
            auto state = g_monitor->getState();
            std::cout << "[" << state.messages_processed << " msgs | " 
                     << state.bursts_detected << " clips] ";
            
            // Show recent activity indicator
            static int last_bursts = 0;
            if (state.bursts_detected > last_bursts) {
                std::cout << "*";
                last_bursts = state.bursts_detected;
            }
            std::cout << std::endl;
            
            last_status = now;
        }
    }
    
    // Cleanup
    g_monitor->stop();
    auto state = g_monitor->getState();
    delete g_monitor;
    g_monitor = nullptr;
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Monitoring Complete" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Messages processed: " << state.messages_processed << std::endl;
    std::cout << "Clips detected: " << state.bursts_detected << std::endl;
    std::cout << std::endl;
    std::cout << "Output files:" << std::endl;
    std::cout << "  .ctic/outputs/" << target_channel << "/high/" << std::endl;
    std::cout << "  .ctic/outputs/" << target_channel << "/medium/" << std::endl;
    std::cout << "  .ctic/outputs/" << target_channel << "/easy/" << std::endl;
    std::cout << std::endl;
    
    return 0;
}

int cmd_status(const std::string& channel) {
    core::ConfigManager config_mgr;
    
    if (channel.empty()) {
        // Show all configured channels
        auto creators = config_mgr.list_creators();
        if (creators.empty()) {
            std::cout << "No channels configured." << std::endl;
            std::cout << "Run: ./ctic configure <channel>" << std::endl;
            return 0;
        }
        
        std::cout << "Configured channels:" << std::endl;
        for (const auto& name : creators) {
            auto config = config_mgr.load_creator(name);
            std::cout << "  - " << name << " [" << config.profile << "]" << std::endl;
            std::cout << "    Tiers: ";
            for (const auto& tier : config.enabled_tiers) {
                std::cout << tier << " ";
            }
            std::cout << std::endl;
        }
    } else {
        // Show specific channel
        if (!config_mgr.creator_exists(channel)) {
            std::cerr << "Channel '" << channel << "' not found." << std::endl;
            return 1;
        }
        
        auto config = config_mgr.load_creator(channel);
        std::cout << "Channel: " << channel << std::endl;
        std::cout << "Profile: " << config.profile << std::endl;
        std::cout << "Tiers: ";
        for (const auto& tier : config.enabled_tiers) {
            std::cout << tier << " ";
        }
        std::cout << std::endl;
        std::cout << "Created: " << config.created_at << std::endl;
        std::cout << "Config: .ctic/creators/" << channel << ".json" << std::endl;
    }
    
    return 0;
}

int cmd_remove(const std::string& name) {
    core::ConfigManager config_mgr;
    
    if (!config_mgr.creator_exists(name)) {
        std::cerr << "Channel '" << name << "' not found." << std::endl;
        return 1;
    }
    
    if (config_mgr.remove_creator(name)) {
        std::cout << "Configuration for '" << name << "' removed." << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to remove configuration." << std::endl;
        return 1;
    }
}

}
}
