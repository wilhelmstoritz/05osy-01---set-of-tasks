# POSIX IPC Implementation - Producer-Consumer

## Changes Made

### 1. Process-based Architecture
- Replaced `pthread_create` with `fork()` for each client connection
- Each client runs in a separate process instead of thread
- Child processes exit after handling client

### 2. Named POSIX Semaphores
- Replaced anonymous semaphores (`sem_init`) with named semaphores (`sem_open`)
- Semaphore names:
  - `/prodcons_mutex` - mutual exclusion for critical sections
  - `/prodcons_empty` - counts empty buffer slots (SHM mode only)
  - `/prodcons_full` - counts full buffer slots (SHM mode only)
- Proper initialization with `sem_unlink` before creation to handle restarts
- Cleanup with `sem_close` and `sem_unlink`

### 3. Shared Memory or Message Queue
Two modes supported via command-line switch:

#### Shared Memory Mode (`-shm`)
- Uses POSIX shared memory (`shm_open`, `ftruncate`, `mmap`)
- Implements circular buffer in shared memory
- Structure: `shared_buffer` with char arrays (no pointers!)
- Name: `/prodcons_shm`
- All three semaphores used for synchronization

#### Message Queue Mode (`-mq`)
- Uses POSIX message queue (`mq_open`)
- Name: `/prodcons_mq`
- Queue capacity: 10 messages, max size: 256 bytes
- Only mutex semaphore needed (queue has built-in synchronization)

## Compilation

```bash
cd task05---threads+processes/src
make
# or manually:
g++ -o socket_srv socket_srv.cpp -lrt -lpthread
g++ -o socket_cl socket_cl.cpp -lpthread
```

Note: `-lrt` is required for POSIX IPC functions (semaphores, shared memory, message queue)

## Testing Multiple Servers

### Test 1: Two servers with shared memory (on different ports)

Terminal 1:
```bash
./socket_srv -shm 12345
```

Terminal 2:
```bash
./socket_srv -shm 12346
```

Terminal 3 (Producer to first server):
```bash
./socket_cl localhost 12345
# Choose: producer
# Enter names per minute rate
```

Terminal 4 (Consumer from second server):
```bash
./socket_cl localhost 12346
# Choose: consumer
```

**Expected result**: Servers share the same IPC resources, so data produced on port 12345 can be consumed from port 12346!

### Test 2: Two servers with message queue

Terminal 1:
```bash
./socket_srv -mq 12347
```

Terminal 2:
```bash
./socket_srv -mq 12348
```

Terminal 3 & 4: Same as above but with different ports

### Test 3: Mixed mode (should NOT share data)

If you run one server with `-shm` and another with `-mq`, they use different IPC mechanisms and will NOT share data (which is expected).

## Important Notes

### IPC Resource Management
1. **Clean shutdown**: Type `quit` in server to properly cleanup IPC resources
2. **Unclean shutdown**: If server crashes, IPC resources may persist
3. **Manual cleanup** (if needed):
```bash
# List IPC resources
ls /dev/shm/
ls /dev/mqueue/

# Remove if needed
rm /dev/shm/prodcons_*
rm /dev/mqueue/prodcons_*

# Or use:
ipcs
ipcrm
```

### Semaphore Initialization
- Server unlocks previous semaphores with `sem_unlink()` before creating new ones
- This ensures clean restart even after crashes
- First server to start initializes the resources
- Subsequent servers on different ports connect to same resources

### Message Queue Limits
Check system limits:
```bash
cat /proc/sys/fs/mqueue/msg_max      # max messages in queue
cat /proc/sys/fs/mqueue/msgsize_max  # max message size
```

To adjust (requires root):
```bash
sudo sysctl fs.mqueue.msg_max=20
sudo sysctl fs.mqueue.msgsize_max=512
```

### Shared Memory Buffer Design
- No pointers in shared memory (won't work across processes)
- Fixed-size char arrays for strings
- Indices stored directly in shared memory
- Circular buffer implementation

## Architecture Differences

### Shared Memory Mode
```
Producer Client -> Process -> sem_wait(empty) -> sem_wait(mutex) 
                                -> insert to SHM -> sem_post(mutex) 
                                -> sem_post(full)

Consumer Client -> Process -> sem_wait(full) -> sem_wait(mutex) 
                                -> remove from SHM -> sem_post(mutex) 
                                -> sem_post(empty)
```

### Message Queue Mode
```
Producer Client -> Process -> mq_send (blocks if queue full)

Consumer Client -> Process -> mq_receive (blocks if queue empty)
```

## Testing Checklist

- [ ] Start server 1 with `-shm` on port 12345
- [ ] Start server 2 with `-shm` on port 12346
- [ ] Connect producer client to server 1
- [ ] Connect consumer client to server 2
- [ ] Verify data flows between different ports (shared IPC!)
- [ ] Repeat with `-mq` mode
- [ ] Test clean shutdown with `quit` command
- [ ] Verify IPC cleanup (no resources left in /dev/shm or /dev/mqueue)
- [ ] Test server restart (should cleanly reinitialize)
- [ ] Monitor with `ipcs` command during operation
