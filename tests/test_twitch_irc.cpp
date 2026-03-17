#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include <iostream>
#include "providers/twitch_irc.h" 

TEST_CASE("TwitchIRC can connect to Twitch", "[twitch]") {
    // Replace with your application token and channel
    std::string host = "irc.twitch.tv";
    int port = 6667;
    std::string password = "oauth:3k5p5o5s36u38x4p8nxyclo24wouxe";
    std::string channel = "tfue";

    ctic::providers::TwitchIRC twitch_irc(host, port, password, channel);

    REQUIRE(twitch_irc.connect());
}

TEST_CASE("TwitchIRC can read a line from Twitch", "[twitch]") {
    // Replace with your application token and channel
    std::string host = "irc.twitch.tv";
    int port = 6667;
    std::string password = "oauth:3k5p5o5s36u38x4p8nxyclo24wouxe";
    std::string channel = "tfue";

    ctic::providers::TwitchIRC twitch_irc(host, port, password, channel);

    REQUIRE(twitch_irc.connect());
    std::string line = twitch_irc.readLine();

    REQUIRE(!line.empty());
    std::cout << "Received: " << line << std::endl;
}

TEST_CASE("TwitchIRC can parse a message", "[twitch]") {
    std::string raw = ":justin!justin@justin.tmi.twitch.tv PRIVMSG #justin :Hello, world!";
    std::string username, content;

    ctic::providers::TwitchIRC twitch_irc("irc.twitch.tv", 6667, "oauth:test", "test");
    REQUIRE(twitch_irc.parse_message(raw, username, content));
    REQUIRE(username == "justin");
    REQUIRE(content == "Hello, world!");
}
