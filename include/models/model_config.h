#ifndef CTIC_MODEL_CONFIG_H
#define CTIC_MODEL_CONFIG_H

#include <string>
#include <vector>
#include <map>
#include "../nlohmann/json.hpp"

namespace ctic {
namespace models {

/**
 * ModelInfo - Metadata about a model
 * 
 * This struct contains all the metadata about a model including
 * its name, version, source, and performance characteristics.
 * Used for model discovery and validation.
 */
struct ModelInfo {
    std::string name;                  // Human-readable model name
    std::string version;                // Model version
    std::string description;            // What the model does
    std::string author;                 // Model creator
    std::string source;                 // Original source (HuggingFace URL, etc)
    std::string license;                // Model license (MIT, Apache, etc)
    
    // Convert to/from JSON for serialization
    nlohmann::json toJson() const {
        return {
            {"name", name},
            {"version", version},
            {"description", description},
            {"author", author},
            {"source", source},
            {"license", license}
        };
    }
    
    static ModelInfo fromJson(const nlohmann::json& j) {
        ModelInfo info;
        info.name = j.value("name", "");
        info.version = j.value("version", "1.0");
        info.description = j.value("description", "");
        info.author = j.value("author", "unknown");
        info.source = j.value("source", "");
        info.license = j.value("license", "unknown");
        return info;
    }
};

/**
 * PreprocessingConfig - Configuration for input preprocessing
 * 
 * Defines how raw text should be preprocessed before being
 * fed to the model. Supports various tokenization methods
 * and text normalization options.
 */
struct PreprocessingConfig {
    std::string tokenizer;              // Tokenizer type (bert, roberta, gpt2, etc)
    int max_length = 512;               // Maximum sequence length
    bool lowercase = false;             // Convert to lowercase
    bool remove_urls = true;            // Remove URLs from text
    bool expand_emotes = true;          // Expand emotes to text
    std::string padding = "max_length"; // Padding strategy
    std::string truncation = "longest_first"; // Truncation strategy
    
    static PreprocessingConfig fromJson(const nlohmann::json& j) {
        PreprocessingConfig config;
        config.tokenizer = j.value("tokenizer", "bert");
        config.max_length = j.value("max_length", 512);
        config.lowercase = j.value("lowercase", false);
        config.remove_urls = j.value("remove_urls", true);
        config.expand_emotes = j.value("expand_emotes", true);
        config.padding = j.value("padding", "max_length");
        config.truncation = j.value("truncation", "longest_first");
        return config;
    }
};

/**
 * InferenceConfig - Configuration for model inference
 * 
 * Specifies the input/output tensor names, shapes, and
 * execution providers for ONNX Runtime.
 */
struct InferenceConfig {
    std::vector<std::string> input_names;   // Names of input tensors
    std::vector<std::string> output_names;  // Names of output tensors
    std::vector<int64_t> input_shape;       // Expected input shape
    int batch_size = 1;                     // Batch size for inference
    std::vector<std::string> providers;     // ONNX execution providers
    int intra_threads = 1;                   // Threads for single op
    int inter_threads = 1;                   // Threads for parallel ops
    std::string optimization_level = "all";  // Graph optimization level
    
    static InferenceConfig fromJson(const nlohmann::json& j) {
        InferenceConfig config;
        
        if (j.contains("input_names")) {
            config.input_names = j["input_names"].get<std::vector<std::string>>();
        }
        if (j.contains("output_names")) {
            config.output_names = j["output_names"].get<std::vector<std::string>>();
        }
        if (j.contains("input_shape")) {
            config.input_shape = j["input_shape"].get<std::vector<int64_t>>();
        }
        
        config.batch_size = j.value("batch_size", 1);
        config.intra_threads = j.value("intra_threads", 1);
        config.inter_threads = j.value("inter_threads", 1);
        config.optimization_level = j.value("optimization_level", "all");
        
        if (j.contains("providers")) {
            config.providers = j["providers"].get<std::vector<std::string>>();
        } else {
            config.providers = {"CPUExecutionProvider"};
        }
        
        return config;
    }
};

/**
 * PostprocessingConfig - Configuration for output postprocessing
 * 
 * Defines how model outputs should be interpreted and
 * converted into meaningful results.
 */
struct PostprocessingConfig {
    std::string type;                       // Output type (classification, regression, etc)
    std::string activation;                 // Activation function (softmax, sigmoid, none)
    std::vector<std::string> labels;        // Class labels for classification
    float threshold = 0.5f;                 // Confidence threshold
    int top_k = 1;                          // Return top K predictions
    
    static PostprocessingConfig fromJson(const nlohmann::json& j) {
        PostprocessingConfig config;
        config.type = j.value("type", "classification");
        config.activation = j.value("activation", "softmax");
        
        if (j.contains("labels")) {
            config.labels = j["labels"].get<std::vector<std::string>>();
        }
        
        config.threshold = j.value("threshold", 0.5f);
        config.top_k = j.value("top_k", 1);
        return config;
    }
};

/**
 * ClipDetectionConfig - Configuration for clip detection triggers
 * 
 * Specifies when and how model outputs should trigger
 * clip detection events.
 */
struct ClipDetectionConfig {
    std::vector<std::string> trigger_on;    // Labels that trigger detection
    float min_confidence = 0.7f;            // Minimum confidence to trigger
    std::string description_template;       // Template for detection description
    bool combine_with_spike = false;        // Require spike detection too
    
    static ClipDetectionConfig fromJson(const nlohmann::json& j) {
        ClipDetectionConfig config;
        
        if (j.contains("trigger_on")) {
            config.trigger_on = j["trigger_on"].get<std::vector<std::string>>();
        }
        
        config.min_confidence = j.value("min_confidence", 0.7f);
        config.description_template = j.value("description_template", 
            "{label} detected with {confidence}% confidence");
        config.combine_with_spike = j.value("combine_with_spike", false);
        
        return config;
    }
};

/**
 * PerformanceProfile - Model performance characteristics
 * 
 * Documents the expected performance of the model to help
 * users choose appropriate models for their hardware.
 */
struct PerformanceProfile {
    int avg_inference_ms = 0;           // Average inference time
    int memory_mb = 0;                   // Memory usage in MB
    int throughput_msgs_sec = 0;         // Messages per second
    std::string quantization = "none";   // Quantization type (none, int8, fp16)
    
    static PerformanceProfile fromJson(const nlohmann::json& j) {
        PerformanceProfile profile;
        profile.avg_inference_ms = j.value("avg_inference_ms", 0);
        profile.memory_mb = j.value("memory_mb", 0);
        profile.throughput_msgs_sec = j.value("throughput_msgs_sec", 0);
        profile.quantization = j.value("quantization", "none");
        return profile;
    }
};

/**
 * ModelConfig - Complete model configuration
 * 
 * This is the main configuration class that combines all
 * the sub-configurations. Each model package has a config.json
 * file that is loaded into this structure.
 */
class ModelConfig {
public:
    ModelInfo info;                         // Model metadata
    PreprocessingConfig preprocessing;       // Input preprocessing
    InferenceConfig inference;              // Inference settings
    PostprocessingConfig postprocessing;    // Output postprocessing
    ClipDetectionConfig clip_detection;     // Clip detection settings
    PerformanceProfile performance;         // Performance characteristics
    
    std::string model_path;                 // Path to ONNX model file
    std::string config_path;                // Path to this config file
    
    /**
     * Load configuration from JSON file
     * @param path Path to config.json file
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::string& path);
    
    /**
     * Load configuration from JSON object
     * @param j JSON object containing configuration
     * @return true if loaded successfully
     */
    bool loadFromJson(const nlohmann::json& j);
    
    /**
     * Validate that configuration is complete and valid
     * @return true if configuration is valid
     */
    bool validate() const;
    
    /**
     * Get a human-readable summary of the model
     * @return Summary string
     */
    std::string getSummary() const;
    
    /**
     * Convert configuration to JSON
     * @return JSON representation
     */
    nlohmann::json toJson() const;
};

} // namespace models
} // namespace ctic

#endif // CTIC_MODEL_CONFIG_H