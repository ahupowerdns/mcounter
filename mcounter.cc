#include "mcounter.hh"
#include <thread>
#include <iostream>

using namespace std;

template<typename T>
T UnsharedCounterStructParent<T>::get()
{
  T ret = d_formerChildren;
  std::lock_guard<std::mutex> l(d_mutex);
  for(const auto& c : d_children) {
    ret += c->d_value;
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
  //  cout<<"Adding "<<uc->d_value<<" from former child"<<endl;
  d_formerChildren += uc->d_value;
  std::lock_guard<std::mutex> l(d_mutex);

  d_children.erase(uc);
}

struct MyCounters
{
  std::atomic<uint64_t> a, b;

  MyCounters()
  {
    a=0;
    b=0;
  }
  MyCounters(const MyCounters& orig)
  {
    a.store(orig.a.load());
    b.store(orig.b.load());
  }
  MyCounters& operator+=(const MyCounters& rhs)
  {
    a += rhs.a;
    b += rhs.b;
    return *this;
  }
};

UnsharedCounterStructParent<MyCounters> myc;

void unsharedStructWorker(unsigned int a)
{
  UnsharedCounterStruct<MyCounters> uc(&myc);
  for(unsigned int n = 0; n < a; ++n)
    ++uc.d_value.a;
}



int main()
{
  std::thread t1(unsharedStructWorker, 1000000000);
  t1.join();

  cout << myc.get().a <<endl;
}
