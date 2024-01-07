#pragma once

#include <tuple>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <format>
#include <string_view>

class Letter {
	char val;

   public:
	constexpr Letter(char val) : val(val) {}
	constexpr Letter() : val(0) {}

	constexpr operator char() const { return val; }
	static const Letter eps;
	static const size_t size;
};

constexpr const Letter Letter::eps	= '\0';
constexpr const size_t Letter::size = 256;

class State {
	size_t val;

   public:
	constexpr State(size_t val) : val(val) {}
	constexpr State() : val(0){};

	constexpr operator size_t() const { return val; }
};

template <class Letter>
struct ParseNode {
	Letter							 value;
	std::vector<ParseNode<Letter> *> children;

	ParseNode(const Letter value, const std::vector<ParseNode<Letter> *> &children)
		: value(value), children(children) {}
};

namespace std {
template <>
struct hash<Letter> {
	size_t operator()(const Letter &x) const { return hash<char>()(x); }
};

template <>
struct hash<State> {
	size_t operator()(const State &x) const { return hash<size_t>()(x); }
};
}	  // namespace std

template <int N, typename... Ts>
using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

template <class... Args>
std::ostream &operator<<(std::ostream &out, const std::tuple<Args...> &t);
template <class A, class B>
std::ostream &operator<<(std::ostream &out, const std::pair<A, B> &t);
template <class T>
std::ostream &operator<<(std::ostream &out, std::vector<T> v);

struct tuple_hash {
	template <class... Args>
	std::size_t operator()(const std::tuple<Args...> &t) const {
		return [&]<std::size_t... p>(std::index_sequence<p...>) {
			return ((std::hash<NthTypeOf<p, Args...>>{}(std::get<p>(t))) ^ ...);
		}(std::make_index_sequence<std::tuple_size_v<std::tuple<Args...>>>{});
	}
};

template <class... Args>
std::ostream &operator<<(std::ostream &out, const std::tuple<Args...> &t) {
	out << "(";
	[&]<std::size_t... p>(std::index_sequence<p...>) {
		((out << (p ? ", " : "") << "\'" << std::get<p>(t) << "\'"), ...);
	}(std::make_index_sequence<std::tuple_size_v<std::tuple<Args...>>>{});
	return out << ")";
}

template <class A, class B>
std::ostream &operator<<(std::ostream &out, const std::pair<A, B> &t) {
	return out << "(" << t.first << ", " << t.second << ")";
}

template <class T>
std::ostream &operator<<(std::ostream &out, std::vector<T> v) {
	for (const T &x : v)
		out << x;
	return out;
}

template <class T>
std::ostream &operator<<(std::ostream &out, std::unordered_set<T> v) {
	out << "( ";
	for (const T &x : v)
		out << x << " ";
	out << ")";
	return out;
}

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
			p_show(out, r->children[i], b);
			b.pop_back();
		}

		if (!r->children.empty()) {
			p_tabs(out, b);
			out << " \u2514";	  // └
			b.push_back(false);
			p_show(out, r->children.back(), b);
			b.pop_back();
		}
	} else out << " \u25cb" << std::endl;	  // ○
}
}	  // namespace

template <class Letter>
std::ostream &operator<<(std::ostream &out, const ParseNode<Letter> *node) {
	bits b;
	p_show(out, node, b);
	return out;
}

template <class Letter>
void deleteParseTree(const ParseNode<Letter> *root) {
	for (const auto *t : root->children) {
		deleteParseTree<Letter>(t);
	}
	delete root;
}

std::ostream &operator<<(std::ostream &out, const std::exception &e);
std::ostream &operator<<(std::ostream &out, Letter l);
std::ostream &operator<<(std::ostream &out, State s);

template <typename Char>
struct basic_ostream_formatter : std::formatter<std::basic_string_view<Char>, Char> {
  template <typename T, typename OutputIt>
  auto format(const T& value, std::basic_format_context<OutputIt, Char>& ctx) const
      -> OutputIt {
    std::basic_stringstream<Char> ss;
    ss << value;
    return std::formatter<std::basic_string_view<Char>, Char>::format(
        ss.view(), ctx);
  }
};

using ostream_formatter = basic_ostream_formatter<char>;
template <> struct std::formatter<Letter> : ostream_formatter {};
template <> struct std::formatter<State> : ostream_formatter {};
template <> struct std::formatter<std::exception> : ostream_formatter {};
template <class Letter> struct std::formatter<ParseNode<Letter>> : ostream_formatter {};
template <class... Args> struct std::formatter<std::tuple<Args...>> : ostream_formatter {};
template <class A, class B> struct std::formatter<std::pair<A, B>> : ostream_formatter {};


