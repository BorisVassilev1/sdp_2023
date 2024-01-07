#pragma once

#include <sstream>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <ranges>

#include "cfg.h"
#include "utils.h"

template <class State = std::size_t, class Letter = char>
class DPDA {
   public:
	using DeltaMap =
		std::unordered_map<std::tuple<State, Letter, Letter>, std::tuple<State, std::vector<Letter>>, tuple_hash>;
	DeltaMap delta;

	State qFinal	   = 0;
	bool  enable_print = false;

	DPDA() {}

	DPDA(const CFG<Letter> &grammar) {
		addTransition(0, Letter::eps, Letter::eps, 1, {grammar.start});

		const auto nullable = grammar.findNullables();
		const auto first	= grammar.findFirsts(nullable);
		const auto follow	= grammar.findFollows(nullable, first);

		auto f = [](const Letter l) -> State { return Letter::size + l; };

		try {
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

		for (const auto l : grammar.terminals) {
			addTransition(1, l, Letter::eps, f(l), {});
			if (l != grammar.eof) addTransition(f(l), Letter::eps, l, 1, {});
		}
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to create parser for grammar"));
		}

		qFinal = f(grammar.eof);
	}

	void printState(State state, size_t offset, const std::vector<Letter> stack, const std::vector<Letter> &word) {
		std::cout << "(" << state << " , ";
		if (offset < word.size()) {
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

	template <class T, class U>
		requires std::ranges::random_access_range<T> && std::ranges::random_access_range<U>
	ParseNode<Letter> *makeParseTree(const U &productions, const T &word, int &k, int &j) {
		auto node	 = new ParseNode<Letter>(std::get<2>(productions[k].first), {});
		auto product = std::get<1>(productions[k].second);
		++k;
		if (product.empty()) node->children.push_back(new ParseNode<Letter>{Letter::eps, {}});

		for (size_t i = 0; i < product.size(); ++i) {
			if (product[i] == word[j]) {
				node->children.push_back(new ParseNode<Letter>{word[j], {}});
				++j;
			} else {
				node->children.push_back(makeParseTree(productions, word, k, j));
			}
		}

		return node;
	}

	void detectMistake(const std::vector<Letter> &word, std::size_t offset, const std::vector<Letter> &stack,
					   State current_state) {
		for (size_t i = 0; i < word.size(); ++i) {
			std::cout << word[i];
		}
		std::cout << std::endl;
		if (offset > 0)
			for (size_t i = 0; i < offset - 1; ++i) {
				std::cout << " ";
			}
		std::cout << "^ mistake here" << std::endl;
		if (!stack.empty() && current_state > Letter::size) {
			std::cout << "expected a/an " << stack.back() << ", but got " << Letter(current_state - Letter::size)
					  << std::endl;
		} else if (stack.empty() && offset == word.size()) {
			std::cout << "unexpected end of file" << std::endl;
		} else if (current_state > Letter::size) {
			std::cout << "unexpected " << Letter(current_state - Letter::size) << std::endl;
		}
	}

	bool recognize(const std::vector<Letter> &word) {
		std::size_t					offset = 0;
		std::vector<Letter>			stack;
		State						current_state = 0;
		typename DeltaMap::iterator res			  = delta.begin();

		while ((current_state != qFinal || !stack.empty()) && res != delta.end()) {
			const Letter &l = offset < word.size() ? word[offset] : Letter::eps;
			const Letter &s = stack.empty() ? Letter::eps : stack.back();
			if (enable_print) { printState(current_state, offset, stack, word); }

			res = transition(current_state, l, s, stack, offset);
			if (res == delta.end() && s) { res = transition(current_state, l, '\0', stack, offset); }
			if (res == delta.end() && l) { res = transition(current_state, '\0', s, stack, offset); }
			if (res == delta.end() && s && l) { res = transition(current_state, '\0', '\0', stack, offset); }
		}
		if (enable_print) { printState(current_state, offset, stack, word); }

		if (current_state != qFinal || !stack.empty()) {
			detectMistake(word, offset, stack, current_state);
		} else {
			for (size_t i = 0; i < word.size(); ++i) {
				std::cout << word[i];
			}
			std::cout << std::endl << "\taccepted" << std::endl;
		}

		return current_state == qFinal && stack.empty();
	}

	ParseNode<Letter> *parse(const std::vector<Letter> &word) {
		std::size_t								   offset = 0;
		std::vector<Letter>						   stack;
		State									   current_state = 0;
		typename DeltaMap::iterator				   res			 = delta.begin();
		std::vector<typename DeltaMap::value_type> productions;

		while ((current_state != qFinal || !stack.empty()) && res != delta.end()) {
			const Letter &l = offset < word.size() ? word[offset] : Letter::eps;
			const Letter &s = stack.empty() ? Letter::eps : stack.back();
			if (enable_print) { printState(current_state, offset, stack, word); }

			res = transition(current_state, l, s, stack, offset);
			if (res == delta.end() && s) { res = transition(current_state, l, '\0', stack, offset); }
			if (res == delta.end() && l) { res = transition(current_state, '\0', s, stack, offset); }
			if (res == delta.end() && s && l) { res = transition(current_state, '\0', '\0', stack, offset); }

			if (res != delta.end() && std::get<0>(res->first) == std::get<0>(res->second)) productions.push_back(*res);
		}
		if (enable_print) { printState(current_state, offset, stack, word); }

		if (current_state != qFinal || !stack.empty()) {
			detectMistake(word, offset, stack, current_state);
		} else {
			for (size_t i = 0; i < word.size(); ++i) {
				std::cout << word[i];
			}
			std::cout << std::endl << "\taccepted" << std::endl;

			int	 i = 0, k = 0;
			auto parseTree = makeParseTree(productions, word, k, i);

			return parseTree;
		}

		return nullptr;
	}

	template <typename U = Letter>
		requires std::is_convertible_v<U, char>
	ParseNode<Letter> *parse(const std::string &word) {
		return parse(std::vector<Letter>(word.begin(), word.end()));
	}

	template <typename U = Letter>
		requires std::is_convertible_v<U, char>
	bool recognize(const std::string &word) {
		return recognize(std::vector<Letter>(word.begin(), word.end()));
	}

	DeltaMap::iterator transition(State &q, Letter a, Letter top, std::vector<Letter> &stack, std::size_t &offset) {
		std::tuple<State, Letter, Letter> search = {q, a, top};
		auto							  res	 = delta.find(search);
		if (res == delta.end()) { return res; }

		if (top) stack.pop_back();
		for (const auto &l : std::ranges::views::reverse(std::get<1>(res->second))) {
			stack.push_back(l);
		}
		q = std::get<0>(res->second);
		if (a != '\0') ++offset;
		return res;
	}

	void addTransition(State q, Letter a, Letter x, State q1, const std::vector<Letter> &w) {
		auto to_insert =
			std::make_pair(std::tuple<State, Letter, Letter>{q, a, x}, std::tuple<State, std::vector<Letter>>{q1, w});
		auto found = delta.find(to_insert.first);
		if (found != delta.end()) {
			std::stringstream s;
			s << "Failed to add transition: New transition \n"
			  << to_insert << "\n conflicts with the already existing \n"
			  << *found << std::endl;
			throw std::runtime_error(s.str());
		}
		delta.insert(to_insert);
	}

	template <class U = Letter>
		requires std::is_convertible_v<Letter, char>
	void addTransition(State q, Letter a, Letter x, State q1, const std::string &w) {
		return addTransition(q, a, x, q1, std::vector<Letter>(w.begin(), w.end()));
	}

	void printToStream(std::ostream &out) {
		out << "digraph {\n overlap = false; splines = true; nodesep = 0.3; layout = dot;\n";
		for (const auto &p : delta) {
			out << "\t\"" << std::get<0>(p.first) << "\" -> \"" << std::get<0>(p.second)
				<< "\" [xlabel = \"" << std::get<1>(p.first) << ", " << std::get<2>(p.first);
			if(!std::get<1>(p.second).empty()) {
				out << " / " << std::get<1>(p.second);
			}
			out << "\" , minlen = \"3\"];" << std::endl;
		}
		out << "}";
	}
};
