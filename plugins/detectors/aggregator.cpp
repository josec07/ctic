// Multi-signal aggregator node - combines multiple detection signals
#include "../../include/pipeline/node.h"
#include <algorithm>
#include <numeric>

using namespace ctic::pipeline;

class AggregatorNode : public INode {
private:
    std::string strategy = "weighted";  // weighted, voting, any, all
    std::map<std::string, float> weights;
    float threshold = 0.6f;
    int min_signals = 1;
    bool generate_reason = true;
    
public:
    bool initialize(const nlohmann::json& config) override {
        if (config.contains("strategy")) {
            strategy = config["strategy"];
        }
        if (config.contains("weights")) {
            for (auto& [key, value] : config["weights"].items()) {
                weights[key] = value;
            }
        }
        if (config.contains("threshold")) {
            threshold = config["threshold"];
        }
        if (config.contains("min_signals")) {
            min_signals = config["min_signals"];
        }
        if (config.contains("generate_reason")) {
            generate_reason = config["generate_reason"];
        }
        return true;
    }
    
    void shutdown() override {}
    
    NodeResult process(const std::vector<NodeData>& inputs) override {
        NodeResult result;
        
        if (inputs.empty()) {
            return result;
        }
        
        // Aggregate all signals
        NodeData aggregated = inputs[0];
        aggregated.lineage.path.clear();
        aggregated.lineage.triggers.clear();
        aggregated.lineage.detection_type = "";
        
        std::vector<std::string> detection_types;
        std::vector<std::string> all_triggers;
        std::map<std::string, float> all_scores;
        std::vector<std::string> all_reasons;
        
        // Collect all signals
        for (const auto& input : inputs) {
            // Merge paths
            for (const auto& node : input.lineage.path) {
                if (std::find(aggregated.lineage.path.begin(), 
                             aggregated.lineage.path.end(), node) == 
                    aggregated.lineage.path.end()) {
                    aggregated.lineage.path.push_back(node);
                }
            }
            
            // Collect scores
            for (const auto& [node_id, score] : input.lineage.node_scores) {
                all_scores[node_id] = score;
            }
            
            // Collect triggers
            for (const auto& trigger : input.lineage.triggers) {
                all_triggers.push_back(trigger);
            }
            
            // Collect detection types
            if (!input.lineage.detection_type.empty()) {
                detection_types.push_back(input.lineage.detection_type);
            }
            
            // Collect reasons
            for (const auto& [node_id, reason] : input.lineage.node_reasons) {
                if (!reason.empty()) {
                    all_reasons.push_back(reason);
                }
            }
            
            // Merge metadata
            for (auto& [key, value] : input.metadata.items()) {
                aggregated.metadata[key] = value;
            }
        }
        
        // Apply aggregation strategy
        float combined_score = 0.0f;
        int signal_count = 0;
        
        if (strategy == "weighted") {
            // Weighted average
            float total_weight = 0.0f;
            for (const auto& [node_id, score] : all_scores) {
                float weight = weights.count(node_id) ? weights[node_id] : 1.0f;
                combined_score += score * weight;
                total_weight += weight;
                if (score > 0.0f) signal_count++;
            }
            if (total_weight > 0) {
                combined_score /= total_weight;
            }
            
        } else if (strategy == "voting") {
            // Voting - percentage of positive signals
            for (const auto& [node_id, score] : all_scores) {
                if (score > 0.5f) {
                    signal_count++;
                }
            }
            combined_score = all_scores.empty() ? 0.0f : 
                            static_cast<float>(signal_count) / all_scores.size();
            
        } else if (strategy == "any") {
            // Any signal triggers
            for (const auto& [node_id, score] : all_scores) {
                if (score > 0.0f) {
                    combined_score = std::max(combined_score, score);
                    signal_count++;
                }
            }
            
        } else if (strategy == "all") {
            // All signals required
            bool all_positive = !all_scores.empty();
            for (const auto& [node_id, score] : all_scores) {
                if (score <= 0.0f) {
                    all_positive = false;
                    break;
                }
                signal_count++;
            }
            combined_score = all_positive ? 1.0f : 0.0f;
        }
        
        // Check if detection criteria met
        bool is_detection = (combined_score >= threshold) && (signal_count >= min_signals);
        
        // Update aggregated lineage
        aggregated.lineage.combined_confidence = combined_score;
        aggregated.lineage.triggers = all_triggers;
        aggregated.lineage.node_scores = all_scores;
        
        // Generate detection type string
        if (!detection_types.empty()) {
            std::sort(detection_types.begin(), detection_types.end());
            detection_types.erase(std::unique(detection_types.begin(), 
                                             detection_types.end()), 
                                 detection_types.end());
            
            aggregated.lineage.detection_type = "";
            for (size_t i = 0; i < detection_types.size(); i++) {
                if (i > 0) aggregated.lineage.detection_type += "+";
                aggregated.lineage.detection_type += detection_types[i];
            }
        }
        
        // Generate reason if requested
        if (generate_reason && is_detection) {
            std::string reason = "Aggregated detection (" + strategy + "): ";
            reason += std::to_string(signal_count) + " signals, ";
            reason += "confidence: " + std::to_string(combined_score);
            if (!all_reasons.empty()) {
                reason += " [";
                for (size_t i = 0; i < std::min(all_reasons.size(), size_t(3)); i++) {
                    if (i > 0) reason += "; ";
                    reason += all_reasons[i];
                }
                if (all_reasons.size() > 3) reason += "...";
                reason += "]";
            }
            aggregated.lineage.node_reasons["aggregator"] = reason;
        }
        
        // Mark as processed
        aggregated.markProcessed("aggregator", combined_score, 
            is_detection ? "Detection confirmed" : "Below threshold");
        
        // Set detection flag
        aggregated.metadata["is_detection"] = is_detection;
        aggregated.metadata["aggregation"] = {
            {"strategy", strategy},
            {"combined_score", combined_score},
            {"signal_count", signal_count},
            {"threshold", threshold}
        };
        
        result.outputs.push_back(aggregated);
        return result;
    }
    
    bool canRunParallel() const override { 
        return false;  // Needs to aggregate multiple inputs
    }
    
    std::string getType() const override { return "aggregator"; }
    std::string getName() const override { return "Signal Aggregator"; }
    std::string getDescription() const override { 
        return "Aggregates multiple detection signals into a combined decision"; 
    }
    
    nlohmann::json getConfigSchema() const override {
        return {
            {"strategy", {{"type", "string"}, {"default", "weighted"}}},
            {"weights", {{"type", "object"}, {"default", {}}}},
            {"threshold", {{"type", "number"}, {"default", 0.6}}},
            {"min_signals", {{"type", "integer"}, {"default", 1}}},
            {"generate_reason", {{"type", "boolean"}, {"default", true}}}
        };
    }
};

// Plugin entry points
extern "C" {
    const char* get_plugin_version() { return "1.0.0"; }
    
    const char** get_node_types() {
        static const char* types[] = {"aggregator", nullptr};
        return types;
    }
    
    INode* create_node(const char* type) {
        if (std::string(type) == "aggregator") {
            return new AggregatorNode();
        }
        return nullptr;
    }
    
    void destroy_node(INode* node) {
        delete node;
    }
}