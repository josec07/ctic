# The Digital Supply Chain: A Two-Product System

This document outlines the architecture of our local AI video processing pipeline. We have transitioned from a monolithic C++ application to a highly decoupled **Two-Product System**.

This design ensures we keep the "hot path" incredibly lean (using C++) while maintaining the ability to iterate rapidly on complex AI logic (using Python and LangChain).

## Product 1: The Harvester (C++)
**Location:** `/product1_harvester/`
**Role:** The ultra-fast, raw material gatherer.
- Connects directly to Twitch IRC (or other live data firehoses).
- Responsible *only* for network I/O and string parsing.
- Uses **ZeroMQ (PUB socket)** to instantly broadcast standardized JSON messages (like `{"user": "xqc", "msg": "POGGERS", "timestamp": 123456789}`).
- **Why C++?** Zero dropped frames, minimal memory footprint, absolute stability on the network layer.

## Product 2: The Brain (Python + AI)
**Location:** `/product2_brain/`
**Role:** The intelligent sorter and manager.
- Runs entirely independently of Product 1.
- Subscribes to Product 1's ZeroMQ stream.
- Routes data through local ONNX models (for fast, cheap filtering like sentiment analysis).
- Coordinates with large LLMs (like vLLM/Ollama on the 6800XT) via LangChain when deeper inspection is needed.
- **Why Python?** Unmatched iteration speed for AI tooling. We can rewrite logic, swap ONNX models, and change LangChain prompts without ever stopping the C++ Harvester or losing our Twitch connection.

## The Handoff (Inter-Process Communication)
The products communicate over a **ZeroMQ IPC** (or TCP loopback) connection. 
- **Latency:** ~50 microseconds.
- **Decoupling:** If Product 2 crashes while testing a new AI model, Product 1 continues pulling Twitch data and broadcasting it. The system is resilient by design.
