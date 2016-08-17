/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */
 
#ifndef HELPER_H
#define HELPER_H

// key to search for tree root key
#define ROOT_INDEX_KEY leveldb::Slice{}
// key to list all the associated distances
#define CHILDREN_DISTANCES_KEY(parentKey) (parentKey + 'c')
// key to query for real keys
#define CHILD_INDEX_KEY(parentKey, distance) (parentKey + std::to_string(distance))

struct Helper {
  static std::uint32_t parse(const std::string& value) {
    return *reinterpret_cast<const std::uint32_t *>(value.c_str());
  }

  static std::string stringfy(std::uint32_t value) {
    return std::string(reinterpret_cast<const char *>(&value), sizeof(std::uint32_t) / sizeof(char));
  }
};

#endif // HELPER_H