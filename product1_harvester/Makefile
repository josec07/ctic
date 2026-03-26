CXX = g++
CXXFLAGS = -std=c++17 -I./include -O2
LDFLAGS = -lzmq -lpthread

TARGET = harvester
SRCS = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)
