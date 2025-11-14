CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pthread
TARGET1 = interprocess-communication
TARGET2 = socket_srv
TARGET3 = socket_cl
TARGET4 = socket_srv-test
TARGET5 = socket_cl-test

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5)

$(TARGET1): interprocess-communication.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET1) interprocess-communication.cpp

$(TARGET2): socket_srv.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET2) socket_srv.cpp

$(TARGET3): socket_cl.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET3) socket_cl.cpp

$(TARGET4): socket_srv-test.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET4) socket_srv-test.cpp

$(TARGET5): socket_cl-test.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET5) socket_cl-test.cpp

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5)

run: $(TARGET1)
	./$(TARGET1)

run-server: $(TARGET2)
	./$(TARGET2) 12345

run-client: $(TARGET3)
	./$(TARGET3) localhost 12345

test-scripts:
	chmod +x test-producer.sh test-consumer.sh test-multi.sh

.PHONY: all clean run run-server run-client test-scripts
