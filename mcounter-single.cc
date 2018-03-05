#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>
#include "mcounter-single.hh"

using namespace std;

uint64_t UnsharedCounterParent::get()
{
  uint64_t ret=d_formerChildren;
  std::lock_guard<std::mutex> l(d_mutex);
  cout<<"Value: "<<ret;
  for(const auto& c : d_children) {
    cout<<" + "<<c->d_value;
    ret += c->d_value;
  }
  cout<<endl;
  return ret;
}

void UnsharedCounterParent::addChild(UnsharedCounter* uc)
{
  std::lock_guard<std::mutex> l(d_mutex);
  d_children.insert(uc);
}

void UnsharedCounterParent::removeChild(UnsharedCounter* uc)
{
  cout<<"Adding "<<uc->d_value<<" from former child"<<endl;
  d_formerChildren += uc->d_value;
  std::lock_guard<std::mutex> l(d_mutex);

  d_children.erase(uc);
}

UnsharedCounterParent ucp;

void unsharedWorker(unsigned int a)
{
  UnsharedCounter uc(&ucp);
  for(unsigned int n = 0; n < a; ++n)
    ++uc;
}

std::atomic<uint64_t> g_counter{0};

void sharedWorker(unsigned int a)
{
  for(unsigned int n = 0; n < a; ++n)
    ++g_counter;
}

void printWorker()
{
  for(;;) {
    cout<< ucp.get() << " / " <<g_counter<<endl;
    usleep(100000);
  }
}

using namespace std::chrono;
int main(int argc, char **argv)
{
  int num= argc > 1 ? atoi(argv[1]) : 1;
  bool unshared = argc > 2 ? argv[2]==std::string("unshared") : 1;
  
  std::thread progress(printWorker);
  progress.detach();
  
  vector<std::thread> threads;

  auto start = high_resolution_clock::now();
  for(int n=0; n < num; ++n)
    threads.emplace_back(unshared ? unsharedWorker: sharedWorker, 100000000);

  for(auto& t : threads)
    t.join();
                           
  auto finish = high_resolution_clock::now();
  auto msecs = duration_cast<milliseconds>(finish-start);

  cout<<"Took "<<msecs.count()<<" milliseconds of wall time"<<endl;
  cout << ucp.get() <<endl;
  cout << g_counter<<endl;
}
