#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <zmq.hpp>

/**
 * PRODUCT 1: The Harvester (C++ Hot Path)
 * 
 * This is a standalone, ultra-lean C++ application.
 * Its ONLY job is to harvest data (simulating Twitch IRC here)
 * and broadcast it over a ZeroMQ PUB socket instantly.
 * 
 * It runs completely decoupled from any AI logic, ensuring zero dropped frames
 * and absolute stability.
 */

int main() {
    std::cout << "[Harvester] Starting Product 1: High-Performance Ingestion Engine..." << std::endl;

    // Initialize the ZeroMQ context and socket
    zmq::context_t context(1);
    zmq::socket_t publisher(context, ZMQ_PUB);

    // Bind the publisher to a local TCP port (or IPC file)
    publisher.bind("tcp://127.0.0.1:5555");
    std::cout << "[Harvester] Bound to ZeroMQ PUB socket at tcp://127.0.0.1:5555" << std::endl;

    int message_count = 0;
    
    // The "Hot Path" Loop
    while (true) {
        // Simulate receiving a parsed Twitch chat message
        message_count++;
        
        // Simulating a burst of hype vs normal messages
        std::string username = (message_count % 5 == 0) ? "xqc_fan" : "random_user";
        std::string message = (message_count % 5 == 0) ? "POGGERS SO HYPE LETS GO!!!" : "hello chat";
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        // Create a simple JSON string (in a real app, use nlohmann/json or similar)
        std::string json_payload = "{\"user\": \"" + username + "\", " +
                                   "\"msg\": \"" + message + "\", " +
                                   "\"timestamp\": " + std::to_string(timestamp) + "}";

        // Send the topic "twitch_chat" followed by the payload
        zmq::message_t topic("twitch_chat", 11);
        zmq::message_t payload(json_payload.data(), json_payload.size());

        // We use ZMQ_SNDMORE to send the topic first, then the payload
        publisher.send(topic, zmq::send_flags::sndmore);
        publisher.send(payload, zmq::send_flags::none);

        std::cout << "[Harvester] Broadcasted -> " << json_payload << std::endl;

        // Simulate network delay (e.g. 500ms between messages)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
