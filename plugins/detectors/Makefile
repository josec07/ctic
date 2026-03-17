# Makefile for detector plugins
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -Wall -O2
LDFLAGS = -shared

# All detector plugins
PLUGINS = spike_detector.so aggregator.so

all: $(PLUGINS)

%.so: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(PLUGINS)

.PHONY: all clean