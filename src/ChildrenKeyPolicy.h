/*
 * BKTree-LevelDB
 *
 * Copyright (c) 2016 "0of" Magnus
 * Licensed under the MIT license.
 * https://github.com/0of/bktree-leveldb/blob/master/LICENSE
 */

#ifndef CHILDREN_KEY_POLICY_H
#define CHILDREN_KEY_POLICY_H

//
// You need to implement some helper functions to update/parse queried children value
//	The queried children value is a string and you need to desgin how keys chained together
//	inside the string
//
// required
//  static void insert(std::string& keys, const std::string& newKey, std::uint32_t pos)
//	static void split(const std::string& keys, Container& splitKeys)
//


class DisableChildrenKey;

#endif // CHILDREN_KEY_POLICY_H
 