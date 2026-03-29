# Makefile for output plugins
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -Wall -O2
LDFLAGS = -shared

# All output plugins
PLUGINS = comprehensive_csv.so

all: $(PLUGINS)

%.so: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(PLUGINS)

.PHONY: all clean