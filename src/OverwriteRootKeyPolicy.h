/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "Helper.h"

struct OverwriteValueOnlyPolicy {
  static void overwrite(const std::unique_ptr<leveldb::DB>& indexesDB, const std::string& rootKey, const std::vector<std::uint32_t>&) {
    // do nothing
  }
};

struct CleanRootKeyIndexesPolicy {
  static void overwrite(const std::unique_ptr<leveldb::DB>& indexesDB, const std::string& rootKey, const std::vector<std::uint32_t>& children) {
    leveldb::WriteBatch batch;
    batch.Put(ROOT_INDEX_KEY, rootKey);
    batch.Put(CHILDREN_DISTANCES_KEY(rootKey), leveldb::Slice{});

    for (auto d : children) {
      batch.Delete(CHILD_INDEX_KEY(rootKey, d));
    }

    auto status = indexesDB->Write(leveldb::WriteOptions(), &batch);
    if (!status.ok())
      throw std::runtime_error{status.ToString()};
  }
};