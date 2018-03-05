CXXFLAGS:=-std=gnu++14 -Wall -O2 -MMD -MP

all: mcounter msingle

clean:
	rm -f *~ *.o *.d test

-include *.d

mcounter: mcounter.o 
	g++ -std=gnu++14 $^ -o $@ -pthread

msingle: mcounter-single.o 
	g++ -std=gnu++14 $^ -o $@ -pthread

