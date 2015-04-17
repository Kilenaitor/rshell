CXX = g++
CPPFLAGS = -std=c++11
DFLAGS = -Wall -Werror -pedantic

all: rshell

rshell:
	mkdir bin
	$(CXX) $(CPPFLAGS) $(DFLAGS) src/rshell.cc -o bin/rshell
	
clean:
	rm -rf bin/
