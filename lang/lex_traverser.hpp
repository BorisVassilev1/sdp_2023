#pragma once

#include <Regex/SSFT.hpp>
#include <ranges>

template <class Token, std::ranges::input_range Range>
	requires std::convertible_to<std::ranges::range_value_t<Range>, Token>
class LexerRange : public std::ranges::view_interface<LexerRange<Token, Range>> {
   public:
	Range	   *range;
	SSFT<Token> ssft;

	LexerRange(Range &range, SSFT<Token> &&ssft) : range(&range), ssft(std::move(ssft)) {}

	class iterator {
		using InnerIterator = decltype(std::ranges::begin(std::declval<Range &>()));
		using InnerSentinel = decltype(std::ranges::end(std::declval<Range &>()));
		InnerIterator	   current;
		InnerSentinel	   end;
		SSFT<Token>::State current_state;
		SSFT<Token>		  *ssft_ptr;
		std::size_t		   position;
		std::size_t		   output_position;
		mutable bool	   consumed = false;

	   public:
		iterator(InnerIterator &&begin, InnerSentinel &&end, SSFT<Token> *ssft_ptr)
			: current(std::move(begin)),
			  end(std::move(end)),
			  current_state(0),
			  ssft_ptr(ssft_ptr),
			  position(0),
			  output_position(0) {
			++*this;
		}

		bool operator!=(const iterator &other) const { return current != other.current; }
		bool operator!=(const InnerSentinel &other) const { return !consumed || current != other; }
		bool operator==(const iterator &other) const { return current == other.current; }
		bool operator==(const InnerSentinel &other) const { return consumed && current == other; }

		iterator &operator++() {
			// std::cout << "++ at pos " << position << " (output pos " << output_position << ")\n";
			output_position = position;
			current_state	= 0;
			consumed		= current == end;
			while (current != end) {
				// std::cout << "  reading '" << *current << "' at pos " << position << " q: " << current_state << "\n";
				auto it = ssft_ptr->transitions.find({current_state, *current});
				if (it == ssft_ptr->transitions.end()) {
					if (ssft_ptr->qFinals.contains(current_state)) return *this;
					do {
						++position;
						++current;
						it = ssft_ptr->transitions.find({0, *current});
					} while (it == ssft_ptr->transitions.end() && current != end);

					current_state = 0;
					return *this;
				}
				const auto &[outputID, next] = it->second;
				current_state				 = next;
				++current;
				++position;
			}
			return *this;
		}

		std::tuple<std::optional<Token>, size_t, size_t> operator*() const {
			consumed = true;
			if (ssft_ptr->qFinals.contains(current_state)) {
				return std::make_tuple(ssft_ptr->words[ssft_ptr->output.at(current_state)][0], output_position,
									   position - 1);
			} else {
				return std::make_tuple(std::nullopt, output_position, position - 1);
			}
		}
	};

	iterator begin() { return iterator(std::ranges::begin(*range), std::ranges::end(*range), &ssft); }
	auto	 end() { return std::ranges::end(*range); };
};
