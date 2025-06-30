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

	std::unordered_set<StringID> f_eps{};

	void addTransition(State from, Letter w1, StringID w2, State to) { transitions.insert({from, {w1, w2, to}}); }
	void addTransition(State from, Letter w1, const std::vector<Letter> w2, State to) {
		if (w2.empty()) addTransition(from, w1, 0, to);
		else {
			StringID id2 = words.addWord(std::span{w2.data(), w2.size()});
			addTransition(from, w1, id2, to);
		}
	}

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

	State newState() { return N++; }
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
	auto out = getString(p.out()), err = getString(p.err());
	if(!out.empty())
		std::cout << out << std::endl;
	if(!err.empty())
		std::cout << err << std::endl;
}

// https://lml.bas.bg/~stoyan/finite-state-techniques.pdf#theorem.4.4.8
template <class Letter>
auto removeUpperEpsilonFST(TFSA<Letter> &&fsa) {
	using State	   = TFSA<Letter>::State;
	using StringID = TFSA<Letter>::StringID;

	std::stack<int>													 stack;
	std::vector<bool>												 visited(fsa.N, false);
	std::vector<std::vector<std::tuple<State, std::vector<Letter>>>> closure(fsa.N);

	for (State i = 0; i < fsa.N; ++i) {
		stack.push(0);
		visited[i] = true;
		closure[i].push_back({i, {}});	   // add the state itself with an empty word
		while (!stack.empty()) {
			auto p					= stack.top();
			const auto [current, u] = closure[i][p];
			stack.pop();

			auto [i1, i2] = fsa.transitions.equal_range(current);
			for (const auto &[_, value] : std::ranges::subrange(i1, i2)) {
				const auto &[w1, id2, to] = value;
				if (w1 == Letter::eps) {	 // epsilon transition
					visited[to]	  = true;
					auto new_word = u;
					new_word.insert(new_word.end(), fsa.words.getWord(id2).begin(), fsa.words.getWord(id2).end());
					closure[i].push_back({to, std::move(new_word)});
					stack.push(closure[i].size() - 1);
				}
			}
		}

		visited.assign(fsa.N, false);
	}

	for (auto &i : fsa.qFirsts) {
		for (const auto &[c, w] : closure[i]) {
			if (fsa.qFinals.contains(c)) {
				fsa.qFinals.insert(i);
				fsa.f_eps.insert(fsa.words.addWord(w));		// f(eps)
			}
		}
	}

	std::erase_if(fsa.transitions, [](const auto &pair) {
		const auto &[from, value] = pair;
		const auto &[w1, id2, to] = value;
		return w1 == Letter::eps;	  // remove epsilon transitions
	});

	typename TFSA<Letter>::Map new_transitions;
	for (State q1 = 0; q1 < fsa.N; ++q1) {
		for (const auto &[q_, u] : closure[q1]) {
			auto [i1, i2] = fsa.transitions.equal_range(q_);
			for (const auto &[_, value] : std::ranges::subrange(i1, i2)) {
				const auto &[sigma, id2, q__] = value;
				const auto &v				  = fsa.words.getWord(id2);
				for (const auto &[q2, w] : closure[q__]) {
					auto new_word = u;
					new_word.insert(new_word.end(), v.begin(), v.end());
					new_word.insert(new_word.end(), w.begin(), w.end());
					StringID new_id = fsa.words.addWord(new_word);
					new_transitions.insert({q1, {sigma, new_id, q2}});
				}
			}
		}
	}
	fsa.transitions = std::move(new_transitions);

	return std::move(fsa);
}

template <class Letter>
auto trimFSA(TFSA<Letter> &&fsa) {
	if (fsa.qFinals.empty()) {
		fsa.N		= 0;
		fsa.qFirsts = {0};
		fsa.words.clear();
		fsa.words.addWord(std::span<Letter>{});		// add empty word
		fsa.transitions.clear();
		return std::move(fsa);
	}
	using State	   = FST<Letter>::State;
	using StringID = FST<Letter>::StringID;
	std::vector<bool> visited_back(fsa.N, false);
	std::vector<bool> visited_forw(fsa.N, false);

	{
		auto						   &forwardTransitions = fsa.transitions;
		std::vector<std::vector<State>> backwardTransitions;
		backwardTransitions.resize(fsa.N);
		for (const auto &[from, value] : forwardTransitions) {
			const auto &[_, _, to] = value;
			backwardTransitions[to].push_back(from);
		}

		std::vector<State> stack;
		for (const auto &final : fsa.qFinals) {
			visited_back[final] = true;
			stack.push_back(final);
		}
		while (!stack.empty()) {
			State current = stack.back();
			stack.pop_back();
			for (const auto &next : backwardTransitions[current]) {
				if (!visited_back[next]) {
					visited_back[next] = true;
					stack.push_back(next);
				}
			}
		}

		for (const auto &first : fsa.qFirsts) {
			visited_forw[first] = true;		// mark initial states as visited
			stack.push_back(first);
		}
		while (!stack.empty()) {
			State current = stack.back();
			stack.pop_back();
			auto [i1, i2] = forwardTransitions.equal_range(current);
			for (const auto &[_, value] : std::ranges::subrange(i1, i2)) {
				const auto &[id1, id2, to] = value;
				if (!visited_forw[to]) {
					visited_forw[to] = true;
					stack.push_back(to);
				}
			}
		}
	}
	int				   cnt = 0;
	std::vector<State> new_map(fsa.N, -1);
	for (unsigned int i = 0; i < fsa.N; ++i) {
		if (visited_back[i] && visited_forw[i]) { new_map[i] = cnt++; }
	}
	TFSA<Letter> new_fsa;
	new_fsa.N = cnt;
	new_fsa.qFirsts.reserve(fsa.qFirsts.size());
	for (const auto &q : fsa.qFirsts) {
		if (new_map[q] != -1u) { new_fsa.qFirsts.insert(new_map[q]); }
	}

	new_fsa.qFinals.reserve(fsa.qFinals.size());
	for (const auto &q : fsa.qFinals) {
		if (new_map[q] != -1u) { new_fsa.qFinals.insert(new_map[q]); }
	}

	std::vector<bool> words_used(fsa.words.size(), false);
	words_used[0] = true;
	for (const auto &q : fsa.f_eps) {
		words_used[q] = true;
	}
	for (const auto &[from, value] : fsa.transitions) {
		const auto &[_, id, to] = value;
		if (new_map[from] != -1u && new_map[to] != -1u) { words_used[id] = true; }
	}

	std::vector<StringID> words_index_map(fsa.words.size(), -1);
	for (size_t i = 0; i < fsa.words.size(); ++i) {
		if (words_used[i]) {
			auto id			   = new_fsa.words.addWord(std::move(fsa.words.getWord(i)));
			words_index_map[i] = id;
		}
	}
	std::unordered_set<StringID> new_f_eps;
	for (const auto &q : fsa.f_eps) {
		if (words_used[q]) { new_f_eps.insert(words_index_map[q]); }
	}
	new_fsa.f_eps = std::move(new_f_eps);

	for (const auto &[from, value] : fsa.transitions) {
		const auto &[sigma, id, to] = value;
		if (new_map[from] != -1u && new_map[to] != -1u) {
			State	 new_from = new_map[from];
			State	 new_to	  = new_map[to];
			StringID new_id	  = words_index_map[id];
			new_fsa.transitions.insert({new_from, {sigma, new_id, new_to}});
		}
	}

	return std::move(new_fsa);
}

template <class Letter>
auto realtimeFST(FST<Letter> &&fst) {
	return trimFSA(removeUpperEpsilonFST(expandFST(removeEpsilonFST(trimFSA(std::move(fst))))));
}
