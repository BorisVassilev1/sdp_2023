#pragma once

#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include <Regex/FST.hpp>

// Classical Two-Tape Finite State Automaton (TFSA)
template <class Letter>
class TFSA {
   public:
	using State	   = unsigned int;
	using StringID = unsigned int;

	using Map = std::unordered_multimap<State, std::tuple<Letter, Letter, State>>;

	unsigned int			  N;
	std::unordered_set<State> qFirsts;
	std::unordered_set<State> qFinals;
	Map						  transitions;

	void addTransition(State from, Letter w1, Letter w2, State to) { transitions.insert({from, {w1, w2, to}}); }

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
			auto &[w1, w2, to] = value;
			if (w2 > 32 && w2 < 128) out << "  " << from << " -> " << to << " [label=\"" << w1 << "," << w2 << "\"];\n";
			else out << "  " << from << " -> " << to << " [label=\"" << w1 << "," << (int)w2 << "\"];\n";
		}
		out << "}\n";
	}

	State newState() { return ++N; }
};

template <class Letter>
auto expandFST(FST<Letter> &&fst) {
	TFSA<Letter> expanded;
	using State		 = TFSA<Letter>::State;
	expanded.N		 = fst.N;
	expanded.qFirsts = std::move(fst.qFirsts);
	expanded.qFinals = std::move(fst.qFinals);

	for (const auto &[from, value] : fst.transitions) {
		auto [id1, id2, to] = value;
		auto w1				= &fst.words[id1];
		auto w2				= &fst.words[id2];
		if (w2->size() < w1->size()) std::swap(w1, w2);		// |w2| >= |w1|
		auto  size = std::max(w1->size(), w2->size());
		State prev = from;
		for (unsigned int i = 0; i < w1->size(); ++i) {
			State next = (i == size - 1) ? to : expanded.newState();
			expanded.addTransition(prev, (*w1)[i], (*w2)[i], next);
			prev = next;
		}
		for (unsigned int i = w1->size(); i < w2->size(); ++i) {
			State next = (i == size - 1) ? to : expanded.newState();
			expanded.addTransition(prev, Letter::eps, (*w2)[i], next);	   // fill with empty letters
			prev = next;
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
