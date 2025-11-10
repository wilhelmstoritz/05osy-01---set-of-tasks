#!/bin/bash

PORT=12347

echo "=== Test Producer-Consumer Socket Server ==="
echo ""

# Cleanup
echo "1. Cleaning up old processes..."
killall socket_srv 2>/dev/null
killall socket_cl 2>/dev/null
sleep 1

# Start server
echo "2. Starting server on port $PORT..."
cd /home/vlk/working-area/github/05osy-01---set-of-tasks/task04---threads/src
./socket_srv $PORT > server.log 2>&1 &
SERVER_PID=$!
echo "   Server PID: $SERVER_PID"
sleep 2

# Check server is running
if ! ps -p $SERVER_PID > /dev/null; then
    echo "   ERROR: Server failed to start!"
    cat server.log
    exit 1
fi
echo "   Server is running!"

# Test 1: Producer fills buffer
echo ""
echo "3. Test 1: Producer fills buffer (should stop after buffer full)..."
echo "   Starting producer client..."
(echo "producer" | timeout 15 ./socket_cl localhost $PORT > producer1.log 2>&1) &
PROD_PID=$!
sleep 12

echo "   Checking producer log..."
tail -15 producer1.log
echo ""
echo "   Server buffer status:"
tail -10 server.log | grep -E "(Produced|Buffer|waiting)"

# Test 2: Consumer empties buffer
echo ""
echo "4. Test 2: Consumer empties buffer..."
echo "   Starting consumer client..."
(echo "consumer" | timeout 15 ./socket_cl localhost $PORT > consumer1.log 2>&1) &
CONS_PID=$!
sleep 12

echo "   Checking consumer log..."
tail -15 consumer1.log
echo ""
echo "   Server buffer status:"
tail -10 server.log | grep -E "(Consumed|Buffer|waiting)"

# Test 3: Multiple producers and consumers
echo ""
echo "5. Test 3: Multiple producers and consumers in parallel..."
(echo "producer" | timeout 10 ./socket_cl localhost $PORT > producer2.log 2>&1) &
sleep 1
(echo "producer" | timeout 10 ./socket_cl localhost $PORT > producer3.log 2>&1) &
sleep 1
(echo "consumer" | timeout 10 ./socket_cl localhost $PORT > consumer2.log 2>&1) &
sleep 1
(echo "consumer" | timeout 10 ./socket_cl localhost $PORT > consumer3.log 2>&1) &

echo "   Running for 8 seconds..."
sleep 8

echo ""
echo "   Final server log:"
tail -20 server.log

# Cleanup
echo ""
echo "6. Cleaning up..."
killall socket_srv 2>/dev/null
killall socket_cl 2>/dev/null

echo ""
echo "=== Test Complete ==="
echo "Check logs: server.log, producer*.log, consumer*.log"
