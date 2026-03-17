#include <iostream>
#include <string>
#include "CLI11.hpp"
#include "cli/commands.h"

int main(int argc, char* argv[]) {
    CLI::App app{"CTIC - Clip Detection Engine for Live Streams"};
    app.require_subcommand(1);
    
    // Configure command - setup a channel
    std::string config_channel;
    auto config_cmd = app.add_subcommand("configure", "Configure clip detection for a channel");
    config_cmd->add_option("channel", config_channel, "Twitch channel name or URL")->required();
    config_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_configure(config_channel));
    });
    
    // Monitor command - start monitoring
    std::string monitor_channel;
    auto monitor_cmd = app.add_subcommand("monitor", "Start monitoring chat for clips");
    monitor_cmd->add_option("channel", monitor_channel, "Twitch channel name (optional, uses configured if omitted)");
    monitor_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_monitor(monitor_channel));
    });
    
    // Status command - show configuration
    std::string status_channel;
    auto status_cmd = app.add_subcommand("status", "Show configuration status");
    status_cmd->add_option("channel", status_channel, "Channel name (shows all if omitted)");
    status_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_status(status_channel));
    });
    
    // Remove command - delete configuration
    std::string remove_name;
    auto remove_cmd = app.add_subcommand("remove", "Remove channel configuration");
    remove_cmd->add_option("name", remove_name, "Channel name")->required();
    remove_cmd->callback([&]() {
        std::exit(ctic::cli::cmd_remove(remove_name));
    });
    
    CLI11_PARSE(app, argc, argv);
    
    return 0;
}
