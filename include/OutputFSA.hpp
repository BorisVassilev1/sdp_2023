#pragma once

#include <SSFT.hpp>
#include <TFSA.hpp>

namespace fl {
template <class Letter>
class OutputFSA {
   protected:
	OutputFSA() = default;

   public:
	using State	   = unsigned int;
	using StringID = unsigned int;

	unordered_multimap<State, std::tuple<Letter, State>> transitions;
	unordered_set<State>								 qFinals;
	unordered_set<State>								 qFirsts;
	unsigned int										 N = 0;
	unordered_map<State, Letter>					 output;

	constexpr OutputFSA(const std::string_view &regex, Letter fixedOutput) {
		auto ast = rgx::parseRegex(std::string(regex));
		auto fst = (FST<Letter>)makeFSA_BerriSethi<Letter>(*ast);
		fst		 = trimFSA<Letter>(std::move(fst));

		auto realtime = realtimeFST<Letter>(std::move(fst));
		*this		  = OutputFSA<Letter>(pseudoDeterminizeFST<Letter>(std::move(realtime)), fixedOutput);
	}

	OutputFSA(TFSA<Letter> &&tfsa, Letter fixedOutput) {
		this->N		  = tfsa.N;
		this->qFinals = std::move(tfsa.qFinals);
		this->qFirsts = std::move(tfsa.qFirsts);
		for (const auto &[from, rhs] : tfsa.transitions) {
			const auto &[letter, outputID, to] = rhs;
			if (!tfsa.words.getWord(outputID).empty())
				throw std::runtime_error("OutputFSA can only have epsilon on second tape");
			this->transitions.emplace(from, std::make_tuple(letter, to));
		}

		for (const auto &final : this->qFinals) {
			this->output.emplace(final, fixedOutput);
		}
	}

	OutputFSA(SSFT<Letter> &&ssft, Letter fixedOutput) {
		this->N		  = ssft.N;
		this->qFinals = std::move(ssft.qFinals);
		this->qFirsts = {0};
		for (const auto &[lhs, rhs] : ssft.transitions) {
			const auto &[from, letter] = lhs;
			const auto &[outputID, to] = rhs;
			if (!ssft.words.getWord(outputID).empty())
				throw std::runtime_error("OutputFSA can only have epsilon on second tape");
			this->transitions.emplace(from, std::make_tuple(letter, to));
		}

		for (const auto &final : this->qFinals) {
			this->output.emplace(final, fixedOutput);
		}
	}

	void print(std::ostream &out) const {
		// print in DOT
		out << "digraph OutputFSA {\n";
		out << "  rankdir=LR;\n";
		out << "  node [shape=circle];\n";
		out << "  init [label=\"N=" << this->N << "\", shape=square];\n";
		for (const int i : qFinals) {
			out << "  " << i << " [shape=doublecircle,label=\"" << output.at(i) << "\"];\n";	 // final States
		}
		for (const int i : qFirsts) {
			out << "  init -> " << i << " [style=dotted];\n";	  // initial states
		}
		for (const auto &[from, rhs] : transitions) {
			const auto &[letter, to] = rhs;
			out << "  " << from << " -> " << to << " [label=\"" << ((letter == '\"' || letter == '\\') ? "\\" : "")
				<< letter << "\"];\n";
		}
		out << "}\n";
	}

	SSFT<Letter> determinizeToSSFT() const {
		SSFT<Letter> ssft;

		using BigState = std::vector<State>;
		std::vector<std::reference_wrapper<const BigState>> states;		// states of the SSFST
		std::map<BigState, State> stateMap;								// maps sets of states to index in states vector
		std::queue<State>		  queue;

		BigState initial;
		for (const auto &q : this->qFirsts)
			initial.push_back(q);
		std::sort(initial.begin(), initial.end());

		auto getStateID = [&](BigState &&bs) -> std::pair<State, bool> {
			std::ranges::sort(bs);
			auto it = stateMap.find(bs);
			if (it == stateMap.end()) {
				State new_id	   = ssft.N++;
				State winningState = -1;
				for (const auto &s : bs) {
					if (this->qFinals.contains(s)) {
						auto   it		  = ssft.output.find(new_id);
						Letter out_letter = this->output.at(s);
						if (it != ssft.output.end()) {	   // aka winningState is not -1
							Letter old_letter = ssft.words.getWord(it->second)[0];
							if (old_letter != out_letter) {
								// dbLog(dbg::LOG_DEBUG, "OutputFSA::determinizeToSSFT: conflict at state ", new_id,
								//	  " between outputs ", old_letter, " and ", out_letter);

								if (s < winningState) {
									// dbLog(dbg::LOG_DEBUG, "OutputFSA::determinizeToSSFT: keeping ", out_letter);
									ssft.output[new_id] = ssft.words.addWord(std::array<Letter, 1>{out_letter});
									winningState		= s;
								} else {
									// dbLog(dbg::LOG_DEBUG, "OutputFSA::determinizeToSSFT: keeping ", old_letter);
								}
							}
						} else {
							ssft.qFinals.insert(new_id);
							ssft.output[new_id] = ssft.words.addWord(std::array<Letter, 1>{out_letter});
							winningState		= s;
						}
					}
				}
				auto [it, _] = stateMap.emplace(std::move(bs), new_id);
				states.emplace_back(it->first);
				return {new_id, true};
			} else {
				return {it->second, false};
			}
		};

		auto [initial_state, _] = getStateID(BigState{std::from_range, initial});
		queue.push(initial_state);

		while (!queue.empty()) {
			State current = queue.front();
			queue.pop();
			const BigState &current_bs = states[current];

			unordered_map<Letter, BigState> current_transitions;

			for (const auto &q : current_bs) {
				auto [i1, i2] = this->transitions.equal_range(q);
				for (const auto &[_, rhs] : std::ranges::subrange(i1, i2)) {
					const auto &[letter, to] = rhs;
					current_transitions[letter].push_back(to);
				}
			}

			for (const auto &[letter, next_bs] : current_transitions) {
				auto [next_state, is_new] = getStateID(BigState(next_bs));
				ssft.transitions.emplace(std::make_tuple(current, letter), std::make_pair(0, next_state));
				if (is_new) { queue.push(next_state); }
			}
		}

		return std::move(ssft);
	}
};

template <class Letter>
class UnionOutputFSA : public OutputFSA<Letter> {
   public:
	template <class... FSAS>
	UnionOutputFSA(FSAS &&...fsas) : OutputFSA<Letter>() {
		if constexpr (sizeof...(fsas) == 0) {
			this->N		  = 0;
			this->qFirsts = {0};
			return;
		}
		this->N		  = 1;	   // new initial state
		this->qFirsts = {};

		unsigned int offset = 0;
		((addFSA(std::forward<FSAS>(fsas), offset), offset += fsas.N), ...);
	}

   private:
	template <class FSAType>
	void addFSA(FSAType &&fsa, unsigned int offset) {
		for (const auto &q : fsa.qFirsts) {
			this->qFirsts.insert(q + offset);
		}
		for (const auto &q : fsa.qFinals) {
			this->qFinals.insert(q + offset);
			this->output.emplace(q + offset, fsa.output.at(q));
		}
		for (const auto &[from, rhs] : fsa.transitions) {
			const auto &[letter, to] = rhs;
			this->transitions.emplace(from + offset, std::make_tuple(letter, to + offset));
		}
	}
};

template <class Letter>
void drawFSA(const OutputFSA<Letter> &fsa) {
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
