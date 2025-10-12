#!/bin/bash

# demo script for testing tail program

cd "$(dirname "$0")"

# create test file
echo "+++ Creating test file +++"
echo "line 1: initial content" > demo_file.txt
echo "line 2: more content"   >> demo_file.txt

# start tail in background
echo "+++ starting tail monitor +++"
./tail demo_file.txt &
TAIL_PID=$!

echo "tail PID: $TAIL_PID"
sleep 3

# add more content
echo ""
echo "+++ adding content after 3 seconds +++"
echo "line 3: added at $(date +%T)" >> demo_file.txt
sleep 2

echo "+++ adding content after 5 seconds +++"
echo "line 4: added at $(date +%T)" >> demo_file.txt
sleep 2

echo "+++ adding content after 7 seconds +++"
echo "line 5: added at $(date +%T)" >> demo_file.txt
sleep 2

echo ""
echo "+++ stopping tail +++"
kill $TAIL_PID
wait $TAIL_PID 2>/dev/null

echo "+++ demo complete +++"
