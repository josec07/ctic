// Example plugin: Text Normalizer Node
#include "../../include/pipeline/node.h"
#include <algorithm>
#include <cctype>

using namespace ctic::pipeline;

// Text normalizer node implementation
class TextNormalizerNode : public INode {
private:
    bool lowercase = true;
    bool remove_punctuation = false;
    bool trim_spaces = true;
    
public:
    bool initialize(const nlohmann::json& config) override {
        if (config.contains("lowercase")) {
            lowercase = config["lowercase"];
        }
        if (config.contains("remove_punctuation")) {
            remove_punctuation = config["remove_punctuation"];
        }
        if (config.contains("trim_spaces")) {
            trim_spaces = config["trim_spaces"];
        }
        return true;
    }
    
    void shutdown() override {
        // Cleanup if needed
    }
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        for (const auto& input : inputs) {
            if (input.type == "text") {
                try {
                    std::string text = std::any_cast<std::string>(input.payload);
                    
                    // Apply transformations
                    if (lowercase) {
                        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
                    }
                    
                    if (remove_punctuation) {
                        text.erase(
                            std::remove_if(text.begin(), text.end(), 
                                [](char c) { return std::ispunct(c); }),
                            text.end()
                        );
                    }
                    
                    if (trim_spaces) {
                        // Trim leading spaces
                        text.erase(0, text.find_first_not_of(" \t\n\r"));
                        // Trim trailing spaces
                        text.erase(text.find_last_not_of(" \t\n\r") + 1);
                    }
                    
                    // Create output
                    NodeData output("normalized_text", text);
                    output.metadata = input.metadata;
                    output.metadata["normalized"] = true;
                    result.outputs.push_back(output);
                    
                } catch (const std::bad_any_cast& e) {
                    return NodeResult::error("Input is not text type");
                }
            }
        }
        
        return result;
    }
    
    bool canRunParallel() const override {
        return true;  // Text normalization is stateless
    }
    
    std::string getType() const override {
        return "text_normalizer";
    }
    
    std::string getName() const override {
        return "Text Normalizer";
    }
    
    std::string getDescription() const override {
        return "Normalizes text by applying lowercase, removing punctuation, etc.";
    }
    
    nlohmann::json getConfigSchema() const override {
        return {
            {"lowercase", {{"type", "boolean"}, {"default", true}}},
            {"remove_punctuation", {{"type", "boolean"}, {"default", false}}},
            {"trim_spaces", {{"type", "boolean"}, {"default", true}}}
        };
    }
};

// Plugin entry points
extern "C" {
    const char* get_plugin_version() {
        return "1.0.0";
    }
    
    const char** get_node_types() {
        static const char* types[] = {"text_normalizer", nullptr};
        return types;
    }
    
    INode* create_node(const char* type) {
        if (std::string(type) == "text_normalizer") {
            return new TextNormalizerNode();
        }
        return nullptr;
    }
    
    void destroy_node(INode* node) {
        delete node;
    }
}