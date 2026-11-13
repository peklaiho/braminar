CXX = gcc
CXXFLAGS = -O2 -Wall -Wextra -Wpedantic -std=c99
LDFLAGS = -O2 -Wall -Wextra -Wpedantic -std=c99

braminar: braminar.o
	$(CXX) $(LDFLAGS) braminar.o -o braminar

braminar.o: braminar.c
	$(CXX) $(CXXFLAGS) -c braminar.c -o braminar.o

extract_data: extract_data.c
	$(CXX) $(CXXFLAGS) extract_data.c -o extract_data

.PHONY: clean
clean:
	rm -f braminar braminar.o extract_data
