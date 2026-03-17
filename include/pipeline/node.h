#ifndef CTIC_PIPELINE_NODE_H
#define CTIC_PIPELINE_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <any>
#include "../nlohmann/json.hpp"

namespace ctic {
namespace pipeline {

// Node data passed between nodes
struct NodeData {
    std::string type;           // Data type identifier
    std::any payload;           // Actual data
    nlohmann::json metadata;    // Additional metadata
    
    NodeData() = default;
    NodeData(const std::string& t, std::any p, nlohmann::json m = {})
        : type(t), payload(std::move(p)), metadata(std::move(m)) {}
};

// Result from node processing
struct NodeResult {
    bool success;
    std::vector<NodeData> outputs;
    std::string error_message;
    
    NodeResult() : success(true) {}
    static NodeResult error(const std::string& msg) {
        NodeResult r;
        r.success = false;
        r.error_message = msg;
        return r;
    }
};

// Base interface for all pipeline nodes
class INode {
public:
    virtual ~INode() = default;
    
    // Initialize node with configuration
    virtual bool initialize(const nlohmann::json& config) = 0;
    
    // Cleanup resources
    virtual void shutdown() = 0;
    
    // Process input data and produce output
    virtual NodeResult process(const std::vector<NodeData>& inputs) = 0;
    
    // Check if this node can process in parallel with other instances
    virtual bool canRunParallel() const { return false; }
    
    // Get node metadata
    virtual std::string getType() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    
    // Get configuration schema for validation
    virtual nlohmann::json getConfigSchema() const { return {}; }
};

// Factory function type for creating nodes
using NodeFactory = std::unique_ptr<INode>(*)();

// Plugin entry point - each .so/.dll must implement this
extern "C" {
    // Return plugin version
    const char* get_plugin_version();
    
    // Return list of node types this plugin provides
    const char** get_node_types();
    
    // Create a node instance
    INode* create_node(const char* type);
    
    // Destroy a node instance
    void destroy_node(INode* node);
}

} // namespace pipeline
} // namespace ctic

#endif // CTIC_PIPELINE_NODE_H