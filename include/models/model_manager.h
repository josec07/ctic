#ifndef CTIC_MODEL_MANAGER_H
#define CTIC_MODEL_MANAGER_H

#include "model_config.h"
#include <map>
#include <memory>
#include <filesystem>

namespace ctic {
namespace models {

/**
 * ModelManager - Central manager for model discovery and loading
 * 
 * This class handles:
 * - Automatic discovery of models in the models/ directory
 * - Validation of model configurations
 * - Caching of loaded configurations
 * - Model availability checking
 * 
 * The manager scans the models directory on initialization and
 * registers all valid models found. Each subdirectory with a
 * config.json file is considered a model package.
 */
class ModelManager {
private:
    std::string models_dir;                                    // Root models directory
    std::map<std::string, std::unique_ptr<ModelConfig>> models; // Loaded model configs
    std::map<std::string, std::string> model_paths;           // Model ID to path mapping
    bool auto_discover;                                       // Auto-discover on init
    
    /**
     * Scan a directory for model packages
     * @param dir Directory to scan
     */
    void scanDirectory(const std::string& dir);
    
    /**
     * Validate model package structure
     * @param path Path to model directory
     * @return true if valid model package
     */
    bool validateModelPackage(const std::string& path) const;
    
public:
    /**
     * Constructor
     * @param models_directory Root directory for models (default: "models/")
     * @param auto_discover Whether to auto-discover models on init
     */
    explicit ModelManager(const std::string& models_directory = "models/", 
                         bool auto_discover = true);
    
    /**
     * Discover all models in the models directory
     * Scans for model packages and validates them
     * @return Number of models discovered
     */
    int discoverModels();
    
    /**
     * Load a specific model configuration
     * @param model_id Model identifier (directory name)
     * @return Pointer to loaded config, nullptr if failed
     */
    ModelConfig* loadModel(const std::string& model_id);
    
    /**
     * Check if a model is available
     * @param model_id Model identifier
     * @return true if model exists and is valid
     */
    bool hasModel(const std::string& model_id) const;
    
    /**
     * Get list of available models
     * @return Vector of model IDs
     */
    std::vector<std::string> getAvailableModels() const;
    
    /**
     * Get model info without fully loading
     * @param model_id Model identifier
     * @return Model info or empty struct if not found
     */
    ModelInfo getModelInfo(const std::string& model_id);
    
    /**
     * Validate a model configuration
     * @param model_id Model to validate
     * @return true if valid
     */
    bool validateModel(const std::string& model_id);
    
    /**
     * Get summary of all available models
     * @return Human-readable summary string
     */
    std::string getModelsSummary() const;
    
    /**
     * Test a model with sample input
     * @param model_id Model to test
     * @param sample_text Sample text to process
     * @return Test results as string
     */
    std::string testModel(const std::string& model_id, 
                         const std::string& sample_text);
    
    /**
     * Register a model from a custom path
     * @param model_id ID to register as
     * @param path Path to model directory
     * @return true if registered successfully
     */
    bool registerModel(const std::string& model_id, const std::string& path);
    
    /**
     * Get model performance profile
     * @param model_id Model identifier
     * @return Performance profile
     */
    PerformanceProfile getPerformanceProfile(const std::string& model_id);
    
    /**
     * Find models by capability
     * @param capability Capability to search for (e.g., "sentiment", "toxicity")
     * @return Vector of matching model IDs
     */
    std::vector<std::string> findModelsByCapability(const std::string& capability) const;
};

/**
 * ModelValidator - Utility class for model validation
 * 
 * Provides static methods to validate model packages and
 * configurations before loading.
 */
class ModelValidator {
public:
    /**
     * Check if directory is a valid model package
     * @param path Directory path
     * @return true if valid package
     */
    static bool isValidPackage(const std::string& path);
    
    /**
     * Validate ONNX model file
     * @param model_path Path to ONNX file
     * @return true if valid ONNX model
     */
    static bool validateOnnxFile(const std::string& model_path);
    
    /**
     * Check model compatibility with system
     * @param config Model configuration
     * @return true if compatible
     */
    static bool checkCompatibility(const ModelConfig& config);
    
    /**
     * Get validation errors
     * @param path Model package path
     * @return Vector of error messages
     */
    static std::vector<std::string> getValidationErrors(const std::string& path);
};

} // namespace models
} // namespace ctic

#endif // CTIC_MODEL_MANAGER_H