/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */

#include <exception>
#include <atomic>

#include "LevenshteinDistance.h"
#include "BKTree.h"

#include "TestSuite.h"

using TestSpec = LTest::SequentialTestSpec;
using Container = LTest::SequentialTestRunnableContainer;

class AssertionFailed : public std::exception {};

template<typename Spec>
void cases(Spec& spec) {
  spec.it("should query key1 & key2", []() {
    std::unique_ptr<BKTree<LevenshteinDistancePolicy>> bktree{ BKTree<LevenshteinDistancePolicy>::New("/tmp/tmpdb", "/tmp/tmpdb_i") };
    bktree->insert("key1", "value1");
    bktree->insert("key2", "value2");

    auto q = bktree->query("key1", 999, 999);

    if (q.find("key1") == q.end() || q.find("key2") == q.end())
      throw AssertionFailed{};
  });
}

int main(void) {  
  auto container = std::make_unique<Container>();
  auto spec = std::make_shared<TestSpec>();

  cases(*spec);

  container->scheduleToRun(spec);
  container->start();
  std::this_thread::sleep_for(1s);

  return 0;
}