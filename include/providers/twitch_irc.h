#ifndef TWEETHC_IRC_H
#define TWEETHC_IRC_H

#include "network/irc_connection.h"
#include <string>
#include <sstream>
#include <algorithm>

namespace ctic {
namespace providers {

class TwitchIRC {
public:
    TwitchIRC(const std::string& host, int port, const std::string& password, const std::string& channel);
    void run();
    bool connect();
    std::string readLine();
    std::string extract_twitch_channel_from_url(const std::string& url);
    bool parse_message(const std::string& raw, std::string& username, std::string& content);
    void disconnect();
    bool test_connection(const std::string& channel, int timeout_seconds);

private:
    ctic::network::IrcConnection irc_connection_;
    std::string password_;
    std::string channel_;
    bool connected_;
    int message_count_;
};

}
}
#endif