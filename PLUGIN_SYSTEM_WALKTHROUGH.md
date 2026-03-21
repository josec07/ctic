# CTIC Plugin System Walkthrough
## Understanding the Current Architecture (March 2024)

This document explains how the CTIC plugin system **currently works** - not future plans, but the actual implementation as it exists today.

---

## Table of Contents
1. [Core Concepts](#core-concepts)
2. [Plugin Architecture](#plugin-architecture)
3. [How Plugins Are Built](#how-plugins-are-built)
4. [How Plugins Are Loaded](#how-plugins-are-loaded)
5. [Data Flow Through Nodes](#data-flow-through-nodes)
6. [Creating Your Own Plugin](#creating-your-own-plugin)
7. [Current Plugin Inventory](#current-plugin-inventory)
8. [Known Issues & Quirks](#known-issues--quirks)

---

## Core Concepts

### What is a Node?
A node is the fundamental processing unit in CTIC. Each node:
- Takes input data (messages, text, etc.)
- Processes it somehow (detection, transformation, output)
- Passes results to the next node
- Tracks its contribution to the detection (lineage)

### The INode Interface
Every plugin must implement the `INode` interface defined in [`include/pipeline/node.h`](include/pipeline/node.h):

```cpp
class INode {
public:
    virtual bool initialize(const nlohmann::json& config) = 0;
    virtual void shutdown() = 0;
    virtual NodeResult process(const std::vector<NodeData>& inputs) = 0;
    virtual bool canRunParallel() const { return false; }
    virtual std::string getType() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
};
```

### NodeData Structure
Data flows between nodes as `NodeData` objects ([`include/pipeline/node.h:63-94`](include/pipeline/node.h)):

```cpp
struct NodeData {
    std::string type;           // "text", "normalized_text", etc.
    std::any payload;           // The actual data
    nlohmann::json metadata;    // Additional info
    Lineage lineage;           // Tracking info (unique to CTIC!)
};
```

### The Lineage System (CTIC's Secret Sauce)
Unlike other tools, CTIC tracks WHY something was detected ([`include/pipeline/node.h:16-60`](include/pipeline/node.h)):

```cpp
struct Lineage {
    std::vector<std::string> path;                    // Node path: input→spike→output
    std::map<std::string, float> node_scores;         // spike_detector: 0.95
    std::map<std::string, std::string> node_reasons;  // spike_detector: "Z-score: 4.5"
    std::vector<std::string> triggers;                // ["spike", "sentiment"]
};
```

---

## Plugin Architecture

### Directory Structure
```
plugins/
├── Makefile                  # Master makefile
├── inputs/                   # Input source plugins
│   ├── chat_replay.cpp      # VOD replay plugin
│   └── Makefile
├── preprocessing/            # Text processing plugins
│   ├── tokenizer.cpp
│   └── Makefile
├── detectors/               # Detection algorithms
│   ├── spike_detector.cpp   # Statistical spike detection
│   ├── aggregator.cpp       # Combines multiple signals
│   └── Makefile
└── outputs/                 # Output writers
    ├── comprehensive_csv.cpp # CSV with full lineage
    └── Makefile
```

### Plugin Entry Points
Every plugin MUST export these C functions ([`plugins/detectors/spike_detector.cpp:127-145`](plugins/detectors/spike_detector.cpp)):

```cpp
extern "C" {
    // Return version string
    const char* get_plugin_version() { 
        return "1.0.0"; 
    }
    
    // Return array of node types this plugin provides
    const char** get_node_types() {
        static const char* types[] = {"spike_detector", nullptr};
        return types;
    }
    
    // Create a node instance
    INode* create_node(const char* type) {
        if (std::string(type) == "spike_detector") {
            return new SpikeDetectorNode();
        }
        return nullptr;
    }
    
    // Clean up a node instance
    void destroy_node(INode* node) {
        delete node;
    }
}
```

---

## How Plugins Are Built

### Current Build System (Makefiles)

Each plugin directory has a simple Makefile ([`plugins/detectors/Makefile`](plugins/detectors/Makefile)):

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -Wall -O2
LDFLAGS = -shared

PLUGINS = spike_detector.so aggregator.so

%.so: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
```

### Building Everything
```bash
# From project root
make clean
make all

# This runs:
# 1. Builds main executable (ctic)
# 2. Builds all plugins as .so files
# 3. Places .so files in their directories
```

### What Gets Created
```
ctic                              # Main executable
plugins/inputs/chat_replay.so     # Input plugin
plugins/detectors/spike_detector.so  # Detector plugin
plugins/outputs/comprehensive_csv.so # Output plugin
```

---

## How Plugins Are Loaded

### The Registry System
[`include/pipeline/registry.h`](include/pipeline/registry.h) handles dynamic loading:

```cpp
class NodeRegistry {
    // Load a single plugin
    bool loadPlugin(const std::string& path) {
        // 1. Open .so file with dlopen()
        handle = dlopen(path.c_str(), RTLD_LAZY);
        
        // 2. Get function pointers
        create_node = (INode*(*)(const char*))dlsym(handle, "create_node");
        destroy_node = (void(*)(INode*))dlsym(handle, "destroy_node");
        
        // 3. Register available node types
        const char** types = get_node_types();
        for (int i = 0; types[i]; i++) {
            node_to_plugin[types[i]] = path;
        }
    }
};
```

### Loading Process (main_pipeline.cpp)
[`src/main_pipeline.cpp:145-152`](src/main_pipeline.cpp) shows the loading sequence:

```cpp
// Create registry
NodeRegistry registry;

// Load plugins from directories
registry.loadPluginsFromDirectory("plugins/preprocessing");
registry.loadPluginsFromDirectory("plugins/detectors");
registry.loadPluginsFromDirectory("plugins/outputs");
// NOTE: inputs/ directory is NOT loaded! (bug?)
```

### Creating Node Instances
When a pipeline needs a node:

```cpp
// Pipeline JSON specifies: {"type": "spike_detector"}
auto node = registry.createNode("spike_detector");
// 1. Registry looks up which plugin provides "spike_detector"
// 2. Calls that plugin's create_node() function
// 3. Returns the INode* instance
```

---

## Data Flow Through Nodes

### Pipeline Execution Flow
[`include/pipeline/executor.h`](include/pipeline/executor.h) manages execution:

1. **Pipeline loads configuration**
   ```json
   {
     "nodes": [
       {"id": "input", "type": "chat_replay"},
       {"id": "spike", "type": "spike_detector", "inputs": ["input"]},
       {"id": "output", "type": "comprehensive_csv", "inputs": ["spike"]}
     ]
   }
   ```

2. **Executor builds dependency graph**
   - Determines execution order
   - Identifies which nodes can run in parallel

3. **Nodes process in layers**
   ```
   Layer 1: input (chat_replay)
   Layer 2: spike (spike_detector)  
   Layer 3: output (comprehensive_csv)
   ```

### Example: Spike Detection Flow

Let's trace a message through the spike detector ([`plugins/detectors/spike_detector.cpp`](plugins/detectors/spike_detector.cpp)):

```cpp
NodeResult process(const std::vector<NodeData>& inputs) {
    // 1. Update statistics (lines 47-63)
    double current_rate = inputs.size();
    window.push_back({now, current_rate});
    
    // 2. Calculate Z-score (lines 65-70)
    double z_score = (current_rate - mean) / stddev;
    
    // 3. Check for spike (line 73)
    bool is_spike = std::abs(z_score) > threshold;
    
    // 4. Process each input (lines 76-103)
    for (const auto& input : inputs) {
        NodeData output = input.cloneWithPayload(input.payload);
        
        // Add detection metadata
        output.metadata["spike_detection"] = {
            {"is_spike", is_spike},
            {"z_score", z_score}
        };
        
        // Update lineage (THE IMPORTANT PART!)
        output.markProcessed("spike_detector", confidence, reason);
        
        result.outputs.push_back(output);
    }
}
```

---

## Creating Your Own Plugin

### Step 1: Create the .cpp file
```cpp
// plugins/detectors/my_detector.cpp
#include "../../include/pipeline/node.h"

class MyDetectorNode : public INode {
    bool initialize(const nlohmann::json& config) override {
        // Setup from config
        return true;
    }
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        for (const auto& input : inputs) {
            // Your detection logic here
            bool detected = /* your logic */;
            
            // Clone input and add your metadata
            NodeData output = input.cloneWithPayload(input.payload);
            
            // CRITICAL: Update lineage
            output.markProcessed("my_detector", 
                               confidence_score,
                               "Detection reason");
            
            result.outputs.push_back(output);
        }
        return result;
    }
    
    std::string getType() const override { return "my_detector"; }
    // ... other required methods
};

// REQUIRED: Plugin entry points
extern "C" {
    const char* get_plugin_version() { return "1.0.0"; }
    const char** get_node_types() {
        static const char* types[] = {"my_detector", nullptr};
        return types;
    }
    INode* create_node(const char* type) {
        return new MyDetectorNode();
    }
    void destroy_node(INode* node) { delete node; }
}
```

### Step 2: Add to Makefile
```makefile
# plugins/detectors/Makefile
PLUGINS = spike_detector.so aggregator.so my_detector.so  # Add yours
```

### Step 3: Build and Test
```bash
cd plugins/detectors
make
# Should create my_detector.so

# Test it
cd ../..
./ctic plugins list
# Should show "my_detector" in the list
```

---

## Current Plugin Inventory

### Working Plugins

| Plugin | Type | File | Purpose | Status |
|--------|------|------|---------|--------|
| spike_detector | Detector | [`plugins/detectors/spike_detector.cpp`](plugins/detectors/spike_detector.cpp) | Statistical anomaly detection using Z-scores | ✅ Works |
| aggregator | Detector | [`plugins/detectors/aggregator.cpp`](plugins/detectors/aggregator.cpp) | Combines multiple detection signals | ✅ Works |
| comprehensive_csv | Output | [`plugins/outputs/comprehensive_csv.cpp`](plugins/outputs/comprehensive_csv.cpp) | Writes CSV with full lineage | ✅ Works |
| tokenizer | Preprocessing | [`plugins/preprocessing/tokenizer.cpp`](plugins/preprocessing/tokenizer.cpp) | Text tokenization | ✅ Works |
| chat_replay | Input | [`plugins/inputs/chat_replay.cpp`](plugins/inputs/chat_replay.cpp) | Replay VOD chat data | ⚠️ Built but not loaded |

### Missing/Referenced Plugins

| Plugin | Referenced In | Status |
|--------|--------------|--------|
| stream_input | Templates | ❌ Doesn't exist |
| text_normalizer | Templates | ❌ Doesn't exist |
| twitch_irc | Old code | ❌ Not migrated to plugin |

---

## Known Issues & Quirks

### 1. Input Plugins Not Loaded
**Issue**: [`src/main_pipeline.cpp:148-150`](src/main_pipeline.cpp) doesn't load from `plugins/inputs/`
```cpp
// Current code (missing inputs!)
registry.loadPluginsFromDirectory("plugins/preprocessing");
registry.loadPluginsFromDirectory("plugins/detectors");
registry.loadPluginsFromDirectory("plugins/outputs");
// MISSING: registry.loadPluginsFromDirectory("plugins/inputs");
```

**Impact**: Can't use chat_replay plugin even though it compiles

### 2. Template/Plugin Mismatch
**Issue**: Templates reference non-existent plugins
- [`config/templates/simple_spike.json:20`](config/templates/simple_spike.json): Uses `"type": "stream_input"` (doesn't exist)
- [`config/templates/simple_spike.json:27`](config/templates/simple_spike.json): Uses `"type": "text_normalizer"` (doesn't exist)

**Workaround**: Need to either create these plugins or update templates

### 3. No Error Recovery
**Issue**: If a plugin fails to load, the entire pipeline fails
- No fallback mechanism
- No clear error messages to user

### 4. Manual Memory Management
**Issue**: Plugins use raw `new`/`delete` instead of smart pointers
```cpp
INode* create_node(const char* type) {
    return new MyNode();  // Raw pointer!
}
```

**Risk**: Potential memory leaks if destroy_node() not called

### 5. No Plugin Versioning
**Issue**: `get_plugin_version()` returns a string but it's never checked
- No compatibility checks
- Could load incompatible plugins

---

## Quick Start Debugging

### Check What's Loaded
```bash
./ctic plugins list
# Shows only plugins that loaded successfully
```

### Common Problems

**"Plugin not found"**
- Check if the .so file exists in the plugin directory
- Check if the directory is being loaded in main_pipeline.cpp

**"Failed to create node type: X"**
- Plugin might not be exporting the correct type name
- Check get_node_types() returns the expected string

**Segmentation fault**
- Usually means a plugin returned nullptr
- Check plugin's create_node() logic

**No output**
- Check if nodes are connected properly in the pipeline JSON
- Verify the lineage tracking is being updated

---

## Next Steps for Fixing

1. **Immediate Fix**: Add `registry.loadPluginsFromDirectory("plugins/inputs");` to main_pipeline.cpp
2. **Create Missing Plugins**: Implement text_normalizer and stream_input
3. **Update Templates**: Fix references to use existing plugin types
4. **Add Error Handling**: Better error messages when plugins fail
5. **Consider CMake**: Replace Makefiles for better dependency management

---

*This document reflects CTIC's state as of March 2024. For the latest code, check the git repository.*