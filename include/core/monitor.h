#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include "config.h"
#include "chat_types.h"
#include "chat_buffer.h"
#include "../providers/twitch_irc.h"
#include <iostream>

namespace ctic {
namespace core {

struct MonitorState {
    int messages_processed = 0;
    int bursts_detected = 0;
    std::chrono::system_clock::time_point start_time;
    bool running = false;
};

class Monitor {
public:
    Monitor(const std::string& channel, ConfigManager& config_mgr) 
        : channel_(channel), config_mgr_(config_mgr), running_(false) {
        state_.start_time = std::chrono::system_clock::now();
    }
    
    bool start() {
        // Initialize Twitch IRC connection
        irc_ = new providers::TwitchIRC("irc.twitch.tv", 6667, "SCHMOOPIIE", channel_);
        
        if (!irc_->connect()) {
            std::cerr << "[Monitor] Failed to connect to Twitch IRC" << std::endl;
            return false;
        }
        
        running_ = true;
        state_.running = true;
        
        // Start monitoring thread
        monitor_thread_ = std::thread([this]() {
            this->run();
        });
        
        return true;
    }
    
    void stop() {
        running_ = false;
        state_.running = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        if (irc_) {
            irc_->disconnect();
            delete irc_;
            irc_ = nullptr;
        }
    }
    
    MonitorState getState() const {
        return state_;
    }
    
private:
    void run() {
        while (running_) {
            std::string line = irc_->readLine();
            if (!line.empty()) {
                std::string username, content;
                if (irc_->parse_message(line, username, content)) {
                    state_.messages_processed++;
                    // Simple burst detection - just count for now
                    if (state_.messages_processed % 100 == 0) {
                        state_.bursts_detected++;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    std::string channel_;
    ConfigManager& config_mgr_;
    providers::TwitchIRC* irc_ = nullptr;
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    MonitorState state_;
};

}
}
