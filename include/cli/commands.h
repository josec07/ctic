#pragma once

#include <string>
#include "../core/config.h"

namespace ctic {
namespace cli {

// Single-streamer configuration
int cmd_configure(const std::string& url_or_channel);
void toggle_tier(core::CreatorConfig& creator, const std::string& tier);

// Single-streamer monitoring
int cmd_monitor(const std::string& channel);

// Status and management (simplified for single channel)
int cmd_status(const std::string& channel);
int cmd_remove(const std::string& name);

// Legacy commands (deprecated, kept for compatibility)
int cmd_add(const std::string& url_or_channel);  // redirects to configure
int cmd_list();  // redirects to status
int cmd_run();   // redirects to monitor

}
}
