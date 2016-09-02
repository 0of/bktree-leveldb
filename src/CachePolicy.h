/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */

#ifndef CACHE_POLICY_H
#define CACHE_POLICY_H

struct NoCachePolicy {};

//
// require
//	bool get(const std::string& key, std::queue<std::string>& keys, const std::pair<std::uint32_t, std::uint32_t>& range)
//	void update(const std::string& key, const std::vector<std::uint32_t>& distances, LoadSingleCallable loadChildKey, LoadAllCallable loadChildrenKeys)
//
class ChildrenKeysCache {};

#endif // CACHE_POLICY_H
