// CTIC Pipeline Engine - Main CLI
#include "../include/pipeline/executor.h"
#include "../include/nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>

using namespace ctic::pipeline;
namespace fs = std::filesystem;

// Template manager for loading templates
class TemplateManager {
private:
    std::string template_dir = "config/templates/";
    
public:
    nlohmann::json loadTemplate(const std::string& name) {
        std::string path = template_dir + name + ".json";
        if (!fs::exists(path)) {
            throw std::runtime_error("Template not found: " + name);
        }
        
        std::ifstream file(path);
        nlohmann::json tmpl;
        file >> tmpl;
        return tmpl;
    }
    
    std::vector<std::string> listTemplates() {
        std::vector<std::string> templates;
        for (const auto& entry : fs::directory_iterator(template_dir)) {
            if (entry.path().extension() == ".json") {
                templates.push_back(entry.path().stem());
            }
        }
        return templates;
    }
    
    nlohmann::json applyVariables(const nlohmann::json& tmpl, 
                                   const std::map<std::string, std::string>& vars) {
        // Apply variable substitution
        std::string json_str = tmpl.dump();
        for (const auto& [key, value] : vars) {
            std::string placeholder = "${" + key + "}";
            size_t pos = 0;
            while ((pos = json_str.find(placeholder, pos)) != std::string::npos) {
                json_str.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }
        return nlohmann::json::parse(json_str);
    }
};

void printUsage() {
    std::cout << "CTIC Pipeline Engine v3.0" << std::endl;
    std::cout << "The modular clip detection engine" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  ctic pipeline run --template <name> [options]" << std::endl;
    std::cout << "  ctic pipeline run --config <file.json>" << std::endl;
    std::cout << "  ctic pipeline list" << std::endl;
    std::cout << "  ctic pipeline validate <config.json>" << std::endl;
    std::cout << "  ctic plugins list" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  ctic pipeline run --template simple_spike --channel shroud" << std::endl;
    std::cout << "  ctic pipeline run --template multi_signal --channel xqc --threshold 2.5" << std::endl;
    std::cout << "  ctic pipeline run --config my_custom_pipeline.json" << std::endl;
    std::cout << std::endl;
    std::cout << "Available Templates:" << std::endl;
    
    TemplateManager mgr;
    for (const auto& tmpl : mgr.listTemplates()) {
        std::cout << "  - " << tmpl << std::endl;
    }
}

int runPipeline(int argc, char* argv[]) {
    std::string config_file;
    std::string template_name;
    std::map<std::string, std::string> variables;
    
    // Parse arguments
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "--template" && i + 1 < argc) {
            template_name = argv[++i];
        } else if (arg == "--channel" && i + 1 < argc) {
            variables["CHANNEL"] = argv[++i];
        } else if (arg == "--threshold" && i + 1 < argc) {
            variables["SPIKE_THRESHOLD"] = argv[++i];
        } else if (arg.substr(0, 2) == "--" && i + 1 < argc) {
            // Generic variable
            std::string key = arg.substr(2);
            variables[key] = argv[++i];
        }
    }
    
    // Load pipeline configuration
    nlohmann::json pipeline_config;
    
    if (!config_file.empty()) {
        // Load from file
        std::ifstream file(config_file);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open config file: " << config_file << std::endl;
            return 1;
        }
        file >> pipeline_config;
    } else if (!template_name.empty()) {
        // Load from template
        try {
            TemplateManager mgr;
            auto tmpl = mgr.loadTemplate(template_name);
            
            // Set default variables
            if (!variables.count("CHANNEL")) {
                std::cerr << "Error: --channel required when using template" << std::endl;
                return 1;
            }
            
            // Apply variables
            pipeline_config = mgr.applyVariables(tmpl, variables);
            
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Error: Either --config or --template required" << std::endl;
        printUsage();
        return 1;
    }
    
    // Create registry and load plugins
    NodeRegistry registry;
    
    // Load plugins from all directories
    registry.loadPluginsFromDirectory("plugins/preprocessing");
    registry.loadPluginsFromDirectory("plugins/detectors");
    registry.loadPluginsFromDirectory("plugins/outputs");
    registry.loadPluginsFromDirectory("plugins/models");
    
    std::cout << "\n=== Pipeline Engine Starting ===" << std::endl;
    std::cout << "Loaded " << registry.getAvailableTypes().size() << " node types" << std::endl;
    
    // Create and configure executor
    PipelineExecutor executor(&registry);
    
    if (!executor.loadPipeline(pipeline_config)) {
        std::cerr << "Error: Failed to load pipeline configuration" << std::endl;
        return 1;
    }
    
    // Show pipeline info
    auto status = executor.getStatus();
    std::cout << "\nPipeline loaded:" << std::endl;
    std::cout << "  Nodes: " << status["nodes"].size() << std::endl;
    std::cout << "  Layers: " << status["layers"] << std::endl;
    
    // Run pipeline
    std::cout << "\n=== Running Pipeline ===" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    
    // For now, run once as a test
    // In production, this would connect to real data sources
    executor.runOnce();
    
    std::cout << "\n=== Pipeline Complete ===" << std::endl;
    return 0;
}

int listPlugins() {
    NodeRegistry registry;
    
    // Load all plugins
    registry.loadPluginsFromDirectory("plugins/preprocessing");
    registry.loadPluginsFromDirectory("plugins/detectors");
    registry.loadPluginsFromDirectory("plugins/outputs");
    registry.loadPluginsFromDirectory("plugins/models");
    
    std::cout << "Available Node Types:" << std::endl;
    std::cout << std::endl;
    
    auto types = registry.getAvailableTypes();
    std::sort(types.begin(), types.end());
    
    for (const auto& type : types) {
        std::cout << "  - " << type << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Total: " << types.size() << " node types" << std::endl;
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }
    
    std::string command = argv[1];
    
    if (command == "pipeline" && argc >= 3) {
        std::string subcommand = argv[2];
        
        if (subcommand == "run") {
            return runPipeline(argc, argv);
        } else if (subcommand == "list") {
            TemplateManager mgr;
            std::cout << "Available Pipeline Templates:" << std::endl;
            std::cout << std::endl;
            
            for (const auto& tmpl : mgr.listTemplates()) {
                // Load template to show description
                auto t = mgr.loadTemplate(tmpl);
                std::cout << "  " << tmpl;
                if (t.contains("description")) {
                    std::cout << " - " << t["description"].get<std::string>();
                }
                std::cout << std::endl;
            }
            return 0;
        }
    } else if (command == "plugins" && argc >= 3) {
        std::string subcommand = argv[2];
        if (subcommand == "list") {
            return listPlugins();
        }
    } else if (command == "help" || command == "--help") {
        printUsage();
        return 0;
    }
    
    std::cerr << "Unknown command: " << command << std::endl;
    printUsage();
    return 1;
}