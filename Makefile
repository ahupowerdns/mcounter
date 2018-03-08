CXXFLAGS:=-std=gnu++14 -Wall -O2 -MMD -MP

all: mmulti msingle

clean:
	rm -f *~ *.o *.d test

-include *.d

mmulti: mcounter-multi.o 
	g++ -std=gnu++14 $^ -o $@ -pthread

msingle: mcounter-single.o 
	g++ -std=gnu++14 $^ -o $@ -pthread

mtests: mtests.o
	g++ -std=gnu++14 $^ -o $@ -pthread
