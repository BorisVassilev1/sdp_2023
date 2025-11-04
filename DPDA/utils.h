#pragma once

#include <tuple>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <format>
#include <string_view>
#include <sstream>
#include <string>
#include <ranges>

/**
 * @brief A simple Letter class
 *
 */
class Letter {
	char val = 0;

   public:
	constexpr Letter(char val) : val(val) {}
	constexpr Letter() : val(0) {}
	constexpr Letter(const Letter &other)			 = default;
	constexpr Letter &operator=(const Letter &other) = default;

	constexpr			operator char() const { return val; }
	static const Letter eps;
	static const Letter eof;
	static const size_t size;
};

constexpr const Letter Letter::eps	= '\xFF';
constexpr const Letter Letter::eof	= '#';
constexpr const size_t Letter::size = 256;

inline std::vector<Letter> toLetter(const char *s) {
	std::vector<Letter> res;
	for (const char *c = s; *c != '\0'; ++c) {
		res.push_back(Letter(*c));
	}
	return res;
}

template <class Letter_t = Letter, std::ranges::viewable_range T>
std::vector<Letter_t> toLetter(T &&t) {
	return std::vector<Letter_t>(std::begin(t), std::end(t));
}


/**
 * @brief A simple Letter class
 *
 */
template <class Letter>
class State {
	size_t val;

   public:
	constexpr State(size_t val) : val(val) {}
	constexpr State() : val(0) {};

	constexpr operator size_t() const { return val; }
};

template <>
struct std::hash<Letter> {
	constexpr hash() = default;
	constexpr size_t operator()(const Letter &x) const { return hash<char>()(x); }
};

template <>
struct std::hash<std::span<Letter>> {
	constexpr hash() = default;
	constexpr size_t operator()(const std::span<Letter> &x) const {
		return std::hash<std::string_view>()(std::string_view(reinterpret_cast<const char *>(x.data()), x.size()));
	}
};

template <>
struct std::hash<std::vector<Letter>> {
	constexpr hash() = default;
	constexpr size_t operator()(const std::vector<Letter> &x) const {
		return std::hash<std::string_view>()(std::string_view(reinterpret_cast<const char *>(x.data()), x.size()));
	}
};

template <class Letter>
struct std::hash<State<Letter>> {
	constexpr hash() = default;
	constexpr size_t operator()(const State<Letter> &x) const { return hash<size_t>()(x); }
};

template <int N, typename... Ts>
using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

template <class... Args>
std::ostream &operator<<(std::ostream &out, const std::tuple<Args...> &t);
template <class A, class B>
std::ostream &operator<<(std::ostream &out, const std::pair<A, B> &t);
template <class T>
std::ostream &operator<<(std::ostream &out, std::vector<T> v);

template <class... Args>
struct std::hash<std::tuple<Args...>> {
	constexpr hash() = default;
	constexpr std::size_t operator()(const std::tuple<Args...> &t) const {
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
		out << '\'' << x << "\' ";
	out << ")";
	return out;
}

std::ostream &operator<<(std::ostream &out, const std::exception &e);
std::ostream &operator<<(std::ostream &out, Letter l);
template <class Letter>
std::ostream &operator<<(std::ostream &out, State<Letter> s) {
	if (s >= Letter::size) {
		out << "f" << Letter(s - Letter::size);
	} else out << size_t(s);
	return out;
}

template <class A, class B>
std::ostream &operator<<(std::ostream &out, const std::unordered_map<A, B> &m) {
	out << "{";
	for (auto &[K, V] : m) {
		out << '(' << K << " : " << V << ')' << std::endl;
	}
	out << "}";
	return out;
}

template <typename Char>
struct basic_ostream_formatter : std::formatter<std::basic_string_view<Char>, Char> {
	template <typename T, typename OutputIt>
	auto format(const T &value, std::basic_format_context<OutputIt, Char> &ctx) const -> OutputIt {
		std::basic_stringstream<Char> ss;
		ss << value;
		return std::formatter<std::basic_string_view<Char>, Char>::format(ss.view(), ctx);
	}
};

using ostream_formatter = basic_ostream_formatter<char>;
template <>
struct std::formatter<Letter> : ostream_formatter {};
template <class Letter>
struct std::formatter<State<Letter>> : ostream_formatter {};
template <>
struct std::formatter<std::exception> : ostream_formatter {};
template <class... Args>
struct std::formatter<std::tuple<Args...>> : ostream_formatter {};
template <class A, class B>
struct std::formatter<std::pair<A, B>> : ostream_formatter {};
template <class T>
struct std::formatter<std::unordered_set<T>> : ostream_formatter {};
template <class T>
struct std::formatter<std::vector<T>> : ostream_formatter {};
template <class A, class B>
struct std::formatter<std::unordered_multimap<A, B>> : ostream_formatter {};

template <class T>
class RangeFromPair {
	T range;

   public:
	RangeFromPair(const T &range) : range(range) {}

	auto begin() { return range.first; }
	auto end() { return range.second; }
};
