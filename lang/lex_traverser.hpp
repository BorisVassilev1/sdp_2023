#pragma once

#include <Regex/SSFT.hpp>
#include <cstdio>
#include <iterator>
#include <ranges>
#include <string>
#include "util/utils.hpp"
#include <DPDA/token.h>

template <isLetter Letter>
class CharInputStream : public std::ranges::view_interface<CharInputStream<Letter>> {
   public:
	std::istream *input_stream = nullptr;

	CharInputStream() = default;
	CharInputStream(std::istream &input_stream) : input_stream(&input_stream) {}
	CharInputStream(const CharInputStream &other)			 = default;
	CharInputStream(CharInputStream &&other)				 = default;
	CharInputStream &operator=(const CharInputStream &other) = default;
	CharInputStream &operator=(CharInputStream &&other)		 = default;

	struct sentinel;

	class iterator {
	   public:
		using iterator_category = std::input_iterator_tag;
		using value_type		= Letter;
		using difference_type	= std::ptrdiff_t;
		using pointer			= Letter *;
		using reference			= Letter;

		std::istream *input_stream;
		Letter		  current_char = Letter::eps;
		bool		  end_reached  = false;

		iterator(std::istream *input_stream) : input_stream(input_stream) { ++*this; }

		iterator(const iterator &other)			   = default;
		iterator(iterator &&other)				   = default;
		iterator &operator=(const iterator &other) = default;
		iterator &operator=(iterator &&other)	   = default;

		bool operator!=(const sentinel &) const { return !end_reached; }
		bool operator==(const sentinel &) const { return end_reached; }

		iterator &operator++() {
			int ch = input_stream->get();
			end_reached = (current_char == Letter::eof);
			
			if (input_stream->eof()) {
				current_char = Letter::eof;
			} else {
				current_char = Letter((char)ch);
			}
			return *this;
		}
		iterator operator++(int) {
			iterator temp = *this;
			++*this;
			return temp;
		}

		Letter operator*() const {
			return current_char;
		}
	};
	struct sentinel {
		bool operator==(const iterator &it) const { return it.end_reached; }
		bool operator!=(const iterator &it) const { return !it.end_reached; }
	};

	iterator begin() { return iterator(input_stream); }
	sentinel end() { return sentinel{}; }
};

static_assert(std::sentinel_for<CharInputStream<Token>::sentinel, CharInputStream<Token>::iterator>);
static_assert(std::ranges::viewable_range<CharInputStream<Token>>);
static_assert(std::ranges::input_range<CharInputStream<Token>>);

template <class Token, std::ranges::input_range Range>
	requires std::convertible_to<std::ranges::range_value_t<Range>, Token>
class LexerRange : public std::ranges::view_interface<LexerRange<Token, Range>> {
   public:
	using InnerIterator = decltype(std::ranges::begin(std::declval<Range &>()));
	using InnerSentinel = decltype(std::ranges::end(std::declval<Range &>()));

	Range	   *range;
	SSFT<Token> ssft;
	Token		error_token;

	LexerRange(Range &range, SSFT<Token> &&ssft, Token error_token)
		: range(&range), ssft(std::move(ssft)), error_token(error_token) {}

	class iterator {
	   public:
		InnerIterator			  current;
		InnerSentinel			  end;
		SSFT<Token>::State		  current_state;
		LexerRange<Token, Range> *lex_ptr;
		std::size_t				  position;
		std::size_t				  output_position;
		std::size_t				  line_number = 1;
		mutable bool			  consumed	  = false;
		std::vector<Token>		  buffer;
		mutable Token			  queued_token = Token::eps;

		iterator(InnerIterator &&begin, InnerSentinel &&end, LexerRange<Token, Range> *lex_ptr)
			: current(std::move(begin)),
			  end(std::move(end)),
			  current_state(0),
			  lex_ptr(lex_ptr),
			  position(0),
			  output_position(0) {
			++*this;
		}

		bool operator!=(const InnerSentinel &other) const { return !consumed || current != other; }
		bool operator==(const InnerSentinel &other) const { return consumed && current == other; }

		iterator &operator++() {
			auto ssft_ptr	= &lex_ptr->ssft;
			output_position = position;
			current_state	= 0;
			consumed		= current == end;
			buffer.clear();
			while (current != end) {
				//if(int(*current) == 0) {
				//	buffer.push_back(*current);
				//	++current;
				//	++position;
				//	continue;
				//}
				auto it = ssft_ptr->transitions.find({current_state, *current});
				dbLog(dbg::LOG_DEBUG, "At state ", current_state, ", char '", *current, "'");
				if (it == ssft_ptr->transitions.end()) {
					dbLog(dbg::LOG_DEBUG, "No transition found");
					if (ssft_ptr->qFinals.contains(current_state)) {
						Token output = ssft_ptr->words[ssft_ptr->output.at(current_state)][0];
						auto  it	 = lex_ptr->skippers.find(output);
						if (it != lex_ptr->skippers.end()) {
							auto skipper  = it->second;
							auto [len, t] = skipper(*this);
							position += len;
							queued_token = t;
						}
						return *this;
					}
					buffer.clear();
					do {
						if (*current == '\n') ++line_number;
						buffer.push_back(*current);
						++position;
						++current;
						it = ssft_ptr->transitions.find({0, *current});
					} while (it == ssft_ptr->transitions.end() && current != end);

					current_state = 0;
					return *this;
				}
				if (*current == '\n') ++line_number;
				const auto &[outputID, next] = it->second;
				dbLog(dbg::LOG_DEBUG, "Transition on '", *current, "' to state ", next);
				current_state = next;
				buffer.push_back(*current);
				++current;
				++position;
			}
			return *this;
		}

		std::tuple<Token, size_t, size_t, size_t, std::span<const Token>> operator*() const {
			consumed = true;
			if (queued_token != Token::eps) {
				auto t		 = queued_token;
				queued_token = Token::eps;
				return std::make_tuple(t, output_position, position - 1, line_number,
									   std::span(buffer.begin(), buffer.end()));
			}
			if (lex_ptr->ssft.qFinals.contains(current_state)) {
				return std::make_tuple(lex_ptr->ssft.words[lex_ptr->ssft.output.at(current_state)][0], output_position,
									   position - 1, line_number, std::span(buffer.begin(), buffer.end()));
			} else {
				return std::make_tuple(lex_ptr->error_token, output_position, position - 1, line_number,
									   std::span(buffer.begin(), buffer.end()));
			}
		}
	};
	using SkipperFunction = std::function<std::tuple<size_t, Token>(iterator &it)>;
	std::map<Token, SkipperFunction> skippers;
	void							 attachSkipper(Token token, SkipperFunction skipper) { skippers[token] = skipper; }

	iterator begin() { return iterator(std::ranges::begin(*range), std::ranges::end(*range), this); }
	auto	 end() { return std::ranges::end(*range); };
};
