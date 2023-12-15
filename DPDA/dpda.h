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

template <int N, typename... Ts>
using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

template <class... Args>
std::ostream &operator<<(std::ostream &out, const std::tuple<Args...> &t);
template <class A, class B>
std::ostream &operator<<(std::ostream &out, const std::pair<A, B> &t);
template <class T>
std::ostream &operator<<(std::ostream &out, std::vector<T> v);

struct tuple_hash {
	template <class... Args>
	std::size_t operator()(const std::tuple<Args...> &t) const {
		return [&]<std::size_t... p>(std::index_sequence<p...>) {
			return ((std::hash<NthTypeOf<p, Args...>>{}(std::get<p>(t))) ^ ...);
		}(std::make_index_sequence<std::tuple_size_v<std::tuple<Args...>>>{});
	}
};

template <class... Args>
std::ostream &operator<<(std::ostream &out, const std::tuple<Args...> &t) {
	out << "(";
	[&]<std::size_t... p>(std::index_sequence<p...>) {
		((out << (p ? ", " : "") << "\'" << std::get<p>(t) << "\'"), ...);
	}(std::make_index_sequence<std::tuple_size_v<std::tuple<Args...>>>{});
	return out << ")";
}

template <class A, class B>
std::ostream &operator<<(std::ostream &out, const std::pair<A, B> &t) {
	return out << "(" << t.first << ", " << t.second << ")";
}

template <class T>
std::ostream &operator<<(std::ostream &out, std::vector<T> v) {
	for (const T &x : v)
		out << x;
	return out;
}

template <class State = std::size_t, class Letter = char>
class DPDA {
   public:
	std::unordered_map<std::tuple<State, Letter, Letter>, std::tuple<State, std::vector<Letter>>, tuple_hash> delta;

	State qFinal = 0;

	DPDA() {}

	void printState(State state, size_t offset, const std::vector<Letter> stack, const std::vector<Letter> &word) {
		std::cout << "(";
		if (state < 128) std::cout << state;
		else std::cout << "f" << char(state - 128);

		std::cout << " , ";
		for (auto a = word.begin() + offset; a != word.end(); ++a) {
			std::cout << *a;
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
		stack.push_back('Z');
		State current_state;
		bool  res = true;

		while ((current_state != qFinal || stack.size() > 1) && res) {
			const Letter &l = offset < word.size() ? word[offset] : '\0';
			//printState(current_state, offset, stack, word);
			res = transition(current_state, l, stack.back(), stack, offset);
			if (!res) {
				res = transition(current_state, l, '\0', stack, offset);
			}
			if (!res) {
				res = transition(current_state, '\0', stack.back(), stack, offset);
			}
			if (!res) {
				res = transition(current_state, '\0', '\0', stack, offset);
			}
		}
		//printState(current_state, offset, stack, word);

		if (current_state != qFinal || !(stack.size() == 1)) {
			for (size_t i = 0; i < word.size(); ++i) {
				std::cout << word[i];
			}
			std::cout << std::endl;
			for (size_t i = 0; i < offset - 1; ++i) {
				std::cout << " ";
			}
			std::cout << "^ mistake here" << std::endl;
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
