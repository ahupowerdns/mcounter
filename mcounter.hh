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
  std::lock_guard<std::mutex> l(d_mutex);
  T ret = d_formerChildren;

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

