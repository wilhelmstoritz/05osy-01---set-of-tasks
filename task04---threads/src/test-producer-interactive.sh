#!/bin/bash

# Interactive test for producer client
# Usage: ./test-producer-interactive.sh [host] [port]

HOST=${1:-localhost}
PORT=${2:-12345}

echo "==================================================="
echo "Producer Client Test (Interactive)"
echo "==================================================="
echo "Connecting to $HOST:$PORT"
echo ""
echo "Instructions:"
echo "1. The client will ask for role - it will auto-send 'producer'"
echo "2. Then you can change speed by typing numbers (names per minute)"
echo "3. Press Ctrl+C to stop"
echo ""
echo "Starting client..."
echo ""

# Run the client and auto-select producer role
(sleep 1 && echo "producer") | ./socket_cl $HOST $PORT
