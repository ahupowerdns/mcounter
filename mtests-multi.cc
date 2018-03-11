#include "ext/catch.hpp"
#include "mcounter.hh"
#include <thread>

TEST_CASE("Mcounter multiple basic", "[multbas]") {
  struct mystruct
  {
    uint64_t a,b;
  };
  UnsharedCounterStructParent<mystruct> ucp;
  
  auto func = [](UnsharedCounterStructParent<mystruct>* ucp) {
    UnsharedCounterStruct<mystruct> ucs(ucp);

    for(unsigned int n = 0; n < 10000000; ++n)
      ++ucs.d_value.a;
  };

  std::thread t1(func, &ucp), t2(func, &ucp);
  t1.join();
  t2.join();
  REQUIRE(ucp.get().a == 20000000);
  REQUIRE(ucp.get().b == 0);
}
