#!/bin/bash
# cleanup script for POSIX IPC resources

echo "cleaning up POSIX IPC resources..."
echo ""

# remove semaphores
echo "removing named semaphores..."
rm -f /dev/shm/sem.prodcons_* 2>/dev/null
rm -f /dev/shm/prodcons_* 2>/dev/null

# remove message queues
echo "removing message queues..."
rm -f /dev/mqueue/prodcons_* 2>/dev/null

echo ""
echo "current IPC resources:"
echo "----------------------"
echo "shared memory objects:"
ls -la /dev/shm/ | grep prodcons 2>/dev/null || echo "  (none)"
echo ""
echo "message queues:"
ls -la /dev/mqueue/ | grep prodcons 2>/dev/null || echo "  (none)"
echo ""
echo "cleanup complete!"
