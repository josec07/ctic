# CTIC Pipeline Engine - Quick Start

## Overview
Node-based pipeline engine for CTIC v3.0 with dynamic plugin support (.so/.dll files).

## Core Components

### 1. Node Interface (`include/pipeline/node.h`)
- Base interface all nodes must implement
- Handles initialization, processing, and shutdown
- Supports parallel execution flag

### 2. Node Registry (`include/pipeline/registry.h`)
- Manages node types (built-in and plugins)
- Dynamic loading of .so/.dll files
- Plugin discovery from directories

### 3. Pipeline Executor (`include/pipeline/executor.h`)
- Loads pipeline from JSON config
- Manages node execution order (topological sort)
- Supports parallel and sequential execution
- Thread pool for concurrent nodes

## Building & Testing

```bash
# Build pipeline test and example plugin
make -f Makefile.pipeline all

# Run test
./pipeline_test
```

## Creating a Plugin

1. Implement INode interface
2. Export C functions: `get_plugin_version`, `get_node_types`, `create_node`, `destroy_node`
3. Compile as shared library (.so)

Example: see `plugins/example/text_normalizer.cpp`

## Pipeline Configuration

```json
{
  "pipeline": {
    "nodes": [
      {
        "id": "unique_id",
        "type": "node_type",
        "inputs": ["input_node_id"],
        "parallel": true,
        "config": {}
      }
    ]
  }
}
```

## Key Features
- **Dynamic Loading**: Plugins loaded at runtime
- **Parallel Execution**: Nodes can run concurrently
- **JSON Configuration**: Easy to share and modify
- **Type Safety**: Strong typing with std::any
- **Extensible**: Add new node types without recompiling

## Node Data Flow
```
Input Node -> Processing Nodes (parallel) -> Aggregator -> Output Node
```

## Next Steps
- Add ONNX runtime support
- Create more node types
- Implement profile management
- Add real-time monitoring