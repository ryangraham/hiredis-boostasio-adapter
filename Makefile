all: library example

library:
	g++ -lboost_system -lhiredis -shared -fPIC -o libboosthiredis.so boostasio.cpp

example:
	g++ -lboosthiredis example-boostasio.cpp

