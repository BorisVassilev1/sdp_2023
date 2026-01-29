#pragma once

#include <cstddef>
#include <ostream>
#include "concepts.hpp"
#include "formatting.hpp"

namespace fl {
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

template <class Letter>
std::ostream &operator<<(std::ostream &out, State<Letter> s) {
	if (s >= Letter::size) {
		out << "f" << Letter(s - Letter::size);
	} else out << size_t(s);
	return out;
}
}	  // namespace fl

template <fl::isLetter Letter>
struct std::formatter<fl::State<Letter>> : fl::ostream_formatter {};
