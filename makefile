CXX = g++
CPPFLAGS = -std=c++11
DFLAGS = -Wall -Werror -pedantic
BFLAGS = -lboost_regex

all: rshell ls

rshell: bin
	$(CXX) $(CPPFLAGS) $(BFLAGS) $(DFLAGS) src/rshell.cc -o bin/rshell

ls: bin
	$(CXX) $(CPPFLAGS) $(DFLAGS) src/ls.cpp	-o bin/ls

bin:
	mkdir bin

clean:
	rm -rf bin/
