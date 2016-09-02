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

// called before update root key indexes
struct OverwriteValueOnlyPolicy {
  static void overwrite(const std::unique_ptr<leveldb::DB>& indexesDB, const std::string&, const std::vector<std::uint32_t>&, leveldb::WriteBatch&) {
    // do nothing
  }
};

struct CleanRootKeyIndexesPolicy {
  static void overwrite(const std::unique_ptr<leveldb::DB>&, const std::string& rootKey, const std::vector<std::uint32_t>& children, leveldb::WriteBatch& batch) {
    batch.Put(CHILDREN_DISTANCES_KEY(rootKey), leveldb::Slice{});
    batch.Put(CHILDREN_KEY(rootKey), leveldb::Slice{});

    for (auto d : children) {
      batch.Delete(CHILD_INDEX_KEY(rootKey, d));
    }
  }
};