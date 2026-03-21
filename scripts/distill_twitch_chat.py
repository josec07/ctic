#!/usr/bin/env python3
"""
Distill raw Twitch JSON to CTIC format
Usage: python distill_twitch_chat.py <raw_file.json>
"""

import json
import sys
import os
from datetime import datetime

def distill_chat(raw_file, output_file=None):
    """Transform raw Twitch JSON to minimal CTIC format"""
    
    print(f"Processing {raw_file}...")
    
    # Load raw Twitch data
    with open(raw_file, 'r') as f:
        twitch_data = json.load(f)
    
    # Extract metadata
    video_info = twitch_data.get("video", {})
    streamer = twitch_data.get("streamer", {})
    
    metadata = {
        "vod_id": str(video_info.get("id", "unknown")),
        "channel": streamer.get("name", "unknown"),
        "title": video_info.get("title", "Untitled"),
        "game": video_info.get("game", "Unknown"),
        "duration": video_info.get("length", 0),
        "view_count": video_info.get("viewCount", 0),
        "created_at": twitch_data.get("FileInfo", {}).get("CreatedAt", datetime.now().isoformat())
    }
    
    # Distill comments to essential fields
    messages = []
    unique_users = set()
    
    for comment in twitch_data.get("comments", []):
        # Extract badges
        badges = []
        for badge in comment.get("message", {}).get("user_badges", []):
            badge_id = badge.get("_id", "")
            if badge_id:
                badges.append(badge_id)
        
        # Create minimal message object
        msg = {
            "time": comment.get("content_offset_seconds", 0),
            "user": comment.get("commenter", {}).get("display_name", "unknown"),
            "message": comment.get("message", {}).get("body", ""),
            "badges": badges
        }
        
        # Optional: Add color for visualization
        user_color = comment.get("message", {}).get("user_color")
        if user_color:
            msg["color"] = user_color
        
        # Optional: Add bits for hype detection
        bits = comment.get("message", {}).get("bits_spent", 0)
        if bits > 0:
            msg["bits"] = bits
        
        messages.append(msg)
        unique_users.add(msg["user"])
    
    # Build CTIC structure
    ctic_data = {
        **metadata,
        "messages": messages,
        "stats": {
            "total_messages": len(messages),
            "unique_users": len(unique_users),
            "messages_per_minute": len(messages) / (metadata["duration"] / 60) if metadata["duration"] > 0 else 0
        }
    }
    
    # Determine output filename
    if not output_file:
        vod_id = metadata["vod_id"]
        output_file = f"data/vods/{vod_id}.json"
    
    # Ensure directory exists
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    
    # Save CTIC format
    with open(output_file, 'w') as f:
        json.dump(ctic_data, f, indent=2)
    
    # Calculate size reduction
    raw_size = os.path.getsize(raw_file)
    ctic_size = os.path.getsize(output_file)
    reduction = (1 - ctic_size / raw_size) * 100
    
    print(f"\n✓ Distilled {len(messages)} messages")
    print(f"  Duration: {metadata['duration']} seconds ({metadata['duration']/3600:.1f} hours)")
    print(f"  Unique users: {len(unique_users)}")
    print(f"  Size reduction: {raw_size/1024/1024:.1f}MB → {ctic_size/1024/1024:.1f}MB ({reduction:.0f}% smaller)")
    print(f"  Output: {output_file}")
    
    return ctic_data

def analyze_chat_patterns(messages):
    """Quick analysis of chat patterns"""
    
    # Count messages per minute buckets
    from collections import defaultdict
    minute_buckets = defaultdict(int)
    
    for msg in messages:
        minute = int(msg["time"] / 60)
        minute_buckets[minute] += 1
    
    # Find spike moments (>3x average)
    avg_per_minute = len(messages) / (max(msg["time"] for msg in messages) / 60) if messages else 0
    spikes = []
    
    for minute, count in sorted(minute_buckets.items()):
        if count > avg_per_minute * 3:
            spikes.append({
                "time": minute * 60,
                "count": count,
                "intensity": count / avg_per_minute if avg_per_minute > 0 else 0
            })
    
    print(f"\n📊 Chat Analysis:")
    print(f"  Average: {avg_per_minute:.0f} messages/minute")
    print(f"  Spike moments (>3x avg): {len(spikes)}")
    
    if spikes:
        print(f"  Top 3 spikes:")
        for spike in sorted(spikes, key=lambda x: x["count"], reverse=True)[:3]:
            minutes = int(spike["time"] / 60)
            print(f"    {minutes}m: {spike['count']} msgs ({spike['intensity']:.1f}x avg)")
    
    return spikes

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python distill_twitch_chat.py <raw_twitch_file.json>")
        print("Example: python distill_twitch_chat.py data/vods/raw/2722559424_twitch.json")
        sys.exit(1)
    
    raw_file = sys.argv[1]
    
    # Distill
    ctic_data = distill_chat(raw_file)
    
    # Analyze
    analyze_chat_patterns(ctic_data["messages"])
    
    print(f"\n✅ Ready for CTIC pipeline!")
