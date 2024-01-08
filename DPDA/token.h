#pragma once
#include <cstddef>
#include <ostream>
#include <unordered_map>
#include <format>
#include <DPDA/utils.h>

/**
 * @brief A Token with a name
 * 
 */
struct Token {
	std::size_t value;

	Token(std::size_t value) : value(value) {}

   public:
	Token(char value) : value(value) {}

	explicit constexpr operator std::size_t() const { return value; }
	explicit constexpr operator char() const { return value; }
	explicit constexpr operator bool() const { return value; }

	static std::size_t size;
	static const Token eps;
	static const Token eof;

	static Token createToken(const std::string &name, std::size_t value = ++size);

	bool				 operator==(const Token &other) const { return value == other.value; }
	friend std::ostream &operator<<(std::ostream &out, const Token &v);
};


namespace std {
template <>
struct hash<Token> {
	size_t operator()(const Token &x) const noexcept { return hash<std::size_t>()(x.value); }
};
}	  // namespace std

std::ostream &operator<<(std::ostream &out, const Token &v);
template <>
struct std::formatter<Token> : ostream_formatter {};
