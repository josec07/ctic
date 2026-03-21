# Object-Oriented Inheritance in CTIC

**Purpose:** Understanding the OOP patterns used in the plugin system
**Key Concept:** Interface inheritance enables flexible, extensible architecture

---

## The Core Pattern: Interface + Implementation

### 1. The Interface (Abstract Base Class)

**File:** `include/pipeline/node.h` **Lines 112-135**

```cpp
class INode {
public:
    virtual ~INode() = default;
    
    // Pure virtual = MUST implement
    virtual bool initialize(const nlohmann::json& config) = 0;
    virtual void shutdown() = 0;
    virtual NodeResult process(const std::vector<NodeData>& inputs) = 0;
    virtual std::string getType() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
};
```

**Key OOP Concept:** Abstract Base Class
- `= 0` means "pure virtual" - no implementation in base class
- Cannot create `INode` objects directly
- Forces all derived classes to implement these methods
- Think of it as a contract: "If you want to be a node, you MUST have these capabilities"

---

### 2. The Implementation (Concrete Class)

**File:** `plugins/inputs/chat_replay.cpp` (example structure)

```cpp
// ChatReplayNode INHERITS from INode
class ChatReplayNode : public INode {
private:
    std::string vod_id;
    std::ifstream file_handle;
    
public:
    // Implements the interface
    bool initialize(const nlohmann::json& config) override {
        vod_id = config["vod_id"];
        file_handle.open("data/" + vod_id + ".json");
        return file_handle.is_open();
    }
    
    void shutdown() override {
        file_handle.close();
    }
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        // Read VOD chat data
        std::vector<Message> messages = parseChatFile();
        
        // Create result
        NodeResult result;
        result.success = true;
        result.outputs.push_back(NodeData("messages", messages));
        return result;
    }
    
    std::string getType() const override { return "chat_replay"; }
    std::string getName() const override { return "Chat Replay Input"; }
    std::string getDescription() const override { 
        return "Reads Twitch VOD chat data from file"; 
    }
};
```

**Key OOP Concept:** Inheritance + Override
- `class ChatReplayNode : public INode` = "ChatReplayNode IS-A INode"
- `override` keyword ensures we're actually overriding base class methods
- ChatReplayNode provides specific implementations for each pure virtual method
- Think of it as: "I'm a specific type of node that reads VOD files"

---

### 3. The Power: Polymorphism

**File:** `include/pipeline/executor.h` **Line 89**

```cpp
// Executor doesn't know or care what type of node this is
std::unique_ptr<INode> node = registry->createNode("chat_replay");

// Just calls process() - works for ANY node type
NodeResult result = node->process(inputs);
```

**What Actually Happens:**

```cpp
// Registry creates the specific type
INode* node = new ChatReplayNode();  // Actually ChatReplayNode*

// But we treat it as base pointer
INode* base_ptr = node;

// Virtual function call
base_ptr->process(inputs);  // Calls ChatReplayNode::process(), not INode::process()
```

**Key OOP Concept:** Polymorphism ("many forms")
- Base pointer (`INode*`) can point to any derived object
- Virtual function table (vtable) determines which implementation to call
- Executor treats all nodes uniformly
- Think of it as: "I don't care what kind of worker you are, just do your job"

**Why This Matters:**
- Executor code doesn't change when you add new node types
- Same `process()` call works for chat_replay, spike_detector, or sentiment nodes
- New functionality without modifying existing code

---

### 4. The Factory: Creating Objects by Name

**File:** `include/pipeline/registry.h` **Lines 116-133**

```cpp
std::unique_ptr<INode> createNode(const std::string& type) {
    // Check if type exists in plugin map
    auto plugin_it = node_to_plugin.find(type);
    if (plugin_it != node_to_plugin.end()) {
        // Get the plugin that provides this type
        auto& plugin = plugins[plugin_it->second];
        
        // Call plugin's factory function
        return std::unique_ptr<INode>(plugin->create_node(type.c_str()));
    }
    return nullptr;
}
```

**Key OOP Concept:** Factory Pattern
- Don't use `new` directly to create objects
- Use a factory function that decides which class to instantiate
- Allows creating objects by string name at runtime
- Decouples object creation from object usage

**Traditional approach (bad):**
```cpp
if (type == "chat_replay") {
    return new ChatReplayNode();
} else if (type == "spike_detector") {
    return new SpikeDetectorNode();
}
// Must modify this code for every new node type!
```

**Factory approach (good):**
```cpp
// Plugin provides the factory function
// Registry just calls it - doesn't know about specific classes
return plugin->create_node(type);  // Plugin decides what to create
```

---

### 5. The Relationship Diagram

```
                    INode (Abstract Base Class)
                    ├─ virtual process() = 0
                    ├─ virtual initialize() = 0
                    ├─ virtual shutdown() = 0
                    └─ virtual getType() = 0
                           ↑
                           │  "IS-A" relationship
                           │  (Inheritance)
           ┌───────────────┼───────────────┐
           │               │               │
    ChatReplayNode  SpikeDetectorNode  SentimentNode
           │               │               │
    ├─ process()    ├─ process()    ├─ process()
    ├─ initialize() ├─ initialize() ├─ initialize()
    └─ shutdown()   └─ shutdown()   └─ shutdown()
           │               │               │
           └───────────────┴───────────────┘
                           │
                           ↓
              Executor (uses base class pointer)
              ├─ INode* node = createNode("chat_replay");
              ├─ node->process();  // Calls ChatReplayNode::process()
              └─ Works with ANY node type!
```

---

## Why This Architecture Works

### 1. Open/Closed Principle
**Open** for extension (add new node types)  
**Closed** for modification (don't change executor/registry)

### 2. Dependency Inversion
- Executor depends on `INode` (abstraction)
- Not on `ChatReplayNode` (concrete implementation)
- High-level modules don't depend on low-level details

### 3. Separation of Concerns
- **INode:** Defines the interface (what nodes must do)
- **ChatReplayNode:** Implements specific functionality (how to read VOD)
- **Registry:** Manages discovery and creation (how to find nodes)
- **Executor:** Orchestrates execution (when to run nodes)

### 4. Type Safety
```cpp
// Without inheritance (unsafe):
void* node = createNode("chat_replay");  // void* = no type info
((ChatReplayNode*)node)->process();       // Cast required, can crash

// With inheritance (safe):
std::unique_ptr<INode> node = createNode("chat_replay");
node->process();  // Compiler knows this is valid, vtable handles dispatch
```

---

## Real-World Example: Adding a New Node

**Scenario:** You want to add "sentiment_analysis" node

**Step 1:** Create class (no changes to existing code)
```cpp
// plugins/models/sentiment_analysis.cpp
class SentimentNode : public INode {
    bool initialize(const nlohmann::json& config) override { /* ... */ }
    NodeResult process(const std::vector<NodeData>& inputs) override { /* ... */ }
    // ... other overrides
};

extern "C" {
    INode* create_node(const char* type) {
        if (strcmp(type, "sentiment_analysis") == 0) {
            return new SentimentNode();
        }
        return nullptr;
    }
}
```

**Step 2:** Compile to .so file, drop in plugins/models/

**Step 3:** Reference in pipeline config
```json
{
  "nodes": [
    {"id": "input", "type": "chat_replay"},
    {"id": "sentiment", "type": "sentiment_analysis"}  // NEW!
  ]
}
```

**Result:**
- Executor works with it immediately (polymorphism)
- No changes to executor.h or registry.h
- Type-safe, extensible, maintainable

---

## Summary: The OOP Hierarchy

| Concept | Location | Purpose |
|---------|----------|---------|
| **Abstract Base Class** | `node.h` lines 112-135 | Defines interface contract |
| **Inheritance** | `chat_replay.cpp` | Implements interface |
| **Polymorphism** | `executor.h` line 89 | Treats all nodes uniformly |
| **Factory Pattern** | `registry.h` lines 116-133 | Creates objects by name |
| **Virtual Functions** | Throughout | Runtime dispatch to correct implementation |

**The Magic:** Write once (executor), run everywhere (any node type).

---

*Key Takeaway:* CTIC uses classic OOP inheritance to achieve plugin architecture - new functionality without modifying core code.
