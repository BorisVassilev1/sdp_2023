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

	constexpr static size_t size = 5;
	const static MyLetter	eps;
	const static MyLetter	eof;
	friend std::ostream &operator<<(std::ostream &out, const MyLetter &l) {
		static const char *names[] = {"a", "b", "c", "d", "_", "ε", "eof"};
		if (l.value >= 0 && l.value < 7) return out << names[(int)l.value];
		else return out << "Unknown(" << (int)l.value << ")";
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

int main() {
	using ml = MyLetter;
	// fl::ReplaceWithMarkerSSFT<MyLetter> ssft({{{'a'}, {'b'}}, {{'b'}, {'c'}}, {{'a'}, {'c'}}, {{'d'}, {'c'}}}, '_');
	fl::ReplaceWithMarkerSSFT<MyLetter> ssft(
		{{{ml::a}, {ml::b}}, {{ml::b}, {ml::c}}, {{ml::a}, {ml::c}}, {{ml::d}, {ml::c}}}, ml::_);

	fl::drawFSA(ssft);
}
