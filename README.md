# mcounter
## Unshared counters that can be read easily

This code aims to provide simple high-performance counters for multithreaded
C++ software.

To get correct counters that can be updated from multiple threads
simultaneously either requires locks or atomic operations. Atomic operations
sound fast ('because no locks') but in fact are not.

Every time a counter is updated atomically, it requires the CPU to own that
cacheline exclusively. If multiple CPUs are attempting to update the same
counter at the same time they keep fighting over the cacheline.

When two threads do this 200 million times on my system, this takes 4 whole
seconds, or 50 million operations/second. When further threads are added,
performance gradually goes down to 45 million operations/second.

This means that if a process updates 10 counters per packet, whatever you
do, you'll never scale beyond 5 million packets/second, no matter how many
cores you throw at it - assuming the rest of your code takes no time at all! 
(Note that one might expect several counters to not interfere with each
other, but the cache line communications appear to be limited to around 50
million/second on my system, no matter how many lines these are spread
over).

Meanwhile, when two threads update unrelated 64 bit counters, they achieve
over a billion updates per second. Using 8 threads on my 4-core system
improves that number to 3 billion updates per second. 

Per-thread counters are super fast, but not what we need - our process needs
to report what it is doing from an operator's perspective, and the operator
does not care about our threads.

## Unshared counters that can safely be read
This code assumes you'll have a relatively low number of threads ('dozens to
hundreds'), and that counters will be updated furiously, but read only
sporadically.

Sample code for a single counter:

```
UnsharedCounterParent ucp;

void worker()
{
	UnsharedCounter uc(&ucp);
	for(int n = 0 ; n < 1000000000; ++n)
		++uc;
}

int main() {
	std::thread t1(worker), t2(worker);
	t1.join();
	t2.join();
	cout << ucp.get() << "\n"; // prints 2000000000
}
```

It is safe to call `ucp.get()` at any time. However, an `UnsharedCounter`
should never be updated from more than one thread at once.

## Extending this to structs
Real code frequently keeps dozens of counters, often organized in a struct.
To make life easy, code is provided to make such struct unshared updatable
as well:

```
struct MyCounters
{
	uint64_t a;
	uint64_t b;
};

UnsharedCounterStructParent<MyCounters> myc;

void worker()
{
	auto uc = myc.getLocal();
	for(unsigned int n = 0; n < 1000000000; ++n)
		++uc.d_value.a;
}

int main() {
	std::thread t1(worker), t2(worker);
	t1.join();
	t2.join();
	cout << myc.get().a << "\n"; // prints 2000000000
	cout << myc.get().b << "\n"; // prints 0
}
```

## Usage rules
For `UnsharedCounter(Struct)` instances, ensure that the `UnsharedCounter(Struct)Parent` does not ever get destroyed before its children are. This requirement is easy to meet if the Parent is always in global scope, whereas its children are always in function scope.

`UnsharedCounter(Struct)` instances must always be destroyed properly. If for example a thread is killed using `pthread_kill` (which you should not do), the destructor may not get called. This will leave the parent pointing to bad data.

The struct behind an `UnsharedCounterStruct` most consist exclusively of 64 bit counters. Any deviation from this will mess up the addition of structs, which is done on a per-64 bit basis.

## Technicalities
The UnsharedCounter objects use non-atomic counters. These counters are
however read from other threads, with no locking. As long as counters do not
straddle cache lines, this is safe. Given that we use 64 bit counters, on
structs that will be 64 bit aligned, this should be true. Even if it isn't
true, the effect will only be momentarily.

The code currently specifies counters as 'volatile'. This is not strictly
necessary, but does remove some surprises. If counters would not be
volatile, compilers would be free to move a counter to a register and update
it there. This means the Parent object may not 'see' updates for prolonged
amounts of time.

If you know what you are doing, you can remove 'volatile' and gain another
speedup. Note that benchmarking then becomes very difficult as compilers
tend to "see what you are doing", and optimize your loops away.

## Performance
As noted, these counters are aimed to support typical numbers of threads,
frequent updates and infrequent reads. Zooming in a little bit, the actual
limit is not so much on the number of threads, but the number of
`UnsharedCounter` instances.

On creation of an `UnsharedCounter` it reports to its parent, and this
involves a lock. If such an instantiation happens once for every thread, the
overhead is minimal. So in other words, aim for long-lived `UnsharedCounter`
instances.

Retrieving the value of a counter (or a set of counters in struct) similarly
involves taking a lock, as the list `UnsharedCounter`s needs to be
traversed.
