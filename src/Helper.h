/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */
 
#ifndef HELPER_H
#define HELPER_H

struct Helper {
  static std::uint32_t parse(const std::string& value) {
    return *reinterpret_cast<const std::uint32_t *>(value.c_str());
  }

  static std::string stringfy(std::uint32_t value) {
    return std::string(reinterpret_cast<const char *>(&value), sizeof(std::uint32_t) / sizeof(char));
  }
};

#endif // HELPER_H