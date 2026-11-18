CXX = gcc
CXXFLAGS = -O2 -Wall -Wextra -Wpedantic -std=c99
LDFLAGS = -O2 -Wall -Wextra -Wpedantic -std=c99

braminar: braminar.o combat.o events.o town.o final.o
	$(CXX) $(LDFLAGS) $^ -o $@

%.o: %.c braminar.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f braminar *.o
