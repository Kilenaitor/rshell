CXX = g++
CPPFLAGS = -std=c++11
DFLAGS = -Wall -Werror -ansi -pedantic

all:
	$(CXX) $(CPPFLAGS) $(DFLAGS) rshell.cc -o rshell

rshell:
