#pragma once
#include <mutex>
#include <atomic>
#include <unordered_set>

class UnsharedCounter;

class UnsharedCounterParent
{
public:
  uint64_t get();
  void addChild(UnsharedCounter*);
  void removeChild(UnsharedCounter*);
private:
  std::unordered_set<UnsharedCounter*> d_children;
  std::atomic<uint64_t> d_formerChildren{0};
  std::mutex d_mutex;
};

class UnsharedCounter
{
public:
  explicit UnsharedCounter(UnsharedCounterParent* ucp) : d_ucp(ucp)
  {
    d_ucp->addChild(this);
  }

  ~UnsharedCounter()
  {
    d_ucp->removeChild(this);
  }

  uint64_t operator++()
  {
    return d_value++;
  }

  
  volatile uint64_t d_value{0};
private:
  UnsharedCounterParent* d_ucp;
};
