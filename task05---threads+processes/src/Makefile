CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -pthread
TARGET1 = socket_srv
TARGET2 = socket_cl

all: $(TARGET1) $(TARGET2)

$(TARGET1): socket_srv.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET1) socket_srv.cpp

$(TARGET2): socket_cl.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET2) socket_cl.cpp

clean:
	rm -f $(TARGET1) $(TARGET2)

run-server: $(TARGET1)
	./$(TARGET1) 12345

run-client: $(TARGET2)
	./$(TARGET2) localhost 12345

.PHONY: all clean run-server run-client
