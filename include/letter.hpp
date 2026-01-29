#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

#include "formatting.hpp"

namespace fl {
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

std::ostream &operator<<(std::ostream &out, Letter l);

}	  // namespace fl

template <>
struct std::formatter<fl::Letter> : fl::ostream_formatter {};
