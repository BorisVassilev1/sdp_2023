#pragma once

#include <DPDA/dpda.h>
#include <concepts>
#include <memory>
#include <type_traits>

template <class Letter>
struct ParseNode {
	Letter											value;
	std::vector<std::unique_ptr<ParseNode<Letter>>> children;

	ParseNode(const Letter value) : value(value) {}
};

/**
 * @brief A custom exception type for errors, emitted while a DPDA is parsing a string
 */
class ParseError : public std::exception {
   protected:
	std::string msg;
	std::size_t position;

   public:
	ParseError(const std::string &msg, std::size_t position)
		: msg(msg + " at " + std::to_string(position)), position(position) {}
	ParseError(const char *msg, std::size_t position)
		: msg(std::string(msg) + " at " + std::to_string(position)), position(position) {}

	virtual ~ParseError() noexcept {}

	virtual const char *what() const noexcept { return msg.c_str(); }
};

template <class State, class Letter>
class Parser : public DPDA<State, Letter> {
	using typename DPDA<State, Letter>::DeltaMap;
	using DPDA<State, Letter>::printState;
	using DPDA<State, Letter>::addTransition;
	using DPDA<State, Letter>::transition;

	CFG<Letter> g;

	void detectMistake(const std::vector<Letter> &word, std::size_t offset, const std::vector<Letter> &stack,
					   State current_state) const {
		size_t		position = offset > 0 ? offset - 1 : 0;
		std::string msg;
		if (!stack.empty() && current_state > Letter::size) {
			msg = std::format("expected a/an {}, but got '{}'", stack.back(), Letter(current_state - Letter::size));
		} else if (stack.empty() && offset == word.size()) {
			msg = std::format("unexpected end of file");
		} else if (current_state > Letter::size) {
			msg = std::format("unexpected {}", Letter(current_state - Letter::size));
		} else {
			msg = "unknown error";
		}
		throw ParseError(msg, position);
	}

   public:
	using DPDA<State, Letter>::delta;
	using DPDA<State, Letter>::qFinal;
	using DPDA<State, Letter>::enable_print;
	using DPDA<State, Letter>::printTransitions;

	/**
	 * @brief Construct a Parser from an LL(1) grammar. Throws if grammar is not LL(1)
	 *
	 * @param grammar
	 */
	Parser(const CFG<Letter> &grammar) : g(grammar) {
		addTransition(0, Letter::eps, Letter::eps, 1, {grammar.start});

		const auto nullable = grammar.findNullables();
		const auto first	= grammar.findFirsts(nullable);
		const auto follow	= grammar.findFollows(nullable, first);

		auto f = [](const Letter l) -> State { return Letter::size + std::size_t(l); };

		try {
			for (const auto &[A, v] : grammar.rules) {
				if (v.empty()) {
					const auto &followA = follow.find(A)->second;
					for (const auto l : followA) {
						addTransition(f(l), Letter::eps, A, f(l), v);
					}
				} else {
					const auto &firstA = grammar.first(v, nullable, first);
					for (const auto l : firstA) {
						addTransition(f(l), Letter::eps, A, f(l), v);
					}
				}
			}

			for (const auto l : grammar.terminals) {
				addTransition(1, l, Letter::eps, f(l), {});
				if (l != grammar.eof) addTransition(f(l), Letter::eps, l, 1, {});
			}
		} catch (...) { std::throw_with_nested(std::runtime_error("Failed to create parser for grammar")); }

		qFinal = f(grammar.eof);
	}

	/**
	 * @brief Constructs a parse tree from a word and a series of productions
	 *
	 * @tparam T
	 * @tparam U
	 * @param productions
	 * @param word
	 * @param k - start index for productions
	 * @param j - start index for word
	 * @return requires&&
	 */
	std::unique_ptr<ParseNode<Letter>> makeParseTree(const std::vector<typename DeltaMap::value_type> &productions,
													 const std::vector<Letter> &word, int &k, int &j) const {
		auto node	 = std::make_unique<ParseNode<Letter>>(std::get<2>(productions[k].first));
		auto product = std::get<1>(productions[k].second);
		++k;
		if (product.empty()) node->children.push_back(std::make_unique<ParseNode<Letter>>(Letter::eps));

		for (size_t i = 0; i < product.size(); ++i) {
			if (product[i] == word[j]) {
				node->children.push_back(std::make_unique<ParseNode<Letter>>(word[j]));
				++j;
			} else {
				node->children.push_back(makeParseTree(productions, word, k, j));
			}
		}

		return node;
	}

	/**
	 * @brief parses a word and builds its parse tree. If it fails, throws a ParseError
	 *
	 * @param word
	 * @return std::unique_ptr<ParseNode<Letter>>
	 */
	std::unique_ptr<ParseNode<Letter>> parse(const std::vector<Letter> &word) const {
		std::size_t								   offset = 0;
		std::vector<Letter>						   stack;
		State									   current_state = 0;
		typename DeltaMap::const_iterator				   res			 = delta.begin();
		std::vector<typename DeltaMap::value_type> productions;

		while ((current_state != qFinal || !stack.empty()) && res != delta.end()) {
			const Letter &l = offset < word.size() ? word[offset] : Letter::eps;
			const Letter &s = stack.empty() ? Letter::eps : stack.back();
			if (enable_print) { printState(current_state, offset, stack, word); }

			res = transition(current_state, l, s, stack, offset);
			if (res == delta.end() && s) { res = transition(current_state, l, '\0', stack, offset); }
			if (res == delta.end() && l) { res = transition(current_state, '\0', s, stack, offset); }
			if (res == delta.end() && s && l) { res = transition(current_state, '\0', '\0', stack, offset); }

			if (res != delta.end() && std::get<0>(res->first) == std::get<0>(res->second)) productions.push_back(*res);
		}
		if (enable_print) { printState(current_state, offset, stack, word); }

		bool accepted = current_state == qFinal && stack.empty() && offset == word.size();

		if (!accepted) {
			detectMistake(word, offset, stack, current_state);
		} else {
			int	 i = 0, k = 0;
			auto parseTree = makeParseTree(productions, word, k, i);

			return parseTree;
		}

		return nullptr;
	}

	/**
	 * @brief same as the other method, but for strings
	 *
	 * @tparam U
	 * @param word
	 * @return std::unique_ptr<ParseNode<Letter>>
	 */
	template <typename U = Letter>
	std::unique_ptr<ParseNode<Letter>> parse(const std::string &word) const {
		std::vector<Letter> w;
		for (std::size_t i = 0; i < word.size(); ++i) {
			w.push_back(Letter(word[i]));
		}
		if (word.back() != Letter::eof) w.push_back(Letter::eof);
		return parse(w);
	}
};

// reworked version of
// https://hbfs.wordpress.com/2016/12/06/pretty-printing-a-tree-in-a-terminal/
namespace {
using bits = std::vector<bool>;

static inline void p_tabs(std::ostream &out, const bits &b) {
	for (auto x : b)
		out << (x ? " \u2502" : "  ");
}

template <class Letter>
static void p_show(std::ostream &out, const ParseNode<Letter> *r, bits &b) {
	// https://en.wikipedia.org/wiki/Box-drawing_character
	if (r) {
		out << "-" << r->value << std::endl;

		for (size_t i = 0; i + 1 < r->children.size(); ++i) {
			p_tabs(out, b);
			out << " \u251c";	  // ├
			b.push_back(true);
			p_show(out, r->children[i].get(), b);
			b.pop_back();
		}

		if (!r->children.empty()) {
			p_tabs(out, b);
			out << " \u2514";	  // └
			b.push_back(false);
			p_show(out, r->children.back().get(), b);
			b.pop_back();
		}
	} else out << " \u25cb" << std::endl;	  // ○
}
}	  // namespace

template <class Letter>
std::ostream &operator<<(std::ostream &out, const std::unique_ptr<ParseNode<Letter>> &node) {
	bits b;
	p_show(out, node.get(), b);
	return out;
}
template <class Letter>
struct std::formatter<ParseNode<Letter>> : ostream_formatter {};
