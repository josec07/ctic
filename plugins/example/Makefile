# Makefile for example plugin
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -Wall -O2
LDFLAGS = -shared

# Plugin name
PLUGIN_NAME = text_normalizer.so

# Source files
SOURCES = text_normalizer.cpp

# Build plugin
$(PLUGIN_NAME): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# Clean
clean:
	rm -f $(PLUGIN_NAME)

.PHONY: clean