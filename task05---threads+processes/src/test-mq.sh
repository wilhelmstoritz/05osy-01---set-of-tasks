#!/bin/bash
# Test script for POSIX IPC implementation - Message Queue mode

echo "==================================================================="
echo "Testing POSIX IPC with Message Queue (-mq mode)"
echo "==================================================================="
echo ""
echo "This script will:"
echo "1. Clean up any existing IPC resources"
echo "2. Start two servers on different ports (both using -mq)"
echo "3. Demonstrate that they share the same message queue"
echo ""
echo "You will need to manually:"
echo "  - Connect producer client to port 12347"
echo "  - Connect consumer client to port 12348"
echo "  - Verify data flows between them"
echo ""
echo "Press Enter to continue..."
read

# Clean up existing IPC resources
echo "Cleaning up existing IPC resources..."
rm -f /dev/shm/prodcons_* 2>/dev/null
rm -f /dev/mqueue/prodcons_* 2>/dev/null

echo ""
echo "Starting Server 1 on port 12347 (message queue mode)..."
echo "  Command: ./socket_srv -mq 12347"
echo ""
echo "In another terminal, run:"
echo "  ./socket_srv -mq 12348"
echo ""
echo "Then connect clients:"
echo "  Terminal 3: ./socket_cl localhost 12347 (choose 'producer')"
echo "  Terminal 4: ./socket_cl localhost 12348 (choose 'consumer')"
echo ""
echo "Starting server now..."
echo ""

./socket_srv -mq 12347
