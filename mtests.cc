

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "ext/doctest.h"
#include "mcounter-single.hh"
#include <thread>

TEST_CASE("Mcounter single basic") {
  UnsharedCounterParent ucp;
  REQUIRE(ucp.get()==0);
  UnsharedCounter uc(&ucp);
  REQUIRE(ucp.get()==0);
  ++uc;
  REQUIRE(ucp.get()==1);

  UnsharedCounter uc2(&ucp);
  for(int n=0; n < 1000000; ++n)
    ++uc2;
  REQUIRE(ucp.get()==1000001);
}

TEST_CASE("Mcounter single threads") {
  UnsharedCounterParent ucp;

  auto func = [](UnsharedCounterParent* ucp) {
    UnsharedCounter uc(ucp);
    for(unsigned int n = 0; n < 10000000; ++n)
      ++uc;
  };

  std::thread t1(func, &ucp), t2(func, &ucp);
  t1.join();
  t2.join();
  REQUIRE(ucp.get() == 20000000);
}

TEST_CASE("Mcounter single torture") {
  UnsharedCounterParent ucp;

  auto func = [](UnsharedCounterParent* ucp) {
    for(int m=0 ; m < 1000; ++m) {
      UnsharedCounter uc(ucp);
      for(unsigned int n = 0; n < 10000; ++n)
        ++uc;
    }
  };

  for(int n=0; n < 10; ++n) {
    std::thread t1(func, &ucp), t2(func, &ucp);
    t1.join();
    t2.join();
  }
  REQUIRE(ucp.get() == 200000000);
}

