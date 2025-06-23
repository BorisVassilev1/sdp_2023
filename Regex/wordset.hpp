#pragma once

#include <iterator>
#include <span>
#include <stdexcept>
#include <unordered_map>
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
	WordSet() { addWord(std::span<Letter>{}); }

	template <class Input>
	WordID addWord(Input &&word) {
		wordsData.emplace_back(words.size(), word.size());
		words.insert(words.end(), word.begin(), word.end());
		return nextWordID++;
	}

	WordID copyWord(WordID id) {
		if (id >= nextWordID) { throw std::out_of_range("Invalid WordID"); }
		wordsData.emplace_back(wordsData[id].start, wordsData[id].length);
		return nextWordID++;
	}

	auto getWord(WordID id) const {
		if (id >= nextWordID) { throw std::out_of_range("Invalid WordID"); }
		const auto &[start, length] = wordsData[id];
		return std::span{words.data() + start, length};
	}

	auto getWordData(WordID id) const {
		if (id >= nextWordID) { throw std::out_of_range("Invalid WordID"); }
		return wordsData[id];
	}

	auto operator[](WordID id) const { return getWord(id); }

	size_t totalLength() const { return words.size(); }
	size_t size() const { return nextWordID; }

	void replaceWithSubstr(WordID id, size_t offset, size_t length) {
		if (id >= nextWordID) { throw std::out_of_range("Invalid WordID"); }
		if (offset + length > wordsData[id].length) { throw std::out_of_range("Substring exceeds word length"); }
		auto &[start, wordLength] = wordsData[id];
		start += offset;
		wordLength = length;
	}

	void clear() {
		words.clear();
		wordsData.clear();
		nextWordID = 0;
	}

	class Iterator {
	   private:
		const WordSet &wordSet;
		WordID		   currentID;

	   public:
		Iterator(const WordSet &ws, WordID id) : wordSet(ws), currentID(id) {}

		bool operator!=(const Iterator &other) const { return currentID != other.currentID; }

		auto operator*() const { return wordSet.getWord(currentID); }

		Iterator &operator++() {
			if (currentID < wordSet.nextWordID) { ++currentID; }
			return *this;
		}
	};

	auto begin() const { return Iterator(*this, 0); }
	auto end() const { return Iterator(*this, nextWordID); }
};

template <class Letter>
class UniqueWordSet {
	std::vector<Letter> words;

   public:
	using WordID = unsigned int;
	class mySpan {
	   public:
		size_t				 start;
		size_t				 size;
		std::vector<Letter> &words;

		auto begin() const { return words.data() + start; }
		auto end() const { return words.data() + start + size; }

		mySpan(const std::vector<Letter> &words, size_t start, size_t size)
			: start(start), size(size), words(const_cast<std::vector<Letter> &>(words)) {}

		bool operator==(const mySpan &other) const {
			return this->size == other.size && std::equal(this->begin(), this->end(), other.begin());
		}
		bool operator==(const std::span<Letter> &other) const {
			return this->size == other.size() && std::equal(this->begin(), this->end(), other.begin());
		}
		bool operator!=(const mySpan &other) const { return !(*this == other); }
		bool operator!=(const std::span<Letter> &other) const { return !(*this == other); }
	};

   private:
	struct WordData {
		size_t start;
		size_t length;
	};

	std::vector<WordData> wordsData;

	struct myHash {
		using is_transparent = void;	 // Allows this hash to be used in unordered_map with std::span<Letter>
		size_t operator()(const mySpan &span) const {
			return std::hash<std::string_view>()(
				std::string_view(reinterpret_cast<const char *>(span.begin()), span.size));
		}
		size_t operator()(const std::span<Letter> &span) const {
			return std::hash<std::string_view>()(
				std::string_view(reinterpret_cast<const char *>(span.data()), span.size()));
		}
	};

	struct myEqual {
		using is_transparent = void;	 // Allows this equal to be used in unordered_map with std::span<Letter>
		template <class U, class V>
		bool operator()(const U &a, const V &b) const {
			return std::distance(a.begin(), a.end()) == std::distance(b.begin(), b.end()) &&
				   std::equal(a.begin(), a.end(), b.begin(), b.end());
		}
	};

	std::unordered_map<mySpan, WordID, myHash, myEqual> wordMap;

	WordID nextWordID = 0;

   public:
	UniqueWordSet() { addWord(std::span<Letter>{}); }

	template <class Input>
	WordID addWord(Input &&word) {
		auto it = wordMap.find(std::span{word.begin(), word.end()});
		if (it != wordMap.end()) {
			return it->second;	   // Word already exists, return its ID
		}
		wordsData.emplace_back(words.size(), word.size());
		words.insert(words.end(), word.begin(), word.end());
		wordMap.emplace(mySpan{words, wordsData.back().start, word.size()}, nextWordID);
		return nextWordID++;
	}

	WordID addWord(Letter *word, size_t length) {
		if (length == 0) return addWord(std::span<Letter>{});
		auto it = wordMap.find(std::span{word, length});
		if (it != wordMap.end()) {
			return it->second;	   // Word already exists, return its ID
		}
		wordsData.emplace_back(words.size(), length);
		words.insert(words.end(), word, word + length);
		wordMap.emplace(mySpan{words, wordsData.back().start, length}, nextWordID);
		return nextWordID++;
	}

	auto getWord(WordID id) const {
		if (id >= nextWordID) { throw std::out_of_range("Invalid WordID"); }
		const auto &[start, length] = wordsData[id];
		return std::span{words.data() + start, length};
	}

	auto operator[](WordID id) const { return getWord(id); }

	size_t totalLength() const { return words.size(); }
	size_t size() const { return nextWordID; }

	void clear() {
		words.clear();
		wordsData.clear();
		nextWordID = 0;
	}
};
