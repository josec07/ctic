#ifndef CTIC_PIPELINE_NODE_H
#define CTIC_PIPELINE_NODE_H

#include <string>
#include <vector>
#include <memory>
#include <any>
#include <chrono>
#include <map>
#include "../nlohmann/json.hpp"

namespace ctic {
namespace pipeline {

// Comprehensive lineage tracking
struct Lineage {
    std::vector<std::string> path;                    // Full node path (input->process->output)
    std::map<std::string, float> node_scores;         // Node ID -> confidence/score
    std::map<std::string, std::string> node_reasons;  // Node ID -> detection reason
    std::vector<std::string> triggers;                // What triggered this detection
    std::string detection_type;                       // Combined type (e.g., "spike+sentiment")
    float combined_confidence = 0.0f;                 // Overall confidence
    std::chrono::milliseconds total_processing_time{0}; // Total processing time
    
    // Add node contribution to lineage
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
    
    // Generate human-readable path string
    std::string getPathString() const {
        std::string result;
        for (size_t i = 0; i < path.size(); i++) {
            if (i > 0) result += "→";
            result += path[i];
        }
        return result;
    }
    
    // Convert to JSON for CSV output
    nlohmann::json toJson() const {
        return {
            {"path", getPathString()},
            {"scores", node_scores},
            {"reasons", node_reasons},
            {"triggers", triggers},
            {"type", detection_type},
            {"confidence", combined_confidence},
            {"processing_ms", total_processing_time.count()}
        };
    }
};

// Node data passed between nodes with full lineage
struct NodeData {
    std::string type;           // Data type identifier
    std::any payload;           // Actual data
    nlohmann::json metadata;    // Additional metadata
    Lineage lineage;           // Complete lineage tracking
    
    // Timestamps
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point processed_at;
    
    NodeData() : created_at(std::chrono::system_clock::now()) {}
    
    NodeData(const std::string& t, std::any p, nlohmann::json m = {})
        : type(t), payload(std::move(p)), metadata(std::move(m)),
          created_at(std::chrono::system_clock::now()) {}
    
    // Mark as processed by a node
    void markProcessed(const std::string& node_id, 
                       float score = 0.0f, 
                       const std::string& reason = "") {
        processed_at = std::chrono::system_clock::now();
        lineage.addNode(node_id, score, reason);
    }
    
    // Clone with new payload but preserve lineage
    NodeData cloneWithPayload(std::any new_payload) const {
        NodeData clone(type, std::move(new_payload), metadata);
        clone.lineage = lineage;
        clone.created_at = created_at;
        return clone;
    }
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