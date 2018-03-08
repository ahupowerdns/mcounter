#include "mcounter.hh"
#include <thread>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <vector>
using namespace std;
struct MyCounters
{
  uint64_t a, b;
};

struct MyCountersAtomic
{
  std::atomic<uint64_t> a __attribute__ ((aligned (8)));
  std::atomic<uint64_t> b __attribute__ ((aligned (8)));
};

UnsharedCounterStructParent<MyCounters> myc;
MyCountersAtomic mca;

void unsharedStructWorker(unsigned int a)
{
  UnsharedCounterStruct<MyCounters> uc(&myc);
  for(unsigned int n = 0; n < a; ++n) {
    ++uc.d_value.a;
    ++uc.d_value.b;
  }
}


void sharedStructWorker(unsigned int a)
{
  UnsharedCounterStruct<MyCounters> uc(&myc);
  for(unsigned int n = 0; n < a; ++n) {
    ++mca.a;
    ++mca.b;
  }
}


void printWorker()
{
  for(;;) {
    cout<< myc.get().a << " / " << mca.a <<", "<<mca.b<<endl;
    usleep(100000);
  }
}

using namespace std::chrono;

int main(int argc, char **argv)
{
  cout<<sizeof(mca)<<endl;
  int num= argc > 1 ? atoi(argv[1]) : 1;
  bool unshared = argc > 2 ? argv[2]==std::string("unshared") : 1;
  std::thread progress(printWorker);
  progress.detach();

  vector<std::thread> threads;

  auto start = high_resolution_clock::now();
  for(int n=0; n < num; ++n)
    threads.emplace_back(unshared ? unsharedStructWorker: sharedStructWorker,
                         100000000);

  for(auto& t : threads)
    t.join();
                           
  auto finish = high_resolution_clock::now();
  auto msecs = duration_cast<milliseconds>(finish-start);
  cout<<msecs.count()<<" milliseconds"<<endl;
  cout << "Final: "<<myc.get().a << " / "<<mca.a<<", "<<mca.b<<endl;
}
