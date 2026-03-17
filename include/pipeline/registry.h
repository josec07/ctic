#ifndef CTIC_PIPELINE_REGISTRY_H
#define CTIC_PIPELINE_REGISTRY_H

#include "node.h"
#include <map>
#include <functional>
#include <iostream>
#include <dlfcn.h>  // For Unix/Linux
#include <filesystem>

namespace ctic {
namespace pipeline {

// Plugin handle for dynamic libraries
class PluginHandle {
public:
    void* handle;
    std::string path;
    std::vector<std::string> node_types;
    
    // Function pointers from plugin
    std::function<const char*()> get_version;
    std::function<const char**()> get_types;
    std::function<INode*(const char*)> create_node;
    std::function<void(INode*)> destroy_node;
    
    PluginHandle() : handle(nullptr) {}
    ~PluginHandle() {
        if (handle) {
            dlclose(handle);
        }
    }
};

// Node registry manages all available node types
class NodeRegistry {
private:
    // Built-in node factories
    std::map<std::string, NodeFactory> builtin_factories;
    
    // Plugin handles
    std::map<std::string, std::unique_ptr<PluginHandle>> plugins;
    
    // Node type to plugin mapping
    std::map<std::string, std::string> node_to_plugin;
    
public:
    NodeRegistry() = default;
    
    // Register built-in node type
    void registerBuiltin(const std::string& type, NodeFactory factory) {
        builtin_factories[type] = factory;
    }
    
    // Load plugin from .so/.dll file
    bool loadPlugin(const std::string& path) {
        auto plugin = std::make_unique<PluginHandle>();
        plugin->path = path;
        
        // Load the shared library
        plugin->handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!plugin->handle) {
            std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
            return false;
        }
        
        // Get function pointers
        plugin->get_version = (const char*(*)())dlsym(plugin->handle, "get_plugin_version");
        plugin->get_types = (const char**(*)())dlsym(plugin->handle, "get_node_types");
        plugin->create_node = (INode*(*)(const char*))dlsym(plugin->handle, "create_node");
        plugin->destroy_node = (void(*)(INode*))dlsym(plugin->handle, "destroy_node");
        
        if (!plugin->create_node || !plugin->destroy_node) {
            std::cerr << "Plugin missing required functions" << std::endl;
            return false;
        }
        
        // Get node types from plugin
        if (plugin->get_types) {
            const char** types = plugin->get_types();
            for (int i = 0; types[i] != nullptr; i++) {
                plugin->node_types.push_back(types[i]);
                node_to_plugin[types[i]] = path;
            }
        }
        
        plugins[path] = std::move(plugin);
        return true;
    }
    
    // Load all plugins from directory
    void loadPluginsFromDirectory(const std::string& dir) {
        namespace fs = std::filesystem;
        
        if (!fs::exists(dir)) {
            return;
        }
        
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                auto path = entry.path();
                if (path.extension() == ".so" || path.extension() == ".dll") {
                    std::cout << "Loading plugin: " << path << std::endl;
                    loadPlugin(path.string());
                }
            }
        }
    }
    
    // Create node instance
    std::unique_ptr<INode> createNode(const std::string& type) {
        // Check built-in factories first
        auto builtin_it = builtin_factories.find(type);
        if (builtin_it != builtin_factories.end()) {
            return builtin_it->second();
        }
        
        // Check plugins
        auto plugin_it = node_to_plugin.find(type);
        if (plugin_it != node_to_plugin.end()) {
            auto& plugin = plugins[plugin_it->second];
            if (plugin && plugin->create_node) {
                return std::unique_ptr<INode>(plugin->create_node(type.c_str()));
            }
        }
        
        return nullptr;
    }
    
    // List all available node types
    std::vector<std::string> getAvailableTypes() const {
        std::vector<std::string> types;
        
        // Built-in types
        for (const auto& [type, _] : builtin_factories) {
            types.push_back(type);
        }
        
        // Plugin types
        for (const auto& [type, _] : node_to_plugin) {
            types.push_back(type);
        }
        
        return types;
    }
};

} // namespace pipeline
} // namespace ctic

#endif // CTIC_PIPELINE_REGISTRY_H