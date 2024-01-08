#pragma once

#include <tuple>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <format>
#include <string_view>

/**
 * @brief A simple Letter class
 * 
 */
class Letter {
	char val;

   public:
	constexpr Letter(char val) : val(val) {}
	constexpr Letter() : val(0) {}

	constexpr operator char() const { return val; }
	static const Letter eps;
	static const Letter eof;
	static const size_t size;
};

constexpr const Letter Letter::eps	= '\0';
constexpr const Letter Letter::eof	= '#';
constexpr const size_t Letter::size = 256;

/**
 * @brief A simple Letter class
 * 
 */
class State {
	size_t val;

   public:
	constexpr State(size_t val) : val(val) {}
	constexpr State() : val(0){};

	constexpr operator size_t() const { return val; }
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
template <class... Args> struct std::formatter<std::tuple<Args...>> : ostream_formatter {};
template <class A, class B> struct std::formatter<std::pair<A, B>> : ostream_formatter {};


