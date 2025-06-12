CXX=g++
CXXFLAGS=-std=c++11 -I../include -I./stubs

all: run

run: test_teensyBMS
	./test_teensyBMS

test_teensyBMS: test_teensyBMS.o ../src/teensyBMS.o
	$(CXX) $(CXXFLAGS) $^ -o $@

../src/teensyBMS.o: ../src/teensyBMS.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_teensyBMS.o: test_teensyBMS.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f test_teensyBMS *.o ../src/teensyBMS.o
