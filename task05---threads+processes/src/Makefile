CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pthread
LDFLAGS = -lrt -lpthread
TARGET1 = socket_srv
TARGET2 = socket_cl
TARGET3 = socket_cl.processes
TARGET4 = socket_srv-test
TARGET5 = socket_cl-test

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5)

$(TARGET1): socket_srv.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET1) socket_srv.cpp $(LDFLAGS)

$(TARGET2): socket_cl.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET2) socket_cl.cpp -lpthread

$(TARGET3): socket_cl.processes.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET3) socket_cl.processes.cpp

$(TARGET4): socket_srv-test.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET4) socket_srv-test.cpp $(LDFLAGS)

$(TARGET5): socket_cl-test.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET5) socket_cl-test.cpp -lpthread

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5)

run-server-shm: $(TARGET1)
	./$(TARGET1) -shm 1234

run-server-mq: $(TARGET1)
	./$(TARGET1) -mq 1234

run-client: $(TARGET2)
	./$(TARGET2) localhost 12345

.PHONY: all clean run-server-shm run-server-mq run-client
