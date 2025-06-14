#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>
#include "Regex/regexParser.hpp"

#include <DPDA/utils.h>

// Two-Tape Finite State Automaton (TFSA) class template
template <class Letter>
class TFSA {
   public:
	using State	   = unsigned int;
	using StringID = unsigned int;
	using Map	   = std::unordered_multimap<State, std::tuple<StringID, StringID, State>>;
	unsigned int			  N;
	State					  qFirst = 0;
	std::unordered_set<State> qFinals;

	std::vector<std::vector<Letter>> words;		// words on the tapes
	Map								 transitions;

	TFSA() : N(0) {}

	void addTransition(State from, const std::vector<Letter> &w1, const std::vector<Letter> &w2, State to) {
		StringID id1 = words.size();
		words.push_back(w1);
		StringID id2 = words.size();
		words.push_back(w2);
		transitions.insert({from, {id1, id2, to}});
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
		out << "init -> " << qFirst << ";\n";	  // initial transition
		for (const auto &[from, second] : transitions) {
			const auto &[id1, id2, to] = second;
			out << "  " << from << " -> " << to << " [label=\"<" << words[id1] << ", " << words[id2] << ">\"];\n";
		}
		out << "}\n";
	}
};

// Berry-Sethi constructions

template <class Letter>
class BS_WordFSA : public TFSA<Letter> {
   public:
	BS_WordFSA(const std::vector<Letter> &word1, const std::vector<Letter> &word2) : TFSA<Letter>() {
		this->N		  = 2;
		this->qFirst  = 0;
		this->qFinals = {1};
		this->addTransition(this->qFirst, word1, word2, 1);
	}
};

template <class Letter>
class BS_UnionFSA : public TFSA<Letter> {
   public:
	using State = TFSA<Letter>::State;

	BS_UnionFSA(TFSA<Letter> &&fst1, TFSA<Letter> &&fst2) : TFSA<Letter>() {
		if (fst1.qFinals.empty() && fst2.qFinals.empty()) {
			this->N		 = 0;
			this->qFirst = 0;
			return;
		} else if (fst1.qFinals.empty()) {
			(TFSA<Letter> &)(*this) = std::move(fst2);
			return;
		} else if (fst2.qFinals.empty()) {
			(TFSA<Letter> &)(*this) = std::move(fst1);
			return;
		}

		this->N		 = fst1.N + fst2.N - 1;
		this->qFirst = 0;

		this->transitions = std::move(fst1.transitions);
		for (const auto &[from, value] : fst2.transitions) {
			const auto &[id1, id2, to] = value;
			State new_from			   = from + fst1.N - 1;
			if (from == 0) new_from = 0;
			this->transitions.insert({new_from, {id1 + fst1.words.size(), id2 + fst1.words.size(), to + fst1.N - 1}});
		}

		this->words = std::move(fst1.words);
		this->words.reserve(this->words.size() + fst2.words.size());
		for (const auto &word : fst2.words) {
			this->words.push_back(std::move(word));
		}

		this->qFinals = std::move(fst1.qFinals);
		for (const auto &q : fst2.qFinals) {
			this->qFinals.insert(q + fst1.N - 1);
		}
	}
};

template <class Letter>
class BS_ConcatFSA : public TFSA<Letter> {
   public:
	using State = TFSA<Letter>::State;

	BS_ConcatFSA(TFSA<Letter> &&fsa1, TFSA<Letter> &&fsa2) {
		if (fsa1.qFinals.empty() || fsa2.qFinals.empty()) {
			this->N		 = 0;
			this->qFirst = 0;
			return;
		}
		this->N		 = fsa1.N + fsa2.N - 1;
		this->qFirst = 0;

		// all transitions from fsa1
		this->transitions = std::move(fsa1.transitions);
		// add transitions from fsa2, removing the initial state of fsa2
		auto fsa1_final = fsa1.qFinals.begin();
		for (const auto &[from, value] : fsa2.transitions) {
			const auto &[id1, id2, to] = value;
			State new_from			   = from + fsa1.N - 1;
			if (from == 0) new_from = *fsa1_final;
			this->transitions.insert({new_from, {id1 + fsa1.words.size(), id2 + fsa1.words.size(), to + fsa1.N - 1}});
		}

		++fsa1_final;
		for (; fsa1_final != fsa1.qFinals.end(); ++fsa1_final) {
			auto [i1, i2] = fsa2.transitions.equal_range(0);
			for (auto it = i1; it != i2; ++it) {
				const auto &[_, value]	   = *it;
				const auto &[id1, id2, to] = value;
				this->transitions.insert(
					{*fsa1_final, {id1 + fsa1.words.size(), id2 + fsa1.words.size(), to + fsa1.N - 1}});
			}
		}

		this->words = std::move(fsa1.words);
		this->words.reserve(this->words.size() + fsa2.words.size());
		for (const auto &word : fsa2.words) {
			this->words.push_back(std::move(word));
		}

		this->qFinals.reserve(fsa2.qFinals.size());
		for (const auto &q : fsa2.qFinals) {
			this->qFinals.insert(q + fsa1.N - 1);
		}
	}
};

template <class Letter>
class BS_KleeneStarFSA : public TFSA<Letter> {
   public:
	BS_KleeneStarFSA(TFSA<Letter> &&fsa) {
		if (fsa.qFinals.empty()) {
			this->N		 = 0;
			this->qFirst = 0;
			return;
		}
		this->N		 = fsa.N;
		this->qFirst = 0;

		this->transitions = std::move(fsa.transitions);

		auto [i1, i2] = this->transitions.equal_range(0);
		typename TFSA<Letter>::Map toAdd;
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
		this->qFinals.insert(0);	 // add the new initial state as a final state
	}
};

template <class Letter>
TFSA<Letter> makeFSA_BerriSethi(Regex &regex) {
	if (auto *r = dynamic_cast<TupleRegex *>(&regex)) {
		return BS_WordFSA<Letter>(toLetter<Letter>(r->left), toLetter<Letter>(r->right));
	} else if (auto *r = dynamic_cast<UnionRegex *>(&regex)) {
		return BS_UnionFSA<Letter>(makeFSA_BerriSethi<Letter>(*r->left), makeFSA_BerriSethi<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<ConcatRegex *>(&regex)) {
		return BS_ConcatFSA<Letter>(makeFSA_BerriSethi<Letter>(*r->left), makeFSA_BerriSethi<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<KleeneStarRegex *>(&regex)) {
		return BS_KleeneStarFSA<Letter>(makeFSA_BerriSethi<Letter>(*r->child));
	}
	throw std::runtime_error("Unknown regex type for FSA construction: " + std::string(typeid(regex).name()));
}

// Thompson's construction

template <class Letter>
class TH_WordFSA : public TFSA<Letter> {
   public:
	TH_WordFSA(const std::vector<Letter> &word1, const std::vector<Letter> &word2) : TFSA<Letter>() {
		this->N		  = 2;
		this->qFirst  = 0;
		this->qFinals = {1};
		this->words.push_back({});
		this->addTransition(this->qFirst, word1, word2, 1);
	}
};

template <class Letter>
class TH_UnionFSA : public TFSA<Letter> {
   public:
	TH_UnionFSA(TFSA<Letter> &&fst1, TFSA<Letter> &&fst2) : TFSA<Letter>() {
		if (fst1.qFinals.empty() && fst2.qFinals.empty()) {
			this->N		 = 0;
			this->qFirst = 0;
			this->words.push_back({});
			return;
		} else if (fst1.qFinals.empty()) {
			(TFSA<Letter> &)(*this) = std::move(fst2);
			return;
		} else if (fst2.qFinals.empty()) {
			(TFSA<Letter> &)(*this) = std::move(fst1);
			return;
		}

		this->N		  = fst1.N + fst2.N + 2;
		this->qFirst  = this->N - 2;
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
		this->transitions.insert({this->qFirst, {0, 0, fst1.qFirst}});
		this->transitions.insert({this->qFirst, {0, 0, fst2.qFirst + fst1.N}});

		this->words = std::move(fst1.words);
		this->words.reserve(this->words.size() + fst2.words.size());
		for (auto it = ++fst2.words.begin(); it != fst2.words.end(); ++it) {
			this->words.push_back(std::move(*it));
		}
	}
};

template <class Letter>
class TH_ConcatFSA : public TFSA<Letter> {
   public:
	TH_ConcatFSA(TFSA<Letter> &&fst1, TFSA<Letter> &&fst2) {
		if (fst1.qFinals.empty() || fst2.qFinals.empty()) {
			this->N		 = 0;
			this->qFirst = 0;
			this->words.push_back({});
			return;
		}

		this->N		 = fst1.N + fst2.N;
		this->qFirst = fst1.qFirst;
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
			this->transitions.insert({q, {0, 0, fst2.qFirst + fst1.N}});
		}

		this->words = std::move(fst1.words);
		this->words.reserve(this->words.size() + fst2.words.size());
		for (auto it = ++fst2.words.begin(); it != fst2.words.end(); ++it) {
			this->words.push_back(std::move(*it));
		}
	}
};

template <class Letter>
class TH_KleeneStarFSA : public TFSA<Letter> {
   public:
	TH_KleeneStarFSA(TFSA<Letter> &&fst) {
		if (fst.qFinals.empty()) {
			this->N		 = 0;
			this->qFirst = 0;
			this->words.push_back({});
			return;
		}

		this->N		  = fst.N + 2;
		this->qFirst  = this->N - 2;
		this->qFinals = {this->N - 1};

		this->transitions = std::move(fst.transitions);
		for (const auto &q : fst.qFinals) {
			this->transitions.insert({q, {0, 0, this->N - 1}});
			this->transitions.insert({q, {0, 0, fst.qFirst}});
		}
		this->transitions.insert({this->qFirst, {0, 0, fst.qFirst}});
		this->transitions.insert({this->qFirst, {0, 0, this->N - 1}});

		this->words = std::move(fst.words);
	}
};

template <class Letter>
TFSA<Letter> makeFSA_Thompson(Regex &regex) {
	if (auto *r = dynamic_cast<TupleRegex *>(&regex)) {
		return TH_WordFSA<Letter>(toLetter<Letter>(r->left), toLetter<Letter>(r->right));
	} else if (auto *r = dynamic_cast<UnionRegex *>(&regex)) {
		return TH_UnionFSA<Letter>(makeFSA_Thompson<Letter>(*r->left), makeFSA_Thompson<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<ConcatRegex *>(&regex)) {
		return TH_ConcatFSA<Letter>(makeFSA_Thompson<Letter>(*r->left), makeFSA_Thompson<Letter>(*r->right));
	} else if (auto *r = dynamic_cast<KleeneStarRegex *>(&regex)) {
		return TH_KleeneStarFSA<Letter>(makeFSA_Thompson<Letter>(*r->child));
	}
	throw std::runtime_error("Unknown regex type for FSA construction: " + std::string(typeid(regex).name()));
}


