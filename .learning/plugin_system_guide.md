# CTIC Plugin System Deep Dive - CORRECTED

**Purpose:** Understanding the plugin architecture with real code examples  
**Key Files:** 
- `include/pipeline/node.h` (158 lines) - Interface and data structures
- `include/pipeline/registry.h` (156 lines) - Plugin loading and discovery  
**Status:** Use this version - references actual code with line numbers

---

## The Big Picture: Why Plugins?

**Problem:** You want to process Twitch chat data, but the processing steps change:
- Sometimes you want spike detection
- Sometimes you want sentiment analysis  
- Sometimes you want both
- You might want to add new detection types later

**Solution:** Plugin architecture
- Each processing step = plugin (node)
- Mix and match nodes in a pipeline
- Add new nodes without changing core code

**Analogy:** LEGO blocks
- Core engine = the base plate
- Plugins = LEGO bricks that snap in
- Pipeline config = instructions showing which bricks connect where

---

## Part 1: The Interface Contract (INode)

**File:** `include/pipeline/node.h` **Lines 112-135**

**ACTUAL CODE:**
```cpp
// Lines 112-135 in node.h
class INode {
public:
    virtual ~INode() = default;  // Line 114
    
    // Initialize node with configuration - Line 117
    virtual bool initialize(const nlohmann::json& config) = 0;
    
    // Cleanup resources - Line 120
    virtual void shutdown() = 0;
    
    // Process input data and produce output - Line 123
    virtual NodeResult process(const std::vector<NodeData>& inputs) = 0;
    
    // Check if this node can process in parallel - Line 126
    virtual bool canRunParallel() const { return false; }
    
    // Get node metadata - Lines 129-131
    virtual std::string getType() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    
    // Get configuration schema for validation - Line 134
    virtual nlohmann::json getConfigSchema() const { return {}; }
};
```

**Key Concepts:**

1. **Pure Virtual Functions** (`= 0`) - **Lines 117, 120, 123, 129, 130, 131**
   - `initialize()`, `shutdown()`, `process()`, `getType()`, `getName()`, `getDescription()` MUST be implemented
   - This is the contract: "If you want to be a node, you must implement these"
   - Makes `INode` an abstract class (can't create instances directly)
   - Note: The current code has MORE pure virtual functions than my original doc suggested

2. **Virtual Destructor** - **Line 114**
   - `virtual ~INode() = default;`
   - When you delete a node through base pointer, destructor of derived class runs
   - Prevents memory leaks in plugins

3. **Default Implementations** - **Lines 126, 134**
   - `canRunParallel()` returns `false` by default
   - `getConfigSchema()` returns empty JSON by default
   - Plugins CAN override these, but don't have to

**Object-Oriented Pattern:** Interface/Abstract Base Class
- Defines WHAT nodes must do (process data)
- Doesn't care HOW they do it

---

## Part 2: The Node Data Structure

**File:** `include/pipeline/node.h` **Lines 63-94** (NodeData) and **Lines 16-60** (Lineage)

**ACTUAL CODE - NodeData struct:**
```cpp
// Lines 63-94 in node.h
struct NodeData {
    std::string type;           // Line 64 - Data type identifier
    std::any payload;           // Line 65 - Actual data (std::any, not std::variant!)
    nlohmann::json metadata;    // Line 66 - Additional metadata
    Lineage lineage;           // Line 67 - Complete lineage tracking
    
    // Timestamps - Lines 70-71
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point processed_at;
    
    // Default constructor - Line 73
    NodeData() : created_at(std::chrono::system_clock::now()) {}
    
    // Constructor with parameters - Lines 75-77
    NodeData(const std::string& t, std::any p, nlohmann::json m = {})
        : type(t), payload(std::move(p)), metadata(std::move(m)),
          created_at(std::chrono::system_clock::now()) {}
    
    // Mark as processed by a node - Lines 80-85
    void markProcessed(const std::string& node_id, 
                       float score = 0.0f, 
                       const std::string& reason = "");
    
    // Clone with new payload but preserve lineage - Lines 88-94
    NodeData cloneWithPayload(std::any new_payload) const;
};
```

**ACTUAL CODE - Lineage struct:**
```cpp
// Lines 16-60 in node.h
struct Lineage {
    std::vector<std::string> path;                    // Line 17 - Full node path
    std::map<std::string, float> node_scores;         // Line 18 - Node ID -> score
    std::map<std::string, std::string> node_reasons;  // Line 19 - Node ID -> reason
    std::vector<std::string> triggers;                // Line 20 - What triggered detection
    std::string detection_type;                       // Line 21 - Combined type
    float combined_confidence = 0.0f;                 // Line 22 - Overall confidence
    std::chrono::milliseconds total_processing_time{0}; // Line 23 - Total time
    
    // Add node contribution to lineage - Lines 26-36
    void addNode(const std::string& node_id, 
                 float score = 0.0f, 
                 const std::string& reason = "") {
        path.push_back(node_id);
        if (score > 0.0f) {
            node_scores[node_id] = score;
        }
        if (!reason.empty()) {
            node_reasons[node_id] = reason;
        }
    }
    
    // Generate human-readable path string - Lines 39-46
    std::string getPathString() const;
    
    // Convert to JSON for CSV output - Lines 49-59
    nlohmann::json toJson() const;
};
```

**CORRECTION from original doc:**
- ❌ I said NodeData uses `std::variant` - **WRONG**
- ✅ It uses `std::any` (**Line 65**) - type-erased container that can hold any type
- ❌ I said there's a separate node_data.h file - **WRONG**  
- ✅ Everything is in `node.h`

**What This Represents:**
- A packet of data flowing through the pipeline
- Like an envelope with:
  - **type:** What kind of data (e.g., "chat_messages", "detection_events")
  - **payload:** The actual content (stored in `std::any`)
  - **metadata:** Context (timestamp, source channel, config used)
  - **lineage:** Audit trail showing which nodes touched this data (**CTIC's unique feature**)

**Why std::any instead of std::variant?**
- `std::any` can hold ANY type without knowing ahead of time
- `std::variant` requires listing all possible types at compile time
- Trade-off: `std::any` has slight runtime overhead but maximum flexibility
- To extract: `std::any_cast<std::vector<Message>>(nodeData.payload)`

**NodeResult - Lines 97-109:**
```cpp
struct NodeResult {
    bool success;                        // Line 98
    std::vector<NodeData> outputs;       // Line 99 - Note: multiple outputs possible
    std::string error_message;           // Line 100
    
    NodeResult() : success(true) {}      // Line 102
    static NodeResult error(const std::string& msg);  // Line 103
};
```

**Key Insight:** A node can produce MULTIPLE outputs (line 99), not just one!

---

## Part 3: The Registry - Plugin Discovery

**File:** `include/pipeline/registry.h` **Lines 36-151**

**ACTUAL CODE - PluginHandle class:**
```cpp
// Lines 15-33 in registry.h
class PluginHandle {
public:
    void* handle;                                        // Line 17 - OS handle to .so file
    std::string path;                                    // Line 18 - File path
    std::vector<std::string> node_types;                 // Line 19 - What nodes this plugin provides
    
    // Function pointers from plugin - Lines 22-25
    std::function<const char*()> get_version;
    std::function<const char**()> get_types;
    std::function<INode*(const char*)> create_node;
    std::function<void(INode*)> destroy_node;
    
    PluginHandle() : handle(nullptr) {}                  // Line 27
    ~PluginHandle() {                                   // Line 28-31
        if (handle) {
            dlclose(handle);  // Close the .so file
        }
    }
};
```

**ACTUAL CODE - NodeRegistry class:**
```cpp
// Lines 36-151 in registry.h
class NodeRegistry {
private:
    // Built-in node factories - Line 39
    std::map<std::string, NodeFactory> builtin_factories;
    
    // Plugin handles - Line 42
    std::map<std::string, std::unique_ptr<PluginHandle>> plugins;
    
    // Node type to plugin mapping - Line 45
    std::map<std::string, std::string> node_to_plugin;
    
public:
    // Lines 49-53 - Default constructor comment by you!
    /*
    Jose - This is where I am defining a default node registry,
     all private/public class memebers get default initialized. 
    */
    NodeRegistry() = default;
    
    // Register built-in node type - Lines 56-58
    void registerBuiltin(const std::string& type, NodeFactory factory);
    
    // Load plugin from .so/.dll file - Lines 61-94
    bool loadPlugin(const std::string& path) {
        auto plugin = std::make_unique<PluginHandle>();
        plugin->path = path;
        
        // Load the shared library - Line 66
        plugin->handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!plugin->handle) {
            std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
            return false;
        }
        
        // Get function pointers from .so file - Lines 73-76
        plugin->get_version = (const char*(*)())dlsym(plugin->handle, "get_plugin_version");
        plugin->get_types = (const char**(*)())dlsym(plugin->handle, "get_node_types");
        plugin->create_node = (INode*(*)(const char*))dlsym(plugin->handle, "create_node");
        plugin->destroy_node = (void(*)(INode*))dlsym(plugin->handle, "destroy_node");
        
        // Verify required functions exist - Lines 78-81
        if (!plugin->create_node || !plugin->destroy_node) {
            std::cerr << "Plugin missing required functions" << std::endl;
            return false;
        }
        
        // Get node types from plugin - Lines 84-90
        if (plugin->get_types) {
            const char** types = plugin->get_types();
            for (int i = 0; types[i] != nullptr; i++) {
                plugin->node_types.push_back(types[i]);
                node_to_plugin[types[i]] = path;  // Map type name to plugin path
            }
        }
        
        plugins[path] = std::move(plugin);  // Line 92
        return true;
    }
    
    // Load all plugins from directory - Lines 97-113
    void loadPluginsFromDirectory(const std::string& dir) {
        namespace fs = std::filesystem;
        
        if (!fs::exists(dir)) {
            return;
        }
        
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                auto path = entry.path();
                if (path.extension() == ".so" || path.extension() == ".dll") {
                    std::cout << "Loading plugin: " << path << std::endl;
                    loadPlugin(path.string());
                }
            }
        }
    }
    
    // Create node instance - Lines 116-133
    std::unique_ptr<INode> createNode(const std::string& type) {
        // Check built-in factories first - Lines 118-121
        auto builtin_it = builtin_factories.find(type);
        if (builtin_it != builtin_factories.end()) {
            return builtin_it->second();
        }
        
        // Check plugins - Lines 124-130
        auto plugin_it = node_to_plugin.find(type);
        if (plugin_it != node_to_plugin.end()) {
            auto& plugin = plugins[plugin_it->second];
            if (plugin && plugin->create_node) {
                return std::unique_ptr<INode>(plugin->create_node(type.c_str()));
            }
        }
        
        return nullptr;  // Line 132
    }
    
    // List all available node types - Lines 136-150
    std::vector<std::string> getAvailableTypes() const;
};
```

**Line-by-Line Breakdown:**

#### Private Members (Lines 38-45)
```cpp
std::map<std::string, NodeFactory> builtin_factories;           // Line 39
std::map<std::string, std::unique_ptr<PluginHandle>> plugins;   // Line 42
std::map<std::string, std::string> node_to_plugin;             // Line 45
```

**What these store:**
- `builtin_factories`: Hardcoded nodes compiled into main executable
- `plugins`: Loaded .so files with their function pointers
- `node_to_plugin`: Maps node type name (e.g., "chat_replay") → plugin file path

#### loadPlugin function (Lines 61-94)

**Critical Linux System Calls:**
- **Line 66:** `dlopen(path.c_str(), RTLD_LAZY)` - Opens .so file, returns handle
- **Lines 73-76:** `dlsym()` - Gets function pointers by name from the .so file
  - Looks for: `get_plugin_version`, `get_node_types`, `create_node`, `destroy_node`
- **Line 30 (in destructor):** `dlclose(handle)` - Closes .so file when registry destroyed

**The Pattern:**
1. Open .so file with dlopen
2. Get function pointers with dlsym  
3. Call get_types() to learn what nodes this plugin provides
4. Store in maps for later lookup
5. When creating node, lookup which plugin provides it, call plugin's create_node()

#### loadPluginsFromDirectory (Lines 97-113)

**What it does:**
1. Check if directory exists (**Line 100**)
2. Iterate all files (**Line 104**)
3. If file ends with .so (Linux) or .dll (Windows), load it (**Line 107**)

**THE BUG IN MAIN_PIPELINE.CPP:**
This function is called for these directories:
- `plugins/preprocessing` ✓
- `plugins/detectors` ✓
- `plugins/outputs` ✓
- `plugins/models` ✓
- `plugins/inputs` ✗ **MISSING!**

chat_replay.so is in `plugins/inputs/`, so it's never loaded!

#### createNode (Lines 116-133)

**Logic flow:**
1. Check if type exists in `builtin_factories` (**Lines 118-121**)
2. If not built-in, check `node_to_plugin` map (**Line 124**)
3. If found in plugin map, get the PluginHandle (**Line 126**)
4. Call plugin's `create_node()` function (**Line 128**)
5. Wrap in `unique_ptr<INode>` for automatic cleanup
6. Return nullptr if type not found (**Line 132**)

**Key Insight:** The registry doesn't store factory functions for plugins like I originally thought. It stores:
- `node_to_plugin` map: type name → plugin path
- `plugins` map: plugin path → PluginHandle (with function pointers)
- When creating node, looks up which plugin provides it, then calls that plugin's create function

---

## Part 4: The Plugin Interface (What .so Files Must Export)

**File:** `include/pipeline/node.h` **Lines 141-153**

**ACTUAL CODE:**
```cpp
// Lines 141-153 in node.h
extern "C" {  // Line 141 - Disable C++ name mangling
    // Return plugin version - Line 143
    const char* get_plugin_version();
    
    // Return list of node types this plugin provides - Line 146
    const char** get_node_types();
    
    // Create a node instance - Line 149
    INode* create_node(const char* type);
    
    // Destroy a node instance - Line 152
    void destroy_node(INode* node);
}
```

**Why `extern "C"`?**
- C++ "mangles" function names (adds type info) to support overloading
- C doesn't mangle names
- `dlsym()` expects C-style names
- `extern "C"` tells compiler: "Don't mangle these function names"

**Function Purposes:**
1. **get_plugin_version()**: Returns version string (for debugging/logging)
2. **get_node_types()**: Returns array of node type names this plugin provides
3. **create_node(type)**: Creates instance of specific node type
4. **destroy_node(node)**: Cleans up (since plugin allocated it, plugin must free it)

---

## Part 5: Real Example - chat_replay Plugin

**File:** `plugins/inputs/chat_replay.cpp`

**What it must implement:**
```cpp
#include "../../include/pipeline/node.h"

class ChatReplayNode : public ctic::pipeline::INode {
public:
    // Implement pure virtual functions from INode:
    bool initialize(const nlohmann::json& config) override { /* ... */ }
    void shutdown() override { /* ... */ }
    ctic::pipeline::NodeResult process(const std::vector<ctic::pipeline::NodeData>& inputs) override { /* ... */ }
    std::string getType() const override { return "chat_replay"; }
    std::string getName() const override { return "Chat Replay Input"; }
    std::string getDescription() const override { return "Reads Twitch VOD chat data"; }
};

// Export functions for dynamic loading
extern "C" {
    const char* get_plugin_version() { return "1.0.0"; }
    
    const char** get_node_types() {
        static const char* types[] = {"chat_replay", nullptr};
        return types;
    }
    
    ctic::pipeline::INode* create_node(const char* type) {
        if (strcmp(type, "chat_replay") == 0) {
            return new ChatReplayNode();
        }
        return nullptr;
    }
    
    void destroy_node(ctic::pipeline::INode* node) {
        delete node;
    }
}
```

**Key Points:**
1. Inherits from `INode` (base class from node.h)
2. Implements ALL pure virtual functions
3. Exports C functions with `extern "C"`
4. `get_node_types()` returns nullptr-terminated array
5. `create_node()` checks type name, returns appropriate class

---

## Part 6: Object-Oriented Patterns Used (Real Examples)

### 1. Interface/Abstract Base Class
**Where:** `node.h` Lines 112-135
```cpp
class INode {
    virtual NodeResult process(const std::vector<NodeData>& inputs) = 0;
    // ... other pure virtuals
};
```
- Pure virtual functions (= 0) make this abstract
- Cannot create `INode` instances directly
- Plugins inherit and implement

### 2. Factory Pattern
**Where:** `registry.h` Lines 116-133 (createNode function)
```cpp
std::unique_ptr<INode> createNode(const std::string& type) {
    // Looks up type name, returns appropriate instance
    // Doesn't know concrete class at compile time
}
```
- Creates objects based on runtime string
- Registry doesn't know about ChatReplayNode class directly

### 3. RAII (Resource Acquisition Is Initialization)
**Where:** `registry.h` Lines 27-32 (PluginHandle destructor)
```cpp
~PluginHandle() {
    if (handle) {
        dlclose(handle);  // Auto-cleanup when PluginHandle destroyed
    }
}
```
- Automatically closes .so file when registry destroyed
- No manual cleanup needed

**Also:** `unique_ptr<INode>` in createNode - auto-deletes node when done

### 4. Polymorphism
**Where:** Throughout
```cpp
std::unique_ptr<INode> node = registry.createNode("chat_replay");
// node is actually a ChatReplayNode*, but we treat it as INode*
node->process(inputs);  // Calls ChatReplayNode::process via vtable
```
- Virtual function table lookup at runtime
- Core engine treats all nodes uniformly through base pointer

### 5. Type Erasure (std::any)
**Where:** `node.h` Line 65
```cpp
std::any payload;  // Can hold any type without knowing at compile time
```
- More flexible than std::variant
- Runtime cost: small, but enables plugin architecture

---

## Part 7: The Critical Bug (With Line Numbers)

**File:** `src/main_pipeline.cpp` **Lines 157-160**

**ACTUAL CODE:**
```cpp
// Lines 157-160 in main_pipeline.cpp
registry.loadPluginsFromDirectory("plugins/preprocessing");
registry.loadPluginsFromDirectory("plugins/detectors");
registry.loadPluginsFromDirectory("plugins/outputs");
registry.loadPluginsFromDirectory("plugins/models");
```

**MISSING (Line to add):**
```cpp
registry.loadPluginsFromDirectory("plugins/inputs");  // ADD AFTER LINE 160
```

**Why this matters:**
- **Line 157:** Loads preprocessing plugins
- **Line 158:** Loads detector plugins  
- **Line 159:** Loads output plugins
- **Line 160:** Loads model plugins
- **MISSING:** Never loads input plugins!

**Result:**
- `chat_replay.so` sits in `plugins/inputs/` but registry never loads it
- When pipeline config asks for "chat_replay" node:
  - Registry checks builtin_factories - not found
  - Registry checks node_to_plugin - not found (never loaded)
  - Returns nullptr
- Error: "Failed to create node: chat_replay"

**Fix location:** Add after line 160 in `src/main_pipeline.cpp`

---

## Part 8: Summary - Key Files and Line Numbers

### node.h (158 lines total)
- **Lines 16-60:** `Lineage` struct - audit trail
- **Lines 63-94:** `NodeData` struct - data packet
- **Lines 97-109:** `NodeResult` struct - return from process()
- **Lines 112-135:** `INode` class - interface contract
- **Lines 138:** `NodeFactory` type alias
- **Lines 141-153:** Plugin export functions (extern "C")

### registry.h (156 lines total)
- **Lines 15-33:** `PluginHandle` class - manages .so file
- **Lines 36-151:** `NodeRegistry` class - plugin discovery
- **Lines 39:** `builtin_factories` map
- **Lines 42:** `plugins` map
- **Lines 45:** `node_to_plugin` map
- **Lines 61-94:** `loadPlugin()` - loads single .so file
- **Lines 97-113:** `loadPluginsFromDirectory()` - scans directory
- **Lines 116-133:** `createNode()` - creates node by type name
- **Lines 136-150:** `getAvailableTypes()` - lists all types

### main_pipeline.cpp (bug location)
- **Lines 157-160:** Where plugins are loaded
- **NEED TO ADD:** Line 161 - `plugins/inputs` directory

---

## Reading Checklist

Read these in order (with actual files open):

- [ ] **node.h Line 112-135** - Understand INode interface
- [ ] **node.h Line 63-94** - Understand NodeData structure  
- [ ] **node.h Line 16-60** - Understand Lineage (your unique feature)
- [ ] **registry.h Line 15-33** - Understand PluginHandle
- [ ] **registry.h Line 36-151** - Understand NodeRegistry
- [ ] **registry.h Line 61-94** - Understand loadPlugin (dlopen/dlsym)
- [ ] **registry.h Line 116-133** - Understand createNode
- [ ] **main_pipeline.cpp Line 157-160** - See where bug is
- [ ] Fix the bug (add `plugins/inputs`)
- [ ] Test: `./ctic_pipeline --list` should show "chat_replay"

---

## Key Takeaways

1. **INode (lines 112-135)** defines the contract: pure virtual functions every node must implement

2. **NodeData (lines 63-94)** is the envelope: type, payload (std::any), metadata, lineage

3. **Lineage (lines 16-60)** is your moat: tracks which nodes processed data and why

4. **Registry (lines 36-151)** manages plugins: loads .so files, stores function pointers, creates nodes

5. **Plugin interface (lines 141-153)** is what .so files export: 4 C functions

6. **The bug:** Missing `plugins/inputs` in main_pipeline.cpp line ~160

7. **Architecture flow:** Registry loads .so → gets function pointers → maps type names → creates nodes on demand

---

*Version 2.0 - CORRECTED with real code and line numbers*  
*Updated: March 19, 2026*  
*Files referenced: node.h (158 lines), registry.h (156 lines)*
