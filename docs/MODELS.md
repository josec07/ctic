# CTIC Model System Documentation

## Overview

CTIC supports drop-in ONNX models for ML-based clip detection. Simply download a model, place it in the `models/` directory, and reference it in your pipeline configuration.

## Architecture

### Model Package Structure
Each model is a self-contained package with:
```
models/<model-id>/
├── config.json      # Model configuration (required)
├── model.onnx       # The ONNX model file (required)
├── labels.txt       # Output labels (optional)
├── tokenizer.json   # Tokenizer config (optional)
└── README.md        # Usage documentation (optional)
```

### Model Configuration Schema

The `config.json` file defines everything about the model:

```json
{
  "model_info": {
    "name": "Human-readable name",
    "version": "1.0.0",
    "description": "What this model does",
    "author": "Model creator",
    "source": "Original source URL",
    "license": "MIT/Apache/etc"
  },
  
  "preprocessing": {
    "tokenizer": "bert/roberta/gpt2",
    "max_length": 128,
    "lowercase": false,
    "remove_urls": true,
    "expand_emotes": true
  },
  
  "inference": {
    "input_names": ["input_ids", "attention_mask"],
    "output_names": ["logits"],
    "providers": ["CPUExecutionProvider"]
  },
  
  "postprocessing": {
    "type": "classification",
    "activation": "softmax",
    "labels": ["negative", "neutral", "positive"],
    "threshold": 0.7
  },
  
  "clip_detection": {
    "trigger_on": ["positive", "negative"],
    "min_confidence": 0.8,
    "description_template": "{label} detected at {confidence}%"
  },
  
  "performance": {
    "avg_inference_ms": 15,
    "memory_mb": 125,
    "throughput_msgs_sec": 500
  }
}
```

## Using Models

### 1. Discovery
Models are automatically discovered from the `models/` directory:
```bash
./ctic models list
```

### 2. Information
Get detailed information about a model:
```bash
./ctic models info sentiment-twitter
```

### 3. Testing
Test a model with sample text:
```bash
./ctic models test sentiment-twitter --text "This stream is amazing!"
```

### 4. Pipeline Integration
Reference models in your pipeline configuration:
```json
{
  "nodes": [
    {
      "id": "sentiment",
      "type": "onnx_model",
      "config": {
        "model_dir": "models/sentiment-twitter/"
      }
    }
  ]
}
```

## Model Types

### Classification Models
- **Type**: `classification`
- **Output**: Class labels with confidence scores
- **Use cases**: Sentiment, toxicity, emotion detection

### Multi-label Models
- **Type**: `multi_label`
- **Output**: Multiple labels with individual scores
- **Use cases**: Emotion detection, content categorization

### Regression Models
- **Type**: `regression`
- **Output**: Continuous values
- **Use cases**: Intensity scoring, rating prediction

## Preprocessing Options

### Tokenizers
- `bert`: BERT-style tokenization with WordPiece
- `roberta`: RoBERTa tokenization
- `gpt2`: GPT-2 BPE tokenization
- `simple`: Whitespace tokenization

### Text Normalization
- `lowercase`: Convert to lowercase
- `remove_urls`: Strip URLs
- `expand_emotes`: Convert emotes to text
- `remove_punctuation`: Strip punctuation

## Performance Profiles

### Real-time (Low Latency)
- Models < 50MB
- Inference < 10ms
- Example: MiniLM models

### Balanced
- Models 50-150MB
- Inference 10-30ms
- Example: DistilBERT, RoBERTa-base

### High Accuracy
- Models > 150MB
- Inference > 30ms
- Example: DeBERTa, Large models

## Clip Detection Configuration

### Trigger Conditions
Models can trigger clip detection based on:
- Specific labels (e.g., "positive", "toxic")
- Confidence thresholds
- Combined with other signals (spikes, patterns)

### Description Templates
Use variables in templates:
- `{label}`: The detected label
- `{confidence}`: Confidence percentage
- `{score}`: Raw score value

Example:
```json
"description_template": "Strong {label} sentiment detected ({confidence}%)"
```

## Downloading Models

### Using the Download Script
```bash
./scripts/download_model.sh sentiment-twitter
```

### Manual Download
1. Download ONNX model from source
2. Create model directory: `models/<model-id>/`
3. Place model as `model.onnx`
4. Create `config.json` with proper settings

## Creating Custom Models

### 1. Export to ONNX
From PyTorch:
```python
torch.onnx.export(model, dummy_input, "model.onnx")
```

From TensorFlow:
```python
import tf2onnx
tf2onnx.convert.from_keras(model, output_path="model.onnx")
```

### 2. Create Configuration
Copy and modify an example configuration from `models/sentiment-twitter/config.json`

### 3. Test Integration
```bash
./ctic models test my-custom-model --text "Test input"
```

## Troubleshooting

### Model Not Found
- Check model directory exists in `models/`
- Verify `config.json` is present
- Run `./ctic models list` to see discovered models

### Validation Errors
- Check `config.json` format
- Verify required fields are present
- Ensure model.onnx file exists

### Performance Issues
- Use quantized models for better speed
- Reduce batch size in config
- Consider using GPU providers if available

## Best Practices

1. **Model Selection**
   - Choose models based on your latency requirements
   - Use specialized models for specific tasks
   - Test models with real data before production

2. **Configuration**
   - Set appropriate confidence thresholds
   - Configure preprocessing for your text type
   - Document trigger conditions clearly

3. **Organization**
   - Use descriptive model IDs
   - Keep README.md updated
   - Version your model configurations

## Example Models

### Sentiment Analysis
- `sentiment-twitter`: Optimized for social media
- `sentiment-general`: General purpose sentiment

### Toxicity Detection
- `toxicity-gaming`: Gaming-specific toxicity
- `toxicity-minimal`: Lightweight detector

### Emotion Classification
- `emotion-go`: 28 emotion labels
- `emotion-basic`: 6 basic emotions

## Community Models

Share your model configurations:
1. Create a GitHub repo with your config
2. Include download instructions
3. Submit to community registry

## Future Enhancements

- Automatic model downloading from Hugging Face
- Model versioning and updates
- A/B testing between models
- Model ensemble support
- Custom preprocessing plugins