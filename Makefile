CXX = gcc
CXXFLAGS = -O2 -Wall -Wextra -Wpedantic -std=c99
LDFLAGS = -O2 -Wall -Wextra -Wpedantic -std=c99

braminar: braminar.o
	$(CXX) $(LDFLAGS) braminar.o -o braminar

braminar.o: braminar.c
	$(CXX) $(CXXFLAGS) -c braminar.c -o braminar.o

.PHONY: clean
clean:
	rm -f braminar braminar.o
