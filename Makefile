CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -O2
BINDIR = bin

# Auth module sources
AUTH_SOURCES = src/auth/device_flow.cpp src/auth/credential_store.cpp
AUTH_OBJECTS = $(AUTH_SOURCES:src/%.cpp=$(BINDIR)/%.o)

CURL_LIBS = $(shell pkg-config --libs libcurl)
JSON_LIBS = $(shell pkg-config --libs nlohmann_json 2>/dev/null || echo "")

.PHONY: all clean directories twitch_irc twitch_vod twitch_chat

all: directories twitch_irc twitch_vod twitch_chat

directories:
	@mkdir -p $(BINDIR)/auth

# Compile auth modules
$(BINDIR)/auth/%.o: src/auth/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# IRC tool (anonymous only, no deps)
twitch_irc: directories
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/twitch_irc src/twitch_irc.cpp

# VOD chat tool (needs curl + json)
twitch_vod: directories
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/twitch_vod_chat src/twitch_vod_chat.cpp $(CURL_LIBS)

# NEW: Interactive chat tool with authentication (needs auth modules)
twitch_chat: directories $(AUTH_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/twitch_chat src/twitch_chat_main.cpp \
		$(AUTH_OBJECTS) $(CURL_LIBS) $(JSON_LIBS)

clean:
	rm -rf $(BINDIR)
