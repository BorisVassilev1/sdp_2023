#pragma once

#include <iterator>
#include <ranges>
#include <string_view>
#include <unordered_set>
#include <Regex/FST.hpp>
#include <span>
#include <cassert>
#include <queue>
#include "Regex/TFSA.hpp"

using sv = std::string_view;

template <class T>
std::string toString(T &&t) {
	// if(t.begin() == t.end()) return "";
	return std::string(t.begin(), t.end());
}

template <class Letter, class T>
std::vector<Letter> toLetter(T &&t) {
	return std::vector(std::begin(t), std::end(t));
}

template <class U, class V>
auto commonPrefix(U &&a, V &&b) {
	auto it1 = std::begin(a);
	auto it2 = std::begin(b);
	while (it1 != std::end(a) && it2 != std::end(b) && *it1 == *it2) {
		++it1;
		++it2;
	}
	return std::ranges::subrange(std::begin(a), it1);
}

template <class U, class V>
auto commonPrefixLen(U &&a, V &&b) {
	auto it1 = std::begin(a);
	auto it2 = std::begin(b);
	while (it1 != std::end(a) && it2 != std::end(b) && *it1 == *it2) {
		++it1;
		++it2;
	}
	return std::distance(std::begin(a), it1);
}

template <class U, class V>
auto remainderSuffix(U &&w, V &&s) {
	assert(std::distance(std::begin(w), std::end(w)) <= std::distance(std::begin(s), std::end(s)));
	return std::ranges::subrange(std::begin(s) + std::distance(std::begin(w), std::end(w)), std::end(s));
}

template <class U, class V, class W, class X>
auto w(U &&u, V &&v, W &&alpha, X &&beta) {
	auto u_alpha = std::views::concat(u, alpha);
	auto v_beta	 = std::views::concat(v, beta);
	auto c		 = commonPrefix(u_alpha, v_beta);
	return std::tuple((remainderSuffix(c, std::move(u_alpha))), (remainderSuffix(c, std::move(v_beta))));
}

template <class U, class V, class W, class X>
auto w_noref(U &&u, V &&v, W &&alpha, X &&beta) {
	auto u_alpha = std::views::concat(u, alpha);
	auto v_beta	 = std::views::concat(v, beta);
	auto c		 = commonPrefix(u_alpha, v_beta);
	return std::tuple(toLetter<Letter>(remainderSuffix(c, std::move(u_alpha))),
					  toLetter<Letter>(remainderSuffix(c, std::move(v_beta))));
}

template <class U, class V>
bool balancible(U &&u, V &&v) {
	// std::cout << "(" << u.size() << " " << v.size() << ")";
	return u.size() == 0 || v.size() == 0;
}

template <class T>
bool balancible(T &&t) {
	return balancible(std::get<0>(t), std::get<1>(t));
}

template <class U, class V>
bool eq(U &&u, V &&v) {
	if (u.size() != v.size()) return false;
	for (const auto &[a, b] : std::views::zip(u, v)) {
		if (a != b) return false;
	}
	return true;
}

/// expects trimmed real-time FST
template <class Letter>
bool isFunctional(const TFSA<Letter> &fst) {
	// create the squared putput transducer and compute Adm(q) for every state q in it;

	using State = typename TFSA<Letter>::State;

	// check output of empty word
	int eps_out = -1;
	for (const auto &q : fst.f_eps) {
		if (eps_out == -1) {
			eps_out = q;
		} else if (!std::ranges::equal(fst.words[q], fst.words[eps_out])) return false;
	}

	std::unordered_map<std::tuple<State, State>, std::tuple<std::vector<Letter>, std::vector<Letter>>> Adm;
	std::queue<std::tuple<State, State>>															   queue;

	std::vector<bool> coFinals(fst.N * fst.N, false);
	{
		std::vector<std::vector<std::tuple<Letter, State>>> reverseTransitions;
		reverseTransitions.resize(fst.N);
		for (const auto &[from, rhs] : fst.transitions) {
			const auto &[l, _, to] = rhs;
			reverseTransitions[to].emplace_back(l, from);	  // reverse transitions
		}

		auto DeltaRev = [&reverseTransitions](State i, State j) {
			return std::views::cartesian_product(reverseTransitions[i], reverseTransitions[j]) |
				   std::views::filter([](const auto &pair) {
					   const auto &[t1, t2] = pair;
					   const auto &[a, _]	= t1;
					   const auto &[b, _]	= t2;
					   return a == b;	  // only consider transitions with the same Letter
				   });
		};

		std::queue<std::tuple<State, State>> queueRev;
		for (const auto &q : fst.qFinals) {
			for (const auto &q2 : fst.qFinals) {
				queueRev.push({q, q2});
				coFinals[q * fst.N + q2] = true;	 // mark as co-final
			}
		}
		while (!queueRev.empty()) {
			auto Q = queueRev.front();
			queueRev.pop();
			auto &[q, h] = Q;

			auto Dq = DeltaRev(q, h);
			for (const auto &[t1, t2] : Dq) {
				auto &[_, i] = t1;
				auto &[_, j] = t2;
				if (coFinals[i * fst.N + j]) continue;
				coFinals[i * fst.N + j] = true;		// mark as co-final
				queueRev.push({i, j});
			}
		}
	}
	int cnt = 0;
	for (const auto q : coFinals) {
		if (q) cnt++;
	}
	std::cout << "coFinals: " << cnt << " / " << fst.N * fst.N << std::endl;

	auto isCoFinal = [&coFinals, &fst](State i, State j) { return coFinals[i * fst.N + j]; };
	auto isFinal   = [&fst](State i, State j) { return fst.qFinals.contains(i) && fst.qFinals.contains(j); };

	auto Delta = [&](State i, State j) {
		auto [b1, e1] = fst.transitions.equal_range(i);
		auto [b2, e2] = fst.transitions.equal_range(j);
		auto r1		  = std::ranges::subrange(b1, e1);
		auto r2		  = std::ranges::subrange(b2, e2);
		return std::views::cartesian_product(r1, r2) | std::views::filter([&](const auto &pair) {
				   const auto &[t1, t2]	   = pair;
				   const auto &[_, value1] = t1;
				   const auto &[_, value2] = t2;
				   const auto &[a, _, to1] = value1;
				   const auto &[b, _, to2] = value2;
				   return a == b && isCoFinal(to1, to2);	 // only consider transitions with the same letter
			   });
	};

	for (const auto &q : fst.qFirsts) {
		for (const auto &q2 : fst.qFirsts) {
			queue.push({q, q2});
			Adm.insert({{q, q2}, {{}, {}}});
		}
	}
	bool functional = true;
	while (!queue.empty() && functional) {
		auto Q = queue.front();
		// std::cout << Q << std::endl;
		auto &[q, h] = Q;
		queue.pop();

		auto &[u, v] = Adm[Q];	   // always computed
		// std::cout << "\'" << u << "\',\'" << v << "\'" << std::endl;
		auto Dq = Delta(q, h);
		for (const auto &[t1, t2] : Dq) {
			auto &[_, value1] = t1;
			auto &[_, value2] = t2;
			auto &[_, id1, i] = value1;
			auto &[_, id2, j] = value2;
			auto [h_1, h_2]	  = w_noref(u, v, fst.words[id1], fst.words[id2]);

			auto q2 = std::tuple(i, j);
			//  functional(i+1) := ∀(q′, h′) ∈ Dq : (balancible(h′) ∧
			// ((q′ ∈ F ) → (h′ = (ε, ε))) ∧ (! Adm(i)(q′) → (h′ = Adm(i)(q′))));
			functional &= balancible(h_1, h_2);
			functional &= !isFinal(i, j) || (h_1.empty() && h_2.empty());
			auto q2_it = Adm.find(q2);
			functional &=
				q2_it == Adm.end() || (eq(h_1, std::get<0>(q2_it->second)) && eq(h_2, std::get<1>(q2_it->second)));

			if (functional) {
				if (q2_it == Adm.end()) queue.push(q2);
				Adm.insert({{i, j}, {toLetter<Letter>(h_1), toLetter<Letter>(h_2)}});
			} else {
				std::cout << "-> \"" << toString(h_1) << "\", \"" << toString(h_2) << "\"" << std::endl;
				std::cout << "len: " << h_1.size() << " " << h_2.size() << std::endl;
				std::cout << "balancible: " << balancible(h_1, h_2) << std::endl;
				std::cout << "isFinal: " << isFinal(i, j) << std::endl;
				std::cout << "Q = (" << q << ", " << h << ")" << std::endl;
				std::cout << "q2 = (" << std::get<0>(q2) << ", " << std::get<1>(q2) << ")" << std::endl;
				if (q2_it != Adm.end()) {
					std::cout << "Adm(" << i << ", " << j << ") = (" << toString(std::get<0>(q2_it->second)) << ", "
							  << toString(std::get<1>(q2_it->second)) << ")" << std::endl;
				} else {
					std::cout << "Adm(" << i << ", " << j << ") not found" << std::endl;
				}
				std::cout << "cofinal(" << i << ", " << j << ") = " << isCoFinal(i, j) << std::endl;

				return false;	  // not functional
			}
		}
	}

	return true;
}
