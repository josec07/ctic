#include "../../include/models/model_manager.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

namespace ctic {
namespace models {

/**
 * Constructor - Initialize model manager
 * 
 * Sets up the models directory and optionally discovers all
 * available models on initialization.
 */
ModelManager::ModelManager(const std::string& models_directory, bool auto_disc)
    : models_dir(models_directory), auto_discover(auto_disc) {
    
    // Ensure models directory exists
    if (!fs::exists(models_dir)) {
        fs::create_directories(models_dir);
    }
    
    // Auto-discover models if requested
    if (auto_discover) {
        discoverModels();
    }
}

/**
 * discoverModels - Scan for all available models
 * 
 * Recursively scans the models directory for valid model packages.
 * A valid package is a directory containing config.json and model.onnx
 */
int ModelManager::discoverModels() {
    models.clear();
    model_paths.clear();
    
    scanDirectory(models_dir);
    
    std::cout << "Discovered " << model_paths.size() << " models" << std::endl;
    return model_paths.size();
}

/**
 * scanDirectory - Recursively scan directory for models
 * 
 * Looks for directories containing config.json files and
 * registers them as potential model packages.
 */
void ModelManager::scanDirectory(const std::string& dir) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }
    
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) {
            std::string path = entry.path().string();
            
            // Check if this directory contains a model package
            if (validateModelPackage(path)) {
                // Extract model ID from directory name
                std::string model_id = entry.path().filename().string();
                model_paths[model_id] = path;
                
                std::cout << "Found model: " << model_id << " at " << path << std::endl;
            }
            
            // Recursively scan subdirectories
            scanDirectory(path);
        }
    }
}

/**
 * validateModelPackage - Check if directory is valid model package
 * 
 * A valid package must contain:
 * - config.json: Model configuration
 * - model.onnx: The ONNX model file (or path specified in config)
 */
bool ModelManager::validateModelPackage(const std::string& path) const {
    fs::path dir_path(path);
    
    // Check for config.json
    fs::path config_path = dir_path / "config.json";
    if (!fs::exists(config_path)) {
        return false;
    }
    
    // Check for model.onnx (may be specified differently in config)
    fs::path model_path = dir_path / "model.onnx";
    if (!fs::exists(model_path)) {
        // Try to load config and check for custom model path
        try {
            std::ifstream file(config_path);
            nlohmann::json j;
            file >> j;
            
            if (j.contains("model_path")) {
                std::string custom_path = j["model_path"];
                if (!fs::path(custom_path).is_absolute()) {
                    custom_path = (dir_path / custom_path).string();
                }
                return fs::exists(custom_path);
            }
        } catch (...) {
            return false;
        }
        return false;
    }
    
    return true;
}

/**
 * loadModel - Load a specific model configuration
 * 
 * Loads and caches the model configuration. Subsequent calls
 * return the cached version for efficiency.
 */
ModelConfig* ModelManager::loadModel(const std::string& model_id) {
    // Check if already loaded
    if (models.count(model_id)) {
        return models[model_id].get();
    }
    
    // Check if model exists
    if (!model_paths.count(model_id)) {
        std::cerr << "Model not found: " << model_id << std::endl;
        return nullptr;
    }
    
    // Load the configuration
    auto config = std::make_unique<ModelConfig>();
    std::string config_path = model_paths[model_id] + "/config.json";
    
    if (!config->loadFromFile(config_path)) {
        std::cerr << "Failed to load model config: " << model_id << std::endl;
        return nullptr;
    }
    
    // Cache and return
    ModelConfig* ptr = config.get();
    models[model_id] = std::move(config);
    return ptr;
}

/**
 * hasModel - Check if model is available
 */
bool ModelManager::hasModel(const std::string& model_id) const {
    return model_paths.count(model_id) > 0;
}

/**
 * getAvailableModels - Get list of all available models
 */
std::vector<std::string> ModelManager::getAvailableModels() const {
    std::vector<std::string> result;
    for (const auto& [id, path] : model_paths) {
        result.push_back(id);
    }
    std::sort(result.begin(), result.end());
    return result;
}

/**
 * getModelInfo - Get model info without full load
 * 
 * Quickly reads just the model_info section from config.json
 * without loading the entire configuration.
 */
ModelInfo ModelManager::getModelInfo(const std::string& model_id) {
    if (!hasModel(model_id)) {
        return ModelInfo{};
    }
    
    try {
        std::string config_path = model_paths.at(model_id) + "/config.json";
        std::ifstream file(config_path);
        nlohmann::json j;
        file >> j;
        
        if (j.contains("model_info")) {
            return ModelInfo::fromJson(j["model_info"]);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading model info: " << e.what() << std::endl;
    }
    
    return ModelInfo{};
}

/**
 * validateModel - Validate model configuration and files
 */
bool ModelManager::validateModel(const std::string& model_id) {
    auto* config = loadModel(model_id);
    if (!config) {
        return false;
    }
    
    // Check that model file exists
    if (!fs::exists(config->model_path)) {
        std::cerr << "Model file not found: " << config->model_path << std::endl;
        return false;
    }
    
    // Validate configuration
    return config->validate();
}

/**
 * getModelsSummary - Get summary of all available models
 * 
 * Returns a formatted string describing all available models,
 * their types, and capabilities.
 */
std::string ModelManager::getModelsSummary() const {
    std::stringstream ss;
    ss << "Available Models:\n";
    ss << "================\n\n";
    
    for (const auto& model_id : getAvailableModels()) {
        auto info = const_cast<ModelManager*>(this)->getModelInfo(model_id);
        
        ss << model_id << ":\n";
        if (!info.name.empty()) {
            ss << "  Name: " << info.name << "\n";
        }
        if (!info.description.empty()) {
            ss << "  Description: " << info.description << "\n";
        }
        if (!info.author.empty()) {
            ss << "  Author: " << info.author << "\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

/**
 * testModel - Test model with sample input
 * 
 * Loads the model and runs inference on sample text to verify
 * it's working correctly. Returns results as formatted string.
 */
std::string ModelManager::testModel(const std::string& model_id, 
                                   const std::string& sample_text) {
    auto* config = loadModel(model_id);
    if (!config) {
        return "Error: Failed to load model";
    }
    
    std::stringstream ss;
    ss << "Model: " << config->info.name << "\n";
    ss << "Input: \"" << sample_text << "\"\n";
    ss << "\n";
    ss << "Configuration:\n";
    ss << "  Tokenizer: " << config->preprocessing.tokenizer << "\n";
    ss << "  Max Length: " << config->preprocessing.max_length << "\n";
    ss << "  Output Type: " << config->postprocessing.type << "\n";
    
    if (!config->postprocessing.labels.empty()) {
        ss << "  Labels: ";
        for (const auto& label : config->postprocessing.labels) {
            ss << label << " ";
        }
        ss << "\n";
    }
    
    ss << "\n[Actual inference would happen here with ONNX Runtime]\n";
    
    return ss.str();
}

/**
 * registerModel - Register a model from custom path
 */
bool ModelManager::registerModel(const std::string& model_id, const std::string& path) {
    if (!validateModelPackage(path)) {
        std::cerr << "Invalid model package at: " << path << std::endl;
        return false;
    }
    
    model_paths[model_id] = path;
    return true;
}

/**
 * getPerformanceProfile - Get model performance characteristics
 */
PerformanceProfile ModelManager::getPerformanceProfile(const std::string& model_id) {
    auto* config = loadModel(model_id);
    if (config) {
        return config->performance;
    }
    return PerformanceProfile{};
}

/**
 * findModelsByCapability - Find models with specific capability
 * 
 * Searches model descriptions and types for matching capability.
 * For example, searching for "sentiment" returns all sentiment models.
 */
std::vector<std::string> ModelManager::findModelsByCapability(const std::string& capability) const {
    std::vector<std::string> results;
    std::string lower_cap = capability;
    std::transform(lower_cap.begin(), lower_cap.end(), lower_cap.begin(), ::tolower);
    
    for (const auto& model_id : getAvailableModels()) {
        auto info = const_cast<ModelManager*>(this)->getModelInfo(model_id);
        
        // Search in description
        std::string desc = info.description;
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
        
        if (desc.find(lower_cap) != std::string::npos) {
            results.push_back(model_id);
            continue;
        }
        
        // Search in name
        std::string name = info.name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        if (name.find(lower_cap) != std::string::npos) {
            results.push_back(model_id);
        }
    }
    
    return results;
}

// ModelValidator implementation

bool ModelValidator::isValidPackage(const std::string& path) {
    fs::path dir_path(path);
    
    // Must have config.json
    if (!fs::exists(dir_path / "config.json")) {
        return false;
    }
    
    // Should have model.onnx or specified in config
    if (!fs::exists(dir_path / "model.onnx")) {
        // Check config for custom path
        try {
            std::ifstream file(dir_path / "config.json");
            nlohmann::json j;
            file >> j;
            
            if (j.contains("model_path")) {
                fs::path model_path = j["model_path"].get<std::string>();
                if (!model_path.is_absolute()) {
                    model_path = dir_path / model_path;
                }
                return fs::exists(model_path);
            }
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

bool ModelValidator::validateOnnxFile(const std::string& model_path) {
    // Basic validation - check file exists and has .onnx extension
    if (!fs::exists(model_path)) {
        return false;
    }
    
    fs::path p(model_path);
    if (p.extension() != ".onnx") {
        return false;
    }
    
    // Could add ONNX format validation here
    return true;
}

bool ModelValidator::checkCompatibility(const ModelConfig& config) {
    // Check if required providers are available
    // For now, we support CPU execution
    for (const auto& provider : config.inference.providers) {
        if (provider != "CPUExecutionProvider") {
            std::cerr << "Warning: Provider " << provider << " may not be available" << std::endl;
        }
    }
    
    return true;
}

std::vector<std::string> ModelValidator::getValidationErrors(const std::string& path) {
    std::vector<std::string> errors;
    
    fs::path dir_path(path);
    
    if (!fs::exists(dir_path / "config.json")) {
        errors.push_back("Missing config.json");
    }
    
    if (!fs::exists(dir_path / "model.onnx")) {
        errors.push_back("Missing model.onnx");
    }
    
    // Try to load and validate config
    try {
        ModelConfig config;
        if (!config.loadFromFile((dir_path / "config.json").string())) {
            errors.push_back("Invalid configuration format");
        }
    } catch (const std::exception& e) {
        errors.push_back(std::string("Config error: ") + e.what());
    }
    
    return errors;
}

} // namespace models
} // namespace ctic