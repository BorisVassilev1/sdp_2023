#pragma once

#include <exception>
#include <format>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <ranges>

#include "cfg.h"
#include "utils.h"

class ParseError : public std::exception {
   protected:
	std::string msg;
	std::size_t position;

   public:
	ParseError(const std::string &msg, std::size_t position) : msg(msg + " at " + std::to_string(position)), position(position) {}
	ParseError(const char *msg, std::size_t position) : msg(std::string(msg) + " at " + std::to_string(position)), position(position) {}

	virtual ~ParseError() noexcept {}

	virtual const char *what() const noexcept { return msg.c_str(); }
};

template <class State = std::size_t, class Letter = char>
class DPDA {
   public:
	using DeltaMap =
		std::unordered_map<std::tuple<State, Letter, Letter>, std::tuple<State, std::vector<Letter>>, tuple_hash>;
	DeltaMap delta;

	State qFinal	   = 0;
	bool  enable_print = false;

	DPDA() {}

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

		bool accepted = current_state == qFinal && stack.empty() && offset == word.size();

		return accepted;
	}


	template <typename U = Letter>
	bool recognize(const std::string &word) {
		std::vector<Letter> w;
		for(std::size_t i = 0; i < word.size(); ++i) {
			w.push_back(Letter(word[i]));
		}
		return recognize(w);
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
			throw std::runtime_error(std::format("Failed to add transition: New transition {} conflicts with {}",
				to_insert, *found
			));
		}
		delta.insert(to_insert);
	}

	template <class U = Letter>
		requires std::is_convertible_v<Letter, char>
	void addTransition(State q, Letter a, Letter x, State q1, const std::string &w) {
		return addTransition(q, a, x, q1, std::vector<Letter>(w.begin(), w.end()));
	}

	void printToDOT(std::ostream &out) {
		out << "digraph {\n overlap = false; splines = true; nodesep = 0.3; layout = dot;\n";
		for (const auto &p : delta) {
			out << "\t\"" << std::get<0>(p.first) << "\" -> \"" << std::get<0>(p.second) << "\" [xlabel = \""
				<< std::get<1>(p.first) << ", " << std::get<2>(p.first);
			if (!std::get<1>(p.second).empty()) { out << " / " << std::get<1>(p.second); }
			out << "\" , minlen = \"3\"];" << std::endl;
		}
		out << "}";
	}
};
