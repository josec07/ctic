# Makefile for preprocessing plugins
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -Wall -O2
LDFLAGS = -shared

# All preprocessing plugins
PLUGINS = tokenizer.so

all: $(PLUGINS)

%.so: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(PLUGINS)

.PHONY: all clean