# CTIC v3.0 - Single Streamer Edition

**Clip Detection Engine for Live Streamers**

A streamlined, single-channel CLI tool for detecting highlight moments in Twitch live chat. Designed for professional media teams and content creators who want reliable, configurable clip detection without complexity.

## What's New in v3.0

### 🎯 Single-Streamer Focus
- **Simplified workflow**: Configure once, monitor anytime
- **Removed multi-streamer complexity**: No thread pools, no connection management overhead
- **Streamlined CLI**: 4 core commands instead of 6
- **Better UX**: Interactive configuration with real-time feedback

### ⚙️ JSON-Based Configuration
- **External word lists**: Customize detection vocabulary without recompiling
- **Flexible profiles**: balanced, conservative, aggressive detection modes
- **Tier-based detection**: high, high-negative, medium, easy intensity levels
- **Hot-pluggable**: Edit configs while tool is running (coming in v3.1)

### 🔧 Technical Improvements
- **Robust JSON parsing**: Using nlohmann/json library (no more fragile string parsing)
- **Better error handling**: Meaningful error messages
- **Anonymous IRC**: Works immediately, no OAuth complexity
- **Simplified architecture**: Removed 30% of code complexity

## Quick Start

### 1. Configure Your Channel

```bash
./ctic configure shroud
```

This will:
- Test connection to the channel
- Sample chat activity
- Launch interactive configuration menu
- Save settings to `.ctic/creators/shroud.json`

### 2. Start Monitoring

```bash
./ctic monitor shroud
```

Or just:
```bash
./ctic monitor
```
(Uses the last configured channel)

### 3. Get Your Clips

Detected moments are saved to:
```
.ctic/outputs/shroud/
├── high/
│   └── clips-high-2026-03-14.csv
├── medium/
└── easy/
```

## Commands

| Command | Description | Example |
|---------|-------------|---------|
| `configure <channel>` | Setup detection for a channel | `./ctic configure shroud` |
| `monitor [channel]` | Start monitoring chat | `./ctic monitor` |
| `status [channel]` | Show configuration | `./ctic status` |
| `remove <channel>` | Delete configuration | `./ctic remove shroud` |

## Configuration

### Interactive Configuration

When you run `./ctic configure`, you get an interactive menu:

```
========================================
CTIC - Single Streamer Configuration
========================================
Channel: shroud

[TEST] Connecting to chat...
[TEST] Connected. Sampling chat activity (10s)...
  user1: POGGERS
  user2: INSANE PLAY
[TEST] Sampled 47 messages

Current Detection Profile: balanced
Enabled Tiers: high high-negative medium

Configuration Options:
  1. Change detection profile (balanced/conservative/aggressive)
  2. Toggle tier: high (clutch moments)
  3. Toggle tier: high-negative (fails/criticism)
  4. Toggle tier: medium (general hype)
  5. Toggle tier: easy (background chat)
  6. Save and exit
  0. Cancel and exit

Enter choice (0-6):
```

### Detection Profiles

| Profile | Description | Best For |
|---------|-------------|----------|
| `balanced` | Moderate sensitivity | Variety streamers |
| `conservative` | High thresholds, fewer clips | High-volume chat |
| `aggressive` | Low thresholds, more clips | Small communities |

### Detection Tiers

| Tier | Triggers | Use Case |
|------|----------|----------|
| `high` | POG, INSANE, CLUTCH, ACE | Peak moments, clutches |
| `high-negative` | L, RIP, TRASH, OMEGALUL | Fails, criticism, drama |
| `medium` | W, GG, KEKW, CLEAN | Solid plays, good moments |
| `easy` | lol, wow, bruh, ok | Background engagement |

### Custom Word Lists

Edit JSON files in `config/wordlists/`:

```json
// config/wordlists/high.json
{
  "version": "1.0",
  "tier": "high",
  "words": [
    "POG", "INSANE", "CLUTCH",
    "YOUR_CUSTOM_WORD"
  ],
  "metadata": {
    "intensity": 5,
    "category": "positive"
  }
}
```

Changes take effect immediately on next monitor session.

## Architecture

### Simplified Design

```
User → CLI → Single Monitor → IRC Connection → Detection Engine → CSV Output
```

**Removed complexity:**
- ❌ Thread pools
- ❌ Multi-creator orchestration
- ❌ Connection pooling
- ❌ Complex signal handling

**Kept essentials:**
- ✅ Robust detection algorithms
- ✅ JSON configuration
- ✅ Anonymous IRC
- ✅ CSV output with metadata

### Detection Algorithms

1. **Semantic Burst Detection** (Levenshtein fuzzy matching)
   - Groups similar words (POG/Poggers/POGGIES)
   - Sliding window with configurable thresholds
   - Unique user counting prevents spam

2. **Volume Spike Detection** (Z-score statistical analysis)
   - Welford's online variance algorithm
   - Detects sudden chat velocity increases
   - Independent of word content

## Output Format

CSV files include:

```csv
timestamp,matched_word,sentiment,burst_count,spike_zscore,unique_users,intensity,sample_messages
2026-03-14T10:23:45Z,INSANE,positive,5,2.34,4,0.95,user1: INSANE;user2: INSANE PLAY
2026-03-14T10:25:12Z,CLUTCH,positive,3,1.89,3,0.88,user1: CLUTCH;user2: CLUTCHED
```

## Comparison: Multi vs Single

| Feature | Multi-Streamer (v2.x) | Single-Streamer (v3.0) |
|---------|------------------------|------------------------|
| Complexity | High | Low |
| Use Case | Agencies, networks | Individual streamers |
| Auth | OAuth recommended | Anonymous IRC |
| Setup | Multi-step | 2 commands |
| Resource Usage | Higher | Minimal |
| Target User | Technical teams | Content creators |

## Roadmap

### v3.1 (Config Hot-Reload)
- File watcher to reload word lists without restart
- Live profile switching

### v3.2 (Plugin System)
- C++ detector plugins
- ONNX Runtime for ML inference
- Custom algorithm support

### v3.3 (GUI Wrapper)
- Qt-based desktop app
- Visual configuration
- Real-time detection dashboard

## Technical Details

### Dependencies
- C++17 compiler
- nlohmann/json (header-only, included)
- CLI11 (header-only, included)
- POSIX sockets (Linux/macOS)

### Performance
- Memory: ~5MB base + ~1MB per detection tier
- CPU: <5% on modern hardware
- Network: 1 IRC connection per session
- Disk: CSV append, ~1KB per detected clip

### Security
- No OAuth tokens stored
- Anonymous IRC connection
- Local file storage only
- No network services exposed

## Migration from v2.x

If you were using multi-streamer mode:

1. **Configurations**: Still compatible, single channel will be used
2. **Commands**: Old commands redirect to new ones
   - `add` → `configure`
   - `run` → `monitor`
   - `list` → `status`
3. **Output**: Same CSV format

To migrate multi-creator setups:
```bash
# Instead of monitoring 5 channels at once
# Run 5 separate instances or pick your primary channel
./ctic configure shroud
./ctic monitor shroud
```

## FAQ

**Q: Can I monitor multiple channels?**
A: Not in one process. Run multiple instances or pick your primary channel. The complexity of multi-monitoring wasn't worth the maintenance for 99% of use cases.

**Q: Do I need OAuth/Twitch API access?**
A: No. Anonymous IRC works for reading public chat. OAuth only needed for posting messages or accessing private data.

**Q: Can I customize detection algorithms?**
A: Not yet. v3.2 will add C++ plugin support for custom detectors.

**Q: What about VOD analysis?**
A: Coming in v3.1. For now, this is live-only.

**Q: Is Windows supported?**
A: Linux/macOS only currently. Windows support planned for v3.2.

## License

MIT License - See LICENSE file

## Contributing

This is a focused, opinionated tool. PRs welcome for:
- Bug fixes
- Performance improvements
- Documentation
- Tests

Large architectural changes should be discussed in an issue first.

---

**Built for streamers who want reliable clip detection without the complexity.**
