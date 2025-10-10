#pragma once

#include <DPDA/cfg.h>
#include <climits>

namespace ll1g {

template <class _Letter>
using Production = typename CFG<_Letter>::Production;

template <class _Letter>
class LL1Grammar : public CFG<_Letter> {
   public:
	LL1Grammar(const _Letter &start) : CFG<_Letter>(start, _Letter::eof) {}
};

template <class _Letter>
class Epsilon : public LL1Grammar<_Letter> {
   public:
	Epsilon(const _Letter &start) : LL1Grammar<_Letter>(start) {
		this->addRule(start, Production<_Letter>({}, {}, false));
	}
};

template <class _Letter>
class Word : public LL1Grammar<_Letter> {
   public:
	Word(const _Letter &start, const std::vector<_Letter> &word, const std::vector<bool> &ignore, bool spill = true)
		: LL1Grammar<_Letter>(start) {
		this->terminals.insert(word.begin(), word.end());
		this->addRule(start, Production<_Letter>(word, ignore));
		if(spill)
			this->getNonTerminalData(start).upwardSpillThreshold = word.size();
	}
	Word(const _Letter &start, const std::vector<_Letter> &word, bool spill = true)
		: Word(start, word, std::vector<bool>(word.size(), true), spill) {}
};

template <class _Letter>
class Optional : public LL1Grammar<_Letter> {
   public:
	template <typename G>
		requires(std::is_base_of_v<LL1Grammar<_Letter>, G> || std::is_same_v<_Letter, G>)
	Optional(const _Letter &start, G &&g) : LL1Grammar<_Letter>(start) {
		this->terminals.insert(g.terminals.begin(), g.terminals.end());
		this->nonTerminals.insert(g.nonTerminals.begin(), g.nonTerminals.end());
		this->nonTerminalData.insert(g.nonTerminalData.begin(), g.nonTerminalData.end());
		this->rules.insert(g.rules.begin(), g.rules.end());
		this->addRule(start, {g.start});
		this->addRule(start, {});
		this->getNonTerminalData(start).upwardSpillThreshold = 1;
	}
};

template <class _Letter>
class Seq : public LL1Grammar<_Letter> {
   public:
	template <typename... Grammars>
		requires((std::is_base_of_v<LL1Grammar<_Letter>, Grammars> || std::is_same_v<_Letter, Grammars>) && ...)
	Seq(const _Letter &start, std::vector<bool> ignore, Grammars &&...gs) : LL1Grammar<_Letter>(start) {
		assert(sizeof...(gs) == ignore.size() && "Ignore vector size must match number of grammars");
		auto params = std::vector<_Letter>{[&]() {
			if constexpr (std::is_same_v<_Letter, Grammars>) {
				this->terminals.insert(gs);
				return gs;
			} else {
				this->terminals.insert(gs.terminals.begin(), gs.terminals.end());
				this->nonTerminals.insert(gs.nonTerminals.begin(), gs.nonTerminals.end());
				this->nonTerminalData.insert(gs.nonTerminalData.begin(), gs.nonTerminalData.end());
				this->rules.insert(gs.rules.begin(), gs.rules.end());
				return gs.start;
			}
		}()...};
		this->addRule(start, {params, ignore});
		this->getNonTerminalData(start).upwardSpillThreshold = INT_MAX;
	}

	template <typename... Grammars>
		requires((std::is_base_of_v<LL1Grammar<_Letter>, Grammars> || std::is_same_v<_Letter, Grammars>) && ...)
	Seq(const _Letter &start, Grammars &&...gs) : Seq(start, std::vector<bool>(sizeof...(gs), false), std::forward<Grammars>(gs)...) {}

};

template <class _Letter>
class Choice : public LL1Grammar<_Letter> {
   public:
	template <typename... Grammars>
		requires(std::is_base_of_v<LL1Grammar<_Letter>, Grammars> && ...)
	Choice(const _Letter &start, Grammars &&...gs) : LL1Grammar<_Letter>(start) {
		(this->terminals.insert(gs.terminals.begin(), gs.terminals.end()), ...);
		(this->nonTerminals.insert(gs.nonTerminals.begin(), gs.nonTerminals.end()), ...);
		(this->nonTerminalData.insert(gs.nonTerminalData.begin(), gs.nonTerminalData.end()), ...);
		(this->rules.insert(gs.rules.begin(), gs.rules.end()), ...);

		(this->addRule(start, {gs.start}), ...);
	}
};

template <class _Letter>
class Repeat : public LL1Grammar<_Letter> {
   public:
	template <typename G>
		requires(std::is_base_of_v<LL1Grammar<_Letter>, G> || std::is_same_v<_Letter, G>)
	Repeat(const _Letter &start, G &&g, int spillThreshold = -1, bool includeEpsilon = true) : LL1Grammar<_Letter>(start) {
		this->terminals.insert(g.terminals.begin(), g.terminals.end());
		this->nonTerminals.insert(g.nonTerminals.begin(), g.nonTerminals.end());
		this->nonTerminalData.insert(g.nonTerminalData.begin(), g.nonTerminalData.end());
		this->rules.insert(g.rules.begin(), g.rules.end());
		this->addRule(start, {g.start, start});
		if (includeEpsilon) this->addRule(start, {});
		else this->addRule(start, {g.start});
		this->getNonTerminalData(start).upwardSpillThreshold = spillThreshold;
	}
};

};	   // namespace ll1g
