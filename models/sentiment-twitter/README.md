# Twitter RoBERTa Sentiment Model

## Description
This model performs sentiment analysis optimized for social media text, particularly Twitter/Twitch chat. It classifies text into three categories: negative, neutral, and positive.

## Usage
This model is configured to work with CTIC's pipeline system. Simply reference it in your pipeline configuration:

```json
{
  "id": "sentiment",
  "type": "onnx_model",
  "config": {
    "model_dir": "models/sentiment-twitter/"
  }
}
```

## Performance
- **Inference Time**: ~15ms per message
- **Memory Usage**: 125MB
- **Throughput**: 500 messages/second

## Labels
- `negative`: Negative sentiment (complaints, criticism, disappointment)
- `neutral`: Neutral sentiment (statements, observations)
- `positive`: Positive sentiment (excitement, praise, joy)

## Clip Detection
This model triggers clip detection on:
- Strong positive sentiment (>85% confidence) - for hype moments
- Strong negative sentiment (>85% confidence) - for fail/drama moments

## Download
To download the actual ONNX model:
```bash
./scripts/download_model.sh sentiment-twitter
```

## License
MIT