CXX = gcc
CXXFLAGS = -O2 -Wall -Wextra -std=gnu23
LDFLAGS = -O2 -Wall -Wextra -std=gnu23

braminar: braminar.o
	$(CXX) $(LDFLAGS) braminar.o -o braminar

braminar.o: braminar.c
	$(CXX) $(CXXFLAGS) -c braminar.c -o braminar.o
