#!/bin/bash

# Consumer client test
# Receives items from the server

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <port> [host] [count]"
    exit 1
fi

PORT=$1
HOST=${2:-localhost}
COUNT=${3:-15}

echo "Connecting to producer-consumer server at $HOST:$PORT as CONSUMER"

{
    # Wait for "Task?" prompt and respond with "consumer"
    read -r prompt
    echo "Received: $prompt"
    echo "consumer"
    
    # Receive items and send OK
    for i in $(seq 1 $COUNT); do
        read -r item
        echo "Received item: $item"
        echo "OK"
        sleep 0.3
    done
    
} | nc $HOST $PORT

echo "Consumer client finished"
