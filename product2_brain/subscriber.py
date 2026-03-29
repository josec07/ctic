import zmq
import json

def main():
    """
    PRODUCT 2: The Brain (Python AI Router)
    
    This incredibly lean Python script subscribes to the C++ Harvester.
    Because Python is doing zero network I/O with Twitch, it has 100% of its
    resources available to run ONNX models and LangChain logic.
    """
    print("[Brain] Starting Product 2: Local AI Router...")
    
    context = zmq.Context()
    subscriber = context.socket(zmq.SUB)
    subscriber.connect("tcp://127.0.0.1:5555")
    
    # Subscribe to the specific "twitch_chat" topic
    subscriber.setsockopt_string(zmq.SUBSCRIBE, "twitch_chat")
    print("[Brain] Subscribed to C++ Harvester on tcp://127.0.0.1:5555")

    try:
        while True:
            # Receive topic (discarded here) and message
            topic, payload = subscriber.recv_multipart()
            data = json.loads(payload.decode('utf-8'))
            
            # Simulated AI Lineage & Logic
            is_hype = "POGGERS" in data["msg"]
            print(f"[Brain] Received from {data['user']}: {data['msg']} | Hype Detected: {is_hype}")
            
            if is_hype:
                print(f"   -> [Action] Triggering LangChain summary for user {data['user']}...")
                # Here is where LangChain / ONNX execution would happen

    except KeyboardInterrupt:
        print("\n[Brain] Shutting down...")

if __name__ == "__main__":
    main()
