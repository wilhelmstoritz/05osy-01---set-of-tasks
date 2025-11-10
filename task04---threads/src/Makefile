CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pthread
TARGET = interprocess-communication

all: $(TARGET)

$(TARGET): interprocess-communication.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) interprocess-communication.cpp

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
