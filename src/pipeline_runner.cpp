/**
 * CTIC Pipeline Runner
 * Execute pipeline configurations from JSON files
 */

#include "include/pipeline/executor.h"
#include "include/pipeline/registry.h"
#include "include/nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

using namespace ctic::pipeline;
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config.json>" << std::endl;
        return 1;
    }
    
    std::cout << "=== CTIC Pipeline Runner ===" << std::endl;
    std::cout << "Loading configuration: " << argv[1] << std::endl;
    
    // Load configuration file
    std::ifstream config_file(argv[1]);
    if (!config_file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << argv[1] << std::endl;
        return 1;
    }
    
    json config;
    try {
        config_file >> config;
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return 1;
    }
    
    // Create node registry
    NodeRegistry registry;
    
    // Load all plugins referenced in the config
    std::cout << "\nLoading plugins..." << std::endl;
    if (config.contains("nodes")) {
        for (const auto& node : config["nodes"]) {
            if (node.contains("plugin")) {
                std::string plugin_path = "plugins/" + node["plugin"].get<std::string>();
                if (registry.loadPlugin(plugin_path)) {
                    std::cout << "  ✓ Loaded: " << plugin_path << std::endl;
                } else {
                    std::cerr << "  ✗ Failed to load: " << plugin_path << std::endl;
                }
            }
        }
    }
    
    // List available node types
    std::cout << "\nAvailable node types:" << std::endl;
    for (const auto& type : registry.getAvailableTypes()) {
        std::cout << "  - " << type << std::endl;
    }
    
    // Convert config to expected format
    json pipeline_config = {
        {"version", "3.0"},
        {"name", config.value("name", "pipeline")},
        {"pipeline", {
            {"nodes", json::array()}
        }}
    };
    
    // Convert nodes to expected format
    for (const auto& node : config["nodes"]) {
        json converted_node = {
            {"id", node["id"]},
            {"type", node["type"]},
            {"config", node.value("config", json::object())}
        };
        
        // Find inputs for this node from connections
        json inputs = json::array();
        if (config.contains("connections")) {
            for (const auto& conn : config["connections"]) {
                if (conn["to"] == node["id"]) {
                    inputs.push_back(conn["from"]);
                }
            }
        }
        if (!inputs.empty()) {
            converted_node["inputs"] = inputs;
        }
        
        pipeline_config["pipeline"]["nodes"].push_back(converted_node);
    }
    
    // Create executor
    PipelineExecutor executor(&registry);
    
    // Load pipeline
    std::cout << "\nLoading pipeline..." << std::endl;
    if (!executor.loadPipeline(pipeline_config)) {
        std::cerr << "Failed to load pipeline" << std::endl;
        return 1;
    }
    
    // Show pipeline status
    std::cout << "\nPipeline loaded successfully!" << std::endl;
    std::cout << "Pipeline: " << config.value("name", "Unnamed") << std::endl;
    std::cout << "Description: " << config.value("description", "No description") << std::endl;
    std::cout << "Nodes: " << config["nodes"].size() << std::endl;
    
    // Determine execution mode
    int max_iterations = 100;
    if (config.contains("execution") && config["execution"].contains("max_iterations")) {
        max_iterations = config["execution"]["max_iterations"];
    }
    
    // Run pipeline
    std::cout << "\n=== Starting Pipeline Execution ===" << std::endl;
    std::cout << "Max iterations: " << max_iterations << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    int iteration = 0;
    bool continue_running = true;
    
    while (continue_running && iteration < max_iterations) {
        iteration++;
        
        // Run one iteration
        if (!executor.runOnce()) {
            std::cerr << "\n[Error] Pipeline execution failed at iteration " << iteration << std::endl;
            break;
        }
        
        // Check if we should continue (e.g., check for end_of_stream marker)
        // For now, just run the specified number of iterations
        
        // Small delay between iterations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Show progress every 10 iterations
        if (iteration % 10 == 0) {
            std::cout << "[Progress] Iteration " << iteration << "/" << max_iterations << std::endl;
        }
    }
    
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "Pipeline completed after " << iteration << " iterations" << std::endl;
    
    // Show final statistics
    auto status = executor.getStatus();
    if (status.contains("stats")) {
        std::cout << "\nPipeline Statistics:" << std::endl;
        std::cout << status["stats"].dump(2) << std::endl;
    }
    
    std::cout << "\nPipeline execution finished successfully!" << std::endl;
    
    return 0;
}