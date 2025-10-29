#!/bin/bash

# Test script for image processing socket server

echo "Testing image processing socket server..."

# Check if podzim.png exists, if not create a test image
if [ ! -f "podzim.png" ]; then
    echo "Creating test image podzim.png..."
    convert -size 800x600 gradient:blue-yellow podzim.png 2>/dev/null || {
        echo "ERROR: ImageMagick (convert) not installed. Please install it first:"
        echo "  sudo apt-get install imagemagick"
        exit 1
    }
fi

# Start server in background
echo "Starting server on port 12345..."
./socket_srv 12345 > server.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# Give server time to start
sleep 1

# Test with client
echo "Testing client with resolution 400x300..."
./socket_cl localhost 12345 400x300 > client.log 2>&1

# Wait a bit
sleep 1

# Check if image.img was created
if [ -f "image.img" ]; then
    echo "SUCCESS: image.img was created"
    ls -lh image.img
else
    echo "ERROR: image.img was not created"
fi

# Stop server
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "=== SERVER LOG ==="
cat server.log
echo ""
echo "=== CLIENT LOG ==="
cat client.log

echo ""
echo "Test completed!"
echo "You can manually test with:"
echo "  Terminal 1: ./socket_srv 12345"
echo "  Terminal 2: ./socket_cl localhost 12345 1500x750"
