CXXFLAGS:=-std=gnu++11 -Wall -O2 -MMD -MP

PROGRAMS = mmulti msingle mtests

all: $(PROGRAMS)

clean:
	rm -f *~ *.o *.d test $(PROGRAMS)

check: mtests
	./mtests

-include *.d

mmulti: mcounter-multi.o 
	g++ -std=gnu++11 $^ -o $@ -pthread

msingle: mcounter-single.o 
	g++ -std=gnu++11 $^ -o $@ -pthread

mtests: mtests.o
	g++ -std=gnu++11 $^ -o $@ -pthread
