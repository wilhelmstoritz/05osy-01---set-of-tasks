# Socket Server and Client - Image Processing Implementation

## Overview

This implementation extends the basic socket server/client to handle:
1. Multiple simultaneous clients using fork()
2. Image processing using ImageMagick's convert command
3. Compression using xz
4. Decompression and display on the client side

## Architecture

### Server (socket_srv.cpp)

The server uses a multi-process architecture:

1. **Parent Process**: 
   - Listens for incoming connections on a specified port
   - Accepts new client connections
   - Forks a child process for each client
   - Reaps terminated child processes

2. **Client Handler Child Process** (handle_client function):
   - Receives resolution request from client (format: "WIDTHxHEIGHT\n")
   - Creates two more child processes connected via pipe:
     - **Convert Process**: Executes `convert -resize WxH! podzim.png -`
     - **XZ Process**: Executes `xz - --stdout`
   - Pipeline: convert → pipe → xz → socket
   - Waits for compression to complete before closing connection

### Client (socket_cl.cpp)

The client workflow:

1. Connects to server with three parameters: hostname, port, resolution
2. Immediately sends resolution request to server
3. Receives compressed image data and saves to `image.img`
4. After receiving all data, creates two child processes:
   - **XZ Decompression**: Executes `xz -d image.img --stdout`
   - **Display Process**: Executes `display -`
5. Pipeline: xz -d → pipe → display
6. Waits for display window to be closed

## Key Implementation Details

### Fork and Process Management

- **Server parent**: Only accepts connections, never communicates with clients
- **Server children**: Each handles one client exclusively
- **Proper cleanup**: Each process closes file descriptors it doesn't need
- **Wait for children**: Parent processes wait for their children to complete

### Pipe Communication

- Pipes created with `pipe(fd)`
- File descriptors redirected using `dup2()`
- Unused pipe ends closed in each process
- Pipeline: Process A stdout → pipe → Process B stdin

### Exec Commands

Server side:
```bash
convert -resize 1500x750! podzim.png - | xz - --stdout
```

Client side:
```bash
xz -d image.img --stdout | display -
```

## Usage

### Prerequisites

Install required tools:
```bash
sudo apt-get install imagemagick xz-utils
```

Create a test image named `podzim.png` in the src directory, or use the test script which creates one automatically.

### Running the Server

```bash
./socket_srv <port>
./socket_srv -d <port>  # Debug mode
```

Example:
```bash
./socket_srv 12345
```

The server will:
- Listen on the specified port
- Accept multiple simultaneous clients
- Process image conversion requests
- Log client connections and disconnections

### Running the Client

```bash
./socket_cl <hostname> <port> <resolution>
./socket_cl -d <hostname> <port> <resolution>  # Debug mode
```

Example:
```bash
./socket_cl localhost 12345 1500x750
```

The client will:
- Connect to the server
- Send the resolution request
- Receive the compressed image
- Save it to `image.img`
- Decompress and display the image
- Exit after the display window is closed

### Testing

Use the provided test script:
```bash
./test-image-server.sh
```

Or test manually in separate terminals:

Terminal 1 (Server):
```bash
./socket_srv 12345
```

Terminal 2 (Client 1):
```bash
./socket_cl localhost 12345 1500x750
```

Terminal 3 (Client 2):
```bash
./socket_cl localhost 12345 800x600
```

## Features Implemented

✅ **Task 1**: Multi-client support with fork()
- Parent process only accepts connections
- Each client handled by separate child process
- Proper file descriptor management

✅ **Task 2**: Image conversion with convert command
- Server executes convert with requested resolution
- Output piped directly to socket via xz compression
- Parent waits for child process completion

✅ **Task 3**: Client accepts resolution parameter
- Three parameters: host, port, resolution
- Sends resolution immediately after connection
- Removed poll() usage
- Saves received data to image.img

✅ **Task 4**: Compression pipeline on server
- Two child processes: convert and xz
- Connected via pipe
- XZ output sent to socket

✅ **Task 5**: Decompression and display on client
- Two child processes: xz -d and display
- Connected via pipe
- Parent waits for both children

## File Descriptor Management

Critical for proper operation:

**Server Parent**:
- Keeps: listening socket, stdin
- Closes: client socket (after fork)

**Server Client Handler Child**:
- Keeps: client socket
- Closes: listening socket

**Server Convert Process**:
- Keeps: pipe write end
- Closes: pipe read end, client socket
- Redirects: stdout → pipe

**Server XZ Process**:
- Keeps: pipe read end, client socket
- Closes: pipe write end
- Redirects: stdin → pipe, stdout → socket

**Client XZ Process**:
- Keeps: pipe write end
- Closes: pipe read end
- Redirects: stdout → pipe

**Client Display Process**:
- Keeps: pipe read end
- Closes: pipe write end
- Redirects: stdin → pipe

## Command Pipeline Verification

You can test the entire pipeline manually:
```bash
convert -resize 1500x750! podzim.png - | xz - --stdout | xz -d --stdout | display -
```

This demonstrates: conversion → compression → decompression → display

## Troubleshooting

**Server won't start**:
- Port already in use: `lsof -i :<port>` to find and kill the process
- Permission denied: Use port > 1024 for non-root users

**Client can't connect**:
- Check server is running: `ps aux | grep socket_srv`
- Check firewall settings
- Verify hostname and port

**Image not displayed**:
- Check ImageMagick is installed: `convert --version`
- Check xz is installed: `xz --version`
- Verify image.img was created: `ls -lh image.img`
- Check file is not empty: `file image.img`

**Display process hangs**:
- Ensure X11 display is available
- Try running `display` manually to test

## Notes

- The server no longer accepts stdin input after connection (except for 'quit' command)
- Each client gets an independent process, allowing true parallel processing
- The compressed image transfer reduces network bandwidth significantly
- Image display is automatic after transfer completes
