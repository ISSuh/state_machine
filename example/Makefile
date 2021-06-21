CC = g++
CXXFLAGS = -Wall -g -std=c++11
DEFINE = -DSTATE_MACHINE_DEBUG_BUILD

all: clean sequence_example concurrent_example

sequence_example : sequence_example.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE) -lpthread

concurrent_example : concurrent_example.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE) -lpthread

.PHONY: clean
clean:
	rm -f sequence_example concurrent_example
