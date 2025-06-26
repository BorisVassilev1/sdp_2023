#pragma once

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include "Regex/TFSA.hpp"
#include "Regex/functionality.hpp"
#include "Regex/wordset.hpp"
#include <ranges>

// Subsequential Finite-State Transducer (SSFST)
template <class Letter>
class SSFT {
   public:
	using State	   = unsigned int;
	using StringID = WordSet<Letter>::WordID;
	using Map	   = std::unordered_map<std::tuple<State, Letter>, std::pair<StringID, State>>;

	WordSet<Letter>						words;
	Map									transitions;
	std::unordered_set<State>			qFinals;
	unsigned int						N = 0;
	std::unordered_map<State, StringID> output;

	SSFT() = default;

	// accepts a trimmed TFSA and builds a subsequential finite-state transducer
	// tests for bounded variation
	SSFT(TFSA<Letter> &&fsa) {
		unsigned int C = 0;
		for (auto w : fsa.words) {
			if (w.size() > C) C = w.size();
		}
		auto MAX_DELAY = C * fsa.N * fsa.N;		// C * |Q|^2
		auto curr_max  = 0u;

		UniqueWordSet<Letter> stateDelays;
		using DelayID = UniqueWordSet<Letter>::WordID;

		// We use vector because noone cares about individual states and delays
		using BigState = std::vector<std::tuple<State, DelayID>>;

		std::vector<std::reference_wrapper<const BigState>> states;		// states of the SSFST
		std::map<BigState, State> stateMap;								// maps sets of states to index in states vector

		const auto newState = []() -> State {
			static State nextState = 0;
			return nextState++;
		};
		std::stack<State> queue;

		// if(fsa.f_eps.size() > 1) {
		//	throw std::runtime_error("not functional");
		// }
		if (!fsa.f_eps.empty()) {
			auto wordID		= *fsa.f_eps.begin();
			this->output[0] = words.addWord(fsa.words[wordID]);		// output for the initial state
		}

		BigState initial;
		for (const auto &q : fsa.qFirsts) {
			initial.push_back({q, 0});
			if (fsa.qFinals.contains(q)) { qFinals.insert(0); }
		}
		std::sort(initial.begin(), initial.end());
		auto [it, _] = stateMap.insert({std::move(initial), 0});
		states.emplace_back(it->first);		// add the initial state
		queue.push(newState());

		std::cout << std::endl;

		while (!queue.empty()) {
			State current = queue.top();
			queue.pop();
			const BigState &currentState = states[current];

			static WordSet<Letter>		 temporaryWords;	 // used for the new state delays
			static std::vector<BigState> nextStates;
			static std::vector<std::reference_wrapper<typename Map::value_type>>
				  currentTransitions;	  // transitions for the current state
			State nextState		= 0;
			auto  localNewState = [&nextState]() {
				 auto &ref = nextStates.emplace_back();
				 return std::tuple(std::reference_wrapper{ref}, nextState++);
			};

			// for each (q,w) in the current state
			for (const auto [q, delay_id] : currentState) {
				auto oldDelay		  = stateDelays[delay_id];
				const auto [it1, it2] = fsa.transitions.equal_range(q);
				for (const auto &[_, right] : std::ranges::subrange(it1, it2)) {
					const auto &[s, id, next] = right;

					auto it = transitions.find({current, s});	  // for each transition from 'current' with letter 's'
					if (it == transitions.end()) {
						// create a new transition
						auto wordToDelay  = std::views::concat(oldDelay, fsa.words[id]);
						auto new_id		  = words.addWord(wordToDelay);
						auto temp_id	  = temporaryWords.addWord(wordToDelay);
						auto [to, to_ind] = localNewState();
						auto k			  = transitions.insert({{current, s}, {new_id, to_ind}});
						currentTransitions.emplace_back(std::ref(*k.first));
						to.get().emplace_back(next, temp_id);
					} else {
						auto &[_, rhs]		= *it;
						auto &[outputID, n] = rhs;
						// update the existing transition
						auto  wordToDelay = std::views::concat(oldDelay, fsa.words[id]);
						auto  temp_id	  = temporaryWords.addWord(wordToDelay);
						auto  outputWord  = words[outputID];
						auto  i			  = commonPrefixLen(wordToDelay, outputWord);
						auto &nextBig	  = nextStates[n];
						words.replaceWithSubstr(outputID, 0, i);
						nextBig.emplace_back(next, temp_id);
					}
				}
			}

			for (const auto &ref : currentTransitions) {
				auto &[_, rhs]		 = ref.get();
				auto &[outputID, to] = rhs;
				auto &nextBig		 = nextStates[to];

				for (auto &[q, delay_id] : nextBig) {
					auto wrongDelay = temporaryWords[delay_id];
					auto eaten		= words[outputID].size();

					if (wrongDelay.size() > curr_max) {
						curr_max = wrongDelay.size();
						// std::cout << "\rCurrent max delay: " << curr_max;
						// std::cout << " Current states count: " << states.size() << " Upper bound: " << MAX_DELAY
						//		  << std::flush;
					}
					if (wrongDelay.size() - eaten > MAX_DELAY) {
						throw std::runtime_error("Delay too long, bounded variation not satisfied");
					}

					auto new_id =
						stateDelays.addWord((Letter *)&*wrongDelay.begin() + eaten, wrongDelay.size() - eaten);
					delay_id = new_id;
				}
			}

			std::vector<int> stateRemap(nextStates.size(), -1);
			for (const auto &[i, nextState] : std::views::enumerate(nextStates)) {
				// check if the next state is already in the states vector
				auto it = stateMap.find(nextState);
				if (it != stateMap.end()) {
					stateRemap[i] = it->second;
					continue;
				}

				// if not, add it to the states vector and map
				State newIndex = newState();
				stateRemap[i]  = newIndex;

				for (const auto &[q, delayID] : nextState) {
					if (fsa.qFinals.contains(q)) {
						qFinals.insert(newIndex);
						auto output			   = words.addWord(stateDelays[delayID]);
						this->output[newIndex] = output;
					}
				}

				std::sort(nextState.begin(), nextState.end());
				nextState.erase(std::unique(nextState.begin(), nextState.end()), nextState.end());

				auto [inserted_it, _] = stateMap.insert({std::move(nextState), newIndex});
				states.emplace_back(inserted_it->first);
				queue.push(newIndex);
			}

			for (const auto &ref : currentTransitions) {
				auto &[_, rhs]		 = ref.get();
				auto &[outputID, to] = rhs;
				if (stateRemap[to] == -1) {
					std::cerr << "Error: state remap failed for state " << to << std::endl;
					continue;
				}
				to = stateRemap[to];
			}

			std::cout << "\rCurrent max delay: " << curr_max;
			std::cout << " Current states count: " << states.size() << " Upper bound: " << MAX_DELAY << std::flush;

			// clear temporary data to conserve memory allocation
			nextStates.clear();
			currentTransitions.clear();
			temporaryWords.clear();
		}
		this->N = states.size();

		// print in dot format
		if (N < 1000) {
			ShellProcess p("dot -Tsvg > a.svg && feh ./a.svg");
			auto		&in = p.in();
			in << "digraph SSFT {\n";
			in << "  rankdir=LR;\n";
			in << "  node [shape=circle];\n";
			in << "  init [label=\"N=" << states.size() << "\", shape=square];\n";
			in << "  init -> 0;\n";		// initial state
			for (const auto [i, state] : std::ranges::views::enumerate(states)) {
				in << "  " << i << " [label=\"";
				for (const auto &[q, id] : state.get()) {
					in << "(" << q << ", ";
					for (const auto &letter : stateDelays[id]) {
						if (letter < 128 && letter >= 32) in << letter;
						else in << (int)letter;
					}
					in << ")\n ";
				}
				in << "\"";
				if (qFinals.contains(i)) in << ", shape=doublecircle";
				in << "];\n";	  // final States
			}

			for (const auto &[from, value] : transitions) {
				const auto &[s, l]		   = from;
				const auto &[outputID, to] = value;
				in << "  " << s << " -> " << to << " [label=\"<" << l << ", ";
				for (const auto &letter : words[outputID]) {
					if (letter < 128 && letter >= 32) in << letter;
					else in << (int)letter;
				}
				in << ">\"];\n";
			}
			in << "}\n";

			p.in() << std::endl;
			p.in().close();
			p.wait();
			std::cout << getString(p.out()) << std::endl;
			std::cout << getString(p.err()) << std::endl;
		}
	}

	auto f(const std::vector<Letter> &input) const {
		std::vector<Letter> output;
		State				current = 0;	 // initial state
		for (const auto &letter : input) {
			auto it = transitions.find({current, letter});
			if (it == transitions.end()) return std::pair{output, false};
			const auto &[outputID, next] = it->second;
			output.insert(output.end(), words[outputID].begin(), words[outputID].end());
			current = next;
		}
		if (qFinals.contains(current))
			output.insert(output.end(), words[this->output.at(current)].begin(), words[this->output.at(current)].end());
		else return std::pair{output, false};	  // not in final state
		return std::pair{output, true};			  // in final state
	}

	void print(std::ostream &out) const {
		out << "digraph SSFT {\n";
		out << "  rankdir=LR;\n";
		out << "  node [shape=circle];\n";
		out << "  init [label=\"N=" << N << "\", shape=square];\n";
		out << "  init -> 0;\n";	 // initial state
		for (const auto &q : qFinals) {
			if (qFinals.contains(q)) {
				out << "  " << q << " [shape=doublecircle, label=\"";
				for (const auto &letter : words[output.at(q)]) {
					if (letter < 128 && letter >= 32) out << letter;
					else out << (int)letter;
				}
				out << "\"];\n";									   // final States with output
			} else out << "  " << q << " [shape=doublecircle];\n";	   // final States
		}
		for (const auto &[from, value] : transitions) {
			const auto &[s, l]		   = from;
			const auto &[outputID, to] = value;
			out << "  " << s << " -> " << to << " [label=\"<" << l << ", ";
			for (const auto &letter : words[outputID]) {
				if (letter < 128 && letter >= 32) out << letter;
				else out << (int)letter;
			}
			out << ">\"];\n";
		}
		out << "}\n";
	}
};

template <class Letter>
void drawFSA(const SSFT<Letter> &fsa) {
	ShellProcess p("dot -Tsvg > a.svg && feh ./a.svg");
	fsa.print(p.in());
	p.in() << std::endl;
	p.in().close();
	p.wait();
	std::cout << getString(p.out()) << std::endl;
	std::cout << getString(p.err()) << std::endl;
}
