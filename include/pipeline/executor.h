#ifndef CTIC_PIPELINE_EXECUTOR_H
#define CTIC_PIPELINE_EXECUTOR_H

#include "node.h"
#include "registry.h"
#include <thread>
#include <future>
#include <queue>
#include <set>
#include <atomic>

namespace ctic {
namespace pipeline {

// Connection between nodes
struct Connection {
    std::string from_node;
    std::string to_node;
    int from_output = 0;  // Output port index
    int to_input = 0;     // Input port index
};

// Node instance with configuration
struct NodeInstance {
    std::string id;
    std::string type;
    std::unique_ptr<INode> node;
    std::vector<std::string> inputs;  // IDs of input nodes
    std::vector<std::string> outputs; // IDs of output nodes
    bool parallel = false;  // Can run in parallel with siblings
    nlohmann::json config;
};

// Execution layer - nodes that can run at the same time
struct ExecutionLayer {
    std::vector<std::string> node_ids;
    bool can_parallelize = false;
};

// Pipeline executor manages node execution
class PipelineExecutor {
private:
    // Node instances
    std::map<std::string, std::unique_ptr<NodeInstance>> nodes;
    
    // Connections between nodes
    std::vector<Connection> connections;
    
    // Node registry
    NodeRegistry* registry;
    
    // Execution layers (topological order)
    std::vector<ExecutionLayer> execution_layers;
    
    // Node outputs (results from processing)
    std::map<std::string, NodeResult> node_results;
    
    // Thread pool for parallel execution
    std::vector<std::thread> thread_pool;
    size_t max_threads;
    
    // Pipeline state
    std::atomic<bool> running{false};
    
public:
    PipelineExecutor(NodeRegistry* reg, size_t threads = 4) 
        : registry(reg), max_threads(threads) {}
    
    ~PipelineExecutor() {
        stop();
    }
    
    // Load pipeline configuration from JSON
    bool loadPipeline(const nlohmann::json& config) {
        try {
            // Clear existing pipeline
            nodes.clear();
            connections.clear();
            execution_layers.clear();
            
            // Load nodes
            if (config.contains("pipeline") && config["pipeline"].contains("nodes")) {
                for (const auto& node_config : config["pipeline"]["nodes"]) {
                    auto instance = std::make_unique<NodeInstance>();
                    instance->id = node_config["id"];
                    instance->type = node_config["type"];
                    
                    // Create node from registry
                    instance->node = registry->createNode(instance->type);
                    if (!instance->node) {
                        std::cerr << "Failed to create node type: " << instance->type << std::endl;
                        return false;
                    }
                    
                    // Initialize node with config
                    if (node_config.contains("config")) {
                        instance->config = node_config["config"];
                        if (!instance->node->initialize(instance->config)) {
                            std::cerr << "Failed to initialize node: " << instance->id << std::endl;
                            return false;
                        }
                    }
                    
                    // Parse inputs
                    if (node_config.contains("inputs")) {
                        if (node_config["inputs"].is_array()) {
                            for (const auto& input : node_config["inputs"]) {
                                instance->inputs.push_back(input);
                            }
                        } else if (node_config["inputs"].is_string()) {
                            instance->inputs.push_back(node_config["inputs"]);
                        }
                    }
                    
                    // Check if can run in parallel
                    if (node_config.contains("parallel")) {
                        instance->parallel = node_config["parallel"];
                    }
                    
                    nodes[instance->id] = std::move(instance);
                }
            }
            
            // Load connections (alternative to inputs specification)
            if (config["pipeline"].contains("connections")) {
                for (const auto& conn : config["pipeline"]["connections"]) {
                    Connection c;
                    c.from_node = conn["from"];
                    c.to_node = conn["to"];
                    if (conn.contains("from_output")) c.from_output = conn["from_output"];
                    if (conn.contains("to_input")) c.to_input = conn["to_input"];
                    connections.push_back(c);
                    
                    // Update node inputs/outputs
                    if (nodes.count(c.from_node)) {
                        nodes[c.from_node]->outputs.push_back(c.to_node);
                    }
                    if (nodes.count(c.to_node)) {
                        nodes[c.to_node]->inputs.push_back(c.from_node);
                    }
                }
            }
            
            // Build execution layers
            buildExecutionLayers();
            
            std::cout << "Pipeline loaded with " << nodes.size() << " nodes and " 
                     << execution_layers.size() << " execution layers" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error loading pipeline: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Build execution layers using topological sort
    void buildExecutionLayers() {
        execution_layers.clear();
        
        // Build output connections from input specs
        for (const auto& [id, node] : nodes) {
            for (const auto& input_id : node->inputs) {
                if (nodes.count(input_id)) {
                    // Add this node as output of input node
                    bool found = false;
                    for (const auto& out : nodes[input_id]->outputs) {
                        if (out == id) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        nodes[input_id]->outputs.push_back(id);
                    }
                }
            }
        }
        
        // Calculate in-degrees
        std::map<std::string, int> in_degree;
        for (const auto& [id, node] : nodes) {
            in_degree[id] = node->inputs.size();
        }
        
        // Find nodes with no inputs (starting nodes)
        std::queue<std::string> queue;
        for (const auto& [id, degree] : in_degree) {
            if (degree == 0) {
                queue.push(id);
            }
        }
        
        // Process layers
        while (!queue.empty()) {
            ExecutionLayer layer;
            size_t layer_size = queue.size();
            
            // Check if all nodes in this layer can run in parallel
            bool can_parallel = true;
            for (size_t i = 0; i < layer_size; i++) {
                std::string node_id = queue.front();
                queue.pop();
                
                layer.node_ids.push_back(node_id);
                
                if (!nodes[node_id]->parallel || !nodes[node_id]->node->canRunParallel()) {
                    can_parallel = false;
                }
                
                // Reduce in-degree for dependent nodes
                for (const auto& output : nodes[node_id]->outputs) {
                    in_degree[output]--;
                    if (in_degree[output] == 0) {
                        queue.push(output);
                    }
                }
            }
            
            layer.can_parallelize = can_parallel && layer.node_ids.size() > 1;
            execution_layers.push_back(layer);
        }
    }
    
    // Execute single node
    NodeResult executeNode(const std::string& node_id) {
        auto& instance = nodes[node_id];
        
        // Gather inputs from previous node results
        std::vector<NodeData> inputs;
        for (const auto& input_id : instance->inputs) {
            if (node_results.count(input_id)) {
                for (const auto& data : node_results[input_id].outputs) {
                    inputs.push_back(data);
                }
            }
        }
        
        // Process node
        return instance->node->process(inputs);
    }
    
    // Run pipeline once
    bool runOnce(const NodeData& initial_input = {}) {
        if (!running) {
            running = true;
            
            // Set initial input for first layer nodes
            if (!initial_input.type.empty()) {
                NodeResult initial_result;
                initial_result.success = true;
                initial_result.outputs.push_back(initial_input);
                node_results["__input__"] = initial_result;
                
                // Update first layer nodes to use __input__
                if (!execution_layers.empty()) {
                    for (const auto& node_id : execution_layers[0].node_ids) {
                        if (nodes[node_id]->inputs.empty()) {
                            nodes[node_id]->inputs.push_back("__input__");
                        }
                    }
                }
            }
            
            // Execute layers in order
            for (const auto& layer : execution_layers) {
                if (layer.can_parallelize && layer.node_ids.size() > 1) {
                    // Parallel execution
                    std::vector<std::future<NodeResult>> futures;
                    
                    for (const auto& node_id : layer.node_ids) {
                        futures.push_back(std::async(std::launch::async, 
                            [this, node_id]() { return executeNode(node_id); }));
                    }
                    
                    // Wait and collect results
                    for (size_t i = 0; i < layer.node_ids.size(); i++) {
                        node_results[layer.node_ids[i]] = futures[i].get();
                    }
                    
                } else {
                    // Sequential execution
                    for (const auto& node_id : layer.node_ids) {
                        node_results[node_id] = executeNode(node_id);
                        
                        // Check for errors
                        if (!node_results[node_id].success) {
                            std::cerr << "Node " << node_id << " failed: " 
                                     << node_results[node_id].error_message << std::endl;
                            running = false;
                            return false;
                        }
                    }
                }
            }
            
            running = false;
            return true;
        }
        return false;
    }
    
    // Run pipeline continuously (for streaming)
    void runContinuous() {
        running = true;
        while (running) {
            runOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    // Stop pipeline
    void stop() {
        running = false;
        
        // Shutdown all nodes
        for (auto& [id, instance] : nodes) {
            if (instance->node) {
                instance->node->shutdown();
            }
        }
    }
    
    // Get pipeline status
    nlohmann::json getStatus() const {
        nlohmann::json status;
        status["running"] = running.load();
        status["layers"] = execution_layers.size();
        
        nlohmann::json nodes_status;
        for (const auto& [id, instance] : nodes) {
            nodes_status[id] = {
                {"type", instance->type},
                {"inputs", instance->inputs},
                {"outputs", instance->outputs},
                {"parallel", instance->parallel}
            };
        }
        status["nodes"] = nodes_status;
        
        return status;
    }
    
    // Get results from specific node
    NodeResult getNodeResult(const std::string& node_id) const {
        if (node_results.count(node_id)) {
            return node_results.at(node_id);
        }
        return NodeResult::error("Node result not found");
    }
};

} // namespace pipeline
} // namespace ctic

#endif // CTIC_PIPELINE_EXECUTOR_H