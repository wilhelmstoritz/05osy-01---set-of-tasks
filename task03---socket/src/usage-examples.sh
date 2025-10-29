#!/bin/bash

# Simple usage example for the image processing socket server

cat << 'EOF'
╔═══════════════════════════════════════════════════════════════╗
║     Image Processing Socket Server - Usage Examples          ║
╚═══════════════════════════════════════════════════════════════╝

PREREQUISITES:
--------------
1. Install ImageMagick: sudo apt-get install imagemagick
2. Install xz-utils: sudo apt-get install xz-utils  
3. Have a test image named 'podzim.png' in the src directory

BASIC USAGE:
------------

Terminal 1 - Start the server:
  cd /home/vlk/working-area/github/05osy-01---set-of-tasks/task03---socket/src
  ./socket_srv 12345

Terminal 2 - Run a client:
  cd /home/vlk/working-area/github/05osy-01---set-of-tasks/task03---socket/src
  ./socket_cl localhost 12345 1500x750

The client will:
  1. Connect to the server
  2. Send resolution request (1500x750)
  3. Receive compressed image data
  4. Save to image.img
  5. Decompress and display the image
  6. Exit when you close the display window

MULTI-CLIENT TEST:
------------------
You can run multiple clients simultaneously:

Terminal 2:
  ./socket_cl localhost 12345 800x600

Terminal 3:
  ./socket_cl localhost 12345 1024x768

Terminal 4:
  ./socket_cl localhost 12345 1920x1080

Each client will be handled by a separate server process!

AUTOMATED TESTS:
----------------
Use the provided test scripts:

1. Basic test:
   ./test-image-server.sh

2. Multi-client test:
   ./test-multi-clients.sh

DEBUG MODE:
-----------
Add -d flag for verbose logging:

Server:
  ./socket_srv -d 12345

Client:
  ./socket_cl -d localhost 12345 1500x750

MANUAL PIPELINE TEST:
---------------------
Test the complete pipeline manually:
  convert -resize 1500x750! podzim.png - | xz - --stdout | xz -d --stdout | display -

This shows: convert → compress → decompress → display

CREATING TEST IMAGE:
--------------------
If you don't have podzim.png:
  convert -size 2000x1500 gradient:blue-yellow podzim.png

Or use any PNG image and rename it to podzim.png

TROUBLESHOOTING:
----------------
- If port is busy: lsof -i :12345
- Check processes: ps aux | grep socket
- Kill server: killall socket_srv
- Check image file: ls -lh image.img && file image.img

STOPPING THE SERVER:
--------------------
Type 'quit' in the server terminal, or press Ctrl+C

EOF

# Check if required tools are installed
echo ""
echo "Checking prerequisites..."

if command -v convert &> /dev/null; then
    echo "✓ ImageMagick (convert) is installed: $(convert --version | head -1)"
else
    echo "✗ ImageMagick is NOT installed. Please run: sudo apt-get install imagemagick"
fi

if command -v xz &> /dev/null; then
    echo "✓ xz is installed: $(xz --version | head -1)"
else
    echo "✗ xz is NOT installed. Please run: sudo apt-get install xz-utils"
fi

if command -v display &> /dev/null; then
    echo "✓ display (ImageMagick) is available"
else
    echo "✗ display is NOT available"
fi

if [ -f "podzim.png" ]; then
    echo "✓ Test image podzim.png exists ($(stat -c%s podzim.png) bytes)"
else
    echo "✗ Test image podzim.png not found"
    echo "  Creating test image..."
    convert -size 800x600 gradient:blue-yellow podzim.png 2>/dev/null && echo "  ✓ Created podzim.png"
fi

if [ -f "socket_srv" ] && [ -f "socket_cl" ]; then
    echo "✓ Programs are compiled"
else
    echo "✗ Programs not compiled. Run: make"
fi

echo ""
echo "Ready to start! Follow the usage examples above."
