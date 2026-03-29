#!/usr/bin/env python3
"""
Download Twitch VOD chat using TwitchDownloaderCLI
Usage: python download_vod_chat.py <vod_id_or_url>
Example: python download_vod_chat.py 2722559424
Example: python download_vod_chat.py https://www.twitch.tv/videos/2722559424

Requirements: TwitchDownloaderCLI must be installed (e.g., paru -S twitch-downloader-bin)
"""

import os
import sys
import json
import subprocess
from datetime import datetime

def download_chat(vod_id):
    """Download chat using TwitchDownloaderCLI"""
    
    print(f"Downloading chat for VOD {vod_id}...")
    print("  This may take a while for long streams...")
    
    # Ensure directories exist
    os.makedirs('data/vods', exist_ok=True)
    os.makedirs('data/vods/raw', exist_ok=True)
    
    # Raw file path (kept as backup)
    raw_file = f"data/vods/raw/{vod_id}_twitch.json"
    
    # Call TwitchDownloaderCLI
    try:
        result = subprocess.run(
            [
                "TwitchDownloaderCLI",
                "chatdownload",
                "--id", vod_id,
                "--output", raw_file
            ],
            capture_output=True,
            text=True,
            check=True
        )
        
        print(f"  CLI output: {result.stdout[-200:] if len(result.stdout) > 200 else result.stdout}")
        
    except subprocess.CalledProcessError as e:
        print(f"Error: TwitchDownloaderCLI failed")
        print(f"  Exit code: {e.returncode}")
        print(f"  Error: {e.stderr}")
        return None
    except FileNotFoundError:
        print("Error: TwitchDownloaderCLI not found")
        print("  Install with: paru -S twitch-downloader-bin")
        return None
    
    # Load and parse the raw Twitch JSON
    try:
        with open(raw_file, 'r') as f:
            twitch_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: Expected output file not found: {raw_file}")
        return None
    except json.JSONDecodeError:
        print(f"Error: Invalid JSON in output file")
        return None
    
    # Transform to CTIC format
    messages = []
    for comment in twitch_data.get("comments", []):
        # Handle different field names in TwitchDownloaderCLI output
        msg = {
            "time": comment.get("content_offset_seconds", 0),
            "user": comment.get("commenter", {}).get("display_name", "unknown"),
            "message": comment.get("message", {}).get("body", ""),
            "badges": [
                badge.get("_id", "") 
                for badge in comment.get("message", {}).get("user_badges", [])
            ]
        }
        messages.append(msg)
    
    print(f"  Downloaded {len(messages)} messages")
    
    return messages, twitch_data

def save_vod(vod_id, messages, twitch_data):
    """Save in CTIC format and keep raw backup"""
    
    # Calculate duration from last message
    duration = int(messages[-1]["time"]) if messages else 0
    
    # Extract metadata from twitch_data if available
    video_info = twitch_data.get("video", {})
    channel = video_info.get("owner", {}).get("login", "unknown")
    title = video_info.get("title", f"VOD {vod_id}")
    game = video_info.get("game", {}).get("name", "Unknown") if video_info.get("game") else "Unknown"
    
    # Build CTIC format structure
    vod = {
        "vod_id": vod_id,
        "channel": channel,
        "title": title,
        "game": game,
        "duration": duration,
        "created_at": datetime.now().isoformat(),
        "messages": messages,
        "stats": {
            "total_messages": len(messages),
            "unique_users": len(set(m["user"] for m in messages))
        }
    }
    
    # Save CTIC format
    ctic_file = f"data/vods/{vod_id}.json"
    with open(ctic_file, "w") as f:
        json.dump(vod, f, indent=2)
    
    print(f"\n✓ Saved CTIC format: {ctic_file}")
    print(f"  Messages: {len(messages)}")
    print(f"  Duration: {duration} seconds")
    print(f"  Channel: {channel}")
    print(f"\n✓ Raw Twitch JSON kept as backup: data/vods/raw/{vod_id}_twitch.json")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python download_vod_chat.py <vod_id_or_url>")
        print("Example: python download_vod_chat.py 2722559424")
        print("Example: python download_vod_chat.py https://www.twitch.tv/videos/2722559424")
        sys.exit(1)
    
    vod_input = sys.argv[1]
    
    # Extract VOD ID from URL
    if "twitch.tv" in vod_input:
        vod_id = vod_input.split("/")[-1].split("?")[0]
    else:
        vod_id = vod_input
    
    # Download chat
    result = download_chat(vod_id)
    
    if result is None:
        print("\nFailed to download chat")
        sys.exit(1)
    
    messages, twitch_data = result
    
    # Save both formats
    save_vod(vod_id, messages, twitch_data)
    
    print(f"\nReady for CTIC pipeline!")
    print(f"  Use: ./ctic_pipeline --config config/templates/simple_spike.json")
