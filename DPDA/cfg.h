#pragma once
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "hashes.h"

template <class Letter>
class CFG {
   public:
	std::unordered_multimap<Letter, std::vector<Letter>> rules;
	std::unordered_set<Letter, std::hash<char>>							 terminals;
	std::unordered_set<Letter, std::hash<char>>							 nonTerminals;
	Letter												 start;
	CFG() {}
};
