# CTIC Pipeline Engine v3.0

**The Modular Clip Detection Engine for Live Streamers**

CTIC is a professional-grade, plugin-based pipeline engine designed for media teams to detect and track important moments in live streams through chat analysis.

## 🎯 Core Philosophy

Everything is a module. Every module is configurable. Every decision is traceable.

## 🚀 Quick Start

```bash
# Build everything
make all

# Run with template
./ctic pipeline run --template simple_spike --channel shroud

# Run with custom config
./ctic pipeline run --config my_pipeline.json

# List available plugins
./ctic plugins list
```

## 📊 Key Features

### Modular Architecture
- **Plugin System**: All processing nodes are plugins (.so files)
- **Hot-swappable**: Add new algorithms without recompiling
- **Composable**: Chain nodes together in any configuration

### Comprehensive Data Lineage
- **Full Attribution**: Know exactly why each clip was detected
- **Score Tracking**: Individual scores from each detector preserved
- **Path Visualization**: See complete data flow through pipeline
- **Decision Transparency**: Human-readable reasons for each detection

### Template System
- **Pre-built Templates**: Start immediately with proven configurations
- **Variable Substitution**: Customize templates without editing JSON
- **Share Configurations**: Export and share successful pipelines

## 🔧 Pipeline Configuration

### Simple Example
```json
{
  "pipeline": {
    "nodes": [
      {"id": "input", "type": "twitch_irc"},
      {"id": "spike", "type": "spike_detector"},
      {"id": "output", "type": "comprehensive_csv"}
    ]
  }
}
```

### Multi-Signal Detection
```json
{
  "pipeline": {
    "nodes": [
      {"id": "input", "type": "twitch_irc"},
      {"id": "tokenizer", "type": "tokenizer", "inputs": ["input"]},
      {"id": "spike", "type": "spike_detector", "inputs": ["input"], "parallel": true},
      {"id": "sentiment", "type": "onnx_model", "inputs": ["tokenizer"], "parallel": true},
      {"id": "aggregator", "type": "aggregator", "inputs": ["spike", "sentiment"]},
      {"id": "output", "type": "comprehensive_csv", "inputs": ["aggregator"]}
    ]
  }
}
```

## 📦 Available Nodes

### Preprocessing
- `tokenizer` - Text tokenization for ML models
- `text_normalizer` - Text cleaning and normalization

### Detectors
- `spike_detector` - Statistical anomaly detection
- `pattern_matcher` - Word/phrase matching
- `aggregator` - Multi-signal combination

### Outputs
- `comprehensive_csv` - Full data export with lineage

### Models (Coming Soon)
- `onnx_model` - ONNX model inference
- Support for sentiment, toxicity, emotion models

## 📈 CSV Output

Every detection includes:
- Timestamp (millisecond precision)
- Channel source
- Raw text
- Detection type(s)
- Complete node path
- Individual node scores
- Combined confidence
- Trigger reasons
- Processing time
- Full metadata

## 🛠️ Creating Plugins

```cpp
// my_detector.cpp
#include "pipeline/node.h"

class MyDetector : public INode {
    NodeResult process(const std::vector<NodeData>& inputs) override {
        // Your detection logic
        output.lineage.addNode("my_detector", score, reason);
        return result;
    }
};

// Export plugin
extern "C" {
    INode* create_node(const char* type) {
        return new MyDetector();
    }
}
```

Compile: `g++ -shared -fPIC my_detector.cpp -o my_detector.so`

## 🎮 Use Cases

- **Highlight Detection**: Find clip-worthy moments automatically
- **Sentiment Tracking**: Monitor chat mood changes
- **Toxicity Filtering**: Identify problematic content
- **Engagement Analysis**: Track viewer excitement levels
- **Custom Alerts**: Trigger on specific patterns

## 🔄 Model Integration

1. Download ONNX model
2. Place in `models/` directory
3. Configure in pipeline:

```json
{
  "type": "onnx_model",
  "config": {
    "model_path": "models/sentiment.onnx",
    "labels": ["negative", "neutral", "positive"]
  }
}
```

## 📋 Templates

### Available Templates
- `simple_spike` - Basic activity spike detection
- `multi_signal` - Combined spike + pattern detection

### Using Templates
```bash
./ctic pipeline run --template multi_signal \
  --channel xqc \
  --threshold 2.5 \
  --output_path outputs/xqc_clips.csv
```

## 🤝 Contributing

Share your:
- Pipeline configurations
- Custom plugins  
- ONNX model configs
- Detection algorithms

## 📄 License

MIT - Use freely in your media production workflows

## 🎯 Philosophy

CTIC believes in:
- **Modularity**: Every component should be replaceable
- **Transparency**: Every decision should be traceable
- **Simplicity**: Complex pipelines from simple nodes
- **Performance**: C++ for speed, plugins for flexibility

---

**The modular engine is the product.** Everything else is configuration.