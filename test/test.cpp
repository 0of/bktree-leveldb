/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */

#include <exception>
#include <atomic>
#include <unordered_map>
#include <map>

#include "LevenshteinDistance.h"
#include "BKTree.h"

#include "TestSuite.h"

using TestSpec = LTest::SequentialTestSpec;
using Container = LTest::SequentialTestRunnableContainer;

class AssertionFailed : public std::exception {};

class CacheEntryContainer {
private:
  std::map<std::uint32_t, std::string> &_entry;
  std::vector<std::uint32_t>::const_iterator _it;

public:
  CacheEntryContainer(std::map<std::uint32_t, std::string> &entry, std::vector<std::uint32_t>::const_iterator begin)
  :_entry(entry)
  , _it(begin)
  {}
  
public:
  void emplace_back(std::string::const_iterator begin, std::string::const_iterator end) {
    _entry[*_it++] = std::string(begin, end);
  }
};

class ChildrenKeysCacheImpl : public ChildrenKeysCache {
private:
  std::unordered_map<std::string, std::map<std::uint32_t, std::string>> _cache;

public:
  bool get(const std::string& key, std::queue<std::string>& keys, const std::pair<std::uint32_t, std::uint32_t>& range) {
    auto found = _cache.find(key);
    if (found != _cache.end()) {
      // hit
      auto lowerBound = found->second.lower_bound(range.first);
      auto upperBound = found->second.upper_bound(range.second);

      for (; lowerBound != upperBound; ++lowerBound) {
        keys.push(lowerBound->second);
      }

      return true;
    }

    return false;
  }

  template<typename Callable, typename LoadAllCallable>
  void update(const std::string& key, const std::vector<std::uint32_t>& distances, Callable&& loadChildKey, LoadAllCallable&& loadAll) {
    std::map<std::uint32_t, std::string> cacheEntry;
    CacheEntryContainer container {cacheEntry, distances.begin()};

    loadAll(key, container);

    _cache[key] = std::move(cacheEntry);
  }
};

struct ChildrenKeyPolicyImpl {
  static void insert(std::string& keys, const std::string& newKey, std::uint32_t pos) {
    keys.insert(keys.begin() + pos * 4, newKey.cbegin(), newKey.cend());
  }

  static void split(const std::string& keys, CacheEntryContainer& splitKeys) {
    for (auto it = keys.cbegin(); it != keys.cend(); it += 4) {
      splitKeys.emplace_back(it, it + 4);
    }
  }
};

template<typename Spec>
void cases(Spec& spec) {
  spec.it("should query key1 & key2", []() {
    std::unique_ptr<BKTree<LevenshteinDistancePolicy, ChildrenKeysCacheImpl, ChildrenKeyPolicyImpl>> bktree{ BKTree<LevenshteinDistancePolicy, ChildrenKeysCacheImpl, ChildrenKeyPolicyImpl>::New("/tmp/tmpdb", "/tmp/tmpdb_i") };
    bktree->insert("key1", "value1");
    bktree->insert("key2", "value2");

    auto q = bktree->query("key1", 999, 999);

    if (q.find("value1") == q.end() || q.find("value2") == q.end())
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