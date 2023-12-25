#pragma once

#include <tuple>
#include <ostream>
#include <unordered_set>
#include <vector>

class Letter {
	char val;
public:
	constexpr Letter(char val) : val(val) {}
	constexpr Letter() : val(0) {}

	constexpr operator char() const {return val;}
	static const Letter eps;
	static const size_t size;
};

constexpr const Letter Letter::eps = '\0';
constexpr const size_t Letter::size = 256;

class State {
	size_t val;
	public:
	constexpr State(size_t val) : val(val) {}
	constexpr State() : val(0) {};
	
	constexpr operator size_t() const {return val;}
};

namespace std {
  template <> struct hash<Letter>
  {
    size_t operator()(const Letter &x) const
    {
      return hash<char>()(x);
    }
  };

  template <> struct hash<State>
  {
    size_t operator()(const State &x) const
    {
      return hash<size_t>()(x);
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

template <class T>
std::ostream &operator<<(std::ostream &out, std::unordered_set<T> v) {
	out << "( ";
	for (const T &x : v)
		out << x << " ";
	out << ")";
	return out;
}

std::ostream &operator<<(std::ostream &out, Letter l) {
    if(l == Letter::eps) {
        out << "ε";
    }
    else out << char(l);
    return out;
}

std::ostream &operator<<(std::ostream &out, State s) {
    if(s >= Letter::size) {
        out << "f" << Letter(s - Letter::size);
    }
    else out << size_t(s);
    return out;
}
