#pragma once

#include <DPDA/cfg.h>
#include <climits>

namespace ll1g {

template <class _Letter>
class LL1Grammar : public CFG<_Letter> {
   public:
	LL1Grammar(const _Letter &start) : CFG<_Letter>(start, _Letter::eof) {}
};

template <class _Letter, class G>
concept is_grammar = std::is_base_of_v<LL1Grammar<_Letter>, std::remove_cvref_t<G>>;

template <class _Letter, class G>
concept is_letter = std::is_same_v<_Letter, std::remove_cvref_t<G>>;

template <class _Letter, class G>
concept is_word = std::is_same_v<std::vector<_Letter>, std::remove_cvref_t<G>> ||
					  std::is_same_v<Production<_Letter>, std::remove_cvref_t<G>>;

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
	Word(const _Letter &start, const std::vector<_Letter> &word, const std::vector<bool> &ignore, bool spill = false)
		: LL1Grammar<_Letter>(start) {
		this->terminals.insert(word.begin(), word.end());
		this->addRule(start, Production<_Letter>(word, ignore));
		if (spill) this->getNonTerminalData(start).upwardSpillThreshold = INT_MAX;
	}
	Word(const _Letter &start, const std::vector<_Letter> &word, bool spill = false)
		: Word(start, word, std::vector<bool>(word.size(), false), spill) {}
};

template <class _Letter>
class Optional : public LL1Grammar<_Letter> {
   public:
	template <typename G>
		requires(is_grammar<_Letter, G> || is_letter<_Letter, G>)
	Optional(const _Letter &start, G &&g) : LL1Grammar<_Letter>(start) {
		if constexpr (is_letter<_Letter, G>) {
			this->terminals.insert(g);
			this->addRule(start, {_Letter(g)});
		} else if constexpr (is_word<_Letter, G>) {
			this->terminals.insert(g.begin(), g.end());
			this->addRule(start, g);
		} else {
			this->terminals.insert(g.terminals.begin(), g.terminals.end());
			this->nonTerminals.insert(g.nonTerminals.begin(), g.nonTerminals.end());
			this->nonTerminalData.insert(g.nonTerminalData.begin(), g.nonTerminalData.end());
			this->rules.insert(g.rules.begin(), g.rules.end());
			this->addRule(start, {g.start});
		}

		this->addRule(start, {});
		this->getNonTerminalData(start).upwardSpillThreshold = -1;
	}
};

template <class _Letter>
class Seq : public LL1Grammar<_Letter> {
   public:
	template <typename... Grammars>
		requires((is_grammar<_Letter, Grammars> || is_letter<_Letter, Grammars>) &&
				 ...)
	Seq(const _Letter &start, std::vector<bool> ignore, Grammars &&...gs) : LL1Grammar<_Letter>(start) {
		assert(sizeof...(gs) == ignore.size() && "Ignore vector size must match number of grammars");
		auto params = std::vector<_Letter>{[&]() {
			if constexpr (is_letter<_Letter, Grammars>) {
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
		this->getNonTerminalData(start).upwardSpillThreshold = -1;
	}

	template <typename... Grammars>
		requires((is_grammar<_Letter, Grammars> || is_letter<_Letter, Grammars>) && ...)
	Seq(const _Letter &start, Grammars &&...gs)
		: Seq(start, std::vector<bool>(sizeof...(gs), false), std::forward<Grammars>(gs)...) {}
};

template <class _Letter>
class Choice : public LL1Grammar<_Letter> {
   public:
	template <typename... Grammars>
		requires((is_grammar<_Letter, Grammars> || is_letter<_Letter, Grammars> || is_word<_Letter, Grammars>) && ...)
	Choice(const _Letter &start, Grammars &&...gs) : LL1Grammar<_Letter>(start) {
		static_assert(sizeof...(gs) > 0, "Choice must have at least one grammar");
		(
			[&]() {
				if constexpr (is_letter<_Letter, Grammars>) {
					this->terminals.insert(gs);
					this->addRule(start, {_Letter(gs)});
				} else if constexpr (is_word<_Letter, Grammars>) {
					this->terminals.insert(gs.begin(), gs.end());
					this->addRule(start, gs);
				} else {
					this->terminals.insert(gs.terminals.begin(), gs.terminals.end());
					this->nonTerminals.insert(gs.nonTerminals.begin(), gs.nonTerminals.end());
					this->nonTerminalData.insert(gs.nonTerminalData.begin(), gs.nonTerminalData.end());
					this->rules.insert(gs.rules.begin(), gs.rules.end());
					this->addRule(start, {gs.start});
				}
			}(),
			...);
	}
};

template <class _Letter>
class Repeat : public LL1Grammar<_Letter> {
   public:
	template <typename G>
		requires(is_grammar<_Letter, G> || is_letter<_Letter, G> || is_word<_Letter, G>)
	Repeat(const _Letter &start, G &&g, int spillThreshold = -1)
		: LL1Grammar<_Letter>(start) {
		if constexpr (std::is_same_v<_Letter, std::remove_cvref_t<G>>) {
			this->terminals.insert(g);
			this->addRule(start, {_Letter(g), start});
		} else if constexpr (is_word<_Letter, G>) {
			this->terminals.insert(g.begin(), g.end());
			auto word = g;
			word.push_back(start);
			this->addRule(start, word);
		} else {
			this->terminals.insert(g.terminals.begin(), g.terminals.end());
			this->nonTerminals.insert(g.nonTerminals.begin(), g.nonTerminals.end());
			this->nonTerminalData.insert(g.nonTerminalData.begin(), g.nonTerminalData.end());
			this->rules.insert(g.rules.begin(), g.rules.end());
			this->addRule(start, {g.start, start});
		}
		this->addRule(start, {});
		this->getNonTerminalData(start).upwardSpillThreshold = spillThreshold;
	}
};

template <class _Letter>
class RepeatChoice : public LL1Grammar<_Letter> {
   public:
	template <typename... Grammars>
		requires((is_grammar<_Letter, Grammars> || is_letter<_Letter, Grammars> || is_word<_Letter, Grammars>) && ...)
	RepeatChoice(const _Letter &start, int spillThreshold, Grammars &&...gs)
		: LL1Grammar<_Letter>(start) {
		static_assert(sizeof...(gs) > 0, "RepeatChoice must have at least one grammar");
		(
			[&]() {
				if constexpr (is_letter<_Letter, Grammars>) {
					this->terminals.insert(gs);
					this->addRule(start, {_Letter(gs), start});
				} else if constexpr (is_word<_Letter, Grammars>) {
					this->terminals.insert(gs.begin(), gs.end());
					auto word = gs;
					word.push_back(start);
					this->addRule(start, word);
				} else {
					this->terminals.insert(gs.terminals.begin(), gs.terminals.end());
					this->nonTerminals.insert(gs.nonTerminals.begin(), gs.nonTerminals.end());
					this->nonTerminalData.insert(gs.nonTerminalData.begin(), gs.nonTerminalData.end());
					this->rules.insert(gs.rules.begin(), gs.rules.end());
					this->addRule(start, {gs.start, start});
				}
			}(),
			...);
		this->addRule(start, {});
		this->getNonTerminalData(start).upwardSpillThreshold = spillThreshold;
	}

	template <typename... Grammars>
		requires((is_grammar<_Letter, Grammars> || is_letter<_Letter, Grammars> || is_word<_Letter, Grammars>) && ...)
	RepeatChoice(const _Letter &start, Grammars &&...gs) :
		RepeatChoice(start, -1, std::forward<Grammars>(gs)...) {}
};

template <class _Letter>
class Combine : public LL1Grammar<_Letter> {
   public:
	template <class StartGrammar, class... Grammars>
		requires(is_grammar<_Letter, Grammars> && ...)
	Combine(StartGrammar &&startGrammar, Grammars &&...gs) : LL1Grammar<_Letter>(startGrammar.start) {
		this->terminals.insert(startGrammar.terminals.begin(), startGrammar.terminals.end());
		this->nonTerminals.insert(startGrammar.nonTerminals.begin(), startGrammar.nonTerminals.end());
		this->nonTerminalData.insert(startGrammar.nonTerminalData.begin(), startGrammar.nonTerminalData.end());
		this->rules.insert(startGrammar.rules.begin(), startGrammar.rules.end());

		(this->terminals.insert(gs.terminals.begin(), gs.terminals.end()), ...);
		(this->nonTerminals.insert(gs.nonTerminals.begin(), gs.nonTerminals.end()), ...);
		(this->nonTerminalData.insert(gs.nonTerminalData.begin(), gs.nonTerminalData.end()), ...);
		(this->rules.insert(gs.rules.begin(), gs.rules.end()), ...);

		this->terminals = this->terminals |
						  std::views::filter([this](const _Letter &l) { return !this->nonTerminals.contains(l); }) |
						  std::ranges::to<std::unordered_set<_Letter, std::hash<_Letter>>>();
	}
};

template <class _Letter, class... Grammars>
Combine(LL1Grammar<_Letter> &&, Grammars &&...) -> Combine<_Letter>;
template <class _Letter, class... Grammars>
Combine(const LL1Grammar<_Letter> &, Grammars &&...) -> Combine<_Letter>;
};	   // namespace ll1g
