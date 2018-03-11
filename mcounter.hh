#pragma once
#include <string.h>
#include <cstdint>
#include <atomic>
#include <unordered_set>
#include <mutex>
#include <stdexcept>

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
  UnsharedCounterStruct<T> getLocal()
  {
    return std::move(UnsharedCounterStruct<T>(this));
  }
  void addChild(UnsharedCounterStruct<T>*);
  void removeChild(UnsharedCounterStruct<T>*);
  void moveChild(const UnsharedCounterStruct<T>* from, UnsharedCounterStruct<T>* to);
private:
  void StructPlusIs(T& dst, const volatile T& src);
  std::unordered_set<UnsharedCounterStruct<T>*> d_children;
  std::mutex d_mutex;
  T d_formerChildren{T()};

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
    if(d_ucp) // perhaps we got moved
      d_ucp->removeChild(this);
  }

  UnsharedCounterStruct() = delete;
  UnsharedCounterStruct(const UnsharedCounterStruct&) = delete;
  UnsharedCounterStruct(UnsharedCounterStruct&& rhs)
  {
    d_ucp = rhs.d_ucp;
    rhs.d_ucp = nullptr;
    // this works around volatile pain
    memcpy((void*)&d_value, (void*)&rhs.d_value, sizeof(d_value));
    d_ucp->moveChild(&rhs, this);
  }
  UnsharedCounterStruct& operator=(const UnsharedCounterStruct& rhs) = delete;
  
  volatile T d_value{T()};
private:
  UnsharedCounterStructParent<T>* d_ucp;
};

template<typename T>
void UnsharedCounterStructParent<T>::StructPlusIs(T& dst, const volatile T& src)
{
  // get your helmet on
  auto dptr = (uint64_t*)&dst;
  auto sptr = (const uint64_t*)&src;
  static_assert((sizeof(T) % 8) == 0, "Struct does not consist of integral number of 64 bit counters");
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

template<typename T>
void UnsharedCounterStructParent<T>::moveChild(const UnsharedCounterStruct<T>* from, UnsharedCounterStruct<T>* to)
{
  std::lock_guard<std::mutex> l(d_mutex);

  auto iter = d_children.find(const_cast<UnsharedCounterStruct<T>*>(from));
  if(iter == d_children.end())
    throw std::runtime_error("attempt to move unknown child");
  d_children.erase(iter);
  d_children.insert(to);
}

