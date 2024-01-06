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
	Letter												 eof;

   public:
	std::unordered_map<Letter, bool> findNullables() const;

	bool nullable(const std::vector<Letter> &w, const std::unordered_map<Letter, bool> &nullable) const;

	std::unordered_map<Letter, std::unordered_set<Letter>> findFirsts(const std::unordered_map<Letter, bool> &nullable) const;

	bool isFirst(Letter x, const std::vector<Letter> &w, const std::unordered_map<Letter, bool> &nullable,
				 const std::unordered_map<Letter, std::unordered_set<Letter>> &first) const {
		for (size_t i = 0; i < w.size(); ++i) {
			if (first.find(w[i])->second.contains(x)) return true;
			if (!nullable.find(w[i])->second) break;
		}
		return false;
	}

	std::unordered_set<Letter> first(const std::vector<Letter> &w, const std::unordered_map<Letter, bool> &nullable,
									 const std::unordered_map<Letter, std::unordered_set<Letter>> &first) const {
		std::unordered_set<Letter> res;
		for (size_t i = 0; i < w.size(); ++i) {
			const auto &firstOfLetter = first.find(w[i])->second;
			for (const auto l : firstOfLetter) {
				res.insert(l);
			}
			if (!nullable.find(w[i])->second) break;
		}
		return res;
	}

	auto findFollows(const std::unordered_map<Letter, bool>						  &nullable,
					 const std::unordered_map<Letter, std::unordered_set<Letter>> &first) const {
		std::unordered_map<Letter, std::unordered_set<Letter>> follow;

		for (Letter l : nonTerminals) {
			follow.insert({l, {}});
		}

		follow.find(start)->second.insert(eof);

		bool change = true;
		while (change) {
			change = false;
			for (const auto &[A, v] : rules) {
				for (size_t i = 0; i < v.size(); ++i) {
					if (!nonTerminals.contains(v[i])) continue;
					// productions of type A -> aBb
					// where A and B are nonterminals and a and b are words
					auto	   &followB = follow.find(v[i])->second;
					const auto	b		= std::vector<Letter>{v.begin() + i + 1, v.end()};
					const auto &firstb	= this->first(b, nullable, first);

					if (!b.empty()) {
						for (const auto l : firstb) {
							if (!followB.contains(l)) {
								followB.insert(l);
								change = true;
							}
						}
					}

					if (this->nullable(b, nullable)) {
						const auto followA = follow.find(A)->second;
						for (const auto l : followA) {
							if (!followB.contains(l)) {
								followB.insert(l);
								change = true;
							}
						}
					}
				}
			}
		}

		/*for (const Letter l : nonTerminals) {
			std::cout << l << " => " << follow.find(l)->second << std::endl;
		}*/
		return follow;
	}

	CFG() {}

	void printParseTable() const {
		const auto nullable = findNullables();
		const auto first	= findFirsts(nullable);
		const auto follow	= findFollows(nullable, first);

		for (const auto &[A, v] : rules) {
			if (v.empty()) {
				const auto &followA = follow.find(A)->second;
				for (const auto l : followA) {
					std::cout << A << ", " << l << " ~~> " << A << " -> " << Letter::eps << std::endl;
				}
			} else {
				const auto &firstA = this->first(v, nullable, first);
				for (const auto l : firstA) {
					std::cout << A << ", " << l << " ~~> " << A << " -> " << v << std::endl;
				}
			}
		}
	}

	void addRule(Letter a, const std::vector<Letter> &w) { rules.insert({a, w}); }

	template <class U = Letter>
		requires std::is_convertible_v<char, Letter>
	void addRule(char a, const std::string &w) {
		addRule(a, std::vector<Letter>(w.begin(), w.end()));
	}
};
