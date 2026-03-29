# CTIC Pipeline Engine Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I./include
LDFLAGS = -ldl -pthread

# Target executables
TARGET = ctic
CLI_TARGET = ctic-cli

# Source files for pipeline engine
SOURCES = src/main_pipeline.cpp \
          src/models/model_config.cpp \
          src/models/model_manager.cpp

# Source files for CLI tool
CLI_SOURCES = src/main.cpp \
              src/cli/commands.cpp \
              src/providers/twitch_irc.cpp \
              src/network/irc_connection.cpp \
              src/core/config.cpp \
              src/core/text.cpp

# Build directories
BUILDDIR = build
PLUGINDIR = plugins
OUTPUTDIR = outputs

# Default target - build both
all: $(TARGET) $(CLI_TARGET) plugins

# Build main executable (pipeline engine)
$(TARGET): $(SOURCES) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build CLI tool with monitor command
$(CLI_TARGET): $(CLI_SOURCES) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built CLI tool: ./$(CLI_TARGET)"

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
	rm -f $(TARGET) $(CLI_TARGET)
	rm -rf $(BUILDDIR) $(OUTPUTDIR)
	$(MAKE) -C $(PLUGINDIR) clean

# Install (copies to /usr/local/bin)
install: $(TARGET) $(CLI_TARGET)
	cp $(TARGET) /usr/local/bin/
	cp $(CLI_TARGET) /usr/local/bin/
	mkdir -p /usr/local/share/ctic
	cp -r config /usr/local/share/ctic/
	cp -r plugins /usr/local/share/ctic/

# Help
help:
	@echo "CTIC Pipeline Engine"
	@echo ""
	@echo "Build targets:"
	@echo "  make          - Build pipeline engine + CLI tool"
	@echo "  make $(TARGET)   - Build pipeline engine only"
	@echo "  make $(CLI_TARGET) - Build CLI tool with monitor command"
	@echo "  make plugins  - Build only plugins"
	@echo ""
	@echo "Run pipeline:"
	@echo "  ./ctic pipeline run --template simple_spike --channel shroud"
	@echo ""
	@echo "Run CLI monitor:"
	@echo "  ./ctic-cli configure <channel>  - Configure a channel"
	@echo "  ./ctic-cli monitor <channel>    - Monitor chat for clips"
	@echo "  ./ctic-cli status               - Show configuration"

.PHONY: all plugins test clean install help