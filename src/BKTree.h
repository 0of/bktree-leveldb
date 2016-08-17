/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */

#ifndef BKTREE_H
#define BKTREE_H

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include <string>
#include <set>
#include <queue>
#include <memory>
#include <algorithm>

#include "OverwriteRootKeyPolicy.h"

template<typename DistancePolicy>
class BKTree {

  // 
  // each index node contains one children distances key and several child index keys
  // the value queried by children distances key would be a list of distances and 
  // each child index key (via CHILD_INDEX_KEY by putting parent key and the distance between them) paired with one or more real keys
  //
  // [real key] -> [value]
  // [children distances key] -> [distances ...]
  // [real key] + [distance] -> [child index key]
  // [child index key] -> [real keys ...]
  //

private:
  using SelfType = BKTree<DistancePolicy>;
  using Storage = leveldb::DB;

private:
  // user input key-values
  std::unique_ptr<Storage> _valuesStorage;
  // BKTree indexes
  std::unique_ptr<Storage> _indexesStorage;
  std::string _rootKey;

private:
  class ChildrenIterator : public std::iterator<std::input_iterator_tag, std::uint32_t> {
  private:
    std::size_t _offset;
    const std::string *_children;

  public:
    ChildrenIterator(const std::string& children, std::size_t offset)
      : _offset{ offset }
      , _children{ &children }
    {}

    std::size_t pos() {
      return _offset * sizeof(std::uint32_t);
    }

    ChildrenIterator& operator ++() {
      ++_offset;
      return *this;
    }

    std::uint32_t operator *() {
      return Helper::parse(_children->substr(_offset * sizeof(std::uint32_t), sizeof(std::uint32_t)));
    }

    bool operator == (const ChildrenIterator& i) const { return _offset == i._offset; }
    bool operator != (const ChildrenIterator& i) const { return _offset != i._offset; }
  };
  
public:
  static SelfType* New(const std::string& path, const std::string& indexStoragePath) {
    leveldb::DB* db = nullptr;
    leveldb::DB* indexesDB = nullptr;

    leveldb::Options options;
    options.create_if_missing = true;

    auto status = leveldb::DB::Open(options, path, &db);
    if (status.ok()) {
      status = leveldb::DB::Open(options, indexStoragePath, &indexesDB);

      if (status.ok()) {
        std::string rootKey;

        // TODO read root key from leveldb
        return new SelfType(db, indexesDB, rootKey);

      } else {
        // close db
        delete db;
      }
    }

    return nullptr;
  }

protected:
  BKTree(Storage *valuesStorage, Storage *indexesStorage, const std::string& rootKey)
    : _valuesStorage{ valuesStorage }
    , _indexesStorage{ indexesStorage }
    , _rootKey{ rootKey }
  {}

public:
  template<class OverwriteRootKeyPolicy = CleanRootKeyIndexesPolicy>
  void insert(const std::string& key, const std::string& value) {
    // if has no root key directly place the first key as root key
    if (_rootKey.empty()) {
      storeRootKey<OverwriteRootKeyPolicy>(key, value);

      _rootKey = key;
      return;
    }

    //
    std::string currentKey = _rootKey;

    while (true) {
      // get distance 
      auto d = DistancePolicy::distance(currentKey, key);

      if (0 == d) {
        // update value
        updateValue(key, value);
        break;
      }

      if (!containsAndGet(CHILD_INDEX_KEY(currentKey, d), currentKey)) {
        // not found
        storeChild(currentKey, d, key, value);
        break;
      } 
      // continue to search storage point
    }
  }

  template<typename ResultContainer = std::set<std::string>>
  ResultContainer query(const std::string& key, std::uint32_t threshold, std::uint32_t limit) {
    ResultContainer values;

    std::queue<std::string> pendingKeys;
    std::string currentKey = _rootKey;

    while (true) {
      auto d = DistancePolicy::distance(currentKey, key);

      if (d < threshold) {
        values.emplace(loadValue(currentKey));
        if (values.size() >= limit)
          break;
      }

      auto distances = childDistances(currentKey);

      if (!distances.empty()) {
        auto lowerBound = d < threshold ? distances.begin() : std::lower_bound(distances.begin(), distances.end(), d - threshold);
        auto upperBound = std::upper_bound(distances.begin(), distances.end(), d + threshold);

        for (; lowerBound != upperBound; ++lowerBound) {
          pendingKeys.push(lookupChildKey(currentKey, *lowerBound));
        }
      }

      if (pendingKeys.empty()) {
        break;
      }

      currentKey = pendingKeys.front();
      pendingKeys.pop();
    }

    return std::move(values);
  }

private:
  void updateValue(const std::string& key, const std::string& value) {
    auto status = _valuesStorage->Put(leveldb::WriteOptions(), key, value);
    if (!status.ok())
      throw std::runtime_error{status.ToString()};
  }

  template<class OverwritePolicy>
  void storeRootKey(const std::string& key, const std::string& value) {
    auto status = _valuesStorage->Put(leveldb::WriteOptions(), key, value);
    if (!status.ok())
       throw std::runtime_error{status.ToString()};

    // overwrite the root key if has children indexes
    auto distances = childDistances(key); 
    if (!distances.empty()) {
      OverwritePolicy::overwrite(_indexesStorage, key, distances);
    }
  }

  bool containsAndGet(const std::string& indexKey, std::string& value) {
    auto status = _indexesStorage->Get(leveldb::ReadOptions(), indexKey, &value);
    if (status.IsNotFound())
      return false;
    else if (status.ok())
      return true;

    throw std::runtime_error{status.ToString()};
  }

  std::string lookupChildKey(const std::string& key, std::uint32_t distance) {
    auto queryKey = CHILD_INDEX_KEY(key, distance);
    std::string queriedKey;

    auto status = _indexesStorage->Get(leveldb::ReadOptions(), queryKey, &queriedKey);
    if (status.ok()) {
      return queriedKey;
    }

    throw std::runtime_error{status.ToString()};
  }

  void storeChild(const std::string& parent, std::uint32_t distance, const std::string& key, const std::string& value) {
    // child distances is a string joined by multi fixed-length substring
    // and each one represents the distance value in hex format 

    // get child distances
    std::string parentsChildren;
    auto status = _indexesStorage->Get(leveldb::ReadOptions(), CHILDREN_DISTANCES_KEY(parent), &parentsChildren);

    if (!status.ok())
      throw std::runtime_error{status.ToString()};

    // find insert position
    auto pos = findInsertionPos(parentsChildren, distance);
    // update children
    parentsChildren.insert(pos, Helper::stringfy(distance));

    if (!_valuesStorage->Put(leveldb::WriteOptions(), key, value).ok())
      throw std::runtime_error("store child key-value failed");

    leveldb::WriteBatch batch;
    batch.Put(CHILDREN_DISTANCES_KEY(key), leveldb::Slice{});
    batch.Put(CHILDREN_DISTANCES_KEY(parent), parentsChildren);
    batch.Put(CHILD_INDEX_KEY(parent, distance), key);


    status = _indexesStorage->Write(leveldb::WriteOptions(), &batch);
    if (!status.ok())
      throw std::runtime_error{status.ToString()};
  }

  std::size_t findInsertionPos(const std::string& children, std::uint32_t distance) {
    // children is sorted ascendingly
    return std::lower_bound(ChildrenIterator{children, 0}, 
                            ChildrenIterator{children, (children.size() / sizeof(std::uint32_t))},
                            distance).pos();
  }

  std::string loadValue(const std::string& key) {
    std::string queryValue;
    auto status = _valuesStorage->Get(leveldb::ReadOptions(), key, &queryValue);
    if (status.ok()) {
      return queryValue;
    }

    throw std::runtime_error{status.ToString()};
  }

  std::vector<std::uint32_t> childDistances(const std::string& key) {
    std::string queryValue;

    auto status = _indexesStorage->Get(leveldb::ReadOptions(), CHILDREN_DISTANCES_KEY(key), &queryValue);
    if (status.ok()) {
      return std::vector<std::uint32_t>{ChildrenIterator{queryValue, 0},
                                        ChildrenIterator{queryValue, (queryValue.size() / sizeof(std::uint32_t))}};
    }

    throw std::runtime_error{status.ToString()};
  }
};

#endif // BKTREE_H