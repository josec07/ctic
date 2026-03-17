CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -I../include -I.
LIBS = -lpthread

CATCH_SRC = catch_amalgamated.cpp

CORE_SRC = ../src/core/text.cpp \
           ../src/core/spike_detector.cpp \
           ../src/core/detection.cpp \
           ../src/core/config.cpp \
           ../src/core/chat_buffer.cpp

NETWORK_SRC = ../src/network/irc_connection.cpp

PROVIDER_SRC = ../src/providers/twitch_irc.cpp

TEST_SRC = test_text.cpp \
           test_url.cpp \
           test_spike_detector.cpp \
           test_detector.cpp \
           test_config.cpp \
           test_twitch_irc.cpp

all: test_runner

test_runner: $(CATCH_SmakRC) $(TEST_SRC) $(CORE_SRC) $(NETWORK_SRC) $(PROVIDER_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(CATCH_SRC) $(TEST_SRC) $(CORE_SRC) $(NETWORK_SRC) $(PROVIDER_SRC) $(LIBS)

test: test_runner
	./test_runner "[core]"

test_text: test_runner
	./test_runner "[text]"

test_url: test_runner
	./test_runner "[url]"

test_spike: test_runner
	./test_runner "[spike]"

test_detector: test_runner
	./test_runner "[detector]"

test_config: test_runner
	./test_runner "[config]"

test_twitch: test_runner
	./test_runner "[twitch]"

clean:
	rm -f test_runner

.PHONY: all test test_text test_url test_spike test_detector test_config test_twitch clean
