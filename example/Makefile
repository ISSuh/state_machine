CC = g++
CXXFLAGS = -Wall -g -std=c++11
DEFINE = -DSTATE_MACHINE_DEBUG_BUILD

all: clean api_example async_server

api_example : api_example.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE)

async_server : async_server.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE)

.PHONY: clean
clean:
	rm -f api_example async_server
