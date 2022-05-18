CXXFLAGS=-std=c++17 -Wall -O2 -MMD -MP -ggdb

PROGRAMS = mmulti msingle mtests

all: $(PROGRAMS)

clean:
	rm -f *~ *.o *.d test $(PROGRAMS)

check: mtests
	./mtests

-include *.d

mmulti: mcounter-multi.o 
	${CXX} $^ -o $@ -pthread

msingle: mcounter-single.o 
	${CXX} $^ -o $@ -pthread

mtests: mtests.o mtests-multi.o
	${CXX} $^ -o $@ -pthread
