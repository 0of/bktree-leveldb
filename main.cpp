/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */
 
#include <iostream>

#include "LevenshteinDistance.h"
#include "BKTree.h"

int main(void) {  

  try {
    std::unique_ptr<BKTree<LevenshteinDistancePolicy>> bktree{ BKTree<LevenshteinDistancePolicy>::New("/tmp/tmpdb") };
    bktree->insert("key1", "value1");
    bktree->insert("key2", "value2");
    for (auto e : bktree->query("key1", 1000, 10)) {
      std::cout << e << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << e.what();
  } catch (...) {
    std::cout << "error";
  }
 
  return 0;
}