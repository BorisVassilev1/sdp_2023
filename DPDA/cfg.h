#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

/**
 * @brief A Context-Free Grammar
 * 
 * @tparam Letter - Type of symbols in the alphabet
 */
template <class Letter>
class CFG {
   public:
	std::unordered_multimap<Letter, std::vector<Letter>> rules;
	std::unordered_set<Letter, std::hash<Letter>>		 terminals;
	std::unordered_set<Letter, std::hash<Letter>>		 nonTerminals;
	Letter												 start;
	Letter												 eof = Letter::eof;

   public:
	CFG(const Letter &start, const Letter &eof) : start(start), eof(eof) {}

	/**
	 * @brief computes if symbols are nullable
	 * http://ll1academy.cs.ucla.edu/static/LLParsing.pdf
	 * @return std::unordered_map<Letter, bool>
	 */
	std::unordered_map<Letter, bool> findNullables() const {
		std::unordered_map<Letter, bool> res;

		for (const auto &[k, v] : rules) {
			if (v.empty()) { res.insert({k, true}); }
		}
		for (Letter l : terminals) {
			res.insert({l, false});
		}

		std::size_t prevSize = 0;
		while (prevSize != res.size()) {
			prevSize = res.size();

			for (const auto &[k, v] : rules) {
				if (res.contains(k)) continue;

				bool isNullable = true;
				bool isKnown = true;
				for (Letter l : v) {
					bool contains = res.contains(l);
					if (contains && !res.find(l)->second) {
						res.insert({k, false});
						isNullable = false;
						break;
					}
					isKnown = isKnown && contains;
				}
				if (isNullable && isKnown) { res.insert({k, true}); }
			}
		}

		return std::move(res);
	}

	/**
	 * @brief computes if epsilon can be derived from a word \ref{w}.
	 * http://ll1academy.cs.ucla.edu/static/LLParsing.pdf
	 *
	 * @param w - word
	 * @param nullable - the result from CFG::findNullables
	 * @return true - if the above statement is true
	 * @return false - else
	 */
	bool nullable(const std::vector<Letter> &w, const std::unordered_map<Letter, bool> &nullable) const {
		if (w.empty()) return true;
		for (const auto x : w) {
			if (nullable.find(x)->second) return true;
		}
		return false;
	}

	/**
	 * @brief finds the FIRST set for all symbols in the grammar
	 * http://ll1academy.cs.ucla.edu/static/LLParsing.pdf
	 *
	 * @param nullable - the result from CFG::findNullables
	 * @return std::unordered_map<Letter, std::unordered_set<Letter>>
	 */
	std::unordered_map<Letter, std::unordered_set<Letter>> findFirsts(
		const std::unordered_map<Letter, bool> &nullable) const {
		std::unordered_map<Letter, std::unordered_set<Letter>> first;

		for (Letter l : terminals) {
			first.insert({l, {l}});
		}
		for (Letter l : nonTerminals) {
			first.insert({l, {}});
		}

		bool change = true;
		while (change) {
			change = false;
			for (const auto &[k, v] : rules) {
				auto &firstSet = first.find(k)->second;
				if (!v.empty() && first.contains(v[0])) {
					for (const auto l : first.find(v[0])->second) {
						if (!firstSet.contains(l)) {
							firstSet.insert(l);
							change = true;
						}
					}
				}
				for (size_t i = 0; i < v.size(); ++i) {
					if (i)
						for (const auto l : first.find(v[i])->second) {
							if (!firstSet.contains(l)) {
								firstSet.insert(l);
								change = true;
							}
						}
					if (!nullable.find(v[i])->second) break;
				}
			}
		}

		return first;
	}
	/**
	 * @brief checks if x is in the FIRST set for the word w
	 * http://ll1academy.cs.ucla.edu/static/LLParsing.pdf
	 *
	 * @param x
	 * @param w
	 * @param nullable
	 * @param first
	 * @return true
	 * @return false
	 */
	bool isFirst(Letter x, const std::vector<Letter> &w, const std::unordered_map<Letter, bool> &nullable,
				 const std::unordered_map<Letter, std::unordered_set<Letter>> &first) const {
		for (size_t i = 0; i < w.size(); ++i) {
			if (first.find(w[i])->second.contains(x)) return true;
			if (!nullable.find(w[i])->second) break;
		}
		return false;
	}

	/**
	 * @brief computes the FIRST set for a word w
	 * http://ll1academy.cs.ucla.edu/static/LLParsing.pdf
	 * @param w
	 * @param nullable
	 * @param first
	 * @return std::unordered_set<Letter>
	 */
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

	/**
	 * @brief Computes the FOLLOW set for all symbols in the grammar
	 * http://ll1academy.cs.ucla.edu/static/LLParsing.pdf
	 * @param nullable
	 * @param first
	 * @return std::unordered_map<Letter, std::unordered_set<Letter>>
	 */
	std::unordered_map<Letter, std::unordered_set<Letter>> findFollows(
		const std::unordered_map<Letter, bool>						 &nullable,
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

		return follow;
	}

	CFG() {}

	/**
	 * @brief Computes and prints the parse table to stdout
	 */
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

	/**
	 * @brief Prints all rules of the grammar to stdout
	 */
	void printRules() const {
		for (const auto &[A, v] : rules) {
			if (v.empty()) std::cout << A << " -> " << Letter::eps << std::endl;
			else std::cout << A << " -> " << v << std::endl;
		}
	}

	/**
	 * @brief Adds the rule (a -> w) to the grammar, where a is a nonterminal symbol and w is a word
	 *
	 * @param a - nonterminal
	 * @param w - a word
	 */
	void addRule(Letter a, const std::vector<Letter> &w) { rules.insert({a, w}); }

	/**
	 * @brief adds the rule (a -> w) to the grammar. Available only if \ref{Letter} can be implicitly constructed from
	 * char
	 *
	 * @tparam U - dummy
	 * @param a - a nonterminal symbol
	 * @param w - a word
	 * @return requires
	 */
	template <class U = Letter>
		requires std::is_convertible_v<char, Letter>
	void addRule(char a, const std::string &w) {
		addRule(a, std::vector<Letter>(w.begin(), w.end()));
	}
};
