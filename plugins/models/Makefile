# Makefile for models plugin
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -O2 \
    -I../../include \
    -I../../include/nlohmann

LDFLAGS = -shared

TARGET = onnx_inference.so
SRC = onnx_inference.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
