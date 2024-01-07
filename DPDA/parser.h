#include <DPDA/dpda.h>

template <class State, class Letter>
class Parser : public DPDA<State, Letter> {
	using typename DPDA<State, Letter>::DeltaMap;
	using DPDA<State, Letter>::delta;
	using DPDA<State, Letter>::qFinal;
	using DPDA<State, Letter>::enable_print;

	using DPDA<State, Letter>::printState;
	using DPDA<State, Letter>::addTransition;
	using DPDA<State, Letter>::printTransitions;
	using DPDA<State, Letter>::transition;

	CFG<Letter> g;

   public:
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

	template <class T, class U>
		requires std::ranges::random_access_range<T> && std::ranges::random_access_range<U>
	ParseNode<Letter> *makeParseTree(const U &productions, const T &word, int &k, int &j) {
		auto node	 = new ParseNode<Letter>(std::get<2>(productions[k].first), {});
		auto product = std::get<1>(productions[k].second);
		++k;
		if (product.empty()) node->children.push_back(new ParseNode<Letter>{Letter::eps, {}});

		for (size_t i = 0; i < product.size(); ++i) {
			if (product[i] == word[j]) {
				node->children.push_back(new ParseNode<Letter>{word[j], {}});
				++j;
			} else {
				node->children.push_back(makeParseTree(productions, word, k, j));
			}
		}

		return node;
	}

	void detectMistake(const std::vector<Letter> &word, std::size_t offset, const std::vector<Letter> &stack,
					   State current_state) {
		size_t		position = offset > 0 ? offset - 1 : 0;
		std::string msg;
		if (!stack.empty() && current_state > Letter::size) {
			msg = std::format("expected a/an {}, but got {}", stack.back(), Letter(current_state - Letter::size));
		} else if (stack.empty() && offset == word.size()) {
			msg = "unexpected end of file";
		} else if (current_state > Letter::size) {
			msg = std::format("unexpected {}", Letter(current_state - Letter::size));
		} else {
			msg = "unknown error";
		}
		throw ParseError(msg, position);
	}

	ParseNode<Letter> *parse(const std::vector<Letter> &word) {
		std::size_t								   offset = 0;
		std::vector<Letter>						   stack;
		State									   current_state = 0;
		typename DeltaMap::iterator				   res			 = delta.begin();
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

	template <typename U = Letter>
	ParseNode<Letter> *parse(const std::string &word) {
		std::vector<Letter> w;
		for(std::size_t i = 0; i < word.size(); ++i) {
			w.push_back(Letter(word[i]));
		}
		if(word.back() != Letter::eof)
			w.push_back(Letter::eof);
		return parse(w);
	}
};
