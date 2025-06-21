#pragma once

#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cassert>

#include <Regex/FST.hpp>
#include "Regex/wordset.hpp"

// Classical Two-Tape Finite State Automaton (TFSA)
template <class Letter>
class TFSA {
   public:
	using State	   = unsigned int;
	using StringID = typename WordSet<Letter>::WordID;

	using Map = std::unordered_multimap<State, std::tuple<Letter, StringID, State>>;

	unsigned int			  N;
	std::unordered_set<State> qFirsts;
	std::unordered_set<State> qFinals;
	Map						  transitions;
	WordSet<Letter>			  words;

	void addTransition(State from, Letter w1, StringID w2, State to) { transitions.insert({from, {w1, w2, to}}); }

	void print(std::ostream &out) const {
		// print in DOT
		out << "digraph TFSA {\n";
		out << "  rankdir=LR;\n";
		out << "  node [shape=circle];\n";
		out << "  init [label=\"N=" << this->N << "\", shape=square];\n";
		for (const int i : qFinals) {
			out << "  " << i << " [shape=doublecircle];\n";		// final States
		}
		for (const int i : qFirsts) {
			out << "  init -> " << i << " [style=dotted];\n";	  // initial states
		}
		for (const auto &[from, value] : transitions) {
			auto &[w1, id2, to] = value;
			auto w2				= words.getWord(id2);
			out << "  " << from << " -> " << to << " [label=\"<" << w1 << ",";
			for (const auto &letter : w2) {
				if (letter < 128 && letter >= 32) out << letter;
				else out << (int)letter;
			}
			out << ">\"];\n";
		}
		out << "}\n";
	}

	State newState() { return ++N; }
};

template <class Letter>
auto expandFST(FST<Letter> &&fst) {
	TFSA<Letter> expanded;
	using State		 = TFSA<Letter>::State;
	using StringID	 = TFSA<Letter>::StringID;
	expanded.N		 = fst.N;
	expanded.qFirsts = std::move(fst.qFirsts);
	expanded.qFinals = std::move(fst.qFinals);

	for (const auto &[from, value] : fst.transitions) {
		auto [id1, id2, to] = value;
		auto &w2			= fst.words[id2];
		if (id1 == 0) {
			auto new_id = expanded.words.addWord(w2);
			expanded.addTransition(from, Letter::eps, new_id, to);
			continue;
		}
		auto &w1   = fst.words[id1];
		State prev = from;

		if (w1.size() < w2.size()) {	 // |w1| < |w2|
			assert(w1.size() > 0);
			for (unsigned int i = 0; i < w1.size() - 1; ++i) {
				const auto &a	 = w1[i];
				const auto &b	 = w2[i];
				State		next = expanded.newState();
				StringID	w2id = expanded.words.addWord(std::span{&b, &b + 1});
				expanded.addTransition(prev, a, w2id, next);
				prev = next;
			}
			StringID w2id = expanded.words.addWord(std::span{w2.data() + w1.size() - 1, w2.size() - w1.size() + 1});
			expanded.addTransition(prev, w1.back(), w2id, to);
		} else {
			for (unsigned int i = 0; i < w2.size(); ++i) {
				const auto &a	 = w1[i];
				const auto &b	 = w2[i];
				State		next = (i == w1.size() - 1) ? to : expanded.newState();
				StringID	w2id = expanded.words.addWord(std::span{&b, &b + 1});
				expanded.addTransition(prev, a, w2id, next);
				prev = next;
			}
			for (unsigned int i = w2.size(); i < w1.size(); ++i) {
				const auto &a	 = w1[i];
				State		next = (i == w1.size() - 1) ? to : expanded.newState();
				expanded.addTransition(prev, a, 0, next);
				prev = next;
			}
		}
	}

	return expanded;
}

template <class Letter>
void drawFSA(const TFSA<Letter> &fsa) {
	ShellProcess p("dot -Tsvg > a.svg && feh ./a.svg");
	fsa.print(p.in());
	p.in() << std::endl;
	p.in().close();
	p.wait();
	std::cout << getString(p.out()) << std::endl;
	std::cout << getString(p.err()) << std::endl;
}


template <class Letter>
auto realtimeFST(TFSA<Letter> &&fsa) {
	using State	   = TFSA<Letter>::State;
	using StringID = TFSA<Letter>::StringID;

	std::stack<int>				stack;
	std::vector<bool>				visited(fsa.N, false);
	std::vector<std::vector<std::tuple<State, std::vector<Letter>>>> closure(fsa.N);

	for (State i = 0; i < fsa.N; ++i) {
		stack.push(0);
		visited[i] = true;
		closure[i].push_back({i, {}});	// add the state itself with an empty word
		while (!stack.empty()) {
			auto p = stack.top();
			const auto [current, u] = closure[i][p];
			stack.pop();

			auto [i1, i2] = fsa.transitions.equal_range(current);
			for (const auto &[_, value] : std::ranges::subrange(i1, i2)) {
				const auto &[w1, id2, to] = value;
				if (w1 == Letter::eps) {		// epsilon transition
					visited[to] = true;
					auto new_word = u;
					new_word.insert(new_word.end(), fsa.words.getWord(id2).begin(), fsa.words.getWord(id2).end());
					closure[i].push_back({to, std::move(new_word)});
					stack.push(closure[i].size() - 1);
				}
			}
		}

		visited.assign(fsa.N, false);
	}

	std::erase_if(fsa.transitions, [](const auto &pair) {
		const auto &[from, value]  = pair;
		const auto &[w1, id2, to] = value;
		return w1 == Letter::eps;	 // remove epsilon transitions
	});

	int size = 0;
	for(const auto &c : closure) {
		size += c.size();
	}
	std::cout << "Closure size: " << size << std::endl;

	//typename TFSA<Letter>::Map new_transitions;
	//new_transitions.insert(fsa.transitions.begin(), fsa.transitions.end());
	//for (const auto &[from, value] : fsa.transitions) {
	//	const auto &[id1, id2, to] = value;
	//	if (id1 == 0 && id2 == 0) assert(false);
	//	for (const auto &next : closure[to]) {
	//		new_transitions.insert({from, {id1, id2, next}});
	//	}
	//}

	//std::unordered_set<State> new_qFirsts;
	//for (const State &i : fsa.qFirsts) {
	//	new_qFirsts.insert(i);
	//	for (const State &j : closure[i]) {
	//		new_qFirsts.insert(j);
	//	}
	//}
	//fsa.qFirsts = std::move(new_qFirsts);

	//fsa.transitions = std::move(new_transitions);
	return std::move(fsa);

}
