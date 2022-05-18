#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <unistd.h>
#include "mcounter-single.hh"

using namespace std;

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

std::atomic<bool> stop;

void printWorker()
{
  while (!stop) {
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
  
  vector<std::thread> threads;

  auto start = high_resolution_clock::now();
  for(int n=0; n < num; ++n)
    threads.emplace_back(unshared ? unsharedWorker: sharedWorker, 100000000);

  for(auto& t : threads)
    t.join();
                           
  stop = true;
  progress.join();

  auto finish = high_resolution_clock::now();
  auto msecs = duration_cast<milliseconds>(finish-start);

  cout<<"Took "<<msecs.count()<<" milliseconds of wall time"<<endl;
  cout << ucp.get() <<endl;
  cout << g_counter<<endl;
}
