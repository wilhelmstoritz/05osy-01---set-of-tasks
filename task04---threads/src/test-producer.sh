#!/bin/bash

# Producer client test
# Sends items to the server

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <port> [host]"
    exit 1
fi

PORT=$1
HOST=${2:-localhost}

echo "Connecting to producer-consumer server at $HOST:$PORT as PRODUCER"

{
    # Wait for "Task?" prompt and respond with "producer"
    read -r prompt
    echo "Received: $prompt"
    echo "producer"
    
    # Send some items
    for i in {1..15}; do
        item="Item_$i"
        echo "$item"
        read -r response
        echo "Sent: $item -> Response: $response"
        sleep 0.2
    done
    
    # Send quit to close connection
    echo "quit"
    
} | nc $HOST $PORT

echo "Producer client finished"
