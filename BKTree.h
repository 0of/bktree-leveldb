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

#include "Helper.h"

template<typename DistancePolicy>
class BKTree {
  //
  //  child key -> parent key : distance number(decimal)
  //  list all children key -> parent key : children
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
      return _offset * 8;
    }

    ChildrenIterator& operator ++() {
      ++_offset;
      return *this;
    }

    std::uint32_t operator *() {
      return Helper::parse(_children->substr(_offset * 8, 8));
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
  void insert(const std::string& key, const std::string& value) {
    // if has no root key directly place the first key as root key
    if (_rootKey.empty()) {
      storeRootKey(key, value);

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

      if (!containsAndGet(currentKey + std::to_string(d), currentKey)) {
        // not found
        storeChild(currentKey, d, key, value);
        break;
      } 
      // continue to search storage point
    }
  }

  std::set<std::string> query(const std::string& key, std::uint32_t threshold, std::uint32_t limit) {
    std::set<std::string> values;

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
    if (!_valuesStorage->Put(leveldb::WriteOptions(), key, value).ok())
      throw 0;
  }

  void storeRootKey(const std::string& key, const std::string& value) {
    if (!_valuesStorage->Put(leveldb::WriteOptions(), key, value).ok())
       throw std::runtime_error("store root failed");

    leveldb::WriteBatch batch;
    batch.Put("", key);
    batch.Put(key + 'c', "");

    if (!_indexesStorage->Write(leveldb::WriteOptions(), &batch).ok())
      throw std::runtime_error("store root failed");
  }

  bool containsAndGet(const std::string& indexKey, std::string& value) {
    auto status = _indexesStorage->Get(leveldb::ReadOptions(), indexKey, &value);
    if (status.IsNotFound())
      return false;
    else if (status.ok())
      return true;

    throw 0;
  }

  std::string lookupChildKey(const std::string& key, std::uint32_t distance) {
    auto queryKey = key + std::to_string(distance);
    std::string queriedKey;

    auto status = _indexesStorage->Get(leveldb::ReadOptions(), queryKey, &queriedKey);
    if (status.ok()) {
      return queriedKey;
    }

    throw 0;
  }

  void storeChild(const std::string& parent, std::uint32_t distance, const std::string& key, const std::string& value) {
    // child distances is a string joined by multi fixed-length(8) substring
    // and each one represents the distance value in hex format 

    // get child distances
    std::string parentsChildren;
    auto status = _indexesStorage->Get(leveldb::ReadOptions(), parent + 'c', &parentsChildren);
    if (!status.ok())
      throw 0;

    // find insert position
    auto pos = findInsertionPos(parentsChildren, distance);
    // update children
    parentsChildren.insert(pos, Helper::stringfy(distance));

    if (!_valuesStorage->Put(leveldb::WriteOptions(), key, value).ok())
      throw std::runtime_error("store child key-value failed");

    leveldb::WriteBatch batch;
    batch.Put(key + 'c', "");
    batch.Put(parent + 'c', parentsChildren);
    batch.Put(parent + std::to_string(distance), key);

    if (!_indexesStorage->Write(leveldb::WriteOptions(), &batch).ok())
      throw 0;
  }

  std::size_t findInsertionPos(const std::string& children, std::uint32_t distance) {
    // children is sorted ascendingly
    return std::lower_bound(ChildrenIterator{children, 0}, 
                            ChildrenIterator{children, (children.size() / 8)},
                            distance).pos();
  }

  std::string loadValue(const std::string& key) {
    std::string queryValue;
    auto status = _valuesStorage->Get(leveldb::ReadOptions(), key, &queryValue);
    if (status.ok()) {
      return queryValue;
    }

    throw 0;
  }

  std::vector<std::uint32_t> childDistances(const std::string& key) {
    auto queryKey = key + 'c';
    std::string queryValue;

    auto status = _indexesStorage->Get(leveldb::ReadOptions(), queryKey, &queryValue);
    if (status.ok()) {
      std::vector<std::uint32_t> keys(queryValue.size() / 8);

      for (std::size_t i = 0; i != queryValue.size() / 8; ++i) {
        keys[i] = Helper::parse(queryValue.substr(i * 8, 8));
      }

      return std::move(keys);
    }

    throw 0;
  }
};

#endif // BKTREE_H