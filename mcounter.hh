#pragma once
#include <cstdint>
#include <atomic>
#include <unordered_set>
#include <mutex>

/* Design goal: bunch of thread local counters that are incremented atomically, but they are never shared.
   There is one entrypoint to get the counters, and that collects them from all threads.

   This entrypoint is threadsafe.

   First let's build this for one counter.

*/


template<typename T>
class UnsharedCounterStruct;


template<typename T>
class UnsharedCounterStructParent
{
public:
  T get();
  void addChild(UnsharedCounterStruct<T>*);
  void removeChild(UnsharedCounterStruct<T>*);
private:
  void StructPlusIs(T& dst, const volatile T& src);
  std::unordered_set<UnsharedCounterStruct<T>*> d_children;
  std::mutex d_mutex;
  T d_formerChildren;

};

template<typename T>
class UnsharedCounterStruct
{
public:
  explicit UnsharedCounterStruct(UnsharedCounterStructParent<T>* ucp) : d_ucp(ucp)
  {
    d_ucp->addChild(this);
  }

  ~UnsharedCounterStruct()
  {
    d_ucp->removeChild(this);
  }

  volatile T d_value;
private:
  UnsharedCounterStructParent<T>* d_ucp;
};
