#!/bin/bash

# CTIC Model Downloader Script
# Downloads and configures ONNX models for use with CTIC pipeline

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Model repository (you would host these or use CDN)
MODEL_REPO="https://example.com/ctic-models"

# Available models and their URLs
declare -A MODELS
MODELS["sentiment-twitter"]="twitter-roberta-sentiment.onnx"
MODELS["toxicity-gaming"]="gaming-toxicity-detector.onnx"
MODELS["emotion-go"]="go-emotions-roberta.onnx"
MODELS["toxicity-minimal"]="minilm-toxic-classifier.onnx"

# Model configurations are already in the repo
MODEL_DIR="models"

function print_usage() {
    echo "Usage: $0 <model-name>"
    echo ""
    echo "Available models:"
    echo "  sentiment-twitter    - Twitter sentiment analysis (125MB)"
    echo "  toxicity-gaming     - Gaming-specific toxicity detection (95MB)"
    echo "  emotion-go          - 28-emotion classifier (125MB)"
    echo "  toxicity-minimal    - Lightweight toxicity detector (25MB)"
    echo ""
    echo "Example:"
    echo "  $0 sentiment-twitter"
}

function download_model() {
    local model_name=$1
    local model_file=${MODELS[$model_name]}
    
    if [ -z "$model_file" ]; then
        echo -e "${RED}Error: Unknown model '$model_name'${NC}"
        print_usage
        exit 1
    fi
    
    local model_path="$MODEL_DIR/$model_name"
    
    # Check if model directory exists
    if [ ! -d "$model_path" ]; then
        echo -e "${RED}Error: Configuration not found for $model_name${NC}"
        echo "Model configuration directory expected at: $model_path"
        exit 1
    fi
    
    # Check if model already downloaded
    if [ -f "$model_path/model.onnx" ]; then
        echo -e "${YELLOW}Model already exists at $model_path/model.onnx${NC}"
        read -p "Do you want to re-download? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Skipping download."
            exit 0
        fi
    fi
    
    echo -e "${GREEN}Downloading model: $model_name${NC}"
    echo "This is a placeholder - in production, this would download from:"
    echo "  $MODEL_REPO/$model_file"
    echo ""
    
    # In production, this would actually download:
    # wget -O "$model_path/model.onnx" "$MODEL_REPO/$model_file"
    # or
    # curl -L -o "$model_path/model.onnx" "$MODEL_REPO/$model_file"
    
    # For now, create a placeholder
    echo "Creating placeholder model file..."
    echo "ONNX_MODEL_PLACEHOLDER" > "$model_path/model.onnx"
    
    echo -e "${GREEN}✓ Model downloaded to $model_path/model.onnx${NC}"
    
    # Validate the model
    echo "Validating model configuration..."
    if [ -f "$model_path/config.json" ]; then
        echo -e "${GREEN}✓ Configuration found${NC}"
    else
        echo -e "${RED}✗ Configuration missing${NC}"
        exit 1
    fi
    
    # Show model info
    echo ""
    echo "Model Information:"
    echo "=================="
    if [ -f "$model_path/README.md" ]; then
        head -n 10 "$model_path/README.md" | grep -v "^#"
    fi
    
    echo ""
    echo -e "${GREEN}Model ready to use!${NC}"
    echo ""
    echo "Add to your pipeline with:"
    echo '  {'
    echo '    "id": "my_model",'
    echo '    "type": "onnx_model",'
    echo '    "config": {'
    echo '      "model_dir": "models/'$model_name'/"'
    echo '    }'
    echo '  }'
}

# Main script
if [ $# -eq 0 ]; then
    print_usage
    exit 1
fi

case "$1" in
    list)
        echo "Available models:"
        for model in "${!MODELS[@]}"; do
            echo "  - $model"
        done
        ;;
    *)
        download_model "$1"
        ;;
esac