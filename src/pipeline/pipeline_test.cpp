// Pipeline test program
#include "../../include/pipeline/executor.h"
#include "../../include/nlohmann/json.hpp"
#include <iostream>
#include <fstream>

using namespace ctic::pipeline;

// Built-in test input node
class TestInputNode : public INode {
public:
    bool initialize(const nlohmann::json& config) override {
        return true;
    }
    
    void shutdown() override {}
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        // Generate test data
        NodeData output("text", std::string("Hello World! This is a TEST message."));
        output.metadata["timestamp"] = std::time(nullptr);
        output.metadata["source"] = "test";
        result.outputs.push_back(output);
        
        return result;
    }
    
    bool canRunParallel() const override { return false; }
    std::string getType() const override { return "test_input"; }
    std::string getName() const override { return "Test Input"; }
    std::string getDescription() const override { return "Generates test messages"; }
};

// Built-in console output node
class ConsoleOutputNode : public INode {
public:
    bool initialize(const nlohmann::json& config) override {
        return true;
    }
    
    void shutdown() override {}
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        for (const auto& input : inputs) {
            std::cout << "[OUTPUT] Type: " << input.type << std::endl;
            
            if (input.type == "text" || input.type == "normalized_text") {
                try {
                    std::string text = std::any_cast<std::string>(input.payload);
                    std::cout << "[OUTPUT] Text: " << text << std::endl;
                } catch (const std::bad_any_cast& e) {
                    std::cout << "[OUTPUT] Failed to cast text" << std::endl;
                }
            }
            
            if (!input.metadata.empty()) {
                std::cout << "[OUTPUT] Metadata: " << input.metadata.dump() << std::endl;
            }
            std::cout << "---" << std::endl;
        }
        
        return result;
    }
    
    bool canRunParallel() const override { return false; }
    std::string getType() const override { return "console_output"; }
    std::string getName() const override { return "Console Output"; }
    std::string getDescription() const override { return "Outputs to console"; }
};

int main(int argc, char* argv[]) {
    std::cout << "=== CTIC Pipeline Engine Test ===" << std::endl;
    
    // Create registry
    NodeRegistry registry;
    
    // Register built-in nodes
    registry.registerBuiltin("test_input", []() -> std::unique_ptr<INode> {
        return std::make_unique<TestInputNode>();
    });
    
    registry.registerBuiltin("console_output", []() -> std::unique_ptr<INode> {
        return std::make_unique<ConsoleOutputNode>();
    });
    
    // Load plugins from directory
    std::cout << "Loading plugins..." << std::endl;
    registry.loadPluginsFromDirectory("plugins/example");
    
    // List available nodes
    std::cout << "\nAvailable node types:" << std::endl;
    for (const auto& type : registry.getAvailableTypes()) {
        std::cout << "  - " << type << std::endl;
    }
    
    // Create test pipeline configuration
    nlohmann::json pipeline_config = {
        {"version", "3.0"},
        {"name", "test_pipeline"},
        {"pipeline", {
            {"nodes", {
                {
                    {"id", "input"},
                    {"type", "test_input"},
                    {"config", {}}
                },
                {
                    {"id", "normalizer"},
                    {"type", "text_normalizer"},
                    {"inputs", {"input"}},
                    {"config", {
                        {"lowercase", true},
                        {"remove_punctuation", false},
                        {"trim_spaces", true}
                    }}
                },
                {
                    {"id", "output"},
                    {"type", "console_output"},
                    {"inputs", {"normalizer"}},
                    {"config", {}}
                }
            }}
        }}
    };
    
    // Create executor
    PipelineExecutor executor(&registry);
    
    // Load pipeline
    std::cout << "\nLoading pipeline..." << std::endl;
    if (!executor.loadPipeline(pipeline_config)) {
        std::cerr << "Failed to load pipeline" << std::endl;
        return 1;
    }
    
    // Show pipeline status
    std::cout << "\nPipeline status:" << std::endl;
    std::cout << executor.getStatus().dump(2) << std::endl;
    
    // Run pipeline
    std::cout << "\nRunning pipeline..." << std::endl;
    std::cout << "Processing test message through pipeline:\n" << std::endl;
    
    if (executor.runOnce()) {
        std::cout << "\nPipeline executed successfully!" << std::endl;
    } else {
        std::cerr << "Pipeline execution failed" << std::endl;
        return 1;
    }
    
    return 0;
}