// Comprehensive CSV output with full lineage tracking
#include "../../include/pipeline/node.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <filesystem>

using namespace ctic::pipeline;

class ComprehensiveCSVNode : public INode {
private:
    std::string output_path = "outputs/detections.csv";
    std::ofstream csv_file;
    bool include_lineage = true;
    bool include_all_scores = true;
    bool include_raw_text = true;
    bool include_metadata = true;
    size_t buffer_size = 100;
    size_t current_buffer = 0;
    bool header_written = false;
    
    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        
        // Add milliseconds
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()) % 1000;
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        
        return ss.str();
    }
    
    std::string escapeCSV(const std::string& field) {
        if (field.find_first_of(",\"\n\r") != std::string::npos) {
            std::string escaped = "\"";
            for (char c : field) {
                if (c == '"') escaped += "\"\"";
                else escaped += c;
            }
            escaped += "\"";
            return escaped;
        }
        return field;
    }
    
    void writeHeader() {
        if (!header_written) {
            csv_file << "timestamp,channel,raw_text,detection_type,node_path,";
            csv_file << "combined_confidence,trigger_reasons,";
            
            if (include_all_scores) {
                csv_file << "spike_score,sentiment_score,word_match_score,model_score,";
            }
            
            csv_file << "processing_ms,";
            
            if (include_metadata) {
                csv_file << "metadata";
            }
            
            csv_file << "\n";
            header_written = true;
        }
    }
    
public:
    bool initialize(const nlohmann::json& config) override {
        if (config.contains("path")) {
            output_path = config["path"];
        }
        if (config.contains("include_lineage")) {
            include_lineage = config["include_lineage"];
        }
        if (config.contains("include_all_scores")) {
            include_all_scores = config["include_all_scores"];
        }
        if (config.contains("include_raw_text")) {
            include_raw_text = config["include_raw_text"];
        }
        if (config.contains("include_metadata")) {
            include_metadata = config["include_metadata"];
        }
        if (config.contains("buffer_size")) {
            buffer_size = config["buffer_size"];
        }
        
        // Create output directory if it doesn't exist
        std::filesystem::path p(output_path);
        std::filesystem::create_directories(p.parent_path());
        
        // Open CSV file
        csv_file.open(output_path, std::ios::out | std::ios::app);
        if (!csv_file.is_open()) {
            std::cerr << "Failed to open CSV file: " << output_path << std::endl;
            return false;
        }
        
        // Check if file is empty (new file) to write header
        csv_file.seekp(0, std::ios::end);
        if (csv_file.tellp() == 0) {
            writeHeader();
        }
        
        return true;
    }
    
    void shutdown() override {
        if (csv_file.is_open()) {
            csv_file.flush();
            csv_file.close();
        }
    }
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        for (const auto& input : inputs) {
            // Only write if there's an actual detection
            bool has_detection = false;
            if (!input.lineage.triggers.empty() || 
                input.lineage.combined_confidence > 0.5 ||
                input.metadata.contains("is_detection")) {
                has_detection = true;
            }
            
            if (has_detection) {
                // Extract text payload
                std::string raw_text;
                try {
                    if (input.type == "text" || input.type == "normalized_text") {
                        raw_text = std::any_cast<std::string>(input.payload);
                    }
                } catch (const std::bad_any_cast& e) {
                    raw_text = "[non-text data]";
                }
                
                // Extract channel from metadata
                std::string channel = input.metadata.value("channel", "unknown");
                
                // Build trigger reasons string
                std::string trigger_reasons;
                for (const auto& [node_id, reason] : input.lineage.node_reasons) {
                    if (!trigger_reasons.empty()) trigger_reasons += "; ";
                    trigger_reasons += reason;
                }
                
                // Write CSV row
                csv_file << formatTimestamp(input.created_at) << ",";
                csv_file << escapeCSV(channel) << ",";
                
                if (include_raw_text) {
                    csv_file << escapeCSV(raw_text) << ",";
                } else {
                    csv_file << ",";
                }
                
                csv_file << escapeCSV(input.lineage.detection_type) << ",";
                csv_file << escapeCSV(input.lineage.getPathString()) << ",";
                csv_file << std::fixed << std::setprecision(3) 
                        << input.lineage.combined_confidence << ",";
                csv_file << escapeCSV(trigger_reasons) << ",";
                
                if (include_all_scores) {
                    // Output individual scores
                    auto scores = input.lineage.node_scores;
                    csv_file << scores["spike_detector"] << ",";
                    csv_file << scores["sentiment_analyzer"] << ",";
                    csv_file << scores["word_matcher"] << ",";
                    csv_file << scores["model_node"] << ",";
                }
                
                csv_file << input.lineage.total_processing_time.count() << ",";
                
                if (include_metadata) {
                    csv_file << escapeCSV(input.metadata.dump()) << ",";
                }
                
                csv_file << "\n";
                
                // Flush periodically
                current_buffer++;
                if (current_buffer >= buffer_size) {
                    csv_file.flush();
                    current_buffer = 0;
                }
            }
            
            // Pass through the data
            result.outputs.push_back(input);
        }
        
        return result;
    }
    
    bool canRunParallel() const override { 
        return false;  // File writing should be sequential
    }
    
    std::string getType() const override { return "comprehensive_csv"; }
    std::string getName() const override { return "Comprehensive CSV Writer"; }
    std::string getDescription() const override { 
        return "Writes detection data to CSV with full lineage tracking"; 
    }
    
    nlohmann::json getConfigSchema() const override {
        return {
            {"path", {{"type", "string"}, {"default", "outputs/detections.csv"}}},
            {"include_lineage", {{"type", "boolean"}, {"default", true}}},
            {"include_all_scores", {{"type", "boolean"}, {"default", true}}},
            {"include_raw_text", {{"type", "boolean"}, {"default", true}}},
            {"include_metadata", {{"type", "boolean"}, {"default", true}}},
            {"buffer_size", {{"type", "integer"}, {"default", 100}}}
        };
    }
};

// Plugin entry points
extern "C" {
    const char* get_plugin_version() { return "1.0.0"; }
    
    const char** get_node_types() {
        static const char* types[] = {"comprehensive_csv", nullptr};
        return types;
    }
    
    INode* create_node(const char* type) {
        if (std::string(type) == "comprehensive_csv") {
            return new ComprehensiveCSVNode();
        }
        return nullptr;
    }
    
    void destroy_node(INode* node) {
        delete node;
    }
}