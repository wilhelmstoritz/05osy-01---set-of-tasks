#!/bin/bash
# script for testing tail with multiple files

cd "$(dirname "$0")"

# clean up
rm -f stop fileA.txt fileB.txt

# create test files
echo "+++ [test] creating test files"
echo "file A: line 1" > fileA.txt
echo "file B: line 1" > fileB.txt

# start tail in background
echo "+++ [test] starting tail monitor for multiple files..."
./tail fileA.txt fileB.txt &
TAIL_PID=$!

echo "+++ [test] tail PID: $TAIL_PID"
sleep 3

# add content to file A
echo "+++ [test] adding content to fileA.txt"
echo "file A: line 2 at $(date +%T)" >> fileA.txt
sleep 2

# add content to file B
echo "+++ [test] adding content to fileB.txt"
echo "file B: line 2 at $(date +%T)" >> fileB.txt
sleep 2

# add content to both files
echo "+++ [test] adding content to both files"
echo "file A: line 3 at $(date +%T)" >> fileA.txt
echo "file B: line 3 at $(date +%T)" >> fileB.txt
sleep 2

echo "+++ [test] creating 'stop' file to terminate monitoring"
touch stop
sleep 2

echo "+++ [test] waiting for tail to finish..."
wait $TAIL_PID 2>/dev/null

echo "+++ [test] cleaning up"
rm -f stop fileA.txt fileB.txt

echo "+++ [test] done"
