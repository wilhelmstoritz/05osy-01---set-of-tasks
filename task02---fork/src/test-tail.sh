#!/bin/bash
# script for testing tail program

cd "$(dirname "$0")"

# clean up any previous stop file
rm -f stop

# create test file
echo "+++ [test] creating test files"
echo "line 1: initial content" > demo_file.txt
echo "line 2: more content"   >> demo_file.txt

# start tail in background
echo "+++ [test] starting tail monitor..."
./tail demo_file.txt &
TAIL_PID=$!

echo "+++ [test] tail PID: $TAIL_PID"
sleep 3

# add more content
echo "+++ [test] adding content after 3 seconds"
echo "line 3: added at $(date +%T)" >> demo_file.txt
sleep 2

echo "+++ [test] adding content after 5 seconds"
echo "line 4: added at $(date +%T)" >> demo_file.txt
sleep 2

echo "+++ [test] adding content after 7 seconds"
echo "line 5: added at $(date +%T)" >> demo_file.txt
sleep 2

echo "+++ [test] creating 'stop' file to terminate monitoring"
touch stop
sleep 2

echo "+++ [test] waiting for tail to finish..."
wait $TAIL_PID 2>/dev/null

echo "+++ [test] cleaning up"
rm -f stop demo_file.txt

echo "+++ [test] done"
