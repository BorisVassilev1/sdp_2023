#include <cassert>
#include <queue>

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
	const Letter		marker;

	using DelayStorage = std::vector<std::vector<Letter>>;

	unsigned int newState(DelayStorage &delays) {
		if (transitions.size() <= N) {
			transitions.push_back({});
			for (auto &t : transitions.back())
				t = {0, -1u};
			output.push_back(0);	 // epsilon
			delays.push_back({});
			assert(transitions.size() == output.size() && transitions.size() == N + 1);
			assert(delays.size() == N + 1);
		}
		return N++;
	}

	State insertIntoTrie(std::span<const Letter> word, State state, DelayStorage &delays) {
		assert(word.size() >= 1);
		for (const auto &letter : word) {
			auto &[outputID, next] = transitions[state][size_t(letter)];
			if (next == -1u) {
				auto new_state					   = newState(delays);
				transitions[state][size_t(letter)] = {words.addWord(std::span{&letter, 1}), new_state};
				state							   = new_state;
			} else state = next;
		}
		return state;
	}

	State insertIntoTrieRight(std::span<const Letter> word, State state, DelayStorage &delays) {
		assert(word.size() >= 1);
		for (const auto &letter : word) {
			auto &[outputID, next] = transitions[state][size_t(letter)];
			if (next == -1u) {
				auto new_state					   = newState(delays);
				transitions[state][size_t(letter)] = {0, new_state};	 // no output for right part
				delays[new_state].insert(delays[new_state].end(), delays[state].begin(), delays[state].end());
				delays[new_state].push_back(letter);

				if (letter == marker) { output[new_state] = words.addWord(delays[new_state]); }

				state = new_state;
			} else state = next;
		}
		return state;
	}

	void printDebugConstruction(std::ostream &out, DelayStorage &delays) const {
		out << "Debug construction:\n";
		for (State s = 0; s < N; ++s) {
			out << "State " << s << ": ";
			for (const auto &letter : delays[s]) {
				out << letter;
			}
			out << "\n";
		}
	}

   public:
	/// @brief A rule _<left>_<right>_ -> _<left><right>_
	struct Rule {
		std::vector<Letter> left;
		std::vector<Letter> right;
	};


	ReplaceWithMarkerSSFT(std::vector<Rule> &&rules, const Letter &marker) : marker(marker) {
		// build a trie of the left parts of the rules

		std::unordered_map<WordID, State> rightChainStarts;
		DelayStorage delays;	 //  will be used to make the fail transitions

		State initial						 = newState(delays);
		State trieStart						 = newState(delays);
		transitions[initial][size_t(marker)] = {words.addWord(std::span{&marker, 1}), trieStart};
		for (const auto &[left, right] : rules) {
			State leftEnd = insertIntoTrie(left, trieStart, delays);

			std::vector<Letter> rightWithMarker = right;
			rightWithMarker.push_back(marker);
			auto				rightID				= words.addWord(rightWithMarker);
			std::vector<Letter> rightWithMarkerLeft = {marker};
			rightWithMarkerLeft.insert(rightWithMarkerLeft.end(), right.begin(), right.end());

			State rightEnd = insertIntoTrieRight(rightWithMarkerLeft, leftEnd, delays);

			// std::cout << "Adding transition from state " << rightEnd << " with marker to state " << trieStart
			//		  << " with outputID " << rightID << std::endl;
			transitions[rightEnd][size_t(marker)] = {rightID, trieStart};
		}
		// printDebugConstruction(std::cout, delays);

		for (Letter l = 0; l < Letter::size; ++l) {
			if (l == marker) continue;
			transitions[initial][size_t(l)] = {words.addWord(std::span{&l, 1}),
											   initial};	 // self-loop for all letters except marker
		}
		std::vector<State> fail(N, -1u);
		fail[initial]	= initial;
		fail[trieStart] = initial;
		std::queue<State> bfsQueue;
		std::vector<bool> visited(N, false);
		bfsQueue.push(initial);
		while (!bfsQueue.empty()) {
			State state = bfsQueue.front();
			bfsQueue.pop();
			if (visited[state]) continue;
			visited[state] = true;
			assert(fail[state] != -1u);
			for (Letter l = 0; l < Letter::size; ++l) {
				const auto &[outputID, next] = transitions[state][size_t(l)];
				if (next != -1u) {
					bfsQueue.push(next);
					if (fail[next] == -1u) {
						State f	   = fail[state];
						fail[next] = std::get<1>(transitions[f][size_t(l)]);
					}
				} else {
					State to								   = std::get<1>(transitions[fail[state]][size_t(l)]);
					std::get<1>(transitions[state][size_t(l)]) = to;
					std::vector<Letter> failOutput;
					// assert delays[fail[state]].size() is a suffix of delays[state].size()

					int cutoff = delays[to].size();
					failOutput.insert(failOutput.end(), delays[state].begin(),
									  delays[state].end() - std::max(0, cutoff - 1));
					if (cutoff <= 0) failOutput.push_back(l);
					std::get<0>(transitions[state][size_t(l)]) = words.addWord(failOutput);
				}
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
				if (next != -1u) {
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

	std::vector<Letter> f(std::span<const Letter> input) const {
		State				state = 0;
		std::vector<Letter> outputWord;
		for (const auto &letter : input) {
			const auto &[outputID, next] = transitions[state][size_t(letter)];
			state						 = next;
			//std::cout << "see " << letter << ", output ";
			for (const auto &outLetter : words[outputID]) {
				outputWord.push_back(outLetter);
			//	std::cout << outLetter;
			}
			//std::cout << ", go to state " << state << " with delay ";
			//for (const auto &delayLetter : delays[state]) {
			//	std::cout << delayLetter;
			//}
			//std::cout << std::endl;
		}
		//std::cout << "final state " << state << ", output ";
		for (const auto &outLetter : words[output[state]]) {
			outputWord.push_back(outLetter);
			//std::cout << outLetter;
		}
		//std::cout << std::endl;
		return outputWord;
	}

	size_t cacheSize() const { return words.size(); }
	size_t size() const { return N; }
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

template <class Letter, size_t alphabetSize>
void statFSA(const ReplaceWithMarkerSSFT<Letter, alphabetSize> &fsa) {
	std::cout << "Subsequential Transtuder : |Q| = " << fsa.size() << ", |Σ| = " << alphabetSize
			  << ", |Δ| = " << fsa.size() * alphabetSize << ", |cache| = " << fsa.cacheSize() << std::endl;
}
}	  // namespace fl
