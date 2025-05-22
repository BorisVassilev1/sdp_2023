#pragma once
#include "DPDA/cfg.h"

template <class Letter>
class EarleyParser {
	CFG<Letter> grammar;

   public:
	// struct DottedRule {
	//	Letter				 lhs;
	//	std::vector<Letter> *rhs;
	//	int					 dotPos;
	//	int					 j;
	// };

	using DottedRule = std::tuple<Letter, std::vector<Letter> *, int, int>;
	EarleyParser(const CFG<Letter> &g) : grammar(g) {}

	auto C(const std::vector<std::unordered_set<DottedRule>> &R, const std::unordered_set<DottedRule> &R_ip, int i) {
		std::unordered_set<DottedRule> C_s	 = R_ip;
		bool						   added = true;

		// auto &terminals	   = grammar.terminals;
		auto &nonTerminals = grammar.nonTerminals;
		auto &rules		   = grammar.rules;

		while (added) {
			added								= false;
			std::unordered_set<DottedRule> C_s1 = C_s;
			// std::cout << C_s << std::endl;
			for (auto &[A, w, dotPos, j] : C_s) {
				auto &β1Xβ2 = *w;
				if (dotPos < β1Xβ2.size() && nonTerminals.contains(β1Xβ2[dotPos])) {
					//  { (X → •β, i) | ∃ (A → β1•Xβ2, j) ∈ C(s) and X → β ∈ P }
					auto &X = β1Xβ2[dotPos];
					auto  r = rules.equal_range(X);
					for (auto &[_X, β] : RangeFromPair(r)) {
						auto [_, s] = C_s1.insert({X, &β, 0, i});
						added |= s;
					}

					// { (A → β1X•β2, j) | ∃ (A → β1X•β2, j) ∈ C(s) and (X → β•, i) ∈ C(s) }
					for (auto &[X_, w, dotPos_, i_] : C_s) {
						auto &β = *w;
						if (X_ == X && dotPos_ == β.size() && i_ == i) {
							auto [_, s] = C_s1.insert({A, &β1Xβ2, dotPos + 1, j});
							added |= s;
						}
					}
				}
			}

			// {(A → β1X • β2, j) | има β и k < i : ((A → β1 • Xβ2, j) ∈ ˜Rk и (X → β•, k) ∈ C(s))}
			for (int k = 0; k < i; ++k) {
				for (auto &[A, w, dotPos, j] : R[k]) {
					auto &β1Xβ2 = *w;
					if (dotPos < β1Xβ2.size() && nonTerminals.contains(β1Xβ2[dotPos])) {
						auto &X = β1Xβ2[dotPos];

						for (auto &[X_, w, dotPos_, k_] : C_s) {
							auto &β = *w;
							if (X_ == X && dotPos_ == β.size() && k_ == k) {
								auto [_, s] = C_s1.insert({A, &β1Xβ2, dotPos + 1, j});
								added |= s;
							}
						}
					}
				}
			}
			std::swap(C_s, C_s1);
		};
		return C_s;
	}

	bool recognize(const std::vector<Letter> &word) {
		std::vector<std::unordered_set<DottedRule>> R(word.size() + 1);
		std::vector<std::unordered_set<DottedRule>> Rp(word.size() + 1);

		auto r = grammar.rules.equal_range(grammar.start);
		for (auto &[S, β] : RangeFromPair(r)) {
			Rp[0].insert({S, &β, 0, 0});
		}
		std::cout << "R'[0] = ";
		print(Rp[0]);
		std::cout << std::endl;

		R[0] = C(R, Rp[0], 0);

		std::cout << "R[0] = ";
		print(R[0]);
		std::cout << std::endl;

		for (int i = 0; i < word.size(); ++i) {
			std::cout << "i = " << i << std::endl;
			// R′[i+1] = {(A → β1a • β2, j) | (A → β1 • aβ2, j) ∈ Ri и a = ai+1}
			for (auto &[A, w, dotPos, j] : R[i]) {
				auto &β1aβ2 = *w;
				if (dotPos < β1aβ2.size() && β1aβ2[dotPos] == word[i]) { Rp[i + 1].insert({A, &β1aβ2, dotPos + 1, j}); }
			}
			// Ri+1 = Cε( R, R[i+1], i + 1)
			R[i + 1] = C(R, Rp[i + 1], i + 1);

			std::cout << "R'[" << i + 1 << "] = ";
			print(Rp[i + 1]);
			std::cout << std::endl;
			std::cout << "R[" << i + 1 << "] = ";
			print(R[i + 1]);
			std::cout << std::endl;
		}

		for (auto &[A, w, dotPos, j] : R[word.size()]) {
			auto &β = *w;
			if (dotPos == β.size() && A == grammar.start) return true;
		}
		return false;
	}

	auto &print(const std::unordered_set<DottedRule> &R, std::ostream &out = std::cout) {
		out << "(";
		int i = 0;
		for (auto &r : R) {
			if (i > 0) out << ", ";
			print(r, out);
			++i;
		}
		out << ")";
		return out;
	}

	auto &print(const DottedRule &r, std::ostream &out = std::cout) {
		auto &[A, w, dotPos, j] = r;
		out << "(" << A << " -> ";
		auto &β = *w;
		for (int i = 0; i < β.size(); ++i) {
			if (i == dotPos) out << "•";
			out << β[i];
		}
		if (dotPos == β.size()) out << "•";
		std::cout << ", " << j << ")";
		return out;
	}
};
