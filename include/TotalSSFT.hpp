#pragma once

#include "concepts.hpp"
#include "wordset.hpp"

namespace fl {

template <isLetter Letter, std::size_t alphabetSize = Letter::size>
class TotalSSFT {
   public:
	using State	   = unsigned int;
	using WordID   = UniqueWordSet<Letter>::WordID;
	struct Transition {
		WordID outputID;
		State  next;
	};
	using Map	   = std::vector<std::array<Transition, alphabetSize>>;
	using Letter_t = Letter;

   protected:
	unsigned int		  N = 0;
	UniqueWordSet<Letter> words;

	/// transitions[from][letter] = (outputID, to)
	Map transitions;
	/// the Ψ-function, output[state] = outputID
	std::vector<WordID> output;

   public:
	TotalSSFT() = default;

	TotalSSFT(const TotalSSFT &) = default;
	TotalSSFT(TotalSSFT &&)		 = default;

	TotalSSFT &operator=(const TotalSSFT &) = default;
	TotalSSFT &operator=(TotalSSFT &&)		= default;

	[[clang::always_inline]] inline constexpr std::size_t size() const { return N; }
	[[clang::always_inline]] inline constexpr std::size_t cacheCount() const { return words.size(); }
	[[clang::always_inline]] inline constexpr std::size_t cacheSize() const { return words.totalLength(); }

	[[clang::always_inline]] inline std::pair<std::span<const Letter>, bool> step(State &state, Letter letter) const {
		auto &[outputID, next] = transitions[state][size_t(letter)];
		if (next == -1u) return std::make_pair(std::span<const Letter>{}, false);
		state = next;
		return std::make_pair(words[outputID], true);
	}
	
	[[clang::always_inline]] inline bool step(State &state, std::span<const Letter> input, std::vector<Letter> &output) const {
		for (Letter letter : input) {
			auto [out, ok] = step(state, letter);
			if (!ok) return false;
			output.insert(output.end(), out.begin(), out.end());
		}
		return true;
	}

	[[clang::always_inline]] inline std::span<const Letter> psi(State state) const {
		return std::span<const Letter>(words[output[state]]);
	}
	[[clang::always_inline]] inline State initial() const { return 0; }

	[[clang::always_inline]] inline bool isFinal(State) const { return true; }


	const TotalSSFT &serialize(std::ostream &out) const {
		out.write(reinterpret_cast<const char *>(&N), sizeof(N));
		words.serialize(out);
		out.write(reinterpret_cast<const char *>(transitions.data()), transitions.size() * sizeof(transitions[0]));	
		out.write(reinterpret_cast<const char *>(output.data()), output.size() * sizeof(output[0]));
		return *this;
	}

	TotalSSFT(std::istream &in) {
		in.read(reinterpret_cast<char *>(&N), sizeof(N));
		words = UniqueWordSet<Letter>(in);
		transitions.resize(N);
		output.resize(N);
		in.read(reinterpret_cast<char *>(transitions.data()), transitions.size() * sizeof(transitions[0]));
		in.read(reinterpret_cast<char *>(output.data()), output.size() * sizeof(output[0]));
	}
};

}	  // namespace fl

#include "letter.hpp"
static_assert(fl::isSubSeqTransducer<fl::TotalSSFT<fl::Letter>>,
			  "TotalSSFT does not satisfy the subsequential transducer concept");
