#!/bin/bash

# Multi-client test - starts server and multiple clients
# Usage: ./test-complete.sh [port]

PORT=${1:-12345}

echo "==================================================="
echo "Complete Producer-Consumer System Test"
echo "==================================================="
echo "This will start:"
echo "  1. Server on port $PORT"
echo "  2. Two producer clients"
echo "  3. Two consumer clients"
echo ""
echo "Press Ctrl+C to stop all processes"
echo "==================================================="
echo ""

# Cleanup function
cleanup() {
    echo ""
    echo "Stopping all processes..."
    kill $(jobs -p) 2>/dev/null
    wait
    echo "All processes stopped."
    exit 0
}

trap cleanup SIGINT SIGTERM

# Start server in background
echo "[1] Starting server on port $PORT..."
./socket_srv $PORT &
SERVER_PID=$!
sleep 2

# Start first producer
echo "[2] Starting Producer #1..."
(sleep 1 && echo "producer") | ./socket_cl localhost $PORT &
sleep 1

# Start second producer
echo "[3] Starting Producer #2..."
(sleep 1 && echo "producer") | ./socket_cl localhost $PORT &
sleep 1

# Start first consumer
echo "[4] Starting Consumer #1..."
(sleep 1 && echo "consumer") | ./socket_cl localhost $PORT &
sleep 1

# Start second consumer
echo "[5] Starting Consumer #2..."
(sleep 1 && echo "consumer") | ./socket_cl localhost $PORT &

echo ""
echo "==================================================="
echo "All clients started. Monitor the output above."
echo "Press Ctrl+C to stop all processes."
echo "==================================================="

# Wait for all background jobs
wait
