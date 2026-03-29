# CTIC - CLI Service Vision

## ASCII Vision Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              CTIC - CLI Service                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│  USER FLOW                                                                   │
└─────────────────────────────────────────────────────────────────────────────┘


$ ctic connect <url>

url functionality

then immeddiatly test the connection.


------------------------------
    $ ctic run


IF there is configs created,

config x, connected (once we have it connected we are already montior on the csv's etc.)
config y, connected (same)
config z, failed did not connect

run <ctic --help>

test with ctic test before adding another run.

use ctic run add <name>

to ensure you can add to the current running 




----------------------------------------------_




This Section needs to be sepeareted into its own tool.

$ ctic watch <name>


    │  Starting monitor...                │
    │                                     │
    │  ┌─────────────────────────────┐    │
    │  │ #xqc Chat Stream            │    │
    │  │─────────────────────────────│    │
    │  │ 12:34:01 user1: POG         │    │
    │  │ 12:34:02 user2: INSANE      │    │
    │  │ 12:34:02 user3: POGGG       │    │
    │  │ 12:34:03 user4: insane play │    │
    │  │ ─────────────────────────── │    │
    │  │ ⚡ [HIGH] BURST DETECTED     │    │
    │  │    Word: INSANE             │    │
    │  │    Burst: 3 messages / 2s   │    │
    │  │    Matched: 4 users         │    │
    │  │    Log: outputs/xqc/high/   │    │
    │  └─────────────────────────────┘    │
    └─────────────────────────────────────┘
```

## FILE STRUCTURE

```
your-project/
│
├── .ctic/                           ← Local data folder (created on first run)
│   │
│   ├── creators/                    ← Creator configurations
│   │   ├── xqc.json
│   │   ├── shroud.json
│   │   └── pokimane.json
│   │
│   ├── outputs/                     ← CSV logs per creator/tier
│   │   ├── xqc/
│   │   │   ├── high/
│   │   │   │   └── matches-20260228-120000.csv
│   │   │   ├── medium/
│   │   │   │   └── matches-20260228-120000.csv
│   │   │   └── easy/
│   │   │       └── matches-20260228-120000.csv
│   │   └── shroud/
│   │       └── high/
│   │           └── matches-20260228-130000.csv
│   │
│   └── state.json                   ← Active sessions, last run, etc.
|

---------------------
already done needs to be adapted and cleaned
|
|-- lib/
|    |
|    |libctic-text/
|    |
|    |--/clean-text-levensthetin,parse,delimter-human readable,etc. 
     |
     |more custome libraries or ones we find in open source. go here
|

--------------------

Already done, needs to be adpated and cleaned
│
├── src/                             ← Core application
│   │
│   ├── providers/                   ← External service connections
│   │   ├── twitch_irc.cpp           ← Twitch IRC client
│   │   └── twitch_url.cpp           ← URL parsing utilities
│   │
│   ├── <features>/
---------------------------------------------------------------------------------------
      MVP is purely just levenshtein
 This area is variable <controllers> <network> <wesbsockets> and more     
      
                        ← Internal engine
│   │   ├── detector.cpp             ← Burst detection logic core uses 
│   │   ├── config_manager.cpp       ← Load/save creator configs
│   │   └── output_logger.cpp        ← CSV generation
│   │ 

----------------------------------------------------------------------------------------

AFTER MVP
│   ├── ui/                          ← Terminal UI
│   │   ├── terminal_ui.cpp          ← Prompts, menus, live display
│   │   └── menu.cpp                 ← Navigation states
│   │
│   └── main.cpp                     ← Entry point (ctic run command)
│
----------------------------------------------------------------------------------------





├── include/
│   └── [header files mirror src/]
│
├── tests/
│   ├── test_detector.cpp
│   └── test_url_parser.cpp
_________________________________________________________________________
├── .context/                         ← Documentation and vision
│   └── VISION.md
│

this is mainly for myself should be added to a .gitingore
________________________
└── Makefile
```




## CREATOR CONFIG

`.ctic/creators/xqc.json`:

```json
{
  "name": "xqc",
  "channel": "xqc",
  "twitch_url": "https://twitch.tv/xqc",
  "enabled_tiers": ["high", "medium", "easy"],
  "created_at": "2026-02-28T12:00:00Z",
  "last_monitored": "2026-02-28T14:30:00Z",
  "total_sessions": 5,
  "total_clips_detected": 127
}
```

## CSV OUTPUT FORMAT

```csv
timestamp,channel,tier,matched_word,sentiment,burst_count,window_seconds,users_matched,sample_messages
2026-02-28T12:34:02Z,xqc,high,INSANE,positive,3,2,4,"user1: INSANE|user2: insane play|user3: INSANITY"
```

## KEY DECISIONS MADE
1. **Data folder location**: `.ctic/` in current directory
2. **Multi-creator mode**: Yes
3. **CLI framework**: CLI11
4. **Detection configs**: JSON files user can edit


Questions I have.

The goal is analysis, I should easily be able to travcers/ query my data incase I get karge detections spikes. 

If I addd another function for pre-processsing or spike detection I should be able to declare that in the config. 
- meaning any data should also be explicit on how we came to that conclusion. so we need our csv data to have links to the metadata or way to understand this, the meta data techincally would be one to many situation. 
-a single creator could have mutlitple types of configurations as i tweak the detection engine. I can then compare different types of detectoin. 
- meta data should capture things like, levneshtien weights, live analyusis, or hostorixcal, analaysis, vod dates, any sub confoguirations oy key words that made this setup, What I am thinking right now is I can simply create the stronger metadata, this ideally should never really change, but the tiers are the components I acn add or take away. 

I am thinking out loud here but it seems like our current seetup is already working very close to this. 

-----------------------

AN MVP STATE :


$ ctic run              → Interactive mode (prompts for options)
$ ctic add <url>        → Add creator
$ ctic list             → List all configured creators
$ ctic remove <name>    → Remove a creator 

$ ctic watch <name>
$ ctic ping           → Show system status, active sessions


------
$ ctic test   -> quick test
$ ctic status -> showcase more verbose test 

--------------

ctic ping -> checks all configs. 
ctic ping <name option> -> checks specific creator
ex. 

```json
{
  "name": "xqc",
  "channel": "xqc",
  "twitch_url": "https://twitch.tv/xqc",
  "enabled_tiers": ["high", "medium", "easy"],
  "created_at": "2026-02-28T12:00:00Z",
  "last_monitored": "2026-02-28T14:30:00Z",
  "total_sessions": 5,
  "total_clips_detected": 127
}
```

```json
{
  "name": "theburntpeanut",
  "channel": "theburntpeanut",
  "twitch_url": "https://twitch.tv/theburntpeanut",
  "enabled_tiers": ["high", "medium", "easy"],
  "created_at": "2026-02-28T12:00:00Z",
  "last_monitored": "2026-02-28T14:30:00Z",
  "total_sessions": 5,
  "total_clips_detected": 127
}
```
---

this should run mini tests 30s each and show a live chat showcase. then say config successful.

---

ctic add <url>

config added. 
--

ctic remove <name>

config removed
---

ctic list

creator 1 
creator 2
creator 3

---

ctic run 

this is the base case, it runs all configs by default

eventually we can add custom flags to this and then create a ui.

MVP is ourely just run all configs in the backend



----


$ ctic watch <name>


    │  Starting monitor...                │
    │                                     │
    │  ┌─────────────────────────────┐    │
    │  │ #xqc Chat Stream            │    │
    │  │─────────────────────────────│    │
    │  │ 12:34:01 user1: POG         │    │
    │  │ 12:34:02 user2: INSANE      │    │
    │  │ 12:34:02 user3: POGGG       │    │
    │  │ 12:34:03 user4: insane play │    │
    │  │ ─────────────────────────── │    │
    │  │ ⚡ [HIGH] BURST DETECTED     │    │
    │  │    Word: INSANE             │    │
    │  │    Burst: 3 messages / 2s   │    │
    │  │    Matched: 4 users         │    │
    │  │    Log: outputs/xqc/high/   │    │
    │  └─────────────────────────────┘    │
    └─────────────────────────────────────┘
```
