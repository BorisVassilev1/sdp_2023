#pragma once

#include <DPDA/dpda.h>
#include <chrono>
#include <functional>
#include <memory>
#include <ratio>
#include <stack>
#include <unordered_map>
#include "DPDA/token.h"
#include "util/utils.hpp"
#include <DPDA/utils.h>
#include <DPDA/cfg.h>

template <class Letter>
struct ParseNode {
	Letter											value;
	std::vector<std::unique_ptr<ParseNode<Letter>>> children;

	ParseNode(const Letter value) : value(value) {}
	ParseNode(const Letter value, std::vector<std::unique_ptr<ParseNode<Letter>>> &&children)
		: value(value), children(std::move(children)) {}
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

template <class Letter>
inline std::size_t getLengthOfTokens(const std::vector<Letter> &word, std::size_t offset, std::size_t length) {
	std::string s;
	for (std::size_t i = 0; i < length; ++i) {
		s += std::format("{}", word[offset + i]);
	}
	return s.size();
}

template <class Letter>
class Parser : public DPDA<State<Letter>, Letter> {
   protected:
	using LState = State<Letter>;
	using typename DPDA<LState, Letter>::DeltaMap;
	using DPDA<LState, Letter>::printState;
	using DPDA<LState, Letter>::addTransition;
	using DPDA<LState, Letter>::transition;

	CFG<Letter> g;

	const std::unordered_map<Letter, bool>						 nullable;
	const std::unordered_map<Letter, std::unordered_set<Letter>> first;
	const std::unordered_map<Letter, std::unordered_set<Letter>> follow;

	void detectMistake(const std::vector<Letter> &word, std::size_t offset, const std::vector<Letter> &stack,
					   LState current_state) const {
		size_t		position = offset > 0 ? offset - 1 : 0;
		std::string msg;
		Letter		currentLetter = Letter(current_state - Letter::size);

		dbLog(dbg::LOG_ERROR, "-------------");
		printState(current_state, offset, stack, word);
		dbLog(dbg::LOG_ERROR, "-------------");

		if (current_state == 1 && !g.terminals.contains(word[offset])) {
			msg = std::format("unexpected '{}' - not a valid terminal", word[offset]);
		} else if (!stack.empty() && current_state > Letter::size) {
			std::unordered_set<Letter> expected;
			auto					  &firsts = *this->first.find(stack.back());
			expected						  = firsts.second;
			if (this->nullable.find(stack.back())->second) {
				auto &follows = *this->follow.find(stack.back());
				expected.insert(follows.second.begin(), follows.second.end());
			}
			auto filtered = expected | std::views::filter([&](const Letter l) { return g.terminals.contains(l); });

			msg = std::format("expected one of {}, but got '{}'", filtered, currentLetter);
		} else if (stack.empty() && offset == word.size()) {
			msg = std::format("unexpected end of file");
		} else if (current_state > Letter::size) {
			msg = std::format("unexpected {}", currentLetter);
		} else {
			msg = "unknown error";
		}
		msg += "\n";

		for (size_t i = std::max((int)position - 5, 0); i < std::min(position + 5, word.size()); ++i) {
			msg += std::format("{}", word[i]);
		}

		int			tokensBefore = std::min((int)position, 5);
		std::size_t len			 = getLengthOfTokens(word, position - tokensBefore, tokensBefore);
		// this does not work on clang 20.1.6 c++26
		// msg += std::format("\n{: >{}}", '^', len + 1);
		msg += "\n";
		for (size_t i = 0; i < len; ++i) {
			msg += " ";
		}
		msg += '^';

		throw ParseError(msg, position);
	}

   public:
	using DPDA<LState, Letter>::delta;
	using DPDA<LState, Letter>::qFinal;
	using DPDA<LState, Letter>::enable_print;
	using DPDA<LState, Letter>::printTransitions;

	/**
	 * @brief Construct a Parser from an LL(1) grammar. Throws if grammar is not LL(1)
	 *
	 * @param grammar
	 */
	Parser(const CFG<Letter> &grammar)
		: g(grammar),
		  nullable(grammar.findNullables()),
		  first(grammar.findFirsts(nullable)),
		  follow(grammar.findFollows(nullable, first)) {
		bool intersection = false;
		for (const auto l : grammar.terminals) {
			if (grammar.nonTerminals.contains(l)) {
				intersection = true;
				break;
			}
		}
		for (const auto l : grammar.nonTerminals) {
			if (grammar.terminals.contains(l)) {
				intersection = true;
				break;
			}
		}
		if (intersection) {
			throw std::runtime_error("Grammar is not LL(1): intersection between terminals and nonterminals");
		}

		addTransition(0, Letter::eps, Letter::eps, 1, {grammar.start});

		auto f = [](const Letter l) -> LState { return Letter::size + std::size_t(l); };

		try {
			for (const auto &[A, v] : grammar.rules) {
				if (v.empty()) {
					const auto &followA = follow.find(A)->second;
					for (const auto l : followA) {
						addTransition(f(l), Letter::eps, A, f(l), v);
					}
				} else {
					const auto &firstA = grammar.first(v.rhs, nullable, first);
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
	 * @param productions
	 * @param word
	 * @return requires&&
	 */
	std::unique_ptr<ParseNode<Letter>> makeParseTree(
		const std::vector<std::reference_wrapper<const typename DeltaMap::value_type>> &productions,
		const std::vector<Letter>													   &word) const {
		// stack implementation
		struct ParsingState {
			std::unique_ptr<ParseNode<Letter>> node;
			unsigned int					   output_index;	 // where to continue from when backtracking
			unsigned int					   prod_index;		 // index of production generating that node
		};

		std::stack<ParsingState> parseStack;
		parseStack.push({std::make_unique<ParseNode<Letter>>(g.start), 0, 0});
		unsigned int word_position	 = 0;	  // position in output_word
		unsigned int next_production = 0;	  // index of production to use

		while (word_position < word.size()) {
			auto &[topNode, idx, prod_index] = parseStack.top();
			const auto &[from, to]			 = productions[prod_index].get();
			const auto &[_, product]		 = to;

			if (idx == product.size()) {
				if (product.empty()) topNode->children.push_back(std::make_unique<ParseNode<Letter>>(Letter::eps));

				if (parseStack.size() == 1) break;
				auto [childNode, _, _] = std::move(parseStack.top());
				parseStack.pop();

				auto &[topNode, idx, prod_index] = parseStack.top();
				topNode->children.emplace_back(std::move(childNode));
				++idx;

				continue;
			}

			if (product[idx] == word[word_position]) {	   // terminal
				topNode->children.push_back(std::make_unique<ParseNode<Letter>>(word[word_position]));
				++word_position;
				++idx;
			} else {	 // non-terminal
				parseStack.push({std::make_unique<ParseNode<Letter>>(product[idx]), 0, ++next_production});
			}
		}

		return std::move(parseStack.top().node);
	}

	std::unique_ptr<ParseNode<Letter>> makeAST(
		const std::vector<std::reference_wrapper<const typename DeltaMap::value_type>> &productions,
		const std::vector<Letter>													   &word) const {
		// stack implementation
		struct ParsingState {
			std::unique_ptr<ParseNode<Letter>> node;
			unsigned int					   output_index;	 // where to continue from when backtracking
			unsigned int					   prod_index;		 // index of production generating that node
		};

		std::stack<ParsingState> parseStack;
		parseStack.push({std::make_unique<ParseNode<Letter>>(g.start), 0, 0});
		unsigned int word_position	 = 0;	  // position in output_word
		unsigned int next_production = 0;	  // index of production to use

		while (word_position < word.size()) {
			auto &[topNode, idx, prod_index] = parseStack.top();
			const auto &[from, to]			 = productions[prod_index].get();
			const auto &[_, _, A]		 = from;
			const auto &[_, product]		 = to;

			if (idx == product.size()) {
				if (parseStack.size() == 1) break;
				auto [childNode, _, _] = std::move(parseStack.top());
				parseStack.pop();

				const auto &NTData = g.getNonTerminalData(A);
				auto &[topNode, idx, prod_index] = parseStack.top();

				// throw out the node if it is empty and we ignore empty
				if (NTData.ignoreEmpty && childNode->children.size() == 0) {
					++idx;
					continue;
				}
				// throw out the node if it has a single child and we ignore single child
				if (NTData.ignoreSingleChild && childNode->children.size() == 1) {
					if (product.replaceWith != Letter::eps) {
						topNode->value = product.replaceWith;
					}

					topNode->children.emplace_back(std::move(childNode->children[0]));
					++idx;
					continue;
				}

				if (NTData.upwardSpillThreshold < 0 || childNode->children.size() > size_t(NTData.upwardSpillThreshold)) {

					if (product.replaceWith != Letter::eps) {
						childNode->value = product.replaceWith;
					}
					topNode->children.emplace_back(std::move(childNode));
				} else { // spill upwards if we have few children
					for (auto &c : childNode->children) {
						topNode->children.emplace_back(std::move(c));
					}
				}
				++idx;

				continue;
			}

			if (product[idx] == word[word_position]) {	   // terminal
				if (!product.ignore[idx])
					topNode->children.push_back(std::make_unique<ParseNode<Letter>>(word[word_position]));
				++word_position;
				++idx;
			} else {	 // non-terminal
				parseStack.push({std::make_unique<ParseNode<Letter>>(product[idx]), 0, ++next_production});
			}
		}

		return std::move(parseStack.top().node);
	}

	using ProductionVector = std::vector<std::reference_wrapper<const typename DeltaMap::value_type>>;

	std::pair<bool, ProductionVector> generateProductions(const std::vector<Letter> &word) const {
		std::size_t																 offset = 0;
		std::vector<Letter>														 stack;
		LState																	 current_state = 0;
		typename DeltaMap::const_iterator										 res		   = delta.begin();
		std::vector<std::reference_wrapper<const typename DeltaMap::value_type>> productions;

		while ((current_state != qFinal || !stack.empty()) && res != delta.end()) {
			const Letter &l = offset < word.size() ? word[offset] : Letter::eps;
			const Letter &s = stack.empty() ? Letter::eps : stack.back();
			if (enable_print) { printState(current_state, offset, stack, word); }

			res = transition(current_state, l, s, stack, offset);
			if (res == delta.end() && s != Letter::eps) { res = transition(current_state, l, Letter::eps, stack, offset); }
			if (res == delta.end() && l != Letter::eps) { res = transition(current_state, Letter::eps, s, stack, offset); }
			if (res == delta.end() && s != Letter::eps && l != Letter::eps) { res = transition(current_state, Letter::eps, Letter::eps, stack, offset); }

			if (res != delta.end() && std::get<0>(res->first) == std::get<0>(res->second))
				productions.push_back(std::ref(*res));
		}
		if (enable_print) { printState(current_state, offset, stack, word); }

		bool accepted = current_state == qFinal && stack.empty() && offset == word.size();

		if (!accepted) { detectMistake(word, offset, stack, current_state); }
		return {accepted, productions};
	}

	/**
	 * @brief parses a word and builds its parse tree. If it fails, throws a ParseError
	 *
	 * @param word
	 * @return std::unique_ptr<ParseNode<Letter>>
	 */
	std::unique_ptr<ParseNode<Letter>> parse(const std::vector<Letter> &word) const {
		auto start					 = std::chrono::high_resolution_clock::now();
		auto [accepted, productions] = generateProductions(word);
		auto end					 = std::chrono::high_resolution_clock::now();
		if (!accepted) { return nullptr; }
		dbLog(dbg::LOG_DEBUG, "productions done: ", productions.size(), " in ",
			  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), "ms");

		auto parseTree = makeParseTree(productions, word);

		return parseTree;
	}

	std::unique_ptr<ParseNode<Letter>> ASTparse(const std::vector<Letter> &word) const {
		auto start					 = std::chrono::high_resolution_clock::now();
		auto [accepted, productions] = generateProductions(word);
		auto end					 = std::chrono::high_resolution_clock::now();
		if (!accepted) { return nullptr; }
		dbLog(dbg::LOG_DEBUG, "productions done: ", productions.size(), " in ",
			  std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), "ms");

		auto parseTree = makeAST(productions, word);

		return parseTree;
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

	template <typename U = Letter>
	std::unique_ptr<ParseNode<Letter>> ASTparse(const std::string &word) const {
		std::vector<Letter> w;
		for (std::size_t i = 0; i < word.size(); ++i) {
			w.push_back(Letter(word[i]));
		}
		if (word.back() != Letter::eof) w.push_back(Letter::eof);
		return ASTparse(w);
	}

	const CFG<Letter> &getGrammar() const { return g; }
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
static inline void printFunctionDefault(std::ostream &out, const ParseNode<Letter> *r) {
	out << "-" << r->value << std::endl;
}

template <class Node>
static void p_show(std::ostream &out, const Node *r, bits &b,
				   std::function<void(std::ostream &, const Node *)> printFunction = printFunctionDefault) {
	// https://en.wikipedia.org/wiki/Box-drawing_character
	if (r) {
		printFunction(out, r);
		// out << "-" << r->value << std::endl;

		for (size_t i = 0; i + 1 < r->children.size(); ++i) {
			p_tabs(out, b);
			out << " \u251c";	  // ├
			b.push_back(true);
			p_show(out, r->children[i].get(), b, printFunction);
			b.pop_back();
		}

		if (!r->children.empty()) {
			p_tabs(out, b);
			out << " \u2514";	  // └
			b.push_back(false);
			p_show(out, r->children.back().get(), b, printFunction);
			b.pop_back();
		}
	} else out << " \u25cb" << std::endl;	  // ○
}
}	  // namespace

template <class Letter>
std::ostream &operator<<(std::ostream &out, const std::unique_ptr<ParseNode<Letter>> &node) {
	bits b;
	p_show<ParseNode<Letter>>(out, node.get(), b, printFunctionDefault<Letter>);
	return out;
}
template <class Letter>
struct std::formatter<ParseNode<Letter>> : ostream_formatter {};
