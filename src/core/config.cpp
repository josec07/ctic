#include "../../include/core/config.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <ctime>
#include <dirent.h>
#include <algorithm>

namespace ctic {
namespace core {


using json = nlohmann::json;

ConfigManager::ConfigManager() {
    ctic_dir_ = ".ctic";
}

bool ConfigManager::ensure_ctic_dir() {
    std::string cmd = "mkdir -p " + ctic_dir_ + "/creators " + ctic_dir_ + "/outputs " + ctic_dir_ + "/detectors " + ctic_dir_ + "/profiles";
    return system(cmd.c_str()) == 0;
}

std::string ConfigManager::get_creators_dir() {
    return ctic_dir_ + "/creators";
}

std::string ConfigManager::get_profiles_dir() {
    return ctic_dir_ + "/profiles";
}

CreatorConfig ConfigManager::load_creator(const std::string& name) {
    CreatorConfig config;
    std::string filepath = ctic_dir_ + "/creators/" + name + ".json";
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return config;
    }
    
    try {
        json j;
        file >> j;
        file.close();
        
        config.name = name;
        config.channel = j.value("channel", "");
        config.twitch_url = j.value("twitch_url", "");
        config.profile = j.value("profile", "balanced");
        config.detector_config_id = j.value("detector_config", "");
        config.created_at = j.value("created_at", "");
        config.last_monitored = j.value("last_monitored", "");
        config.total_sessions = j.value("total_sessions", 0);
        config.total_clips_detected = j.value("total_clips_detected", 0);
        
        if (j.contains("enabled_tiers") && j["enabled_tiers"].is_array()) {
            for (const auto& tier : j["enabled_tiers"]) {
                if (tier.is_string()) {
                    config.enabled_tiers.push_back(tier.get<std::string>());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing creator config for " << name << ": " << e.what() << std::endl;
    }
    
    return config;
}

bool ConfigManager::save_creator(const CreatorConfig& config) {
    ensure_ctic_dir();
    
    std::string filepath = ctic_dir_ + "/creators/" + config.name + ".json";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    try {
        json j;
        j["name"] = config.name;
        j["channel"] = config.channel;
        j["twitch_url"] = config.twitch_url;
        j["profile"] = config.profile;
        j["enabled_tiers"] = config.enabled_tiers;
        j["detector_config"] = config.detector_config_id;
        j["created_at"] = config.created_at;
        j["last_monitored"] = config.last_monitored;
        j["total_sessions"] = config.total_sessions;
        j["total_clips_detected"] = config.total_clips_detected;
        
        file << j.dump(2);
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving creator config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::remove_creator(const std::string& name) {
    std::string filepath = ctic_dir_ + "/creators/" + name + ".json";
    return std::remove(filepath.c_str()) == 0;
}

std::vector<std::string> ConfigManager::list_creators() {
    std::vector<std::string> creators;
    std::string creators_dir = ctic_dir_ + "/creators";
    
    DIR* dir = opendir(creators_dir.c_str());
    if (!dir) {
        return creators;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 5 && filename.substr(filename.length() - 5) == ".json") {
            creators.push_back(filename.substr(0, filename.length() - 5));
        }
    }
    
    closedir(dir);
    std::sort(creators.begin(), creators.end());
    return creators;
}

bool ConfigManager::creator_exists(const std::string& name) {
    std::string filepath = ctic_dir_ + "/creators/" + name + ".json";
    std::ifstream file(filepath);
    return file.good();
}

std::string ConfigManager::get_output_dir(const std::string& creator, const std::string& tier) {
    return ctic_dir_ + "/outputs/" + creator + "/" + tier;
}

json ConfigManager::load_wordlist(const std::string& tier_name) {
    // Try to load from external JSON first
    std::string wordlist_path = "config/wordlists/" + tier_name + ".json";
    std::ifstream file(wordlist_path);
    
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            file.close();
            return j;
        } catch (const std::exception& e) {
            std::cerr << "Error loading wordlist from " << wordlist_path << ": " << e.what() << std::endl;
        }
    }
    
    // Fallback to built-in word lists (embedded for backward compatibility)
    json fallback;
    fallback["tier"] = tier_name;
    fallback["version"] = "1.0";
    
    if (tier_name == "high") {
        fallback["words"] = {
            "POG", "POGGERS", "POGCHAMP", "INSANE", "LETS GO", "CLUTCH", "ACE", "PENTA",
            "CRACKED", "GOATED", "DIFF", "FINAL", "K", "GOD", "CRITICAL", "MONSTER",
            "LEGENDARY", "NUTS", "WTF", "OMFG", "SHEESH", "DAMN", "WHOA", "NO WAY",
            "YOOO", "GOAT", "FINAL BOSS", "200IQ", "BIG BRAIN", "ONE TAP", "SPEEDRUN",
            "WORLD RECORD", "HES HIM", "DIFFERENT BREED", "ABSOLUTE UNIT", "CINEMATIC",
            "MOVIE", "THEATRE", "MAIN CHARACTER", "PROTAGONIST", "HIM", "HERO", "GOATED"
        };
        fallback["metadata"] = {{"intensity", 5}, {"category", "positive"}};
    } else if (tier_name == "high_negative" || tier_name == "high-negative") {
        fallback["words"] = {
            "L", "LMAO", "LFMAO", "RIP", "F", "F IN CHAT", "LOST", "BOT", "DOG",
            "TRASH", "CRINGE", "OMEGALUL", "HUHH", "AIM ASSIST", "WORST", "FAILED",
            "CHOKE", "BRUH", "NOT LIKE THIS", "NOT THE WAY", "NOOO", "YIKES",
            "EMBARRASSING", "OOF", "SKILL ISSUE", "REPORT", "UNINSTALL", "FIX YOUR GAME",
            "LITERALLY UNPLAYABLE", "WHAT WAS THAT", "INTING", "THROWING", "GRIEFING",
            "NPC", "HARDSTUCK", "BOOSTED", "CARRIED", "BAD", "TERRIBLE", "HORRIBLE"
        };
        fallback["metadata"] = {{"intensity", 5}, {"category", "negative"}};
    } else if (tier_name == "medium") {
        fallback["words"] = {
            "W", "GG", "GGS", "EZ", "NICE", "SHEESH", "DAMN", "OH", "YT", "PEPE",
            "MONKA", "KEKW", "BASED", "TRUE", "REAL", "MOGGED", "OWNED", "SAUCE",
            "CLEAN", "NASTY", "P", "VP", "POGU", "POGGIES", "KEKL", "KEKWAIT",
            "MONKAGUN", "PEPELA", "FEELSMAN", "SAVAGE", "HEAT", "ON FIRE", "COOKING",
            "LETHAL", "DEADLY", "VICIOUS", "CRUEL", "UNFAIR", "UNMATCHED", "INHUMAN"
        };
        fallback["metadata"] = {{"intensity", 3}, {"category", "positive"}};
    } else if (tier_name == "easy") {
        fallback["words"] = {
            "lol", "wow", "true", "real", "?", "??", "xd", "lmao", "ok", "sure",
            "yeah", "no", "yes", "ok", "hmm", "oof", "rip", "loll", "lool", "lmaoo",
            "bruh", "bro", "man", "dude", "fr", "for real", "actually", "literally",
            "honestly", "probably", "maybe", "fr fr", "no cap", "no cap fr", "bet",
            "say less", "facts", "fax", "printer", "slaps", "hard", "valid", "fair"
        };
        fallback["metadata"] = {{"intensity", 1}, {"category", "neutral"}};
    }
    
    return fallback;
}

TierConfig ConfigManager::load_tier_config(const std::string& tier_name) {
    TierConfig config;
    config.tier_name = tier_name;
    
    // Load word list from JSON
    json wordlist = load_wordlist(tier_name);
    
    if (wordlist.contains("words") && wordlist["words"].is_array()) {
        for (const auto& word : wordlist["words"]) {
            if (word.is_string()) {
                config.words.push_back(word.get<std::string>());
            }
        }
    }
    
    // Load tier-specific thresholds from engine config
    std::string engine_path = "config/engine.json";
    std::ifstream engine_file(engine_path);
    if (engine_file.is_open()) {
        try {
            json engine;
            engine_file >> engine;
            engine_file.close();
            
            if (engine.contains("tier_configs") && engine["tier_configs"].contains(tier_name)) {
                json tier_config = engine["tier_configs"][tier_name];
                config.burst_threshold = tier_config.value("burst_threshold", 3);
                config.cooldown_seconds = tier_config.value("cooldown_seconds", 60);
                config.require_unique_users = tier_config.value("require_unique_users", 2);
                config.min_word_length = tier_config.value("min_word_length", 3);
                config.window_seconds = tier_config.value("window_seconds", 30);
                config.levenshtein_threshold = tier_config.value("levenshtein_threshold", 0.8);
                config.use_levenshtein = tier_config.value("use_levenshtein", true);
                
                return config;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading engine config: " << e.what() << std::endl;
        }
    }
    
    // Default thresholds if engine.json not found
    if (tier_name == "high") {
        config.burst_threshold = 3;
        config.min_word_length = 3;
        config.cooldown_seconds = 60;
        config.require_unique_users = 2;
    } else if (tier_name == "high_negative" || tier_name == "high-negative") {
        config.burst_threshold = 5;
        config.min_word_length = 1;
        config.cooldown_seconds = 45;
        config.require_unique_users = 3;
    } else if (tier_name == "medium") {
        config.burst_threshold = 8;
        config.min_word_length = 1;
        config.cooldown_seconds = 90;
        config.require_unique_users = 4;
    } else if (tier_name == "easy") {
        config.burst_threshold = 15;
        config.min_word_length = 2;
        config.cooldown_seconds = 120;
        config.require_unique_users = 5;
    }
    
    return config;
}

TierConfig ConfigManager::load_profile_tier(const std::string& profile_name, const std::string& tier_name) {
    TierConfig base_config = load_tier_config(tier_name);
    
    std::string profile_path = ctic_dir_ + "/profiles/" + profile_name + ".json";
    std::ifstream file(profile_path);
    if (!file.is_open()) {
        return base_config;
    }
    
    try {
        json profile;
        file >> profile;
        file.close();
        
        if (profile.contains("tiers") && profile["tiers"].contains(tier_name)) {
            json tier_override = profile["tiers"][tier_name];
            
            if (tier_override.contains("burst_threshold")) {
                base_config.burst_threshold = tier_override["burst_threshold"];
            }
            if (tier_override.contains("min_word_length")) {
                base_config.min_word_length = tier_override["min_word_length"];
            }
            if (tier_override.contains("cooldown_seconds")) {
                base_config.cooldown_seconds = tier_override["cooldown_seconds"];
            }
            if (tier_override.contains("require_unique_users")) {
                base_config.require_unique_users = tier_override["require_unique_users"];
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading profile tier: " << e.what() << std::endl;
    }
    
    return base_config;
}

DetectorConfig ConfigManager::load_detector_config(const std::string& detector_id) {
    DetectorConfig config;
    config.id = detector_id;
    
    // Try to load from engine.json
    std::string engine_path = "config/engine.json";
    std::ifstream engine_file(engine_path);
    
    if (engine_file.is_open()) {
        try {
            json engine;
            engine_file >> engine;
            engine_file.close();
            
            if (engine.contains("detectors") && engine["detectors"].is_array()) {
                for (const auto& det : engine["detectors"]) {
                    if (det.value("id", "") == detector_id) {
                        config.name = det.value("name", detector_id);
                        config.algorithm = det.value("type", "fuzzy_match");
                        
                        if (det.contains("config")) {
                            json det_config = det["config"];
                            config.similarity_threshold = det_config.value("similarity_threshold", 0.8);
                        }
                        break;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading detector config: " << e.what() << std::endl;
        }
    }
    
    // Default detector configuration
    if (config.name.empty()) {
        config.name = "Default Levenshtein Burst";
        config.algorithm = "fuzzy_match";
        config.similarity_threshold = 0.8;
    }
    
    // Load all tier configurations
    config.tiers = {
        load_tier_config("high"),
        load_tier_config("high-negative"),
        load_tier_config("medium"),
        load_tier_config("easy")
    };
    
    return config;
}

std::string format_timestamp(std::chrono::system_clock::time_point tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

}
}
