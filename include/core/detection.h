#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace ctic {
namespace core {

// Stub detection types
enum class DetectionTier {
    HIGH,
    MEDIUM,
    LOW
};

struct DetectionResult {
    bool detected;
    DetectionTier tier;
    std::string reason;
    double score;
    std::chrono::system_clock::time_point timestamp;
};

// Stub detection function
inline DetectionResult detect_spike(const std::vector<std::string>& messages) {
    return {false, DetectionTier::LOW, "", 0.0, std::chrono::system_clock::now()};
}

}
}
