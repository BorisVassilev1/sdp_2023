#pragma once
#include <concepts>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace fl {

/// a concept for classes that can be Letter in a DPDA<State, Letter>
template <class L>
concept isLetter = requires() {
	{ L::eps } -> std::same_as<const L &>;
	{ L::eof } -> std::same_as<const L &>;
	{ L::size } -> std::convertible_to<const std::size_t &>;
	std::is_convertible_v<L, std::size_t>;
	not std::is_fundamental_v<L>;
};

/// a concept for classes that can be State in a DPDA<State, Letter>
template <class S>
concept isState = requires(std::size_t i) {
	std::is_convertible_v<S, std::size_t>;
	{ new S(i) };
	{ new S() };
	not std::is_fundamental_v<S>;
};

template <int N, typename... Ts>
using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

template <isLetter Letter, std::ranges::viewable_range T>
std::vector<Letter> toLetter(T &&t) {
	return std::vector<Letter>(std::begin(t), std::end(t));
}

template <isLetter Letter>
std::vector<Letter> toLetter(const char *s) {
	return toLetter<Letter>(std::string_view(s));
}

}	  // namespace fl
