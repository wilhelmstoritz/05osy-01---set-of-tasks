#!/bin/bash

# Test script for multiple producers and consumers

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

PORT=$1

echo "Testing producer-consumer server with multiple clients"
echo "Make sure the server is running on port $PORT"
echo ""

# Start 2 producers in background
echo "Starting producer 1..."
./test-producer.sh $PORT &
PROD1=$!

sleep 1

echo "Starting producer 2..."
./test-producer.sh $PORT &
PROD2=$!

sleep 1

# Start 2 consumers in background
echo "Starting consumer 1..."
./test-consumer.sh $PORT localhost 10 &
CONS1=$!

sleep 1

echo "Starting consumer 2..."
./test-consumer.sh $PORT localhost 10 &
CONS2=$!

# Wait for all clients to finish
echo ""
echo "Waiting for all clients to finish..."
wait $PROD1 $PROD2 $CONS1 $CONS2

echo ""
echo "All clients finished"
