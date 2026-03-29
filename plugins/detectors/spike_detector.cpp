// Spike detector node - detects abnormal activity spikes
#include "../../include/pipeline/node.h"
#include <deque>
#include <cmath>
#include <numeric>

using namespace ctic::pipeline;

class SpikeDetectorNode : public INode {
private:
    struct Sample {
        std::chrono::system_clock::time_point timestamp;
        double value;
    };
    
    std::deque<Sample> window;
    size_t window_size = 60;  // seconds
    double threshold = 3.0;    // standard deviations
    std::chrono::seconds window_duration{60};
    
    // Running statistics (Welford's algorithm)
    double mean = 0.0;
    double M2 = 0.0;
    size_t count = 0;
    
public:
    bool initialize(const nlohmann::json& config) override {
        if (config.contains("window_size")) {
            window_size = config["window_size"];
            window_duration = std::chrono::seconds(window_size);
        }
        if (config.contains("threshold")) {
            threshold = config["threshold"];
        }
        return true;
    }
    
    void shutdown() override {
        window.clear();
    }
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        auto now = std::chrono::system_clock::now();
        
        // Count messages in this batch
        double current_rate = static_cast<double>(inputs.size());
        
        // Add to window
        window.push_back({now, current_rate});
        
        // Remove old samples
        auto cutoff = now - window_duration;
        while (!window.empty() && window.front().timestamp < cutoff) {
            window.pop_front();
        }
        
        // Update running statistics
        count++;
        double delta = current_rate - mean;
        mean += delta / count;
        double delta2 = current_rate - mean;
        M2 += delta * delta2;
        
        // Calculate standard deviation
        double variance = (count > 1) ? M2 / (count - 1) : 0.0;
        double stddev = std::sqrt(variance);
        
        // Calculate z-score
        double z_score = (stddev > 0) ? (current_rate - mean) / stddev : 0.0;
        
        // Check for spike
        bool is_spike = std::abs(z_score) > threshold;
        
        // Process each input
        for (const auto& input : inputs) {
            NodeData output = input.cloneWithPayload(input.payload);
            
            // Add spike detection metadata
            output.metadata["spike_detection"] = {
                {"is_spike", is_spike},
                {"z_score", z_score},
                {"current_rate", current_rate},
                {"mean_rate", mean},
                {"stddev", stddev},
                {"window_size", window.size()}
            };
            
            // Update lineage
            float confidence = is_spike ? std::min(z_score / threshold, 1.0) : 0.0;
            std::string reason = is_spike ? 
                "Spike detected (z-score: " + std::to_string(z_score) + ")" : 
                "Normal activity";
            
            output.markProcessed("spike_detector", confidence, reason);
            
            if (is_spike) {
                output.lineage.triggers.push_back("spike");
                output.lineage.detection_type = "spike";
            }
            
            result.outputs.push_back(output);
        }
        
        return result;
    }
    
    bool canRunParallel() const override { 
        return false;  // Needs sequential processing for statistics
    }
    
    std::string getType() const override { return "spike_detector"; }
    std::string getName() const override { return "Spike Detector"; }
    std::string getDescription() const override { 
        return "Detects abnormal activity spikes using statistical analysis"; 
    }
    
    nlohmann::json getConfigSchema() const override {
        return {
            {"window_size", {{"type", "integer"}, {"default", 60}}},
            {"threshold", {{"type", "number"}, {"default", 3.0}}}
        };
    }
};

// Plugin entry points
extern "C" {
    const char* get_plugin_version() { return "1.0.0"; }
    
    const char** get_node_types() {
        static const char* types[] = {"spike_detector", nullptr};
        return types;
    }
    
    INode* create_node(const char* type) {
        if (std::string(type) == "spike_detector") {
            return new SpikeDetectorNode();
        }
        return nullptr;
    }
    
    void destroy_node(INode* node) {
        delete node;
    }
}