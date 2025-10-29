#!/bin/bash

# Multi-client test script for socket server

echo "Starting multi-client socket server test..."

# Check if podzim.png exists
if [ ! -f "podzim.png" ]; then
    echo "Creating test image podzim.png..."
    convert -size 800x600 gradient:blue-yellow podzim.png 2>/dev/null || {
        echo "ERROR: ImageMagick (convert) not installed."
        exit 1
    }
fi

# Start server in background
echo "Starting server on port 12345..."
./socket_srv 12345 > server_multi.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Give server time to start
sleep 2

# Start multiple clients simultaneously
echo "Starting 3 clients simultaneously..."
./socket_cl localhost 12345 400x300 > client1.log 2>&1 &
CLIENT1_PID=$!
echo "Client 1 PID: $CLIENT1_PID"

./socket_cl localhost 12345 600x400 > client2.log 2>&1 &
CLIENT2_PID=$!
echo "Client 2 PID: $CLIENT2_PID"

./socket_cl localhost 12345 800x600 > client3.log 2>&1 &
CLIENT3_PID=$!
echo "Client 3 PID: $CLIENT3_PID"

# Wait for clients to finish (timeout after 30 seconds)
echo "Waiting for clients to finish..."
sleep 30

# Force kill any remaining display windows
killall display 2>/dev/null

# Stop server
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "=== SERVER LOG ==="
cat server_multi.log

echo ""
echo "=== CLIENT 1 LOG ==="
cat client1.log

echo ""
echo "=== CLIENT 2 LOG ==="
cat client2.log

echo ""
echo "=== CLIENT 3 LOG ==="
cat client3.log

echo ""
echo "=== FILES CREATED ==="
ls -lh image.img* 2>/dev/null || echo "No image files found"

echo ""
echo "Test completed!"
echo "The server should have accepted and handled 3 clients simultaneously."
