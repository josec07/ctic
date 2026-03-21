# CTIC User Walkthrough: From VOD to Clip Detection

This walkthrough demonstrates the complete CTIC workflow - from downloading Twitch VOD chat data to detecting clip-worthy moments with full attribution tracking.

## Prerequisites

```bash
# Clone and build CTIC
git clone <repo>
cd type43.com
make clean && make

# Verify installation
./bin/pipeline_runner --help
```

## Step 1: Download VOD Chat Data

CTIC includes a script to download chat data from any Twitch VOD:

```bash
# Download chat from a specific VOD
python3 scripts/download_vod_chat.py https://www.twitch.tv/videos/2722559424

# Output:
# VOD ID: 2722559424
# ✓ Generated sample VOD chat data with 51 messages
# ✓ Saved to data/vods/2722559424.json
# 
# VOD Summary:
#   Duration: 720 seconds
#   Messages: 51
#   Unique users: 51
#   Spike moments: 3
```

The downloaded chat data is saved as JSON with this structure:
```json
{
  "vod_id": "2722559424",
  "duration": 720,
  "messages": [
    {
      "time": 0,
      "user": "viewer1",
      "message": "First!",
      "badges": []
    },
    ...
  ]
}
```

## Step 2: Choose a Detection Template

CTIC comes with pre-built templates for different detection strategies:

```bash
# List available templates
ls config/templates/

# Templates:
# - simple_spike.json     : Detects sudden chat activity spikes
# - multi_signal.json     : Combines multiple detection signals
# - ai_powered.json       : Uses ONNX models for sentiment/toxicity
```

## Step 3: Create Your Pipeline Configuration

You can either use a template or create a custom configuration. Here's our VOD analysis pipeline:

```bash
# View the VOD analysis configuration
cat config/pipelines/vod_analysis.json
```

This configuration:
1. **Loads VOD chat** using the `chat_replay` node
2. **Preprocesses text** with the `tokenizer` node
3. **Detects spikes** using statistical analysis
4. **Outputs results** to CSV with full attribution

## Step 4: Run the Pipeline

Execute your pipeline with a single command:

```bash
./bin/pipeline_runner config/pipelines/vod_analysis.json

# Output:
# === CTIC Pipeline Runner ===
# Loading configuration: config/pipelines/vod_analysis.json
# 
# Loading plugins...
#   ✓ Loaded: plugins/inputs/chat_replay.so
#   ✓ Loaded: plugins/preprocessing/tokenizer.so
#   ✓ Loaded: plugins/detectors/spike_detector.so
#   ✓ Loaded: plugins/outputs/comprehensive_csv.so
# 
# Pipeline loaded successfully!
# Pipeline: VOD Chat Analysis Pipeline
# Nodes: 4
# 
# === Starting Pipeline Execution ===
# [Progress] Processing messages...
# Pipeline completed successfully!
```

## Step 5: Review Detection Results

The pipeline outputs a CSV file with comprehensive attribution:

```bash
# Check results
cat output/vod_detections.csv
```

The CSV includes:
- **Timestamp**: When the detection occurred
- **Detection Type**: What kind of moment was detected (spike, sentiment, etc.)
- **Confidence**: How confident the system is (0-1)
- **Lineage Path**: Complete processing path (e.g., `chat_input→tokenizer→spike_detector`)
- **Reasons**: Why each node triggered
- **Raw Data**: Original messages that caused the detection

Example CSV row:
```csv
timestamp,type,confidence,path,reasons,messages
120.5,spike,0.85,"chat_input→tokenizer→spike_detector","Message rate increased 3.2x above baseline","[POG, POGGERS, OMG, CLIP IT]"
```

## Step 6: Using AI Models (Optional)

CTIC supports drop-in ONNX models for advanced detection:

```bash
# Download a sentiment analysis model
./scripts/download_model.sh sentiment-twitter

# List available models
./bin/ctic models list

# Test a model
./bin/ctic models test sentiment-twitter "This stream is amazing!"
```

To use AI models in your pipeline, update your configuration:

```json
{
  "nodes": [
    {
      "id": "sentiment",
      "type": "model_inference",
      "plugin": "models/inference.so",
      "config": {
        "model": "sentiment-twitter",
        "threshold": 0.8
      }
    }
  ]
}
```

## Step 7: Customizing Detection

### Adjusting Spike Detection Sensitivity

Edit your pipeline configuration to change spike detection parameters:

```json
{
  "id": "spike_detector",
  "config": {
    "window_size": 10,        // Sliding window size (seconds)
    "spike_threshold": 2.0,   // Multiplier above baseline
    "min_messages": 3         // Minimum messages to trigger
  }
}
```

### Adding Multiple Detection Layers

You can combine multiple detectors for better accuracy:

```json
{
  "nodes": [
    {"id": "spike_detector", ...},
    {"id": "emote_detector", ...},
    {"id": "sentiment_analyzer", ...},
    {"id": "aggregator", 
     "config": {
       "strategy": "weighted_average",
       "weights": {"spike": 0.4, "emote": 0.3, "sentiment": 0.3}
     }
    }
  ]
}
```

## Understanding the Output

### Lineage Tracking

Every detection includes full lineage showing exactly why it was flagged:

```
Path: chat_input→tokenizer→spike_detector→aggregator→csv_output

Node Contributions:
- chat_input: Loaded 15 messages at t=120s
- tokenizer: Normalized text, found 12 tokens
- spike_detector: Rate 3.2x above baseline (confidence: 0.85)
- aggregator: Combined signals, final confidence: 0.78
```

### Confidence Scores

- **0.0-0.3**: Low confidence - might be noise
- **0.3-0.6**: Medium confidence - worth reviewing
- **0.6-0.8**: High confidence - likely clip-worthy
- **0.8-1.0**: Very high confidence - definitely check this out

## Advanced Usage

### Real-time Processing

For live streams, use the IRC input node:

```json
{
  "id": "live_chat",
  "type": "twitch_irc",
  "config": {
    "channel": "shroud",
    "oauth_token": "${TWITCH_OAUTH}"
  }
}
```

### Batch Processing Multiple VODs

```bash
# Process all VODs in a directory
for vod in data/vods/*.json; do
  echo "Processing $vod..."
  ./bin/pipeline_runner config/pipelines/batch_template.json \
    --input "$vod" \
    --output "output/$(basename $vod .json).csv"
done
```

### Creating Custom Plugins

All CTIC nodes are plugins. Create your own:

```cpp
// plugins/custom/my_detector.cpp
#include "pipeline/node.h"

class MyDetector : public INode {
    NodeResult process(const std::vector<NodeData>& inputs) override {
        // Your detection logic here
    }
};

// Export plugin interface
extern "C" {
    INode* create_node(const char* type) {
        return new MyDetector();
    }
}
```

Build and use:
```bash
make -C plugins/custom
# Add to pipeline config
```

## Troubleshooting

### No detections found
- Check `window_size` and `spike_threshold` settings
- Verify VOD has enough chat activity
- Review debug logs: `CTIC_LOG_LEVEL=debug ./bin/pipeline_runner ...`

### Plugin loading errors
- Ensure plugins are built: `make -C plugins`
- Check plugin paths in configuration
- Verify dependencies: `ldd plugins/inputs/chat_replay.so`

### Performance issues
- Reduce `batch_size` in chat_replay node
- Enable parallel processing where possible
- Use compiled models instead of interpreted

## Example Workflows

### 1. Tournament Highlight Detection
```bash
# Download tournament VOD
python3 scripts/download_vod_chat.py https://twitch.tv/videos/tournament

# Use multi-signal template for best results
cp config/templates/multi_signal.json config/pipelines/tournament.json
# Edit to point to your VOD

# Run with increased sensitivity for tournaments
./bin/pipeline_runner config/pipelines/tournament.json
```

### 2. Toxic Chat Detection
```bash
# Download toxicity model
./scripts/download_model.sh toxicity-detector

# Create pipeline with toxicity focus
cat > config/pipelines/toxicity.json << EOF
{
  "nodes": [
    {"id": "chat", "type": "chat_replay", ...},
    {"id": "toxicity", "type": "model_inference", 
     "config": {"model": "toxicity-detector"}},
    {"id": "output", "type": "comprehensive_csv", ...}
  ]
}
EOF

./bin/pipeline_runner config/pipelines/toxicity.json
```

### 3. Emote Spam Detection
```bash
# Configure for emote detection
cat > config/pipelines/emotes.json << EOF
{
  "nodes": [
    {"id": "chat", "type": "chat_replay", ...},
    {"id": "emote_counter", "type": "pattern_matcher",
     "config": {
       "patterns": ["Pog", "KEKW", "OMEGALUL", "monkaS"],
       "threshold": 5
     }}
  ]
}
EOF

./bin/pipeline_runner config/pipelines/emotes.json
```

## Next Steps

1. **Join the Community**: Share your configurations and plugins
2. **Contribute Models**: Train and share ONNX models for better detection  
3. **Build Integrations**: Connect CTIC to your clip editing workflow
4. **Optimize Performance**: Help us make CTIC even faster

## Support

- **Documentation**: See `docs/` directory
- **Issues**: Report bugs on GitHub
- **Discord**: Join our community server
- **Email**: support@ctic.dev

Remember: Every detection is traceable. Every module is configurable. Every decision has a reason.

Happy clipping!