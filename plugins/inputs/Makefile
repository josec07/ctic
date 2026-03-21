CXX = g++
CXXFLAGS = -std=c++17 -fPIC -shared -I../../include -I../../
LDFLAGS = -shared

SOURCES = chat_replay.cpp
OUTPUTS = $(SOURCES:.cpp=.so)

all: $(OUTPUTS)

%.so: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.so

.PHONY: all clean