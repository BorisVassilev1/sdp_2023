#pragma once

#include <span>
#include <stdexcept>
#include <vector>
template <class Letter>
class WordSet {
   public:
	using WordID = unsigned int;

   private:
	struct WordData {
		size_t start;
		size_t length;
	};

	std::vector<Letter>	  words;
	std::vector<WordData> wordsData;

	WordID nextWordID = 0;

   public:
	WordSet() {
		addWord(std::span<Letter>{});
	}

	template <class Input>
	WordID addWord(Input &&word) {
		wordsData.emplace_back(words.size(), word.size());
		words.insert(words.end(), word.begin(), word.end());
		return nextWordID++;
	}

	auto getWord(WordID id) const {
		if (id >= nextWordID) { throw std::out_of_range("Invalid WordID"); }
		const auto &[start, length] = wordsData[id];
		return std::span{words.data() + start, length};
	}

	auto operator[](WordID id) const {
		return getWord(id);
	}

	size_t totalLength() const { return words.size(); }
	size_t size() const { return nextWordID; }

	void clear() {
		words.clear();
		wordsData.clear();
		nextWordID = 0;
	}
};
