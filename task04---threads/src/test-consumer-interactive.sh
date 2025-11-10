#!/bin/bash

# Interactive test for consumer client
# Usage: ./test-consumer-interactive.sh [host] [port]

HOST=${1:-localhost}
PORT=${2:-12345}

echo "==================================================="
echo "Consumer Client Test (Interactive)"
echo "==================================================="
echo "Connecting to $HOST:$PORT"
echo ""
echo "Instructions:"
echo "1. The client will ask for role - it will auto-send 'consumer'"
echo "2. It will then receive and display names from server"
echo "3. Press Ctrl+C to stop"
echo ""
echo "Starting client..."
echo ""

# Run the client and auto-select consumer role
(sleep 1 && echo "consumer") | ./socket_cl $HOST $PORT
