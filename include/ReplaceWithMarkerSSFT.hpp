#pragma once
#include <cassert>
#include <queue>

#include "concepts.hpp"
#include "pipes.hpp"
#include "utils.h"
#include "wordset.hpp"
#include "TotalSSFT.hpp"
#include "hashing.hpp"

namespace fl {

// direct construction of a SSFT from a set of replace rules with single-letter begin and end markers
//
// rules are all in the form _α_β_ -> _αβ_, where '_' is the marker and α, β are words in the alphabet
// the left and right '_' serve as left and right context so intersection of occurences is allowed within the
// contexts.
//
// the transducer is total, so every state has transitions with each letter.
template <fl::isLetter Letter, size_t alphabetSize = Letter::size>
class ReplaceWithMarkerSSFT : public TotalSSFT<Letter, alphabetSize> {
   public:
	using State		 = unsigned int;
	using WordID	 = UniqueWordSet<Letter>::WordID;
	using Map		 = TotalSSFT<Letter, alphabetSize>::Map;
	using Transition = TotalSSFT<Letter, alphabetSize>::Transition;

	/// @brief A rule _<left>_<right>_ -> _<left><right>_
	struct Rule {
		std::vector<Letter> left;
		std::vector<Letter> right;
	};

	struct RuleMetadata {
		std::vector<Letter> match;
		size_t				markerIndex;

		RuleMetadata(Rule &&rule, const Letter &marker) : match(std::move(rule.left)), markerIndex(match.size()) {
			match.push_back(marker);
			match.insert(match.end(), rule.right.begin(), rule.right.end());
		}

		auto operator<=>(const RuleMetadata &other) const {
			return std::lexicographical_compare(match.begin(), match.end(), other.match.begin(), other.match.end());
		}

		WordID output(UniqueWordSet<Letter> &words, size_t offset) const {
			// assert(offset < match.size());
			if (offset < markerIndex) return words.addWord(std::span{&match[offset], 1});
			else return 0;	   // epsilon
		}
		WordID color(UniqueWordSet<Letter> &words) const {
			return words.addWord(std::span{match.begin() + markerIndex + 1, match.end()});
		}
	};

   private:
	const Letter marker;

	using TotalSSFT<Letter, alphabetSize>::N;
	using TotalSSFT<Letter, alphabetSize>::transitions;
	using TotalSSFT<Letter, alphabetSize>::output;
	using TotalSSFT<Letter, alphabetSize>::words;

	struct TemporaryStateData {
		WordID								 color;
		std::array<Transition, alphabetSize> transitions;
		auto								&operator[](size_t index) { return transitions[index]; }
		auto								 operator[](size_t index) const { return transitions[index]; }
		auto								 begin() { return transitions.begin(); }
		auto								 end() { return transitions.end(); }
	};

	struct StateDataView {
		WordID										color;
		const std::array<Transition, alphabetSize> *transitions;

		constexpr StateDataView(const TemporaryStateData &s) noexcept : color(s.color), transitions(&s.transitions) {}
		constexpr StateDataView(WordID c, const std::array<Transition, alphabetSize> &t) noexcept
			: color(c), transitions(&t) {}
	};

	struct hash {
		constexpr hash()	 = default;
		using is_transparent = void;
		constexpr size_t operator()(const TemporaryStateData &s) const {
			return fl::hash<WordID>()(s.color) ^ fl::hash<std::array<Transition, alphabetSize>>()(s.transitions);
		}
		constexpr size_t operator()(const StateDataView &s) const {
			return fl::hash<WordID>()(s.color) ^ fl::hash<std::array<Transition, alphabetSize>>()(*s.transitions);
		}
	};
	struct equal {
		constexpr equal()	 = default;
		using is_transparent = void;
		constexpr bool operator()(const StateDataView &a, const StateDataView &b) const {
			return a.color == b.color && *a.transitions == *b.transitions;
		}
	};

	struct TemporaryData {
		std::unordered_map<TemporaryStateData, State, hash, equal> minimizedStates;
		std::vector<TemporaryStateData>							   unminimizedStates;	  /// treat as a stack
		RuleMetadata											  *prevRuleMeta = nullptr;

		void newState(WordID color) {
			unminimizedStates.push_back({color, {}});
			for (auto &t : unminimizedStates.back())
				t = {0, -1u};
		}
		auto popState() {
			assert(!unminimizedStates.empty());
			auto transitions = unminimizedStates.back();
			unminimizedStates.pop_back();
			return transitions;
		}
	};

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

	std::pair<State, size_t> traverseTrie(std::span<const Letter> word, const TemporaryData &tempData) const {
		State  state  = 0;
		size_t offset = 0;
		for (; offset < word.size(); ++offset) {
			const Letter l				 = word[offset];
			const auto &[outputID, next] = tempData.unminimizedStates[state][size_t(l)];
			if (next == -1u) return {state, offset};
			state = next;
		}
		return {state, offset};
	}

	void fillFailTransitions(State initial, State trieStart) {
		for (Letter l = 0; l < Letter::size; ++l) {
			if (l == marker) continue;
			// self-loop for all letters except marker
			transitions[initial][size_t(l)] = {words.addWord(std::span{&l, 1}), initial};
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
						fail[next] = transitions[f][size_t(l)].next;
					}
				} else {
					State to						   = transitions[fail[state]][size_t(l)].next;
					transitions[state][size_t(l)].next = to;
					std::vector<Letter> failOutput;
					// assert output[fail[state]].size() is a suffix of output[state].size()

					std::span<const Letter> outputTo	= words[output[to]];
					std::span<const Letter> outputState = words[output[state]];
					int						cutoff		= outputTo.size();
					failOutput.insert(failOutput.end(), outputState.begin(),
									  outputState.end() - std::max(0, cutoff - 1));
					if (cutoff <= 0) failOutput.push_back(l);
					transitions[state][size_t(l)].outputID = words.addWord(failOutput);
				}
			}
		}
	}

	size_t longestCommonPrefix(std::span<const Letter> a, std::span<const Letter> b) const {
		size_t len = std::min(a.size(), b.size());
		for (size_t i = 0; i < len; ++i) {
			if (a[i] != b[i]) return i;
		}
		return len;
	}

	void insertTemporary(const RuleMetadata &ruleMeta, TemporaryData &tempData) {
		while (tempData.unminimizedStates.size() < ruleMeta.match.size()) {
			tempData.newState(0);
		}
		tempData.newState(ruleMeta.color(words));
		assert(tempData.unminimizedStates.size() == ruleMeta.match.size() + 1);
	}

	void popTemporaryUntil(int until, TemporaryData &tempData) {
		assert(until >= -1);
		State		prevState = -1;
		const auto &match	  = tempData.prevRuleMeta->match;

		while (tempData.unminimizedStates.size() > (size_t)until + 1) {
			auto		 newData = tempData.popState();
			const size_t offset	 = tempData.unminimizedStates.size();	  // index of the popped state

			if (offset < match.size()) {	 // the deepest state has no spine successor
				auto &[outputID, next] = newData.transitions[size_t(match[offset])];
				next				   = prevState;
				outputID			   = tempData.prevRuleMeta->output(words, offset);
			}

			State state;
			auto  it = tempData.minimizedStates.find(StateDataView{newData.color, newData.transitions});
			if (it != tempData.minimizedStates.end()) {
				state = it->second;
			} else {
				state					 = newState();
				this->transitions[state] = newData.transitions;
				this->output[state]		 = newData.color;
				tempData.minimizedStates.emplace(std::move(newData), state);
			}
			prevState = state;
		}

		if (until >= 0) {
			auto &[outputID, next] = tempData.unminimizedStates.back().transitions[size_t(match[until])];
			next				   = prevState;
			outputID			   = tempData.prevRuleMeta->output(words, until);
		}
	}

   public:
	ReplaceWithMarkerSSFT(std::vector<Rule> &&rules, const Letter &marker) : marker(marker) {
		// build a trie of the left parts of the rules

		TemporaryData tempData;

		State initial = newState();

		std::vector<RuleMetadata> sortedRules;
		// <left><marker><right> for each rule
		for (auto &rule : rules) {
			sortedRules.emplace_back(std::move(rule), marker);
		}
		// sort lexicographically
		std::sort(sortedRules.begin(), sortedRules.end(), [](const RuleMetadata &a, const RuleMetadata &b) {
			return std::lexicographical_compare(a.match.begin(), a.match.end(), b.match.begin(), b.match.end());
		});

		insertTemporary(sortedRules[0], tempData);
		tempData.prevRuleMeta = &sortedRules[0];
		for (size_t i = 1; i < sortedRules.size(); ++i) {
			auto &ruleMeta = sortedRules[i];

			//std::cerr << "Adding rule: ";
			//for (const auto &l : ruleMeta.match)
			//	std::cerr << l;
			//std::cerr << std::endl;

			size_t lcp = longestCommonPrefix(ruleMeta.match, tempData.prevRuleMeta->match);
			// pop states until we reach the lcp
			popTemporaryUntil(lcp, tempData);
			insertTemporary(ruleMeta, tempData);

			tempData.prevRuleMeta = &ruleMeta;
			// drawFSA(*this);
			//draw(tempData);
		}

		popTemporaryUntil(-1, tempData);

		State trieStart						 = N - 1;
		transitions[initial][size_t(marker)] = {words.addWord(std::span{&marker, 1}), trieStart};
		//draw(tempData);

		fillFailTransitions(initial, trieStart);
	}

	void draw(TemporaryData &tempData) {
		ShellProcess p("dot -Tsvg > a.svg && feh ./a.svg");
		printConstruction(p.in(), tempData);
		p.in() << std::endl;
		p.in().close();
		p.wait();
		auto out = getString(p.out()), err = getString(p.err());
		if (!out.empty()) std::cout << out << std::endl;
		if (!err.empty()) std::cout << err << std::endl;
	}

	void printConstruction(std::ostream &out, TemporaryData &tempData) {
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
		// add the unminimized states
		for (State s = 0; s < tempData.unminimizedStates.size(); ++s) {
			out << "  " << N + s << " [shape=circle, style=dashed];\n";
			for (Letter l = 0; l < Letter::size; ++l) {
				const auto &[outputID, next] = tempData.unminimizedStates[s][size_t(l)];
				if (next != -1u) {
					out << "  " << N + s << " -> " << (next < N ? next : N + next - N) << " [label=\"<" << l << ", ";
					for (const auto &letter : words[outputID]) {
						out << letter;
					}
					out << ">\"];\n";
				}
			}
			// print implicit transition with prevRuleMeta
			if (s < tempData.unminimizedStates.size() - 1) {
				out << "  " << N + s << " -> " << (N + s + 1) << " [label=\"<" << tempData.prevRuleMeta->match[s]
					<< ", ";
				WordID outputID = tempData.prevRuleMeta->output(words, s);
				for (const auto &letter : words[outputID]) {
					out << letter;
				}
				out << ">\", style=dashed];\n";
			}
		}
		out << "}\n";
	}

	ReplaceWithMarkerSSFT(std::istream &in, const Letter &marker)
		: fl::TotalSSFT<Letter, alphabetSize>(in), marker(marker) {}

	ReplaceWithMarkerSSFT(const ReplaceWithMarkerSSFT &)			= default;
	ReplaceWithMarkerSSFT(ReplaceWithMarkerSSFT &&)					= default;
	ReplaceWithMarkerSSFT &operator=(const ReplaceWithMarkerSSFT &) = default;
	ReplaceWithMarkerSSFT &operator=(ReplaceWithMarkerSSFT &&)		= default;
};

}	  // namespace fl
