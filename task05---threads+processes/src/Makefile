CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pthread
LDFLAGS = -lrt -lpthread
TARGET1 = socket_srv
TARGET2 = socket_cl
TARGET3 = socket_cl.processes

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): socket_srv.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET1) socket_srv.cpp $(LDFLAGS)

$(TARGET2): socket_cl.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET2) socket_cl.cpp -lpthread

$(TARGET3): socket_cl.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET3) socket_cl.cpp

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3)

run-server-shm: $(TARGET1)
	./$(TARGET1) -shm 12345

run-server-mq: $(TARGET1)
	./$(TARGET1) -mq 12345

run-client: $(TARGET2)
	./$(TARGET2) localhost 12345

.PHONY: all clean run-server-shm run-server-mq run-client
