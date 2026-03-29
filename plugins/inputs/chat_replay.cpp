/**
 * Chat Replay Input Node
 * Reads VOD chat data from JSON files and emits messages at configurable rates
 */

#include "../../include/pipeline/node.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace ctic::pipeline;
using json = nlohmann::json;

class ChatReplayNode : public INode {
private:
    std::string name_;
    json config_;
    std::vector<json> messages_;
    size_t current_index_ = 0;
    
    // Configuration
    std::string file_path_;
    int batch_size_ = 10;  // Number of messages to emit per process call
    double time_scale_ = 1.0;  // Speed multiplier for replay
    bool loop_ = false;  // Whether to loop when reaching end
    
    // Timing
    std::chrono::system_clock::time_point start_time_;
    double last_timestamp_ = 0.0;
    
public:
    ChatReplayNode() : name_("chat_replay") {}
    
    bool initialize(const json& config) override {
        config_ = config;
        
        // Load configuration
        if (!config.contains("file_path")) {
            std::cerr << "[ChatReplay] Error: file_path is required" << std::endl;
            return false;
        }
        file_path_ = config["file_path"];
        
        if (config.contains("batch_size")) {
            batch_size_ = config["batch_size"];
        }
        
        if (config.contains("time_scale")) {
            time_scale_ = config["time_scale"];
        }
        
        if (config.contains("loop")) {
            loop_ = config["loop"];
        }
        
        // Load messages from file
        std::ifstream file(file_path_);
        if (!file.is_open()) {
            std::cerr << "[ChatReplay] Error: Cannot open file: " << file_path_ << std::endl;
            return false;
        }
        
        json data;
        try {
            file >> data;
        } catch (const json::exception& e) {
            std::cerr << "[ChatReplay] Error parsing JSON: " << e.what() << std::endl;
            return false;
        }
        
        // Extract messages array
        if (data.contains("messages") && data["messages"].is_array()) {
            messages_ = data["messages"].get<std::vector<json>>();
        } else if (data.is_array()) {
            messages_ = data.get<std::vector<json>>();
        } else {
            std::cerr << "[ChatReplay] Error: Invalid JSON format. Expected array or object with 'messages' array" << std::endl;
            return false;
        }
        
        std::cout << "[ChatReplay] Loaded " << messages_.size() << " messages from " << file_path_ << std::endl;
        
        // Initialize timing
        start_time_ = std::chrono::system_clock::now();
        if (!messages_.empty() && messages_[0].contains("timestamp")) {
            last_timestamp_ = messages_[0]["timestamp"];
        }
        
        return true;
    }
    
    void shutdown() override {
        messages_.clear();
    }
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        // Track processing start
        auto proc_start = std::chrono::system_clock::now();
        
        // Collect batch of messages
        json messages_batch = json::array();
        int count = 0;
        
        while (count < batch_size_ && current_index_ < messages_.size()) {
            const auto& msg = messages_[current_index_];
            
            // Add message to batch
            json enriched_msg = msg;
            
            // Add index and replay metadata
            enriched_msg["_index"] = current_index_;
            enriched_msg["_replay_time"] = std::chrono::duration<double>(
                std::chrono::system_clock::now() - start_time_
            ).count();
            
            messages_batch.push_back(enriched_msg);
            
            current_index_++;
            count++;
        }
        
        // Handle looping
        bool end_of_stream = false;
        if (current_index_ >= messages_.size()) {
            if (loop_) {
                current_index_ = 0;
                start_time_ = std::chrono::system_clock::now();
                std::cout << "[ChatReplay] Looping back to start" << std::endl;
            } else {
                end_of_stream = true;
            }
        }
        
        // Create output if we have messages
        if (!messages_batch.empty()) {
            // Create metadata
            json metadata;
            metadata["source"] = "chat_replay";
            metadata["file"] = file_path_;
            metadata["batch_size"] = count;
            metadata["current_index"] = current_index_;
            metadata["total_messages"] = messages_.size();
            metadata["progress"] = (double)current_index_ / messages_.size();
            if (end_of_stream) {
                metadata["end_of_stream"] = true;
            }
            
            // Create output data
            NodeData output("chat_messages", messages_batch, metadata);
            
            // Add processing info to lineage
            auto proc_end = std::chrono::system_clock::now();
            auto processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                proc_end - proc_start
            );
            
            std::stringstream reason;
            reason << "Replayed " << count << " messages from " << file_path_ 
                   << " (progress: " << std::fixed << std::setprecision(1) 
                   << (metadata["progress"].get<double>() * 100) << "%)";
            
            output.markProcessed(name_, 0.0f, reason.str());
            output.lineage.total_processing_time = processing_time;
            
            result.outputs.push_back(output);
        } else if (end_of_stream) {
            // Create end-of-stream marker
            json metadata;
            metadata["end_of_stream"] = true;
            metadata["total_processed"] = messages_.size();
            
            NodeData output("end_of_stream", nullptr, metadata);
            output.markProcessed(name_, 0.0f, "End of chat replay");
            result.outputs.push_back(output);
        }
        
        return result;
    }
    
    bool canRunParallel() const override { return false; }
    std::string getType() const override { return "input"; }
    std::string getName() const override { return name_; }
    std::string getDescription() const override { 
        return "Replays chat messages from VOD JSON files";
    }
    
    json getConfigSchema() const override {
        return {
            {"type", "object"},
            {"required", {"file_path"}},
            {"properties", {
                {"file_path", {
                    {"type", "string"},
                    {"description", "Path to VOD chat JSON file"}
                }},
                {"batch_size", {
                    {"type", "integer"},
                    {"default", 10},
                    {"description", "Number of messages to emit per process call"}
                }},
                {"time_scale", {
                    {"type", "number"},
                    {"default", 1.0},
                    {"description", "Speed multiplier for replay"}
                }},
                {"loop", {
                    {"type", "boolean"},
                    {"default", false},
                    {"description", "Loop back to start when reaching end"}
                }}
            }}
        };
    }
};

// Plugin entry points
extern "C" {
    const char* get_plugin_version() {
        return "1.0.0";
    }
    
    const char** get_node_types() {
        static const char* types[] = {"chat_replay", nullptr};
        return types;
    }
    
    INode* create_node(const char* type) {
        if (std::string(type) == "chat_replay") {
            return new ChatReplayNode();
        }
        return nullptr;
    }
    
    void destroy_node(INode* node) {
        delete node;
    }
}