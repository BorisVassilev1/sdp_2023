#include <iostream>
#include "ReplaceWithMarkerSSFT.hpp"
#include "token.h"

class MyLetter {
   public:
	char value;

	constexpr MyLetter() : value(0) {}
	constexpr MyLetter(char c) : value(c) {}
	constexpr MyLetter(const MyLetter &other)			 = default;
	constexpr MyLetter &operator=(const MyLetter &other) = default;

	constexpr operator size_t() const { return static_cast<size_t>(value); }
	constexpr ~MyLetter() = default;
	constexpr MyLetter operator++() {
		++value;
		return *this;
	}

	constexpr auto operator<=>(std::size_t other) const { return (size_t)value <=> other; }
	constexpr auto operator<=>(const MyLetter &other) const = default;

	constexpr static size_t size = 5;
	const static MyLetter	eps;
	const static MyLetter	eof;
	friend std::ostream	   &operator<<(std::ostream &out, const MyLetter &l) {
		static const char *names[] = {"a", "b", "c", "d", "_", "ε", "eof"};
		if (l.value >= 0 && l.value < 7) return out << names[(int)l.value];
		else return out << "Unknown(" << (int)l.value << ")";
	}

	static MyLetter fromChar(char ch) {
		switch (ch) {
			case 'a': return a;
			case 'b': return b;
			case 'c': return c;
			case 'd': return d;
			case '_': return _;
			default: throw std::invalid_argument("Invalid character for MyLetter");
		}
	}

	const static MyLetter a;
	const static MyLetter b;
	const static MyLetter c;
	const static MyLetter d;
	const static MyLetter _;
};

constexpr inline MyLetter MyLetter::eps = {5};
constexpr inline MyLetter MyLetter::eof = {6};

constexpr inline MyLetter MyLetter::a = 0;
constexpr inline MyLetter MyLetter::b = 1;
constexpr inline MyLetter MyLetter::c = 2;
constexpr inline MyLetter MyLetter::d = 3;
constexpr inline MyLetter MyLetter::_ = 4;

static_assert(fl::isLetter<MyLetter>);

std::vector<MyLetter> toMyLetter(std::string_view input) {
	std::vector<MyLetter> output;
	output.reserve(input.size());
	for (const auto &c : input) {
		output.push_back(MyLetter::fromChar(c));
	}
	return output;
}

int main() {
	using ml = MyLetter;
	// fl::ReplaceWithMarkerSSFT<MyLetter> ssft({{{'a'}, {'b'}}, {{'b'}, {'c'}}, {{'a'}, {'c'}}, {{'d'}, {'c'}}}, '_');
	fl::ReplaceWithMarkerSSFT<MyLetter> ssft(
		{
			//
			{{ml::a}, {ml::b, ml::b}},

			{toMyLetter("abd"), toMyLetter("cc")},
			{toMyLetter("abd"), toMyLetter("c")},
			{toMyLetter("abd"), toMyLetter("cb")},
			{toMyLetter("abc"), toMyLetter("c")},
			{toMyLetter("abdd"), toMyLetter("ab")},
			{toMyLetter("abdd"), toMyLetter("cb")},
			{toMyLetter("abc"), toMyLetter("ab")},
			//
		},
		ml::_, true);

	fl::statFSA(ssft);
	fl::drawFSA(ssft);

	std::cout << "Input: ";
	std::vector<ml> input = toMyLetter("_abd_cc_abc_aa_abd_c_abd_abdd_abdd_ab_c_a_bb_a_");
	for (const auto &l : input) {
		std::cout << l;
	}
	std::cout << std::endl;
	std::vector<ml> output = ssft.f(input);
	std::cout << "Output: ";
	for (const auto &l : output) {
		std::cout << l;
	}
	std::cout << std::endl;

	return 0;
}
