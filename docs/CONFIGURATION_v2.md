# CTIC Configuration System v2.0

## Overview

CTIC now features a fully JSON-based configuration system that enables professional media teams to customize detection without recompiling. The system supports external word lists, configurable detection parameters, and a foundation for future ML plugin integration.

## Architecture

### Directory Structure

```
config/
├── engine.json              # Global engine settings and detector registry
├── wordlists/               # External word list files
│   ├── high.json           # High-intensity positive reactions
│   ├── high_negative.json  # High-intensity negative reactions
│   ├── medium.json         # Medium-intensity reactions
│   └── easy.json          # Low-intensity/filler reactions
└── profiles/              # Profile overrides (existing)
    ├── conservative.json
    ├── balanced.json
    └── aggressive.json
```

### Key Components

1. **nlohmann/json Library**: Header-only JSON parsing for robust configuration handling
2. **External Word Lists**: Words now loaded from JSON files instead of hardcoded C++
3. **Engine Configuration**: Global settings in `engine.json`
4. **Tier Configurations**: Per-tier thresholds and parameters
5. **Backward Compatibility**: Falls back to built-in defaults if JSON files missing

## Configuration Files

### engine.json

Main configuration file defining global settings and detector registry:

```json
{
  "version": "2.0",
  "engine": {
    "reload_interval_seconds": 30,
    "output_format": "csv",
    "buffer_size": 1000,
    "max_creators": 10,
    "thread_pool_size": 4
  },
  "detectors": [
    {
      "id": "semantic_burst",
      "type": "fuzzy_match",
      "enabled": true,
      "config": {
        "algorithm": "levenshtein",
        "similarity_threshold": 0.8,
        "window_seconds": 30
      }
    },
    {
      "id": "volume_spike",
      "type": "statistical",
      "enabled": true,
      "config": {
        "algorithm": "zscore",
        "sigma_threshold": 3.0
      }
    }
  ],
  "tier_configs": {
    "high": {
      "burst_threshold": 3,
      "cooldown_seconds": 60,
      "require_unique_users": 2
    }
  }
}
```

### Word List Files

Each tier has its own JSON word list:

**high.json** (High-intensity reactions):
```json
{
  "version": "1.0",
  "tier": "high",
  "description": "High-intensity positive reactions",
  "words": ["POG", "INSANE", "CLUTCH", "ACE", ...],
  "metadata": {
    "case_sensitive": false,
    "intensity": 5
  }
}
```

## Features

### 1. External Word Lists
- Words loaded from `config/wordlists/*.json`
- Easy to customize without recompiling
- Add custom word lists for specific communities
- Metadata support (intensity, category, case sensitivity)

### 2. Robust JSON Parsing
- Replaced fragile manual string parsing
- Full JSON spec compliance
- Better error handling with meaningful messages
- Type-safe value extraction

### 3. Configurable Detection Parameters
All tier thresholds configurable via JSON:
- `burst_threshold`: Messages needed to trigger
- `window_seconds`: Sliding window size
- `cooldown_seconds`: Post-detection cooldown
- `require_unique_users`: Minimum unique usernames
- `min_word_length`: Minimum matched word length
- `levenshtein_threshold`: Fuzzy match similarity

### 4. Fallback System
If external JSON files are missing:
- Uses built-in word lists (embedded in binary)
- Applies default thresholds
- Logs warnings about missing files
- Tool still functions

## Usage Examples

### Adding Custom Word List

Create `config/wordlists/custom.json`:
```json
{
  "version": "1.0",
  "tier": "custom",
  "description": "My community-specific reactions",
  "words": ["HYDRATE", "SQUAD", "FAM", ...]
}
```

Add to engine.json tier_configs:
```json
"tier_configs": {
  "custom": {
    "burst_threshold": 5,
    "cooldown_seconds": 45,
    "wordlist": "custom.json"
  }
}
```

### Modifying Existing Tier

Edit `config/wordlists/high.json` and add/remove words, then restart CTIC.

### Creating Custom Profile

Create `.ctic/profiles/mystreamer.json`:
```json
{
  "profile_name": "mystreamer",
  "tiers": {
    "high": {
      "burst_threshold": 2,
      "cooldown_seconds": 30
    }
  }
}
```

## Migration from v1.0

### What's Changed
1. **Word lists**: Now external JSON files (previously hardcoded in C++)
2. **Config parsing**: Now uses nlohmann/json (previously manual string parsing)
3. **Engine config**: New `engine.json` for global settings
4. **Backward compatibility**: Profile system unchanged

### What You Need to Do
1. **Nothing** - Tool works with built-in fallbacks
2. **Optional**: Copy word list files to customize
3. **Optional**: Create `engine.json` for advanced configuration

## Plugin Architecture Foundation

This configuration system lays groundwork for C++ plugins:

### Future Plugin Structure
```
plugins/
├── detectors/
│   ├── libsentiment_ml.so
│   ├── libaudio_analysis.so
│   └── manifests/
│       ├── sentiment_ml.json
│       └── audio_analysis.json
```

### Plugin Manifest Example
```json
{
  "name": "sentiment_ml",
  "version": "1.0.0",
  "type": "detector",
  "entry_point": "create_detector",
  "config_schema": {
    "model_path": "string",
    "threshold": "float"
  }
}
```

## Technical Details

### Dependencies
- **nlohmann/json**: Header-only, already included in `include/nlohmann/`
- No external dependencies to install
- C++17 required

### Performance
- Word lists loaded once at startup
- JSON parsing ~10ms for typical word lists
- No runtime JSON parsing during detection
- Memory: ~50KB per tier for word lists

### Error Handling
- Missing word lists: Warning + fallback to built-in
- Invalid JSON: Error message, uses defaults
- Missing fields: Uses sensible defaults
- File permissions: Error with helpful message

## Next Steps

1. **Hot Reload**: File watcher to reload configs without restart
2. **Validation**: JSON Schema validation for configs
3. **Plugin System**: C++ shared library loading
4. **ML Integration**: ONNX Runtime for model inference
5. **GUI Config**: Desktop app for visual configuration

## Configuration Reference

### engine.json Schema

```json
{
  "version": "string (required)",
  "engine": {
    "reload_interval_seconds": "integer",
    "output_format": "csv|json",
    "buffer_size": "integer",
    "max_creators": "integer",
    "thread_pool_size": "integer",
    "log_level": "debug|info|warn|error"
  },
  "defaults": {
    "profile": "string",
    "enabled_tiers": ["array of strings"],
    "log_level": "string"
  },
  "detectors": [{
    "id": "string (unique)",
    "type": "fuzzy_match|statistical|ml_inference",
    "enabled": "boolean",
    "description": "string",
    "config": "object (type-specific)"
  }],
  "tier_configs": {
    "tier_name": {
      "burst_threshold": "integer",
      "window_seconds": "integer",
      "cooldown_seconds": "integer",
      "require_unique_users": "integer",
      "min_word_length": "integer",
      "levenshtein_threshold": "float (0.0-1.0)",
      "use_levenshtein": "boolean",
      "wordlist": "string (filename)"
    }
  }
}
```

### Word List Schema

```json
{
  "version": "string",
  "tier": "string (matches tier_configs key)",
  "description": "string",
  "words": ["array of strings"],
  "metadata": {
    "case_sensitive": "boolean",
    "collapse_repeated_chars": "boolean",
    "category": "positive|negative|neutral",
    "intensity": "integer (1-5)"
  }
}
```

---

**For professional media teams**: This system makes CTIC highly adaptable. Teams can:
- Tune detection for specific communities (gaming, IRL, etc.)
- A/B test word lists
- Share configurations across team members
- Version control their detection rules

**For developers**: The foundation is set for C++ plugins. The detector registry, config loading, and word list system are all ready to support custom algorithms via shared libraries.
