#include "ReplaceWithMarkerSSFT.hpp"
#include "token.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>

class QwenToken {
   public:
	uint16_t value;

	constexpr QwenToken() : value(0) {}
	constexpr QwenToken(uint16_t v) : value(v) {}
	constexpr QwenToken(const QwenToken &other)			   = default;
	constexpr QwenToken &operator=(const QwenToken &other) = default;

	constexpr operator size_t() const { return static_cast<size_t>(value); }
	constexpr ~QwenToken() = default;
	constexpr QwenToken operator++() {
		++value;
		return *this;
	}

	constexpr static size_t size = 256 + 1;		// 256 tokens + 1 marker
	const static QwenToken	eps;
	const static QwenToken	eof;
	const static QwenToken	marker;

	static QwenToken fromChar(char ch) {
		uint16_t value = static_cast<uint16_t>(ch);
		if (value <= 0x20) return QwenToken(188 + value);
		if (value <= 0x7E) return QwenToken(value - 0x21);
		if (value <= 0xA0) return QwenToken(value + 94);
		if (value <= 0xAC) return QwenToken(value - 0x43);
		if (value == 0xAD) return QwenToken(255);
		return QwenToken(value - 0x44);
	}

	QwenToken(std::istream &in) {
		std::string hexStr;
		in >> hexStr;
		if (in.gcount() != 8) { throw std::runtime_error("Failed to read 8 bytes for QwenToken"); }
		value = static_cast<uint16_t>(std::stoul(hexStr, nullptr, 16));
		// remap to 0-255 range
		if (value >= 0x21 && value <= 0x7E) {
			value = value - 0x21;
		} else if (value >= 0xC2A1 && value <= 0xC2BF) {
			value = value - 0xC2A1 + 94;
		} else if (value >= 0xC380 && value <= 0xC3BF) {
			value = value - 0xC380 + 124;
		} else if (value >= 0xC480 && value <= 0xC4BF) {
			value = value - 0xC480 + 188;
		} else if (value >= 0xC580 && value <= 0xC583) {
			value = value - 0xC580 + 252;
		} else {
			throw std::runtime_error("QwenToken value out of range");
		}
	}

	friend std::ostream &operator<<(std::ostream &out, const QwenToken &t) {
		// out << "(" << int(t.value) << ")";
		if (t.value == 256) return out << "_";	   // marker
		if (t.value == 257) return out << "ε";	   // epsilon
		if (t.value == 258) return out << "␄";	   // eof
		// else out << (char)(t.value);
		if (t.value < 94) out << char(t.value + 0x21);
		else if (t.value < 124) out << (char)0xC2 << char(t.value - 94 + 0xA1);
		else if (t.value < 188) out << (char)0xC3 << char(t.value - 124 + 0x80);
		else if (t.value < 252) out << (char)0xC4 << char(t.value - 188 + 0x80);
		else out << char(t.value - 252 + 0xC580);
		return out;
	}
};
constexpr inline QwenToken QwenToken::eps	 = {257};
constexpr inline QwenToken QwenToken::eof	 = {258};
constexpr inline QwenToken QwenToken::marker = {256};

static_assert(fl::isLetter<QwenToken>);

std::vector<QwenToken> toQwenToken(std::string_view input) {
	std::vector<QwenToken> output;
	output.reserve(input.size());
	for (const auto &c : input)
		output.push_back(QwenToken::fromChar(static_cast<uint16_t>(c)));
	return output;
}

int main(int argc, char **argv) {
	// usage: ./replace_dict <batches_file>
	using Rule	= fl::ReplaceWithMarkerSSFT<QwenToken>::Rule;
	using RSSFT = fl::ReplaceWithMarkerSSFT<QwenToken>;
	std::vector<std::vector<Rule>> batches;
	{
		std::vector<std::vector<QwenToken>> alphabet;
		alphabet.resize(151387 + 256);
		// push the base alphabet of 256 tokens
		for (uint16_t i = 0; i < 256; ++i) {
			alphabet[i] = {QwenToken(i)};
		}

		std::ifstream batchesFile(argv[1]);
		std::string	  line;
		std::getline(batchesFile, line);
		// batches are separated by a line with a single dash '-'
		assert(line == "-");
		std::vector<Rule> rules;
		while (std::getline(batchesFile, line)) {
			if (line == "-") {
				batches.push_back(std::move(rules));
				rules.clear();
				continue;
			}
			int				   id1, id2, id3;
			std::istringstream iss(line);
			iss >> id1 >> id2 >> id3;
			std::vector<QwenToken> newTokenString;
			newTokenString.insert(newTokenString.end(), alphabet[id1].begin(), alphabet[id1].end());
			newTokenString.insert(newTokenString.end(), alphabet[id2].begin(), alphabet[id2].end());
			alphabet[id3] = newTokenString;
			rules.push_back({alphabet[id1], alphabet[id2]});
		}
		batches.push_back(std::move(rules));
	}
	std::cout << "Loaded " << batches.size() << " batches of rules." << std::endl;
	// for (size_t i = 0; i < batches[7].size(); ++i) {
	//	const auto &rule = batches[7][i];
	//	std::cout << "Rule " << i << ": ";
	//	for (const auto &token : rule.left)
	//		std::cout << token;
	//	std::cout << ", ";
	//	for (const auto &token : rule.right)
	//		std::cout << token;
	//	std::cout << std::endl;
	// }

	// override a simple test case that merges "ab" and "bc" into "abc" with marker "_"
	// batches = {{
	//			   {toQwenToken("a"), toQwenToken("b")},
	//			   {toQwenToken("b"), toQwenToken("c")},
	//			   {toQwenToken("a"), toQwenToken("c")},
	//		   },
	//		   {
	//			   {toQwenToken("ab"), toQwenToken("c")},
	//			   {toQwenToken("bc"), toQwenToken("d")},
	//		   }};

	std::vector<RSSFT> ssfts;
	for (size_t i = 0; i < batches.size(); ++i) {
		std::cout << "\rBuilding SSFT for batch " << i + 1 << "/" << batches.size() << "...           " << std::flush;
		ssfts.emplace_back(std::move(batches[i]), QwenToken::marker);
		statFSA(ssfts.back());
	}
	batches.clear();

	//ssfts.back().print(std::cout);

	while (std::cin) {
		std::vector<QwenToken> test;
		test.push_back(QwenToken::marker);
		while (std::cin.peek() != '\n' && std::cin.peek() != EOF) {
			char c;
			std::cin.get(c);
			test.push_back(QwenToken::fromChar(c));
			test.push_back(QwenToken::marker);
		}
		std::cin.get();

		for (const auto &token : test) {
			std::cout << token;
		}

		std::cout << std::endl << "Output: " << std::endl;
		std::vector<QwenToken> output;
		for (const auto &[i, ssft] : std::views::enumerate(ssfts)) {
			output = ssft.f(test);
			if (test != output) {
				std::cout << std::setw(4) << i << ": ";
				for (const auto &token : output) {
					std::cout << token;
				}
				std::cout << std::endl;
			}
			test = output;
		}
		for (const auto &token : output) {
			std::cout << token;
		}
		std::cout << std::endl;
	}

	return 0;
}
