#pragma once
#include <concepts>
#include <span>
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
	{ L() } -> std::same_as<L>;
	{ ++std::declval<L &>() } -> std::same_as<L>;
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

template <class T>
concept isSSFST =						  //
	isLetter<typename T::Letter_t> &&	  //
	isState<typename T::State> &&		  //
	requires(T t, typename T::Letter_t l, T::State s) {
		{
			&T::step
		} -> std::same_as<std::pair<std::span<const typename T::Letter_t>, bool> (T::*)(typename T::State &,
																						typename T::Letter_t) const>;
		{ &T::psi } -> std::same_as<std::span<const typename T::Letter_t> (T::*)(typename T::State) const>;
		{ &T::isFinal } -> std::same_as<bool (T::*)(typename T::State) const>;
		{ &T::initial } -> std::same_as<typename T::State (T::*)() const>;
		{ &T::size } -> std::same_as<std::size_t (T::*)() const>;
	};

template <class T>
concept isSSFSTI = isSSFST<T> && requires(T t) {
	{ &T::initialOutput } -> std::same_as<std::span<const typename T::Letter_t> (T::*)() const>;
};

}	  // namespace fl
