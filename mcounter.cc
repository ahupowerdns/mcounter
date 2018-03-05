#include "mcounter.hh"
#include <thread>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <vector>

using namespace std;

template<typename T>
void UnsharedCounterStructParent<T>::StructPlusIs(T& dst, const volatile T& src)
{
  // get your helmet on
  auto dptr = (uint64_t*)&dst;
  auto sptr = (const uint64_t*)&src;
  static_assert((sizeof(T) % 8) == 0);
  int num = sizeof(T)/8;

  for(int n = 0 ; n < num; ++n)
    *dptr++ += *sptr++;

  // so this just added the 64 bit counters in struct src to those in dst
  // "roughly"
}

template<typename T>
T UnsharedCounterStructParent<T>::get()
{
  T ret = d_formerChildren;
  std::lock_guard<std::mutex> l(d_mutex);
  for(const auto& c : d_children) {
    StructPlusIs(ret, c->d_value);
  }
  return ret;
}

template<typename T>
void UnsharedCounterStructParent<T>::addChild(UnsharedCounterStruct<T>* uc)
{
  std::lock_guard<std::mutex> l(d_mutex);
  d_children.insert(uc);
}

template<typename T>
void UnsharedCounterStructParent<T>::removeChild(UnsharedCounterStruct<T>* uc)
{
  std::lock_guard<std::mutex> l(d_mutex);
  StructPlusIs(d_formerChildren, uc->d_value);
  d_children.erase(uc);
}

struct MyCounters
{
  uint64_t a;
  uint64_t b;
};

UnsharedCounterStructParent<MyCounters> myc;

void unsharedStructWorker(unsigned int a)
{
  UnsharedCounterStruct<MyCounters> uc(&myc);
  for(unsigned int n = 0; n < a; ++n)
    ++uc.d_value.a;
}

void printWorker()
{
  for(;;) {
    cout<< myc.get().a << endl;
    usleep(100000);
  }
}

using namespace std::chrono;

int main(int argc, char **argv)
{
  int num= argc > 1 ? atoi(argv[1]) : 1;
  
  std::thread progress(printWorker);
  progress.detach();

  vector<std::thread> threads;

  auto start = high_resolution_clock::now();
  for(int n=0; n < num; ++n)
    threads.emplace_back(unsharedStructWorker, 100000000);

  for(auto& t : threads)
    t.join();
                           
  auto finish = high_resolution_clock::now();
  auto msecs = duration_cast<milliseconds>(finish-start);
  cout<<msecs.count()<<" milliseconds"<<endl;
  cout << "Final: "<<myc.get().a <<endl;
}
