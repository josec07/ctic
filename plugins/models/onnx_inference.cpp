/**
 * ONNX Inference Node
 * Generic ML model inference for CTIC pipeline
 * Supports any ONNX model with configuration-driven setup
 */

#include "../../include/pipeline/node.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <cmath>

// Forward declaration - ONNX Runtime types will be defined in implementation
// For now, using placeholder types that will be replaced with actual ORT API

using namespace ctic::pipeline;
using json = nlohmann::json;

/**
 * Simple tokenizer placeholder
 * In production, this would use ONNX Runtime's built-in tokenizer or load tokenizer.json
 */
class SimpleTokenizer {
public:
    std::vector<int64_t> encode(const std::string& text, size_t max_length = 128) {
        // Placeholder: simple character code tokenization
        // Production would use WordPiece/BPE from tokenizer.json
        std::vector<int64_t> tokens;
        tokens.reserve(max_length);
        
        // Add CLS token (101 for BERT/RoBERTa)
        tokens.push_back(101);
        
        // Convert characters to token IDs (placeholder)
        for (size_t i = 0; i < std::min(text.length(), max_length - 2); ++i) {
            // Simple hash for demonstration
            tokens.push_back((text[i] % 30000) + 1000);
        }
        
        // Add SEP token (102)
        tokens.push_back(102);
        
        // Pad to max_length
        while (tokens.size() < max_length) {
            tokens.push_back(0);  // PAD token
        }
        
        return tokens;
    }
};

class OnnxInferenceNode : public INode {
private:
    std::string name_;
    std::string model_dir_;
    json config_;
    
    // Model configuration
    std::vector<std::string> labels_;
    std::string model_path_;
    size_t max_length_ = 128;
    float threshold_ = 0.5f;
    
    // Tokenizer
    SimpleTokenizer tokenizer_;
    
    // Placeholder for ONNX Runtime session
    // In production: Ort::Session session_;
    bool model_loaded_ = false;

public:
    OnnxInferenceNode() : name_("onnx_inference") {}
    
    bool initialize(const json& config) override {
        config_ = config;
        
        // Required: model directory
        if (!config.contains("model_dir")) {
            std::cerr << "[OnnxInference] Error: model_dir is required" << std::endl;
            std::cerr << "  Example: \"model_dir\": \"models/sentiment-twitter\"" << std::endl;
            return false;
        }
        
        model_dir_ = config["model_dir"];
        
        // Load model config
        std::string config_path = model_dir_ + "/config.json";
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            std::cerr << "[OnnxInference] Error: Cannot load config: " << config_path << std::endl;
            return false;
        }
        
        json model_config;
        try {
            config_file >> model_config;
        } catch (const std::exception& e) {
            std::cerr << "[OnnxInference] Error: Invalid JSON in " << config_path << std::endl;
            std::cerr << "  " << e.what() << std::endl;
            return false;
        }
        
        // Extract model info
        if (model_config.contains("model_info")) {
            auto& info = model_config["model_info"];
            std::cout << "[OnnxInference] Loading model: " 
                      << info.value("name", "unknown") << std::endl;
            std::cout << "  Version: " << info.value("version", "unknown") << std::endl;
        }
        
        // Extract labels
        if (model_config.contains("postprocessing") && 
            model_config["postprocessing"].contains("labels")) {
            labels_ = model_config["postprocessing"]["labels"].get<std::vector<std::string>>();
            std::cout << "  Labels: ";
            for (const auto& label : labels_) {
                std::cout << label << " ";
            }
            std::cout << std::endl;
        }
        
        // Extract preprocessing config
        if (model_config.contains("preprocessing")) {
            auto& preproc = model_config["preprocessing"];
            max_length_ = preproc.value("max_length", 128);
        }
        
        // Extract threshold
        if (model_config.contains("postprocessing")) {
            auto& postproc = model_config["postprocessing"];
            threshold_ = postproc.value("threshold", 0.5f);
        }
        
        // Verify model file exists
        model_path_ = model_dir_ + "/model.onnx";
        std::ifstream model_file(model_path_);
        if (!model_file.is_open()) {
            std::cerr << "[OnnxInference] Warning: model.onnx not found at " << model_path_ << std::endl;
            std::cerr << "  Running in placeholder mode (no actual inference)" << std::endl;
            // Continue anyway - this allows testing with placeholder files
        } else {
            model_file.close();
            std::cout << "  Model file: " << model_path_ << std::endl;
            
            // In production: Load ONNX Runtime session here
            // session_ = Ort::Session(env, model_path_.c_str(), session_options);
            model_loaded_ = true;
        }
        
        std::cout << "[OnnxInference] Initialized successfully" << std::endl;
        return true;
    }
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        result.success = true;
        
        // Extract messages from input
        std::vector<std::map<std::string, std::string>> messages;
        
        for (const auto& input : inputs) {
            // Try to extract chat messages from payload
            // Payload could be vector<Message> or single message
            // For now, simple placeholder logic
            
            std::map<std::string, std::string> msg;
            msg["text"] = "Sample message text";  // Would extract from NodeData
            msg["user"] = "user123";
            messages.push_back(msg);
        }
        
        // If no inputs, create a dummy message for testing
        if (messages.empty()) {
            std::map<std::string, std::string> dummy;
            dummy["text"] = "This is a test message";
            dummy["user"] = "test_user";
            messages.push_back(dummy);
        }
        
        // Process each message
        for (const auto& msg : messages) {
            std::string text = msg.at("text");
            std::string user = msg.at("user");
            
            // Tokenize (placeholder)
            auto tokens = tokenizer_.encode(text, max_length_);
            
            // Run inference (placeholder)
            // In production: session.Run(...)
            std::map<std::string, float> scores;
            
            if (labels_.empty()) {
                // Default labels if not configured
                labels_ = {"negative", "neutral", "positive"};
            }
            
            // Placeholder: Generate random scores for demonstration
            // Production: Run actual ONNX inference
            float sum = 0.0f;
            for (const auto& label : labels_) {
                float score = static_cast<float>(rand()) / RAND_MAX;
                scores[label] = score;
                sum += score;
            }
            
            // Normalize to softmax
            for (auto& [label, score] : scores) {
                score /= sum;
            }
            
            // Find dominant label
            std::string dominant_label;
            float max_score = 0.0f;
            for (const auto& [label, score] : scores) {
                if (score > max_score) {
                    max_score = score;
                    dominant_label = label;
                }
            }
            
            // Create result structure
            json inference_result = {
                {"text", text},
                {"user", user},
                {"scores", scores},
                {"dominant_label", dominant_label},
                {"confidence", max_score},
                {"model", model_dir_}
            };
            
            // Check if exceeds threshold
            bool triggered = max_score > threshold_;
            
            // Create NodeData output
            NodeData output_data;
            output_data.type = "inference_result";
            output_data.payload = inference_result;
            output_data.metadata = {
                {"triggered", triggered},
                {"confidence", max_score},
                {"label", dominant_label}
            };
            
            result.outputs.push_back(output_data);
        }
        
        return result;
    }
    
    void shutdown() override {
        std::cout << "[OnnxInference] Shutting down" << std::endl;
        // In production: session_.release();
    }
    
    std::string getType() const override { return "onnx_inference"; }
    std::string getName() const override { return name_; }
    std::string getDescription() const override { 
        return "Generic ONNX model inference for sentiment, classification, or custom tasks"; 
    }
    
    bool canRunParallel() const override { return true; }
};

// Export plugin interface
extern "C" {
    const char* get_plugin_version() {
        return "1.0.0";
    }
    
    const char** get_node_types() {
        static const char* types[] = {"onnx_inference", nullptr};
        return types;
    }
    
    INode* create_node(const char* type) {
        if (strcmp(type, "onnx_inference") == 0) {
            return new OnnxInferenceNode();
        }
        return nullptr;
    }
    
    void destroy_node(INode* node) {
        delete node;
    }
}
