#pragma once
#include <functional>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include "hashes.h"

template <class Letter>
class CFG {
   public:
	std::unordered_multimap<Letter, std::vector<Letter>> rules;
	std::unordered_set<Letter, std::hash<char>>			 terminals;
	std::unordered_set<Letter, std::hash<char>>			 nonTerminals;
	Letter												 start;

	CFG() {}

	void addRule(Letter a, const std::vector<Letter> &w) { rules.insert({a, w}); }

	template <class U = Letter>
		requires std::is_convertible_v<char, Letter>
	void addRule(char a, const std::string &w) {
		addRule(a, std::vector<Letter>(w.begin(), w.end()));
	}

	auto findNullables() const {
		std::unordered_map<Letter, bool> res;

		for (const auto &[k, v] : rules) {
			if (v.empty()) { res.insert({k, true}); }
		}
		for (Letter l: terminals) {
			res.insert({l, false});
		}

		std::size_t prevSize = 0;
		while (prevSize != res.size()) {
			prevSize = res.size();

			for (const auto &[k, v] : rules) {
				if (res.contains(k)) continue;
					
				bool isNullable = true;
				for (Letter l : v) {
					if (res.contains(l) && !res.find(l)->second) {
						res.insert({k, false});
						isNullable = false;
						break;
					}
				}
				if(isNullable) {
					res.insert({k, true});
				}
			}
		}

		for (const Letter l : nonTerminals) {
			std::cout << l << " : " << res.find(l)->second << std::endl;
		}
		return std::move(res);
	}

	auto findFirsts() {
		std::unordered_map<Letter, std::unordered_set<Letter>> first;
		
		for (Letter l: terminals) {
			first.insert({l, {l}});
		}
		for (Letter l: nonTerminals) {
			first.insert({l, {}});
		}

		bool change = true;
		while(change) {
			change = false;
			for(const auto &[k, v] : rules) {
				if(!v.empty() && first.contains(v[0])) {
					first.find(k)->second.insert_range(first.find(v[0])->second);
					change = true;
				}
			}
		}

		for(const Letter l : nonTerminals) {
			std::cout << l << " -> " << first.find(l)->second << std::endl;
		}
		
	}
};
