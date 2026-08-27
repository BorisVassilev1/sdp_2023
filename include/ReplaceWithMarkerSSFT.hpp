#pragma once

#include <cassert>

#include "pipes.hpp"
#include "utils.h"
#include "wordset.hpp"

namespace fl {

// direct construction of a SSFT from a set of replace rules with single-letter begin and end markers
//
// rules are all in the form _α_β_ -> _αβ_, where '_' is the marker and α, β are words in the alphabet
// the left and right '_' serve as left and right context so intersection of occurences is allowed within the
// contexts.
//
// the transducer is total, so every state has transitions with each letter.
template <class Letter, size_t alphabetSize = Letter::size>
class ReplaceWithMarkerSSFT {
   public:
	using State	 = unsigned int;
	using WordID = UniqueWordSet<Letter>::WordID;
	using Map	 = std::vector<std::array<std::tuple<WordID, State>, alphabetSize>>;

   private:
	unsigned int		  N = 0;
	UniqueWordSet<Letter> words;

	/// transitions[from][letter] = (outputID, to)
	Map transitions;
	/// the Ψ-function, output[state] = outputID
	std::vector<WordID> output;

	unsigned int newState() {
		if (transitions.size() <= N) {
			transitions.push_back({});
			for (auto &t : transitions.back())
				t = {0, -1u};
			output.push_back(0);	 // epsilon
			assert(transitions.size() == output.size() && transitions.size() == N + 1);
		}
		return N++;
	}

	State insertIntoTrie(const std::vector<Letter> &word, State state) {
		assert(word.size() >= 1);
		for (const auto &letter : word) {
			auto &[outputID, next] = transitions[state][size_t(letter)];
			if (next == -1u) {
				auto   new_state		   = newState();
				Letter output[]			   = {letter};
				transitions[state][size_t(letter)] = {words.addWord(std::span{output, output+1}), new_state};
				state					   = new_state;
			} else state = next;
		}
		return state;
	}

	State insertChain(const std::vector<Letter> &word, State start = -1u) {
		assert(word.size() >= 1);
		State state	 = start == -1u ? newState() : start;
		State result = state;
		words.addWord(word);
		for (const auto &letter : word) {
			auto &[outputID, next] = transitions[state][size_t(letter)];
			if (next == -1u) {
				auto new_state			   = newState();
				transitions[state][size_t(letter)] = {0, new_state};
				state					   = new_state;
			} else state = next;
		}
		return result;
	}

   public:
	/// @brief A rule _<left>_<right>_ -> _<left><right>_
	struct Rule {
		std::vector<Letter> left;
		std::vector<Letter> right;
	};

	ReplaceWithMarkerSSFT(const std::vector<Rule> &rules, const Letter &marker) {
		// build a trie of the left parts of the rules

		std::unordered_map<WordID, State> rightChainStarts;

		State initial = newState();
		State trieStart = newState();
		Letter markerArray[] = {marker};
		transitions[initial][size_t(marker)] = {words.addWord(std::span{markerArray, markerArray+1}), trieStart};
		for (const auto &[left, right] : rules) {
			State last = insertIntoTrie(left, trieStart);

			const auto &[markerOut, markerNext] = transitions[last][size_t(marker)];
			if (markerNext != -1u) {
				// if there is already a left half that ends in 'last', then build the right half chain
				// starting from the next state
				insertChain(right, markerNext);
				continue;
			}
			if (words.contains(right)) {
				// if the right half is already in the words set, use it.
				auto rightID			  = words.addWord(right);
				transitions[last][size_t(marker)] = {0, rightChainStarts[rightID]};
			} else {
				// otherwise, build the right half chain starting from a new state
				auto rightStart			  = insertChain(right);
				auto rightID			  = words.addWord(right);
				rightChainStarts[rightID] = rightStart;
				transitions[last][size_t(marker)] = {0, rightStart};
			}
		}
	}

	void print(std::ostream &out) const {
		out << "digraph ReplaceWithMarkerSSFT {\n";
		out << "  rankdir=LR;\n";
		out << "  node [shape=circle];\n";
		out << "  init [label=\"N=" << N << "\", shape=square];\n";
		out << "  init -> 0;\n";	 // initial state
		for (State s = 0; s < N; ++s) {
			if (output[s] != 0) {
				out << "  " << s << " [shape=doublecircle, label=\"";
				for (const auto &letter : words[output[s]]) {
					out << letter;
				}
				out << "\"];\n";								 // final States with output
			} else out << "  " << s << " [shape=circle];\n";	 // final States

			for (Letter l = 0; l < Letter::size; ++l) {
				const auto &[outputID, next] = transitions[s][size_t(l)];
				if (next != -1u ) { 
					out << "  " << s << " -> " << next << " [label=\"<" << l << ", ";
					for (const auto &letter : words[outputID]) {
						out << letter;
					}
					out << ">\"];\n";
				}
			}
		}
		out << "}\n";
	}
};

template <class Letter, size_t alphabetSize>
void drawFSA(const ReplaceWithMarkerSSFT<Letter, alphabetSize> &fsa) {
	ShellProcess p("dot -Tsvg > a.svg && feh ./a.svg");
	fsa.print(p.in());
	p.in() << std::endl;
	p.in().close();
	p.wait();
	auto out = getString(p.out()), err = getString(p.err());
	if (!out.empty()) std::cout << out << std::endl;
	if (!err.empty()) std::cout << err << std::endl;
}

}	  // namespace fl
