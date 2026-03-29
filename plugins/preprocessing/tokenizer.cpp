// Tokenizer preprocessing node
#include "../../include/pipeline/node.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <cctype>

using namespace ctic::pipeline;

class TokenizerNode : public INode {
private:
    std::string method = "whitespace";  // whitespace, bert, regex
    size_t max_length = 512;
    bool lowercase = true;
    bool remove_punctuation = false;
    std::string pad_token = "[PAD]";
    std::string cls_token = "[CLS]";
    std::string sep_token = "[SEP]";
    
public:
    bool initialize(const nlohmann::json& config) override {
        if (config.contains("method")) {
            method = config["method"];
        }
        if (config.contains("max_length")) {
            max_length = config["max_length"];
        }
        if (config.contains("lowercase")) {
            lowercase = config["lowercase"];
        }
        if (config.contains("remove_punctuation")) {
            remove_punctuation = config["remove_punctuation"];
        }
        if (config.contains("pad_token")) {
            pad_token = config["pad_token"];
        }
        if (config.contains("cls_token")) {
            cls_token = config["cls_token"];
        }
        if (config.contains("sep_token")) {
            sep_token = config["sep_token"];
        }
        return true;
    }
    
    void shutdown() override {}
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        for (const auto& input : inputs) {
            if (input.type == "text" || input.type == "normalized_text") {
                try {
                    std::string text = std::any_cast<std::string>(input.payload);
                    
                    // Apply preprocessing
                    if (lowercase) {
                        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
                    }
                    
                    if (remove_punctuation) {
                        text = std::regex_replace(text, std::regex("[[:punct:]]"), " ");
                    }
                    
                    // Tokenize based on method
                    std::vector<std::string> tokens;
                    
                    if (method == "whitespace") {
                        std::istringstream iss(text);
                        std::string token;
                        while (iss >> token) {
                            tokens.push_back(token);
                        }
                    } else if (method == "bert") {
                        // BERT-style tokenization
                        tokens.push_back(cls_token);
                        
                        std::istringstream iss(text);
                        std::string token;
                        while (iss >> token && tokens.size() < max_length - 1) {
                            tokens.push_back(token);
                        }
                        
                        tokens.push_back(sep_token);
                        
                        // Pad to max_length
                        while (tokens.size() < max_length) {
                            tokens.push_back(pad_token);
                        }
                    }
                    
                    // Create output with tokens
                    NodeData output("tokens", tokens);
                    output.metadata = input.metadata;
                    output.metadata["tokenizer"] = method;
                    output.metadata["token_count"] = tokens.size();
                    output.metadata["original_text"] = text;
                    
                    // Preserve and update lineage
                    output.lineage = input.lineage;
                    output.markProcessed("tokenizer", 1.0f, 
                        "Tokenized with " + method + " method");
                    
                    result.outputs.push_back(output);
                    
                } catch (const std::bad_any_cast& e) {
                    return NodeResult::error("Input is not text type");
                }
            }
        }
        
        return result;
    }
    
    bool canRunParallel() const override { return true; }
    std::string getType() const override { return "tokenizer"; }
    std::string getName() const override { return "Text Tokenizer"; }
    std::string getDescription() const override { 
        return "Tokenizes text for model input"; 
    }
    
    nlohmann::json getConfigSchema() const override {
        return {
            {"method", {{"type", "string"}, {"default", "whitespace"}}},
            {"max_length", {{"type", "integer"}, {"default", 512}}},
            {"lowercase", {{"type", "boolean"}, {"default", true}}},
            {"remove_punctuation", {{"type", "boolean"}, {"default", false}}}
        };
    }
};

// Plugin entry points
extern "C" {
    const char* get_plugin_version() { return "1.0.0"; }
    
    const char** get_node_types() {
        static const char* types[] = {"tokenizer", nullptr};
        return types;
    }
    
    INode* create_node(const char* type) {
        if (std::string(type) == "tokenizer") {
            return new TokenizerNode();
        }
        return nullptr;
    }
    
    void destroy_node(INode* node) {
        delete node;
    }
}