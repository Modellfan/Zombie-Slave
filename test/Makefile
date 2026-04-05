CXX=g++
CXXFLAGS=-std=c++11 -I../include -I./stubs

all: run

run: test_teensyBMS test_lvdu
	./test_teensyBMS
	./test_lvdu

test_teensyBMS: test_teensyBMS.o params.o ../src/teensyBMS.o
	$(CXX) $(CXXFLAGS) $^ -o $@

../src/teensyBMS.o: ../src/teensyBMS.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_teensyBMS.o: test_teensyBMS.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

params.o: stubs/params.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test_lvdu: test_lvdu.o params.o digio.o anain.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test_lvdu.o: test_lvdu.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

digio.o: stubs/digio.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

anain.o: stubs/anain.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f test_teensyBMS test_lvdu *.o ../src/teensyBMS.o
