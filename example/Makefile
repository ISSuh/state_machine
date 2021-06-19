CC = g++
CXXFLAGS = -Wall -g -std=c++11
DEFINE = -DSTATE_MACHINE_DEBUG_BUILD

all: clean api_example api_example_2

api_example : api_example.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE)

api_example_2 : api_example_2.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE)

.PHONY: clean
clean:
	rm -f api_example async_server
