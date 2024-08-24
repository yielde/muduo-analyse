CXX = g++
CXXFLAGS =  -g -std=c++17 -Wall

TARGET = echoServer

OBJECTS = echoserver.o

LDFLAGS = -lyieldemuduo

$(TARGET) : $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJECTS)