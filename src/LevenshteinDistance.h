/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */
 
#ifndef LEVENSHTEINDISTANCE
#define LEVENSHTEINDISTANCE

#include <vector>
#include <string>
#include <algorithm>

class LevenshteinDistancePolicy {
public:
  static constexpr const char Prefix = 'L';

public:
  // https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
  static std::uint32_t distance(const std::string& s1, const std::string& s2) {
    auto len1 = s1.size();
    auto len2 = s2.size();

    std::vector<std::uint32_t> col(len2 + 1);
    std::vector<std::uint32_t> prevCol(len2 + 1);
  
    for (std::size_t i = 0; i != prevCol.size(); ++i)
      prevCol[i] = i;
    for (std::size_t i = 0; i != len1; ++i) {
      col[0] = i + 1;
      for (std::size_t j = 0; j != len2; ++j)
        col[j + 1] = std::min({ prevCol[1 + j] + 1, col[j] + 1, prevCol[j] + (s1[i]==s2[j] ? 0 : 1) });
      col.swap(prevCol);
    }

    return prevCol[len2];
  }
};

#endif // LEVENSHTEINDISTANCE