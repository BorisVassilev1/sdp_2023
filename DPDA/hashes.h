#pragma once

#include <tuple>
#include <ostream>
#include <vector>

class Letter {
	char val;
public:
	constexpr Letter(char val) : val(val) {}
	constexpr Letter() : val(0) {}

	constexpr operator char() const {return val;}
	static const Letter eps;
};

constexpr const Letter Letter::eps = '\0';

namespace std {
  template <> struct hash<Letter>
  {
    size_t operator()(const Letter &x) const
    {
      return hash<char>()(x);
    }
  };
}

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
