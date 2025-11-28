#!/bin/bash
# cleanup script for POSIX IPC resources

echo "cleaning up POSIX IPC resources..."
echo ""

# remove named semaphores (on Linux they are in /dev/shm with sem. prefix)
echo "removing named semaphores..."
rm -f /dev/shm/sem.prodcons_mutex 2>/dev/null
rm -f /dev/shm/sem.prodcons_empty 2>/dev/null
rm -f /dev/shm/sem.prodcons_full 2>/dev/null

# remove shared memory
echo "removing shared memory..."
rm -f /dev/shm/prodcons_shm 2>/dev/null

# remove message queues
echo "removing message queues..."
rm -f /dev/mqueue/prodcons_mq 2>/dev/null

echo ""
echo "current IPC resources:"
echo "----------------------"
echo "semaphores:"
ls -la /dev/shm/sem.prodcons* 2>/dev/null || echo "  (none)"
echo ""
echo "shared memory objects:"
ls -la /dev/shm/prodcons* 2>/dev/null || echo "  (none)"
echo ""
echo "message queues:"
ls -la /dev/mqueue/prodcons* 2>/dev/null || echo "  (none)"
echo ""
echo "cleanup complete!"
