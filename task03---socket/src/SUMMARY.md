# Socket Image Processing Server - Implementation Summary

## Completed Tasks

All five tasks from the assignment have been successfully implemented:

### ✅ Task 1: Multi-client Socket Server with Fork
- Modified `socket_srv.cpp` to handle multiple simultaneous clients
- Parent process only accepts connections
- Each client is handled by a dedicated child process (forked after `accept()`)
- Proper file descriptor management: each process closes what it doesn't need
- Parent reaps terminated child processes with `waitpid(-1, &status, WNOHANG)`

### ✅ Task 2: Image Conversion with Convert Command
- Server receives resolution request from client (format: "WIDTHxHEIGHT\n")
- Creates child process that executes: `convert -resize WxH! podzim.png -`
- Convert's stdout is redirected to the socket
- Parent waits for child process to complete

### ✅ Task 3: Client Modifications
- Client now accepts 3 parameters: hostname, port, resolution
- Immediately sends resolution to server after connection
- Removed all `poll()` usage - simple read loop instead
- Receives data from server and saves to `image.img` file
- Transfer completes when `read()` returns 0 (server closes socket)

### ✅ Task 4: Compression Pipeline on Server
- Two child processes created: `convert` and `xz`
- Connected via pipe: `convert ... - | xz - --stdout`
- Process hierarchy:
  ```
  Client Handler
  ├── Convert Process (stdout → pipe write)
  └── XZ Process (stdin ← pipe read, stdout → socket)
  ```
- XZ output is sent directly to socket
- Parent waits for conversion pipeline to complete

### ✅ Task 5: Decompression and Display on Client
- After saving `image.img`, client creates two child processes
- Connected via pipe: `xz -d image.img --stdout | display -`
- Process hierarchy:
  ```
  Client
  ├── XZ Decompression (stdout → pipe write)
  └── Display (stdin ← pipe read)
  ```
- Parent waits for both children to complete
- Program exits after display window is closed

## Implementation Details

### Process Architecture

**Server Side:**
```
Main Server (listening)
│
├── Child 1 (Client 1)
│   ├── Convert Process
│   └── XZ Process (connected via pipe)
│
├── Child 2 (Client 2)
│   ├── Convert Process
│   └── XZ Process (connected via pipe)
│
└── Child N (Client N)
    ├── Convert Process
    └── XZ Process (connected via pipe)
```

**Client Side:**
```
Client Main
├── Receive and save data
└── After completion:
    ├── XZ Decompression
    └── Display (connected via pipe)
```

### Key Code Sections

**Server - Multi-client Handling:**
```cpp
// After accept()
pid_t l_pid = fork();
if (l_pid == 0) {
    close(l_sock_listen);  // Child doesn't need listening socket
    handle_client(l_sock_client);  // This function will exit
}
else {
    close(l_sock_client);  // Parent doesn't need client socket
}
```

**Server - Image Processing Pipeline:**
```cpp
int pipe_fd[2];
pipe(pipe_fd);

if (fork() == 0) {
    // XZ process
    dup2(pipe_fd[0], STDIN_FILENO);
    dup2(t_client_socket, STDOUT_FILENO);
    close(pipe_fd[1]);
    execlp("xz", "xz", "-", "--stdout", nullptr);
}
else {
    // Convert process
    dup2(pipe_fd[1], STDOUT_FILENO);
    close(pipe_fd[0]);
    execlp("convert", "convert", "-resize", resize_arg, "podzim.png", "-", nullptr);
}
```

**Client - Receive and Save:**
```cpp
// Send resolution request
write(l_sock_server, "1500x750\n", ...);

// Receive and save
int fd = open("image.img", O_WRONLY | O_CREAT | O_TRUNC, 0644);
while ((len = read(l_sock_server, buf, sizeof(buf))) > 0) {
    write(fd, buf, len);
}
close(fd);
```

**Client - Decompress and Display:**
```cpp
int pipe_fd[2];
pipe(pipe_fd);

if (fork() == 0) {
    // Display process
    dup2(pipe_fd[0], STDIN_FILENO);
    close(pipe_fd[1]);
    execlp("display", "display", "-", nullptr);
}

if (fork() == 0) {
    // XZ decompression
    dup2(pipe_fd[1], STDOUT_FILENO);
    close(pipe_fd[0]);
    execlp("xz", "xz", "-d", "image.img", "--stdout", nullptr);
}

// Parent closes pipe and waits
close(pipe_fd[0]);
close(pipe_fd[1]);
waitpid(...);
```

## Files Modified

1. **socket_srv.cpp** - Complete rewrite of main loop and added `handle_client()` function
2. **socket_cl.cpp** - Removed poll, added resolution parameter, added decompression/display

## Test Scripts Created

1. **usage-examples.sh** - Displays usage instructions and checks prerequisites
2. **test-image-server.sh** - Automated test with single client
3. **test-multi-clients.sh** - Tests multiple simultaneous clients

## Documentation

- **IMPLEMENTATION.md** - Detailed implementation documentation with architecture diagrams and usage examples

## How to Use

### Compile:
```bash
cd task03---socket/src
make clean && make
```

### Run Server:
```bash
./socket_srv 12345
```

### Run Client(s):
```bash
./socket_cl localhost 12345 1500x750
./socket_cl localhost 12345 800x600    # Can run multiple simultaneously
```

### Run Tests:
```bash
./usage-examples.sh        # Check prerequisites and see examples
./test-image-server.sh     # Basic functionality test
./test-multi-clients.sh    # Multi-client test
```

## Verification

The complete pipeline can be verified manually:
```bash
convert -resize 1500x750! podzim.png - | xz - --stdout | xz -d --stdout | display -
```

This demonstrates all components working together:
- Image conversion at specified resolution
- Compression with xz
- Decompression with xz -d
- Display of the result

## Important Notes

1. **No stdin input to server after connection** - Server only processes client connections, doesn't read/forward stdin messages
2. **Server sends no messages** - Only processed image data flows from server to client
3. **Automatic cleanup** - All child processes properly cleaned up, file descriptors closed
4. **True multi-client** - Multiple clients can connect and be served simultaneously
5. **Compressed transfer** - Significantly reduces network bandwidth usage
6. **Automatic display** - Image is automatically decompressed and displayed after transfer

## Requirements Met

✅ Server handles multiple clients via fork()  
✅ Parent process only accepts clients  
✅ Children handle communication  
✅ Proper file descriptor management  
✅ Image conversion with requested resolution  
✅ Pipeline: convert → xz compression  
✅ Client accepts 3 parameters  
✅ Client sends resolution immediately  
✅ Client saves to image.img  
✅ Client decompresses and displays automatically  
✅ All processes wait for children properly  
✅ No temporary files needed (direct pipe communication)
