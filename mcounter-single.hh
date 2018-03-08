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

  UnsharedCounter() = delete;
  UnsharedCounter(const UnsharedCounter&) = delete;
  UnsharedCounter(const UnsharedCounter&&) = delete;
  
  uint64_t operator++()
  {
    return d_value++;
  }
  
  volatile uint64_t d_value{0};
private:
  UnsharedCounterParent* d_ucp;
};

uint64_t UnsharedCounterParent::get()
{
  uint64_t ret=d_formerChildren;
  std::lock_guard<std::mutex> l(d_mutex);
  for(const auto& c : d_children) {
    ret += c->d_value;
  }
  return ret;
}

void UnsharedCounterParent::addChild(UnsharedCounter* uc)
{
  std::lock_guard<std::mutex> l(d_mutex);
  d_children.insert(uc);
}

void UnsharedCounterParent::removeChild(UnsharedCounter* uc)
{
  std::lock_guard<std::mutex> l(d_mutex);
  d_formerChildren += uc->d_value;
  d_children.erase(uc);
}
