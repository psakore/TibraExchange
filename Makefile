CXX = g++
CXXFLAGS = -std=c++17 -I. #-I./boost_1_83_0/install/include -L./boost_1_83_0/install/lib
LDFLAGS = -lboost_unit_test_framework -lrt

SRCS = MyExchange.cpp Test.cpp
OBJS = $(SRCS:.cpp=.o)
EXECUTABLE = test.out

.PHONY: all clean test

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJS)

test: $(EXECUTABLE)
	./$(EXECUTABLE)

