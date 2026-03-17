# CTIC Pipeline Engine Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I./include
LDFLAGS = -ldl -pthread

# Target executable
TARGET = ctic

# Source files for pipeline engine
SOURCES = src/main_pipeline.cpp

# Build directories
BUILDDIR = build
PLUGINDIR = plugins
OUTPUTDIR = outputs

# Default target
all: $(TARGET) plugins

# Build main executable
$(TARGET): $(SOURCES) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build plugins
plugins:
	$(MAKE) -C $(PLUGINDIR) all

# Create build directories
$(BUILDDIR) $(OUTPUTDIR):
	mkdir -p $@

# Run tests
test: all
	./$(TARGET) pipeline test

# Clean build files
clean:
	rm -f $(TARGET)
	rm -rf $(BUILDDIR) $(OUTPUTDIR)
	$(MAKE) -C $(PLUGINDIR) clean

# Install (copies to /usr/local/bin)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	mkdir -p /usr/local/share/ctic
	cp -r config /usr/local/share/ctic/
	cp -r plugins /usr/local/share/ctic/

# Help
help:
	@echo "CTIC Pipeline Engine"
	@echo ""
	@echo "Usage:"
	@echo "  make          - Build everything"
	@echo "  make plugins  - Build only plugins"
	@echo "  make test     - Run tests"
	@echo "  make clean    - Remove build files"
	@echo "  make install  - Install to system"
	@echo ""
	@echo "Run pipeline:"
	@echo "  ./ctic pipeline run --template simple_spike --channel shroud"
	@echo "  ./ctic pipeline run --config my_pipeline.json"

.PHONY: all plugins test clean install help