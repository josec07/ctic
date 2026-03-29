// src/network/irc_connection.cpp
#include "network/irc_connection.h"
#include <boost/asio.hpp>
#include <iostream>
#include <string>

namespace ctic {
namespace network {

IrcConnection::IrcConnection(const std::string& host, int port)
    : io_context_(), socket_(io_context_), resolver_(io_context_), connected_(false) {
}

bool IrcConnection::connect() {
    try {
        std::cout << "[IRC] Resolving irc.twitch.tv:6667..." << std::endl;
        // Resolve the host and port synchronously
        auto endpoints = resolver_.resolve("irc.twitch.tv", "6667");
        std::cout << "[IRC] Found " << endpoints.size() << " endpoints" << std::endl;
        
        // Connect synchronously
        std::cout << "[IRC] Connecting..." << std::endl;
        boost::asio::connect(socket_, endpoints);
        std::cout << "[IRC] Connected successfully!" << std::endl;
        
        connected_ = true;
        
        // Start async read loop in background
        startRead();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[IRC] Connection failed: " << e.what() << std::endl;
        return false;
    }
}

void IrcConnection::run() {
    io_context_.run();
}

std::string IrcConnection::readLine() {
    if (!connected_) {
        return "";
    }
    
    boost::asio::streambuf buffer;
    std::string line;

    boost::system::error_code error;
    boost::asio::read_until(socket_, buffer, "\r\n", error);

    if (!error) {
        std::istream is(&buffer);
        std::getline(is, line);
        // Remove trailing \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        return line;
    } else {
        return "";
    }
}

void IrcConnection::send(const std::string& message) {
    if (!connected_) {
        throw std::runtime_error("Not connected");
    }
    boost::asio::write(socket_, boost::asio::buffer(message));
}

void IrcConnection::sendPassword(const std::string& password) {
    std::string msg = "PASS " + password + "\r\n";
    send(msg);
}

void IrcConnection::sendJoin(const std::string& channel) {
    std::string msg = "JOIN #" + channel + "\r\n";
    send(msg);
}

void IrcConnection::disconnect() {
    if (connected_) {
        boost::system::error_code ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        connected_ = false;
    }
}

void IrcConnection::startRead() {
    boost::asio::async_read_until(
        socket_,
        buffer_,
        "\r\n",
        [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::string line(
                    static_cast<const char*>(buffer_.data().data()), 
                    bytes_transferred - 2);
                buffer_.consume(bytes_transferred);
                handleLine(line);
                startRead();
            } else {
                std::cerr << "Read error: " << ec.message() << std::endl;
                connected_ = false;
            }
        });
}

void IrcConnection::handleLine(const std::string& line) {
    std::cout << "Received: " << line << std::endl;
}

}
}
