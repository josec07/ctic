# CTIC File Relationships - Simple ASCII

**Three Core Files:** node.h | registry.h | executor.h

```
┌─────────────────────────────────────────────────────────────────┐
│  node.h (Defines what a node IS)                               │
│  ├─ Line 112: class INode { ... }                              │
│  ├─ Line 123: virtual NodeResult process(...) = 0               │
│  └─ Line 149: INode* create_node(const char*)                   │
└─────────────────────────────────────────────────────────────────┘
                              ↑
                              │  Includes
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│  registry.h (Finds and creates nodes)                          │
│  ├─ Line 75:   Gets create_node function from .so file         │
│  ├─ Line 128: Calls plugin->create_node("chat_replay")         │
│  └─ Line 89:  Maps node type name → plugin file path           │
└─────────────────────────────────────────────────────────────────┘
                              ↑
                              │  Uses registry->createNode()
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│  executor.h (Runs the nodes)                                   │
│  ├─ Line 89:  instance->node = registry->createNode(type)      │
│  ├─ Line 241: instance->node->process(inputs)                   │
│  └─ Line 89:  Creates NodeInstance with unique_ptr<INode>       │
└─────────────────────────────────────────────────────────────────┘
```

## The Data Flow

```
1. START: PipelineExecutor created with registry pointer
   executor.h line 66: PipelineExecutor(NodeRegistry* reg)
          ↓
2. LOAD: executor calls registry to create nodes
   executor.h line 89: registry->createNode("chat_replay")
          ↓
3. FIND: registry looks up which plugin provides this type
   registry.h line 124: node_to_plugin.find("chat_replay")
          ↓
4. CALL: registry calls the plugin's create function
   registry.h line 128: plugin->create_node("chat_replay")
          ↓
5. CREATE: Plugin creates actual ChatReplayNode object
   chat_replay.cpp: return new ChatReplayNode();
          ↓
6. RETURN: Node comes back wrapped in unique_ptr<INode>
   registry.h line 128: return unique_ptr<INode>(node)
          ↓
7. STORE: Executor stores it in NodeInstance
   executor.h line 89: instance->node = [the node]
          ↓
8. RUN: Executor calls process() through base pointer
   executor.h line 241: instance->node->process(inputs)
          ↓
   [Virtual dispatch calls ChatReplayNode::process()]
```

## Key Line Numbers

### node.h
- **112:** Interface class starts
- **123:** process() function every node must implement
- **149:** Plugin export function signature

### registry.h
- **75:** Gets function pointer from .so file (dlsym)
- **89:** Maps type name → plugin path
- **128:** Actually creates the node by calling plugin

### executor.h
- **24:** NodeInstance struct holds the node
- **89:** Where nodes get created from registry
- **241:** Where nodes get executed

## What Each File Does

| File | Role | Key Job |
|------|------|---------|
| node.h | Interface | "This is what a node looks like" |
| registry.h | Factory | "I can create nodes by name" |
| executor.h | Operator | "I run nodes in order" |

## Simple Mental Model

```
node.h      = Job description (what skills needed)
registry.h  = HR department (finds and hires workers)
executor.h  = Manager (tells workers what to do)
chat_replay = Worker (does the actual job)
```

**The Flow:** Manager needs worker → Asks HR to hire → HR finds worker → Worker does job

**In Code:** Executor needs node → Asks registry → Registry calls plugin → Node processes data
