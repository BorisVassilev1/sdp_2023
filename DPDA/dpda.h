#pragma once

#include <cstdlib>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stack>
#include <assert.h>
#include <iostream>
#include <ranges>

#include "cfg.h"
#include "hashes.h"

template <class State = std::size_t, class Letter = char>
class DPDA {
   public:
	std::unordered_map<std::tuple<State, Letter, Letter>, std::tuple<State, std::vector<Letter>>, tuple_hash> delta;

	State qFinal = 0;
	bool enable_print = false;

	DPDA() {}

	DPDA(const CFG<Letter> &grammar) {
		addTransition(0, Letter::eps, Letter::eps, 1, {grammar.start});
		
		const auto nullable = grammar.findNullables();
		const auto first = grammar.findFirsts(nullable);
		const auto follow = grammar.findFollows(nullable, first);

		auto f = [](const Letter l) -> State { return Letter::size + l; };

		for (const auto &[A, v] : grammar.rules) {
			if (v.empty()) {
				const auto &followA = follow.find(A)->second;
				for (const auto l : followA) {
					addTransition(f(l), Letter::eps, A, f(l), v);
				}
			} else {
				const auto &firstA = grammar.first(v, nullable, first);
				for (const auto l : firstA) {
					addTransition(f(l), Letter::eps, A, f(l), v);
				}
			}
		}

		for(const auto l : grammar.terminals) {
			addTransition(1   , l          , Letter::eps, f(l), {});
			if(l != grammar.eof)
				addTransition(f(l), Letter::eps, l          , 1   , {});
		}
		
		addTransition(1, grammar.eof, Letter::eps, 2, {});
		
		qFinal = f(grammar.eof);

	}

	void printState(State state, size_t offset, const std::vector<Letter> stack, const std::vector<Letter> &word) {
		std::cout << "(";
		if (state < 128) std::cout << state;
		else std::cout << state;

		std::cout << " , ";
		if(offset < word.size()) {
		    for (auto a = word.begin() + offset; a != word.end(); ++a) {
				std::cout << *a;
			}
		}
		std::cout << " , ";
		for (const auto a : stack) {
			std::cout << a;
		}
		std::cout << ')' << std::endl;
	}

	void printTransitions() {
		for (const auto &t : delta) {
			std::cout << t << std::endl;
		}
	}

	bool parse(const std::vector<Letter> &word) {
		std::size_t			offset = 0;
		std::vector<Letter> stack;
		State current_state = 0;
		bool  res = true;

		while ((current_state != qFinal || !stack.empty()) && res) {
			const Letter &l = offset < word.size() ? word[offset] : Letter::eps;
			const Letter &s = stack.empty() ? Letter::eps : stack.back();
			if(enable_print) { printState(current_state, offset, stack, word); }

			res = transition(current_state, l, s, stack, offset);
			if (!res && s) {
				res = transition(current_state, l, '\0', stack, offset);
			}
			if (!res && l) {
				res = transition(current_state, '\0', s, stack, offset);
			}
			if (!res && s && l) {
				res = transition(current_state, '\0', '\0', stack, offset);
			}
		}
		if(enable_print) { printState(current_state, offset, stack, word); }

		if (current_state != qFinal || !stack.empty()) {
			for (size_t i = 0; i < word.size(); ++i) {
				std::cout << word[i];
			}
			std::cout << std::endl;
			if(offset > 0)
				for (size_t i = 0; i < offset - 1; ++i) {
					std::cout << " ";
				}
			std::cout << "^ mistake here" << std::endl;
			if(!stack.empty() && current_state > Letter::size) {
				std::cout << "expected " << stack.back() << ", but got " << Letter(current_state - Letter::size)  << std::endl;
			}
		} else {
			for (size_t i = 0; i < word.size(); ++i) {
				std::cout << word[i];
			}
			std::cout << std::endl << "\taccepted" << std::endl;
		}

		return current_state == qFinal;
	}

    template<typename U=Letter> requires std::is_convertible_v<U, char>
	bool parse(const std::string &word){ return parse(std::vector<Letter>(word.begin(), word.end())); }

	bool transition(State &q, Letter a, Letter top, std::vector<Letter> &stack, std::size_t &offset) {
		std::tuple<State, Letter, Letter> search = {q, a, top};
		auto							  res	 = delta.find(search);
		if (res == delta.end()) {
			//std::cout << search << " failed " << std::endl;
			return false;
		}
		//std::cout << "apply " << *res << std::endl;

		if (top) stack.pop_back();
		for (const auto &l : std::ranges::views::reverse(std::get<1>(res->second))) {
			stack.push_back(l);
		}
		q = std::get<0>(res->second);
		if (a != '\0') ++offset;
		return true;
	}

	void addTransition(State q, Letter a, Letter x, State q1, const std::vector<Letter> &w) {
		delta.insert(
			std::make_pair(std::tuple<State, Letter, Letter>{q, a, x}, std::tuple<State, std::vector<Letter>>{q1, w}));
	}

	template<class U=Letter> requires std::is_convertible_v<Letter, char>
	void addTransition(State q, Letter a, Letter x, State q1, const std::string &w) {
		return addTransition(q, a, x, q1, std::vector<Letter>(w.begin(),w.end()));
	}
};
