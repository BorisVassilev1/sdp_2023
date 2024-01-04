#include "cfg.h"

template <class Letter>
std::unordered_map<Letter, bool> CFG<Letter>::findNullables() const {
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
			for (Letter l : v) {
				if (res.contains(l) && !res.find(l)->second) {
					res.insert({k, false});
					isNullable = false;
					break;
				}
			}
			if (isNullable) { res.insert({k, true}); }
		}
	}

	/*for (const Letter l : nonTerminals) {
		std::cout << l << " : " << res.find(l)->second << std::endl;
	}*/
	return std::move(res);
}

template <class Letter>
bool CFG<Letter>::nullable(const std::vector<Letter> &w, const std::unordered_map<Letter, bool> &nullable) const {
	if (w.empty()) return true;
	for (const auto x : w) {
		if (nullable.find(x)->second) return true;
	}
	return false;
}
template<class Letter>
auto CFG<Letter>::findFirsts(const std::unordered_map<Letter, bool> &nullable) const {
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

	/*for (const Letter l : nonTerminals) {
		std::cout << l << " -> " << first.find(l)->second << std::endl;
	}*/

	return first;
}
