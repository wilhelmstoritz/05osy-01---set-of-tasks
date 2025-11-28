#!/bin/bash
# Cleanup script for POSIX IPC resources

echo "Cleaning up POSIX IPC resources..."
echo ""

# Remove semaphores
echo "Removing named semaphores..."
rm -f /dev/shm/sem.prodcons_* 2>/dev/null
rm -f /dev/shm/prodcons_* 2>/dev/null

# Remove message queues
echo "Removing message queues..."
rm -f /dev/mqueue/prodcons_* 2>/dev/null

echo ""
echo "Current IPC resources:"
echo "----------------------"
echo "Shared memory objects:"
ls -la /dev/shm/ | grep prodcons 2>/dev/null || echo "  (none)"
echo ""
echo "Message queues:"
ls -la /dev/mqueue/ | grep prodcons 2>/dev/null || echo "  (none)"
echo ""
echo "Cleanup complete!"
