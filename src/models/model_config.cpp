#include "../../include/models/model_config.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace ctic {
namespace models {

/**
 * loadFromFile - Load model configuration from JSON file
 * 
 * This method reads a JSON configuration file and populates all
 * the configuration structures. The file should contain sections
 * for model_info, preprocessing, inference, postprocessing,
 * clip_detection, and performance.
 */
bool ModelConfig::loadFromFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << path << std::endl;
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        config_path = path;
        
        // Extract model file path (assumes model.onnx in same directory)
        size_t lastSlash = path.find_last_of("/");
        if (lastSlash != std::string::npos) {
            std::string dir = path.substr(0, lastSlash + 1);
            model_path = dir + "model.onnx";
        }
        
        return loadFromJson(j);
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

/**
 * loadFromJson - Load configuration from JSON object
 * 
 * Parses the JSON object and populates all sub-configurations.
 * Each section is optional but will use sensible defaults if missing.
 */
bool ModelConfig::loadFromJson(const nlohmann::json& j) {
    try {
        // Load each configuration section
        if (j.contains("model_info")) {
            info = ModelInfo::fromJson(j["model_info"]);
        }
        
        if (j.contains("preprocessing")) {
            preprocessing = PreprocessingConfig::fromJson(j["preprocessing"]);
        }
        
        if (j.contains("inference")) {
            inference = InferenceConfig::fromJson(j["inference"]);
        }
        
        if (j.contains("postprocessing")) {
            postprocessing = PostprocessingConfig::fromJson(j["postprocessing"]);
        }
        
        if (j.contains("clip_detection")) {
            clip_detection = ClipDetectionConfig::fromJson(j["clip_detection"]);
        }
        
        if (j.contains("performance")) {
            performance = PerformanceProfile::fromJson(j["performance"]);
        }
        
        // Direct model path override
        if (j.contains("model_path")) {
            model_path = j["model_path"];
        }
        
        return validate();
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config JSON: " << e.what() << std::endl;
        return false;
    }
}

/**
 * validate - Validate configuration completeness
 * 
 * Checks that all required fields are present and valid.
 * Returns false if configuration is incomplete or invalid.
 */
bool ModelConfig::validate() const {
    // Check required fields
    if (info.name.empty()) {
        std::cerr << "Model name is required" << std::endl;
        return false;
    }
    
    if (model_path.empty()) {
        std::cerr << "Model path is required" << std::endl;
        return false;
    }
    
    if (inference.input_names.empty()) {
        std::cerr << "Input tensor names are required" << std::endl;
        return false;
    }
    
    if (inference.output_names.empty()) {
        std::cerr << "Output tensor names are required" << std::endl;
        return false;
    }
    
    if (postprocessing.type == "classification" && postprocessing.labels.empty()) {
        std::cerr << "Classification models require labels" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * getSummary - Get human-readable model summary
 * 
 * Returns a formatted string describing the model's capabilities
 * and configuration. Useful for displaying to users.
 */
std::string ModelConfig::getSummary() const {
    std::stringstream ss;
    
    ss << "Model: " << info.name;
    if (!info.version.empty()) {
        ss << " v" << info.version;
    }
    ss << "\n";
    
    if (!info.description.empty()) {
        ss << "Description: " << info.description << "\n";
    }
    
    ss << "Type: " << postprocessing.type << "\n";
    
    if (!postprocessing.labels.empty()) {
        ss << "Labels: ";
        for (size_t i = 0; i < postprocessing.labels.size() && i < 5; i++) {
            if (i > 0) ss << ", ";
            ss << postprocessing.labels[i];
        }
        if (postprocessing.labels.size() > 5) {
            ss << " (+" << (postprocessing.labels.size() - 5) << " more)";
        }
        ss << "\n";
    }
    
    if (performance.memory_mb > 0) {
        ss << "Memory: " << performance.memory_mb << " MB\n";
    }
    
    if (performance.avg_inference_ms > 0) {
        ss << "Inference: " << performance.avg_inference_ms << " ms\n";
    }
    
    if (!info.license.empty()) {
        ss << "License: " << info.license << "\n";
    }
    
    return ss.str();
}

/**
 * toJson - Convert configuration to JSON
 * 
 * Serializes the complete configuration to JSON format.
 * Useful for saving configurations or sending over network.
 */
nlohmann::json ModelConfig::toJson() const {
    return {
        {"model_info", info.toJson()},
        {"preprocessing", {
            {"tokenizer", preprocessing.tokenizer},
            {"max_length", preprocessing.max_length},
            {"lowercase", preprocessing.lowercase},
            {"remove_urls", preprocessing.remove_urls},
            {"expand_emotes", preprocessing.expand_emotes},
            {"padding", preprocessing.padding},
            {"truncation", preprocessing.truncation}
        }},
        {"inference", {
            {"input_names", inference.input_names},
            {"output_names", inference.output_names},
            {"input_shape", inference.input_shape},
            {"batch_size", inference.batch_size},
            {"providers", inference.providers},
            {"intra_threads", inference.intra_threads},
            {"inter_threads", inference.inter_threads},
            {"optimization_level", inference.optimization_level}
        }},
        {"postprocessing", {
            {"type", postprocessing.type},
            {"activation", postprocessing.activation},
            {"labels", postprocessing.labels},
            {"threshold", postprocessing.threshold},
            {"top_k", postprocessing.top_k}
        }},
        {"clip_detection", {
            {"trigger_on", clip_detection.trigger_on},
            {"min_confidence", clip_detection.min_confidence},
            {"description_template", clip_detection.description_template},
            {"combine_with_spike", clip_detection.combine_with_spike}
        }},
        {"performance", {
            {"avg_inference_ms", performance.avg_inference_ms},
            {"memory_mb", performance.memory_mb},
            {"throughput_msgs_sec", performance.throughput_msgs_sec},
            {"quantization", performance.quantization}
        }},
        {"model_path", model_path}
    };
}

} // namespace models
} // namespace ctic