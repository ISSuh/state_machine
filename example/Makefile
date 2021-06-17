CC = g++
CXXFLAGS = -Wall -g -std=c++11
DEFINE = -DSTATE_MACHINE_DEBUG_BUILD

all: clean example

example : example.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE)

.PHONY: clean
clean:
	rm -f $(OBJS) example
