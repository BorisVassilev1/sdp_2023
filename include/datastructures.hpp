#pragma once
#include <unordered_map>
#include <unordered_set>

#include "hashing.hpp"

namespace fl {
template <class K, class V, class H = fl::hash<K>>
using unordered_map = std::unordered_map<K, V, H>;

template <class K, class H = fl::hash<K>>
using unordered_set = std::unordered_set<K, H>;

template <class K, class V, class H = fl::hash<K>>
using unordered_multimap = std::unordered_multimap<K, V, H>;
}	  // namespace fl
