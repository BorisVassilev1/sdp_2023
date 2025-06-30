#pragma once

#include <cassert>
#include <fstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>
#include "Regex/regexParser.hpp"
#include "util/pipes.hpp"
#include <ranges>

#include <DPDA/utils.h>

// Classical Finite State Transducer (FST) class template
template <class Letter>
class FST {
   public:
	using State	   = unsigned int;
	using StringID = unsigned int;
	using Map	   = std::unordered_multimap<State, std::tuple<StringID, StringID, State>>;
	unsigned int			  N;
	std::unordered_set<State> qFirsts;
	std::unordered_set<State> qFinals;

	std::vector<std::vector<Letter>> words;		// words on the tapes
	Map								 transitions;

	FST() : N(0) {}

	void addTransition(State from, std::vector<Letter> &&w1, std::vector<Letter> &&w2, State to) {
		StringID id1;
		if (w1.empty()) id1 = 0;
		else {
			id1 = words.size();
			words.emplace_back(std::move(w1));
		}
		StringID id2;
		if (w2.empty()) id2 = 0;
		else if (words.back() == w2) id2 = id1;
		else {
			id2 = words.size();
			words.emplace_back(std::move(w2));
		}
		transitions.insert({from, {id1, id2, to}});
	}

	void print(std::ostream &out) const {
		// print in DOT
		out << "digraph FST {\n";
		out << "  rankdir=LR;\n";
		out << "  node [shape=circle];\n";
		out << "  init [label=\"N=" << this->N << "\", shape=square];\n";
		for (const int i : qFinals) {
			out << "  " << i << " [shape=doublecircle];\n";		// final States
		}
		for (const int i : qFirsts) {
			out << "  init -> " << i << " [style=dotted];\n";	  // initial states
		}
		for (const auto &[from, second] : transitions) {
			const auto &[id1, id2, to] = second;
			out << "  " << from << " -> " << to << " [label=\"<" << words[id1] << ", " << words[id2] << ">\"];\n";
		}
		out << "}\n";
	}
};

// Berry-Sethi constructions

template <class Letter>
class BS_FSA : public FST<Letter> {
   public:
};

template <class Letter>
class BS_WordFSA : public BS_FSA<Letter> {
   public:
	BS_WordFSA(std::vector<Letter> &&word1, std::vector<Letter> &&word2) : BS_FSA<Letter>() {
		this->N		  = 2;
		this->qFirsts = {0};
		this->qFinals = {1};
		this->words.push_back({});
		this->addTransition(*this->qFirsts.begin(), std::move(word1), std::move(word2), 1);
	}
};

template <class Letter>
class BS_UnionFSA : public BS_FSA<Letter> {
   public:
	using State	   = FST<Letter>::State;
	using StringID = FST<Letter>::StringID;

	BS_UnionFSA(BS_FSA<Letter> &&fst1, BS_FSA<Letter> &&fst2) : BS_FSA<Letter>() {
		if (fst1.qFinals.empty() && fst2.qFinals.empty()) {
			this->N		  = 0;
			this->qFirsts = {0};
			return;
		} else if (fst1.qFinals.empty()) {
			(FST<Letter> &)(*this) = std::move(fst2);
			return;
		} else if (fst2.qFinals.empty()) {
			(FST<Letter> &)(*this) = std::move(fst1);
			return;
		}

		bool canOptimizeFinals = true;
		{
			for (auto f : fst1.qFinals) {
				if (fst1.transitions.find(f) != fst1.transitions.end()) {
					canOptimizeFinals = false;
					break;
				}
			}
			for (auto f : fst2.qFinals) {
				if (fst2.transitions.find(f) != fst2.transitions.end()) {
					canOptimizeFinals = false;
					break;
				}
			}
		}

		this->N = fst1.N + fst2.N - 1;
		if (canOptimizeFinals) ++this->N;
		State newFinal = this->N - 1;
		this->qFirsts  = std::move(fst1.qFirsts);

		this->transitions = std::move(fst1.transitions);
		if (canOptimizeFinals)
			for (auto &[from, value] : this->transitions) {
				auto &[id1, id2, to] = value;
				if (fst1.qFinals.contains(to)) { to = newFinal; }
			}
		for (const auto &[from, value] : fst2.transitions) {
			const auto &[id1, id2, to] = value;
			State new_from			   = from + fst1.N - 1;
			if (from == 0) new_from = 0;
			StringID new_id1 = id1 ? id1 + fst1.words.size() - 1 : 0;
			StringID new_id2 = id2 ? id2 + fst1.words.size() - 1 : 0;
			State	 new_to	 = to + fst1.N - 1;
			if (canOptimizeFinals && fst2.qFinals.contains(to)) new_to = newFinal;
			this->transitions.insert({new_from, {new_id1, new_id2, new_to}});
		}

		this->words = std::move(fst1.words);
		this->words.reserve(this->words.size() + fst2.words.size());
		for (auto it = ++fst2.words.begin(); it != fst2.words.end(); ++it) {
			this->words.push_back(std::move(*it));
		}

		if (canOptimizeFinals) {
			this->qFinals = {newFinal};
		} else {
			this->qFinals = std::move(fst1.qFinals);
			for (const auto &q : fst2.qFinals) {
				this->qFinals.insert(q + fst1.N - 1);
			}
		}
	}
};

template <class Letter>
class BS_ConcatFSA : public BS_FSA<Letter> {
   public:
	using State	   = FST<Letter>::State;
	using StringID = FST<Letter>::StringID;

	BS_ConcatFSA(BS_FSA<Letter> &&fsa1, BS_FSA<Letter> &&fsa2) : BS_FSA<Letter>() {
		//: BS_FSA<Letter>(false) {
		if (fsa1.qFinals.empty() || fsa2.qFinals.empty()) {
			this->N		  = 0;
			this->qFirsts = {0};
			return;
		}
		this->N		  = fsa1.N + fsa2.N - 1;
		this->qFirsts = std::move(fsa1.qFirsts);

		// all transitions from fsa1
		this->transitions = std::move(fsa1.transitions);
		// add transitions from fsa2, removing the initial state of fsa2
		auto fsa1_final = fsa1.qFinals.begin();
		for (const auto &[from, value] : fsa2.transitions) {
			const auto &[id1, id2, to] = value;
			State new_from			   = from + fsa1.N - 1;
			if (from == 0) new_from = *fsa1_final;
			StringID new_id1 = id1 ? id1 + fsa1.words.size() - 1 : 0;
			StringID new_id2 = id2 ? id2 + fsa1.words.size() - 1 : 0;
			this->transitions.insert({new_from, {new_id1, new_id2, to + fsa1.N - 1}});
		}

		++fsa1_final;
		for (; fsa1_final != fsa1.qFinals.end(); ++fsa1_final) {
			auto [i1, i2] = fsa2.transitions.equal_range(0);
			for (auto it = i1; it != i2; ++it) {
				const auto &[_, value]	   = *it;
				const auto &[id1, id2, to] = value;
				StringID new_id1		   = id1 ? id1 + fsa1.words.size() - 1 : 0;
				StringID new_id2		   = id2 ? id2 + fsa1.words.size() - 1 : 0;
				this->transitions.insert({*fsa1_final, {new_id1, new_id2, to + fsa1.N - 1}});
			}
		}

		this->words = std::move(fsa1.words);
		this->words.reserve(this->words.size() + fsa2.words.size());
		for (auto it = ++fsa2.words.begin(); it != fsa2.words.end(); ++it) {
			this->words.push_back(std::move(*it));
		}

		if (fsa2.qFinals.contains(*fsa2.qFirsts.begin())) { this->qFinals = std::move(fsa1.qFinals); }
		this->qFinals.reserve(this->qFinals.size() + fsa2.qFinals.size());
		for (const auto &q : fsa2.qFinals) {
			this->qFinals.insert(q + fsa1.N - 1);
		}
	}
};

template <class Letter>
class BS_KleeneStarFSA : public BS_FSA<Letter> {
   public:
	BS_KleeneStarFSA(BS_FSA<Letter> &&fsa, bool includeEpsilon = true) : BS_FSA<Letter>() {
		if (fsa.qFinals.empty()) {
			this->N		  = 0;
			this->qFirsts = {0};
			if (includeEpsilon) this->qFinals = {0};
			return;
		}
		this->N		  = fsa.N;
		this->qFirsts = std::move(fsa.qFirsts);

		this->transitions = std::move(fsa.transitions);

		auto [i1, i2] = this->transitions.equal_range(0);
		typename FST<Letter>::Map toAdd;
		for (auto it = i1; it != i2; ++it) {
			const auto &[_, value]	   = *it;
			const auto &[id1, id2, to] = value;

			for (const auto &f : fsa.qFinals) {
				toAdd.insert({f, {id1, id2, to}});	   // add transitions from final states to initial state
			}
		}
		this->transitions.insert(toAdd.begin(), toAdd.end());

		this->words = std::move(fsa.words);

		this->qFinals = std::move(fsa.qFinals);
		if (includeEpsilon) this->qFinals.insert(0);	 // add the new initial state as a final state
	}
};

template <class Letter>
BS_FSA<Letter> makeFSA_BerriSethi(Regex &regex) {
	// static int counter = 0;
	BS_FSA<Letter> fsa;
	if (auto *r = dynamic_cast<TupleRegex *>(&regex)) {
		fsa = BS_WordFSA<Letter>(toLetter<Letter>(std::move(r->left)), toLetter<Letter>(std::move(r->right)));
	} else if (auto *r = dynamic_cast<UnionRegex *>(&regex)) {
		fsa = BS_UnionFSA<Letter>(makeFSA_BerriSethi<Letter>(*r->left), makeFSA_BerriSethi<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<ConcatRegex *>(&regex)) {
		fsa = BS_ConcatFSA<Letter>(makeFSA_BerriSethi<Letter>(*r->left), makeFSA_BerriSethi<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<KleeneStarRegex *>(&regex)) {
		fsa = BS_KleeneStarFSA<Letter>(makeFSA_BerriSethi<Letter>(*r->child), true);
	} else if (auto *r = dynamic_cast<KleenePlusRegex *>(&regex)) {
		fsa = BS_KleeneStarFSA<Letter>(makeFSA_BerriSethi<Letter>(*r->child), true);
	}
	return fsa;
}

// Thompson's construction

template <class Letter>
class TH_WordFSA : public FST<Letter> {
   public:
	TH_WordFSA(std::vector<Letter> &&word1, std::vector<Letter> &&word2) : FST<Letter>() {
		this->N		  = 2;
		this->qFirsts = {0};
		this->qFinals = {1};
		this->words.push_back({});
		this->addTransition(*this->qFirsts.begin(), std::move(word1), std::move(word2), 1);
	}
};

template <class Letter>
class TH_UnionFSA : public FST<Letter> {
   public:
	TH_UnionFSA(FST<Letter> &&fst1, FST<Letter> &&fst2) : FST<Letter>() {
		if (fst1.qFinals.empty() && fst2.qFinals.empty()) {
			this->N		  = 0;
			this->qFirsts = {0};
			this->words.push_back({});
			return;
		} else if (fst1.qFinals.empty()) {
			(FST<Letter> &)(*this) = std::move(fst2);
			return;
		} else if (fst2.qFinals.empty()) {
			(FST<Letter> &)(*this) = std::move(fst1);
			return;
		}

		this->N		  = fst1.N + fst2.N + 2;
		this->qFirsts = {this->N - 2};
		this->qFinals = {this->N - 1};

		this->transitions = std::move(fst1.transitions);
		for (const auto &[from, value] : fst2.transitions) {
			const auto &[id1, id2, to] = value;
			int new_id1				   = id1 + fst1.words.size() - 1;
			int new_id2				   = id2 + fst1.words.size() - 1;
			if (id1 == 0) new_id1 = 0;
			if (id2 == 0) new_id2 = 0;
			this->transitions.insert({from + fst1.N, {new_id1, new_id2, to + fst1.N}});
		}

		for (const auto &q : fst1.qFinals) {
			this->transitions.insert({q, {0, 0, this->N - 1}});
		}
		for (const auto &q : fst2.qFinals) {
			this->transitions.insert({q + fst1.N, {0, 0, this->N - 1}});
		}
		this->transitions.insert({*this->qFirsts.begin(), {0, 0, *fst1.qFirsts.begin()}});
		this->transitions.insert({*this->qFirsts.begin(), {0, 0, *fst2.qFirsts.begin() + fst1.N}});

		this->words = std::move(fst1.words);
		this->words.reserve(this->words.size() + fst2.words.size());
		for (auto it = ++fst2.words.begin(); it != fst2.words.end(); ++it) {
			this->words.push_back(std::move(*it));
		}
	}
};

template <class Letter>
class TH_ConcatFSA : public FST<Letter> {
   public:
	TH_ConcatFSA(FST<Letter> &&fst1, FST<Letter> &&fst2) {
		if (fst1.qFinals.empty() || fst2.qFinals.empty()) {
			this->N		  = 0;
			this->qFirsts = {0};
			this->words.push_back({});
			return;
		}

		this->N		  = fst1.N + fst2.N;
		this->qFirsts = {fst1.qFirsts};
		this->qFinals.reserve(fst2.qFinals.size());
		for (const auto &q : fst2.qFinals) {
			this->qFinals.insert(q + fst1.N);
		}

		this->transitions = std::move(fst1.transitions);
		for (const auto &[from, value] : fst2.transitions) {
			const auto &[id1, id2, to] = value;
			int new_id1				   = id1 + fst1.words.size() - 1;
			int new_id2				   = id2 + fst1.words.size() - 1;
			if (id1 == 0) new_id1 = 0;
			if (id2 == 0) new_id2 = 0;
			this->transitions.insert({from + fst1.N, {new_id1, new_id2, to + fst1.N}});
		}

		for (const auto &q : fst1.qFinals) {
			this->transitions.insert({q, {0, 0, *fst2.qFirsts.begin() + fst1.N}});
		}

		this->words = std::move(fst1.words);
		this->words.reserve(this->words.size() + fst2.words.size());
		for (auto it = ++fst2.words.begin(); it != fst2.words.end(); ++it) {
			this->words.push_back(std::move(*it));
		}
	}
};

template <class Letter>
class TH_KleeneStarFSA : public FST<Letter> {
   public:
	TH_KleeneStarFSA(FST<Letter> &&fst, bool includeEpsilon = true) {
		if (fst.qFinals.empty()) {
			this->N		  = 0;
			this->qFirsts = {0};
			this->words.push_back({});
			return;
		}

		this->N		  = fst.N + 2;
		this->qFirsts = {this->N - 2};
		this->qFinals = {this->N - 1};

		this->transitions = std::move(fst.transitions);
		for (const auto &q : fst.qFinals) {
			this->transitions.insert({q, {0, 0, this->N - 1}});
			this->transitions.insert({q, {0, 0, *fst.qFirsts.begin()}});
		}
		this->transitions.insert({*this->qFirsts.begin(), {0, 0, *fst.qFirsts.begin()}});
		if (includeEpsilon) this->transitions.insert({*this->qFirsts.begin(), {0, 0, this->N - 1}});

		this->words = std::move(fst.words);
	}
};

template <class Letter>
FST<Letter> makeFSA_Thompson(Regex &regex) {
	if (auto *r = dynamic_cast<TupleRegex *>(&regex)) {
		return TH_WordFSA<Letter>(toLetter<Letter>(std::move(r->left)), toLetter<Letter>(std::move(r->right)));
	} else if (auto *r = dynamic_cast<UnionRegex *>(&regex)) {
		return TH_UnionFSA<Letter>(makeFSA_Thompson<Letter>(*r->left), makeFSA_Thompson<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<ConcatRegex *>(&regex)) {
		return TH_ConcatFSA<Letter>(makeFSA_Thompson<Letter>(*r->left), makeFSA_Thompson<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<KleeneStarRegex *>(&regex)) {
		return TH_KleeneStarFSA<Letter>(makeFSA_Thompson<Letter>(*r->child), true);
	} else if (auto *r = dynamic_cast<KleenePlusRegex *>(&regex)) {
		return TH_KleeneStarFSA<Letter>(makeFSA_Thompson<Letter>(*r->child), false);
	}
	throw std::runtime_error("Unknown regex type for FSA construction: " + std::string(typeid(regex).name()));
}

template <class Letter>
void drawFSA(const FST<Letter> &fsa) {
	ShellProcess p("dot -Tsvg > a.svg && feh ./a.svg");
	fsa.print(p.in());
	p.in() << std::endl;
	p.in().close();
	p.wait();
	std::cout << getString(p.out()) << std::endl;
	std::cout << getString(p.err()) << std::endl;
}

template <class Letter>
inline void saveFSA(const FST<Letter> &fsa, const std::string &filename) {
	std::ofstream out(filename);
	if (!out.is_open()) { throw std::runtime_error("Could not open file " + filename + " for writing."); }
	fsa.print(out);
	out.close();
}

template <class Letter>
auto trimFSA(FST<Letter> &&fsa) {
	if (fsa.qFinals.empty()) {
		fsa.N		= 0;
		fsa.qFirsts = {0};
		fsa.words.clear();
		fsa.words.push_back({});
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
			const auto &[id1, id2, to] = value;
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
	FST<Letter> new_fsa;
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
	words_used[0] = true;	  // always keep the empty word
	for (const auto &[from, value] : fsa.transitions) {
		const auto &[id1, id2, to] = value;
		if (new_map[from] != -1u && new_map[to] != -1u) {
			words_used[id1] = true;
			words_used[id2] = true;
		}
	}

	int					  word_cnt = 0;
	std::vector<StringID> words_index_map(fsa.words.size(), -1);
	for (size_t i = 0; i < fsa.words.size(); ++i) {
		if (words_used[i]) {
			new_fsa.words.push_back(std::move(fsa.words[i]));
			words_index_map[i] = word_cnt++;
		}
	}

	for (const auto &[from, value] : fsa.transitions) {
		const auto &[id1, id2, to] = value;
		if (new_map[from] != -1u && new_map[to] != -1u) {
			State	 new_from = new_map[from];
			State	 new_to	  = new_map[to];
			StringID new_id1  = words_index_map[id1];
			StringID new_id2  = words_index_map[id2];
			new_fsa.transitions.insert({new_from, {new_id1, new_id2, new_to}});
		}
	}

	return std::move(new_fsa);
}

template <class Letter>
auto removeEpsilonFST(FST<Letter> &&fsa) {
	using State = typename FST<Letter>::State;

	std::stack<State>				stack;
	std::vector<bool>				visited(fsa.N, false);
	std::vector<std::vector<State>> closure(fsa.N);

	for (State i = 0; i < fsa.N; ++i) {
		stack.push(i);
		visited[i] = true;
		while (!stack.empty()) {
			State current = stack.top();
			stack.pop();

			auto [i1, i2] = fsa.transitions.equal_range(current);
			for (const auto &[_, value] : std::ranges::subrange(i1, i2)) {
				const auto &[id1, id2, to] = value;
				if (id1 == 0 && id2 == 0 && !visited[to]) {		// epsilon transition
					stack.push(to);
					visited[to] = true;
					closure[i].push_back(to);
				}
			}
		}

		visited.assign(fsa.N, false);
	}

	std::erase_if(fsa.transitions, [](const auto &pair) {
		const auto &[from, value]  = pair;
		const auto &[id1, id2, to] = value;
		return id1 == 0 && id2 == 0;	 // remove epsilon transitions
	});

	typename FST<Letter>::Map new_transitions;
	new_transitions.insert(fsa.transitions.begin(), fsa.transitions.end());
	for (const auto &[from, value] : fsa.transitions) {
		const auto &[id1, id2, to] = value;
		if (id1 == 0 && id2 == 0) assert(false);
		for (const auto &next : closure[to]) {
			new_transitions.insert({from, {id1, id2, next}});
		}
	}

	std::unordered_set<State> new_qFirsts;
	for (const State &i : fsa.qFirsts) {
		new_qFirsts.insert(i);
		for (const State &j : closure[i]) {
			new_qFirsts.insert(j);
		}
	}
	fsa.qFirsts = std::move(new_qFirsts);

	fsa.transitions = std::move(new_transitions);
	return std::move(fsa);
}
